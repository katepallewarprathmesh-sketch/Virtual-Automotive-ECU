#include "SecurityManager.h"
#include <algorithm>

namespace ecu {
namespace uds {

SecurityManager::SecurityManager() : securityUnlocked_(false) {}

bool SecurityManager::handleSecurityAccess(uint8_t subFunction, std::vector<uint8_t>& responsePayload) const {
    if (subFunction == 0x01) {
        responsePayload = {0x12, 0x34};
        return true;
    }
    if (subFunction == 0x02) {
        responsePayload = {0x00};
        return true;
    }
    return false;
}

bool SecurityManager::validateKey(uint8_t subFunction, const std::vector<uint8_t>& key) const {
    (void)subFunction;
    return key.size() >= 2 && key[0] == 0x12 && key[1] == 0x34;
}

} // namespace uds
} // namespace ecu
