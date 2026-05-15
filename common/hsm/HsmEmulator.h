#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ecu {

class HsmEmulator {
public:
    HsmEmulator();
    bool generateRsaKey(const std::string& keyId, int bits = 2048);
    std::vector<uint8_t> sign(const std::string& keyId, const std::vector<uint8_t>& data, bool& success) const;
    bool verify(const std::string& keyId,
                const std::vector<uint8_t>& data,
                const std::vector<uint8_t>& signature) const;

    bool generateSymmetricKey(const std::string& keyId, int size = 32);
    std::vector<uint8_t> computeMac(const std::string& keyId, const std::vector<uint8_t>& data, bool& success) const;
    bool verifyMac(const std::string& keyId,
                   const std::vector<uint8_t>& data,
                   const std::vector<uint8_t>& mac) const;

private:
    std::map<std::string, std::string> publicKeys_;
    std::map<std::string, std::string> privateKeys_;
    std::map<std::string, std::vector<uint8_t>> symmetricKeys_;
};

} // namespace ecu
