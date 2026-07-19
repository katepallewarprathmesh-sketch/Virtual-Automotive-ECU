#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ecu {
namespace uds {

class DidManager {
public:
    DidManager();
    bool readDid(uint16_t did, std::vector<uint8_t>& value) const;
    void setDid(uint16_t did, const std::vector<uint8_t>& value);

private:
    std::unordered_map<uint16_t, std::vector<uint8_t>> didDatabase_;
};

} // namespace uds
} // namespace ecu
