#include "CanArbitrationSimulator.h"
#include <algorithm>

namespace ecu {

void CanArbitrationSimulator::enqueueFrame(uint32_t id, const std::vector<uint8_t>& payload) {
    pending_.push_back({id, payload});
}

std::optional<ArbitrationFrame> CanArbitrationSimulator::arbitrateNext() {
    if (pending_.empty()) {
        return std::nullopt;
    }
    auto winner = std::min_element(pending_.begin(), pending_.end(), [](const ArbitrationFrame& lhs, const ArbitrationFrame& rhs) {
        return lhs.id < rhs.id;
    });
    ArbitrationFrame result = *winner;
    pending_.erase(winner);
    return result;
}

} // namespace ecu
