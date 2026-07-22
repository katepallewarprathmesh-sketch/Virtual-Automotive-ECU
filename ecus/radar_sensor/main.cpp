#include <chrono>
#include <filesystem>
#include <fstream>
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
        std::cerr << "Radar sensor secure boot failed: firmware/signature missing" << std::endl;
        return 1;
    }

    const fs::path trustedPublicKeyPath = resolvePath("ecu_public.pem", {"scripts/keys", "../scripts/keys", "../../scripts/keys", "firmware", "../firmware"});
    if (trustedPublicKeyPath.empty()) {
        std::cerr << "Radar sensor secure boot failed: trusted key missing" << std::endl;
        return 1;
    }

    const std::string trustedPublicKeyPem = readTextFile(trustedPublicKeyPath);
    SecureBoot boot(trustedPublicKeyPem);
    std::string error;
    if (!boot.verifyFirmware(firmwarePath.string(), signaturePath.string(), error)) {
        std::cerr << "Radar sensor secure boot failed: " << error << std::endl;
        return 2;
    }

    HsmClient hsmClient("/tmp/ecu_hsm.sock");
    if (!hsmClient.connect()) {
        std::cerr << "Radar sensor failed to connect to HSM: " << hsmClient.lastError() << std::endl;
        return 3;
    }
    if (!hsmClient.provisionSymmetricKey("radar_key", 16)) {
        std::cerr << "Radar sensor failed to provision key: " << hsmClient.lastError() << std::endl;
        return 4;
    }

    CanSocket socket("vcan0");
    if (!socket.isOpen()) {
        std::cerr << "Radar sensor unable to open CAN socket: " << socket.lastError() << std::endl;
        return 5;
    }

    std::uint8_t sequence = 0;
    TaskScheduler scheduler;
    scheduler.addPeriodicTask(200, [&]() {
        AuthenticatedCanMessage message{};
        message.sequence = sequence++;
        message.messageType = 0x02;
        message.data[0] = 0x01;
        message.data[1] = 0x00;
        message.data[2] = 0x14;
        message.data[3] = 0x00;

        std::vector<uint8_t> raw = CanAuthenticator::rawAuthData(message);
        bool success = false;
        std::vector<uint8_t> mac = hsmClient.computeMac("radar_key", raw, success);
        if (!success || mac.size() < 2) {
            std::cerr << "Radar sensor MAC failed" << std::endl;
            return;
        }

        CanFrame frame;
        if (!CanAuthenticator::createAuthenticatedFrame(0x110, message.sequence, message.messageType, {message.data, message.data + 4}, {mac[0], mac[1]}, frame)) {
            std::cerr << "Radar sensor failed to create frame" << std::endl;
            return;
        }
        if (!socket.send(frame)) {
            std::cerr << "Radar sensor failed to send frame" << std::endl;
            return;
        }
        std::cout << "Radar sensor published distance frame seq=" << static_cast<int>(message.sequence) << std::endl;
    }, "radar_publish");

    scheduler.addPeriodicTask(1500, [&]() {
        CanFrame request;
        std::vector<uint8_t> payload = {static_cast<uint8_t>(uds::kDataIdentifierRadarDistance >> 8), static_cast<uint8_t>(uds::kDataIdentifierRadarDistance & 0xFF)};
        if (!uds::encodeRequest(uds::kReadDataByIdentifier, payload, request, 0x7DF)) {
            std::cerr << "Radar sensor failed to encode UDS request" << std::endl;
            return;
        }
        if (!socket.send(request)) {
            std::cerr << "Radar sensor failed to send UDS request" << std::endl;
            return;
        }
    }, "radar_diag");

    scheduler.start();
    std::this_thread::sleep_for(std::chrono::seconds(20));
    scheduler.stop();
    return 0;
}
