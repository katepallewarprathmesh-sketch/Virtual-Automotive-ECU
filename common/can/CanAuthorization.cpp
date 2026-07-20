#include "CanAuthorization.h"

namespace ecu {

CanAuthorization::CanAuthorization(CanAuthorizationPolicy policy)
    : policy_(std::move(policy)) {}

bool CanAuthorization::isAllowed(const std::string& sender, uint32_t canId) const {
    auto senderIt = policy_.allowedCanIdsBySender.find(sender);
    if (senderIt == policy_.allowedCanIdsBySender.end()) {
        return false;
    }
    const auto& allowedIds = senderIt->second;
    return allowedIds.find(canId) != allowedIds.end();
}

void CanAuthorization::addAllowedCanId(const std::string& sender, uint32_t canId) {
    policy_.allowedCanIdsBySender[sender].insert(canId);
}

void CanAuthorization::removeAllowedCanId(const std::string& sender, uint32_t canId) {
    auto senderIt = policy_.allowedCanIdsBySender.find(sender);
    if (senderIt == policy_.allowedCanIdsBySender.end()) {
        return;
    }
    senderIt->second.erase(canId);
    if (senderIt->second.empty()) {
        policy_.allowedCanIdsBySender.erase(senderIt);
    }
}

void CanAuthorization::setPolicy(const CanAuthorizationPolicy& policy) {
    policy_ = policy;
}

const CanAuthorizationPolicy& CanAuthorization::policy() const {
    return policy_;
}

} // namespace ecu
