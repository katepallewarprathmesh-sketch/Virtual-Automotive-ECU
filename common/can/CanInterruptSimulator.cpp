#include "CanInterruptSimulator.h"
#include <algorithm>

namespace ecu {

bool CanInterruptSimulator::raiseInterrupt(const std::string& name, uint32_t priority) {
    pending_.push_back({name, priority});
    return true;
}

std::optional<InterruptEvent> CanInterruptSimulator::dispatchHighestPriority() {
    if (pending_.empty()) {
        return std::nullopt;
    }
    auto best = std::max_element(pending_.begin(), pending_.end(), [](const InterruptEvent& lhs, const InterruptEvent& rhs) {
        return lhs.priority < rhs.priority;
    });
    InterruptEvent result = *best;
    pending_.erase(best);
    return result;
}

size_t CanInterruptSimulator::pendingCount() const {
    return pending_.size();
}

} // namespace ecu
