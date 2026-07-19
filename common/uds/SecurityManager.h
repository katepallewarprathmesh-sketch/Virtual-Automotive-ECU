#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ecu {
namespace uds {

class SecurityManager {
public:
    SecurityManager();
    bool handleSecurityAccess(uint8_t subFunction, std::vector<uint8_t>& responsePayload) const;
    bool validateKey(uint8_t subFunction, const std::vector<uint8_t>& key) const;

private:
    mutable bool securityUnlocked_;
};

} // namespace uds
} // namespace ecu
