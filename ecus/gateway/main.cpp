#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "CanAuthenticator.h"
#include "CanSocket.h"
#include "HsmClient.h"
#include "TaskScheduler.h"
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

    auto processFrame = [&](const CanFrame& frame) {
        if (frame.id == 0x100) {
            AuthenticatedCanMessage authMessage;
            if (!CanAuthenticator::parseAuthenticatedFrame(frame, authMessage)) {
                std::cerr << "Gateway: invalid authenticated frame" << std::endl;
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
            if (message.service == uds::kReadDataByIdentifier && message.payload.size() >= 2) {
                uint16_t identifier = static_cast<uint16_t>(message.payload[0]) << 8 |
                                      static_cast<uint16_t>(message.payload[1]);
                if (identifier == uds::kDataIdentifierVehicleSpeed) {
                    std::vector<uint8_t> responsePayload = {0xF1, 0x90, 0x09, 0xD0};
                    CanFrame responseFrame;
                    if (uds::encodeResponse(uds::kReadDataByIdentifierResponse, responsePayload, responseFrame, 0x7E8)) {
                        socket.send(responseFrame);
                        std::cout << "Gateway responded to UDS ReadDataByIdentifier." << std::endl;
                    }
                }
            } else if (message.service == uds::kReadDataByIdentifierResponse) {
                std::cout << "Gateway received UDS response." << std::endl;
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
