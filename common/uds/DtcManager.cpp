#include "DtcManager.h"

namespace ecu {
namespace uds {

DtcManager::DtcManager() {}

void DtcManager::addDtc(const std::string& code, uint8_t status, uint32_t timestamp) {
    dtcs_.push_back({code, status, timestamp});
}

std::vector<DtcEntry> DtcManager::listDtc() const {
    return dtcs_;
}

bool DtcManager::clearDtc() {
    dtcs_.clear();
    return true;
}

} // namespace uds
} // namespace ecu
