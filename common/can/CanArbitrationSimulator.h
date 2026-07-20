#pragma once

#include "CanSocket.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace ecu {

struct ArbitrationFrame {
    uint32_t id;
    std::vector<uint8_t> payload;
};

class CanArbitrationSimulator {
public:
    void enqueueFrame(uint32_t id, const std::vector<uint8_t>& payload);
    std::optional<ArbitrationFrame> arbitrateNext();

private:
    std::vector<ArbitrationFrame> pending_;
};

} // namespace ecu
