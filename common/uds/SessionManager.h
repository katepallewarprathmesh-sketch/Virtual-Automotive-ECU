#pragma once

#include <cstdint>

namespace ecu {
namespace uds {

enum class SessionType : uint8_t {
    Default = 0x01,
    Programming = 0x02,
    Extended = 0x03,
};

class SessionManager {
public:
    SessionManager();
    bool setSession(uint8_t subFunction);
    uint8_t currentSession() const;
    bool isProgrammingSession() const;

private:
    uint8_t currentSession_;
};

} // namespace uds
} // namespace ecu
