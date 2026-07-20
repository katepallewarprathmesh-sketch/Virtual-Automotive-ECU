#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "CanAuthenticator.h"
#include "CanAuthorization.h"
#include "CanSocket.h"
#include "HsmClient.h"
#include "TaskScheduler.h"
#include "DidManager.h"
#include "DtcManager.h"
#include "SecurityManager.h"
#include "SessionManager.h"
#include "UdsDispatcher.h"
#include "UdsProtocol.h"

using namespace ecu;

int main() {
    std::cout << "Gateway ECU starting on vcan0." << std::endl;

    HsmClient hsmClient("/tmp/ecu_hsm.sock");
    if (!hsmClient.connect()) {
        std::cerr << "Failed to connect to HSM service: " << hsmClient.lastError() << std::endl;
        return 1;
    }
    if (!hsmClient.provisionSymmetricKey("mac_key", 16)) {
        std::cerr << "Failed to provision MAC key: " << hsmClient.lastError() << std::endl;
        return 2;
    }

    CanSocket socket("vcan0");
    if (!socket.isOpen()) {
        std::cerr << "Unable to open CAN socket: " << socket.lastError() << std::endl;
        return 3;
    }

    std::map<uint32_t, uint8_t> lastSequence;
    std::mutex sequenceMutex;
    std::atomic<bool> running(true);
    ecu::CanAuthorization authorization;
    authorization.addAllowedCanId("sensor_ecu", 0x100);
    authorization.addAllowedCanId("sensor_ecu", 0x7DF);
    authorization.addAllowedCanId("sensor_ecu", 0x7E8);
    uds::SessionManager sessionManager;
    uds::SecurityManager securityManager;
    uds::DidManager didManager;
    uds::DtcManager dtcManager;
    uds::UdsDispatcher dispatcher(sessionManager, securityManager, didManager, dtcManager);

    auto processFrame = [&](const CanFrame& frame) {
        if (!authorization.isAllowed("sensor_ecu", frame.id)) {
            std::cerr << "Gateway: unauthorized sender/can-id combo id=0x" << std::hex << frame.id << std::dec << std::endl;
            return;
        }

        if (frame.id == 0x100) {
            AuthenticatedCanMessage authMessage;
            if (!CanAuthenticator::parseAuthenticatedFrame(frame, authMessage)) {
                std::cerr << "Gateway: invalid authenticated frame" << std::endl;
                return;
            }
            if (!CanAuthenticator::validateFrameIntegrity(frame, authMessage)) {
                std::cerr << "Gateway: integrity check failed for frame id=0x" << std::hex << frame.id << std::dec << std::endl;
                return;
            }
            std::vector<uint8_t> raw = CanAuthenticator::rawAuthData(authMessage);
            bool success = false;
            std::vector<uint8_t> expectedMac = hsmClient.computeMac("mac_key", raw, success);
            if (!success || expectedMac.size() < 2) {
                std::cerr << "Gateway HSM MAC verification failed: " << hsmClient.lastError() << std::endl;
                return;
            }
            if (frame.data[6] != expectedMac[0] || frame.data[7] != expectedMac[1]) {
                std::cerr << "Gateway: auth MAC mismatch for seq=" << static_cast<int>(authMessage.sequence) << std::endl;
                return;
            }
            {
                std::lock_guard<std::mutex> lock(sequenceMutex);
                uint8_t last = lastSequence[frame.id];
                if (authMessage.sequence <= last) {
                    std::cerr << "Gateway: replay detected seq=" << static_cast<int>(authMessage.sequence) << std::endl;
                    return;
                }
                lastSequence[frame.id] = authMessage.sequence;
            }
            std::cout << "Gateway accepted telemetry seq=" << static_cast<int>(authMessage.sequence) << " type=" << static_cast<int>(authMessage.messageType) << std::endl;
            return;
        }

        uds::UdsMessage message;
        if (uds::parseUdsMessage(frame, message)) {
            uds::UdsResponse response;
            if (dispatcher.dispatch(message, response)) {
                CanFrame responseFrame;
                if (uds::encodeResponse(response.service, response.payload, responseFrame, 0x7E8)) {
                    socket.send(responseFrame);
                    std::cout << "Gateway responded to UDS service 0x" << std::hex << static_cast<int>(message.service) << std::dec << std::endl;
                }
            } else if (response.isNegative) {
                CanFrame responseFrame;
                if (uds::encodeResponse(uds::kNegativeResponse, response.payload, responseFrame, 0x7E8)) {
                    socket.send(responseFrame);
                    std::cout << "Gateway sent negative response NRC=0x" << std::hex << static_cast<int>(response.nrc) << std::dec << std::endl;
                }
            }
        }
    };

    std::thread receiverThread([&]() {
        while (running) {
            CanFrame frame;
            if (!socket.receive(frame)) {
                continue;
            }
            processFrame(frame);
        }
    });

    TaskScheduler scheduler;
    scheduler.addPeriodicTask(2000, [&]() {
        std::lock_guard<std::mutex> lock(sequenceMutex);
        std::cout << "Gateway scheduler heartbeat; tracked frames=" << lastSequence.size() << std::endl;
    }, "heartbeat");

    if (!scheduler.start()) {
        std::cerr << "Gateway failed to start scheduler." << std::endl;
        running = false;
        receiverThread.join();
        return 4;
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    running = false;
    scheduler.stop();
    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    std::cout << "Gateway ECU stopped." << std::endl;
    return 0;
}
