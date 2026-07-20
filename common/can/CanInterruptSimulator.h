#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ecu {

struct InterruptEvent {
    std::string name;
    uint32_t priority;
};

class CanInterruptSimulator {
public:
    bool raiseInterrupt(const std::string& name, uint32_t priority);
    std::optional<InterruptEvent> dispatchHighestPriority();
    size_t pendingCount() const;

private:
    std::vector<InterruptEvent> pending_;
};

} // namespace ecu
