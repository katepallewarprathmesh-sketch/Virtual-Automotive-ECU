#include "SessionManager.h"

namespace ecu {
namespace uds {

SessionManager::SessionManager() : currentSession_(static_cast<uint8_t>(SessionType::Default)) {}

bool SessionManager::setSession(uint8_t subFunction) {
    switch (subFunction) {
    case static_cast<uint8_t>(SessionType::Default):
    case static_cast<uint8_t>(SessionType::Programming):
    case static_cast<uint8_t>(SessionType::Extended):
        currentSession_ = subFunction;
        return true;
    default:
        return false;
    }
}

uint8_t SessionManager::currentSession() const {
    return currentSession_;
}

bool SessionManager::isProgrammingSession() const {
    return currentSession_ == static_cast<uint8_t>(SessionType::Programming);
}

} // namespace uds
} // namespace ecu
