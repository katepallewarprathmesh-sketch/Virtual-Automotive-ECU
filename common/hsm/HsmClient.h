#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ecu {

class HsmClient {
public:
    explicit HsmClient(const std::string& socketPath = "/tmp/ecu_hsm.sock");
    ~HsmClient();

    bool connect();
    void disconnect();
    bool isConnected() const;

    bool provisionSymmetricKey(const std::string& keyId, int size = 32);
    std::vector<uint8_t> computeMac(const std::string& keyId, const std::vector<uint8_t>& data, bool& success) const;
    bool verifyMac(const std::string& keyId, const std::vector<uint8_t>& data, const std::vector<uint8_t>& mac, bool& success) const;
    std::string lastError() const;

private:
    std::string socketPath_;
    int fd_;
    mutable std::string lastError_;

    bool sendRequest(const std::string& request) const;
    bool receiveResponse(std::string& response) const;
    static std::string binToHex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> hexToBin(const std::string& hex);
};

} // namespace ecu
