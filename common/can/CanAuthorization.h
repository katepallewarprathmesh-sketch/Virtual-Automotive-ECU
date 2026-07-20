#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>

namespace ecu {

struct CanAuthorizationPolicy {
    std::unordered_map<std::string, std::set<uint32_t>> allowedCanIdsBySender;
};

class CanAuthorization {
public:
    explicit CanAuthorization(CanAuthorizationPolicy policy = {});

    bool isAllowed(const std::string& sender, uint32_t canId) const;
    void addAllowedCanId(const std::string& sender, uint32_t canId);
    void removeAllowedCanId(const std::string& sender, uint32_t canId);
    void setPolicy(const CanAuthorizationPolicy& policy);
    const CanAuthorizationPolicy& policy() const;

private:
    CanAuthorizationPolicy policy_;
};

} // namespace ecu
