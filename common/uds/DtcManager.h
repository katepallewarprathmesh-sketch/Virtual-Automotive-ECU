#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ecu {
namespace uds {

struct DtcEntry {
    std::string code;
    uint8_t status;
    uint32_t timestamp;
};

class DtcManager {
public:
    DtcManager();
    void addDtc(const std::string& code, uint8_t status, uint32_t timestamp);
    std::vector<DtcEntry> listDtc() const;
    bool clearDtc();

private:
    std::vector<DtcEntry> dtcs_;
};

} // namespace uds
} // namespace ecu
