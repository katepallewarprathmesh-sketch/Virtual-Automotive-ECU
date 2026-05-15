#include "HsmService.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <iostream>

namespace ecu {

static std::string binToHex(const std::vector<uint8_t>& data) {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(data.size() * 2);
    for (uint8_t byte : data) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0xF]);
    }
    return result;
}

static std::vector<uint8_t> hexToBin(const std::string& hex) {
    std::vector<uint8_t> out;
    if (hex.size() % 2 != 0) {
        return out;
    }
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t high = static_cast<uint8_t>(std::isdigit(hex[i]) ? hex[i] - '0' : std::toupper(hex[i]) - 'A' + 10);
        uint8_t low = static_cast<uint8_t>(std::isdigit(hex[i + 1]) ? hex[i + 1] - '0' : std::toupper(hex[i + 1]) - 'A' + 10);
        out.push_back((high << 4) | low);
    }
    return out;
}

static std::string readLine(int fd) {
    std::string line;
    while (true) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) {
            break;
        }
        if (c == '\n') {
            break;
        }
        line.push_back(c);
    }
    return line;
}

static bool writeLine(int fd, const std::string& line) {
    std::string output = line + "\n";
    return write(fd, output.data(), output.size()) == static_cast<ssize_t>(output.size());
}

HsmService::HsmService(const std::string& socketPath)
    : socketPath_(socketPath), listenFd_(-1), running_(false) {
}

HsmService::~HsmService() {
    stop();
}

bool HsmService::start() {
    if (running_) {
        return false;
    }

    unlink(socketPath_.c_str());
    listenFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listenFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    chmod(socketPath_.c_str(), 0660);

    if (listen(listenFd_, 4) < 0) {
        close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    running_ = true;
    acceptThread_ = std::thread(&HsmService::run, this);
    return true;
}

void HsmService::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    unlink(socketPath_.c_str());
}

void HsmService::run() {
    while (running_) {
        int clientFd = accept(listenFd_, nullptr, nullptr);
        if (clientFd < 0) {
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

void HsmService::handleClient(int clientFd) {
    while (running_) {
        std::string request = readLine(clientFd);
        if (request.empty()) {
            break;
        }
        std::string response = processRequest(request);
        writeLine(clientFd, response);
    }
}

std::string HsmService::processRequest(const std::string& request) {
    auto split = [&](const std::string& input) {
        std::vector<std::string> tokens;
        size_t start = 0;
        while (start < input.size()) {
            size_t pos = input.find('|', start);
            if (pos == std::string::npos) {
                tokens.push_back(input.substr(start));
                break;
            }
            tokens.push_back(input.substr(start, pos - start));
            start = pos + 1;
        }
        return tokens;
    };

    std::vector<std::string> parts = split(request);
    if (parts.empty()) {
        return "ERROR|invalid";
    }

    const std::string& command = parts[0];
    if (command == "PROVISION_SYM") {
        if (parts.size() < 3) {
            return "ERROR|missing args";
        }
        const std::string& keyId = parts[1];
        int size = std::stoi(parts[2]);
        if (!emulator_.generateSymmetricKey(keyId, size)) {
            return "ERROR|provision failed";
        }
        return "OK";
    }
    if (command == "MAC") {
        if (parts.size() < 3) {
            return "ERROR|missing args";
        }
        const std::string& keyId = parts[1];
        std::vector<uint8_t> data = hexToBin(parts[2]);
        bool success = false;
        std::vector<uint8_t> mac = emulator_.computeMac(keyId, data, success);
        if (!success) {
            return "ERROR|mac failed";
        }
        return std::string("OK|") + binToHex(mac);
    }
    if (command == "VERIFY_MAC") {
        if (parts.size() < 4) {
            return "ERROR|missing args";
        }
        const std::string& keyId = parts[1];
        std::vector<uint8_t> data = hexToBin(parts[2]);
        std::vector<uint8_t> mac = hexToBin(parts[3]);
        bool ok = emulator_.verifyMac(keyId, data, mac);
        return std::string("OK|") + (ok ? "1" : "0");
    }
    return "ERROR|unknown";
}

} // namespace ecu
