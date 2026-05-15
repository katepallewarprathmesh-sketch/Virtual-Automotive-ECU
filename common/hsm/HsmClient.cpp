#include "HsmClient.h"
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cctype>
#include <iostream>

namespace ecu {

HsmClient::HsmClient(const std::string& socketPath)
    : socketPath_(socketPath), fd_(-1) {
}

HsmClient::~HsmClient() {
    disconnect();
}

bool HsmClient::connect() {
    if (fd_ >= 0) {
        return true;
    }
    fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_ < 0) {
        lastError_ = "unable to create socket";
        return false;
    }
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        lastError_ = "unable to connect to HSM service";
        close(fd_);
        fd_ = -1;
        return false;
    }
    return true;
}

void HsmClient::disconnect() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool HsmClient::isConnected() const {
    return fd_ >= 0;
}

bool HsmClient::provisionSymmetricKey(const std::string& keyId, int size) {
    if (!isConnected()) {
        lastError_ = "not connected";
        return false;
    }
    std::string request = "PROVISION_SYM|" + keyId + "|" + std::to_string(size);
    if (!sendRequest(request)) {
        return false;
    }
    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }
    if (response.rfind("OK", 0) == 0) {
        return true;
    }
    lastError_ = response;
    return false;
}

std::vector<uint8_t> HsmClient::computeMac(const std::string& keyId, const std::vector<uint8_t>& data, bool& success) const {
    success = false;
    if (!isConnected()) {
        lastError_ = "not connected";
        return {};
    }
    std::string request = "MAC|" + keyId + "|" + binToHex(data);
    if (!sendRequest(request)) {
        return {};
    }
    std::string response;
    if (!receiveResponse(response)) {
        return {};
    }
    if (response.rfind("OK|", 0) != 0) {
        lastError_ = response;
        return {};
    }
    std::string hex = response.substr(3);
    std::vector<uint8_t> mac = hexToBin(hex);
    if (mac.empty()) {
        lastError_ = "invalid mac response";
        return {};
    }
    success = true;
    return mac;
}

bool HsmClient::verifyMac(const std::string& keyId, const std::vector<uint8_t>& data, const std::vector<uint8_t>& mac, bool& success) const {
    success = false;
    if (!isConnected()) {
        lastError_ = "not connected";
        return false;
    }
    std::string request = "VERIFY_MAC|" + keyId + "|" + binToHex(data) + "|" + binToHex(mac);
    if (!sendRequest(request)) {
        return false;
    }
    std::string response;
    if (!receiveResponse(response)) {
        return false;
    }
    if (response.rfind("OK|", 0) != 0) {
        lastError_ = response;
        return false;
    }
    success = response.substr(3) == "1";
    return success;
}

std::string HsmClient::lastError() const {
    return lastError_;
}

bool HsmClient::sendRequest(const std::string& request) const {
    ssize_t written = write(fd_, request.c_str(), request.size());
    if (written != static_cast<ssize_t>(request.size())) {
        lastError_ = "unable to send request";
        return false;
    }
    if (write(fd_, "\n", 1) != 1) {
        lastError_ = "unable to send newline";
        return false;
    }
    return true;
}

bool HsmClient::receiveResponse(std::string& response) const {
    response.clear();
    char c;
    while (true) {
        ssize_t n = read(fd_, &c, 1);
        if (n <= 0) {
            lastError_ = "connection closed";
            return false;
        }
        if (c == '\n') {
            break;
        }
        response.push_back(c);
    }
    return true;
}

std::string HsmClient::binToHex(const std::vector<uint8_t>& data) {
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(data.size() * 2);
    for (uint8_t byte : data) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0xF]);
    }
    return result;
}

std::vector<uint8_t> HsmClient::hexToBin(const std::string& hex) {
    std::vector<uint8_t> out;
    if (hex.size() % 2 != 0) {
        return out;
    }
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto toNibble = [](char c) {
            if (std::isdigit(c)) return static_cast<uint8_t>(c - '0');
            return static_cast<uint8_t>(std::toupper(c) - 'A' + 10);
        };
        uint8_t high = toNibble(hex[i]);
        uint8_t low = toNibble(hex[i + 1]);
        out.push_back((high << 4) | low);
    }
    return out;
}

} // namespace ecu
