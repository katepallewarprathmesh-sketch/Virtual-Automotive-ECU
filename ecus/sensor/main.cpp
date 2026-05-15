#include <chrono>
#include <cstring>
#include <filesystem>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include "CanAuthenticator.h"
#include "CanSocket.h"
#include "HsmClient.h"
#include "SecureBoot.h"
#include "TaskScheduler.h"
#include "UdsProtocol.h"

using namespace ecu;
namespace fs = std::filesystem;

static std::string readTextFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static fs::path resolvePath(const std::string& filename, const std::vector<fs::path>& prefixes) {
    for (const auto& prefix : prefixes) {
        fs::path candidate = prefix / filename;
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

static fs::path resolveFirmwarePath(const std::string& filename) {
    const std::vector<fs::path> searchPaths = {
        fs::path("firmware") / filename,
        fs::path("../firmware") / filename,
        fs::path("../../firmware") / filename,
        fs::path("../../../firmware") / filename,
    };

    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return path;
        }
    }
    return {};
}

int main() {
    const fs::path firmwarePath = resolveFirmwarePath("ecu_firmware.bin");
    const fs::path signaturePath = resolveFirmwarePath("ecu_firmware.sig");
    if (firmwarePath.empty() || signaturePath.empty()) {
        std::cerr << "Secure Boot failed: firmware or signature file not found" << std::endl;
        return 1;
    }

    const fs::path trustedPublicKeyPath = resolvePath("ecu_public.pem", {"scripts/keys", "../scripts/keys", "../../scripts/keys", "firmware", "../firmware"});
    if (trustedPublicKeyPath.empty()) {
        std::cerr << "Secure Boot failed: trusted public key not found" << std::endl;
        return 1;
    }

    const std::string trustedPublicKeyPem = readTextFile(trustedPublicKeyPath);
    if (trustedPublicKeyPem.empty()) {
        std::cerr << "Secure Boot failed: unable to read trusted public key file " << trustedPublicKeyPath << std::endl;
        return 1;
    }

    std::string bootstrapError;
    SecureBoot boot(trustedPublicKeyPem);
    if (!boot.verifyFirmware(firmwarePath.string(), signaturePath.string(), bootstrapError)) {
        std::cerr << "Secure Boot failed: " << bootstrapError << std::endl;
        return 1;
    }

    std::cout << "Secure Boot passed using firmware=" << firmwarePath << " signature=" << signaturePath << " publickey=" << trustedPublicKeyPath << "." << std::endl;

    HsmClient hsmClient("/tmp/ecu_hsm.sock");
    if (!hsmClient.connect()) {
        std::cerr << "Failed to connect to HSM service: " << hsmClient.lastError() << std::endl;
        return 2;
    }
    if (!hsmClient.provisionSymmetricKey("mac_key", 16)) {
        std::cerr << "Failed to provision MAC key: " << hsmClient.lastError() << std::endl;
        return 3;
    }

    CanSocket socket("vcan0");
    if (!socket.isOpen()) {
        std::cerr << "Unable to open CAN socket: " << socket.lastError() << std::endl;
        return 4;
    }

    std::atomic<uint8_t> sequence{0};
    TaskScheduler scheduler;

    scheduler.addPeriodicTask(100, [&]() {
        AuthenticatedCanMessage authMessage{};
        authMessage.sequence = sequence.fetch_add(1);
        authMessage.messageType = 0x01;
        authMessage.data[0] = 0x10;
        authMessage.data[1] = 0x20;
        authMessage.data[2] = 0x30;
        authMessage.data[3] = static_cast<uint8_t>(authMessage.sequence + 0x55);

        std::vector<uint8_t> raw = CanAuthenticator::rawAuthData(authMessage);
        bool success = false;
        std::vector<uint8_t> mac = hsmClient.computeMac("mac_key", raw, success);
        if (!success || mac.size() < 2) {
            std::cerr << "Sensor HSM MAC failed: " << hsmClient.lastError() << std::endl;
            return;
        }

        CanFrame frame;
        if (!CanAuthenticator::createAuthenticatedFrame(0x100, authMessage.sequence, authMessage.messageType, {authMessage.data, authMessage.data + 4}, {mac[0], mac[1]}, frame)) {
            std::cerr << "Failed to create authenticated CAN frame" << std::endl;
            return;
        }

        if (!socket.send(frame)) {
            std::cerr << "Failed to send authenticated CAN frame: " << socket.lastError() << std::endl;
            return;
        }
        std::cout << "Sensor ECU sent auth telemetry seq=" << static_cast<int>(authMessage.sequence) << std::endl;
    }, "telemetry");

    scheduler.addPeriodicTask(1000, [&]() {
        CanFrame udsRequest;
        std::vector<uint8_t> udsPayload = {
            static_cast<uint8_t>(uds::kDataIdentifierVehicleSpeed >> 8),
            static_cast<uint8_t>(uds::kDataIdentifierVehicleSpeed & 0xFF)
        };
        if (!uds::encodeRequest(uds::kReadDataByIdentifier, udsPayload, udsRequest, 0x7DF)) {
            std::cerr << "Failed to encode UDS request" << std::endl;
            return;
        }
        if (!socket.send(udsRequest)) {
            std::cerr << "Failed to send UDS request: " << socket.lastError() << std::endl;
            return;
        }
        std::cout << "Sensor ECU sent UDS ReadDataByIdentifier." << std::endl;
    }, "uds_request");

    std::cout << "Sensor ECU starting RTOS-style scheduler tasks." << std::endl;
    if (!scheduler.start()) {
        std::cerr << "Failed to start scheduler" << std::endl;
        return 5;
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    scheduler.stop();
    std::cout << "Sensor ECU stopped." << std::endl;
    return 0;
}
