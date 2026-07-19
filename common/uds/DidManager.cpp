#include "DidManager.h"

namespace ecu {
namespace uds {

DidManager::DidManager() {
    setDid(0xF190, {0x09, 0xD0});
    setDid(0xF220, {0x00, 0x14});
}

bool DidManager::readDid(uint16_t did, std::vector<uint8_t>& value) const {
    auto it = didDatabase_.find(did);
    if (it == didDatabase_.end()) {
        return false;
    }
    value = it->second;
    return true;
}

void DidManager::setDid(uint16_t did, const std::vector<uint8_t>& value) {
    didDatabase_[did] = value;
}

} // namespace uds
} // namespace ecu
