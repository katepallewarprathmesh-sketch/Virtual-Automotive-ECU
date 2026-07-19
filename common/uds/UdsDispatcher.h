#pragma once

#include "DidManager.h"
#include "DtcManager.h"
#include "SecurityManager.h"
#include "SessionManager.h"
#include "UdsProtocol.h"
#include <cstdint>
#include <vector>

namespace ecu {
namespace uds {

struct UdsResponse {
    uint8_t service = 0;
    std::vector<uint8_t> payload;
    bool isNegative = false;
    uint8_t nrc = 0;
};

class UdsDispatcher {
public:
    UdsDispatcher(SessionManager& sessionManager,
                  SecurityManager& securityManager,
                  DidManager& didManager,
                  DtcManager& dtcManager);

    bool dispatch(const UdsMessage& request, UdsResponse& response) const;

private:
    SessionManager& sessionManager_;
    SecurityManager& securityManager_;
    DidManager& didManager_;
    DtcManager& dtcManager_;
};

} // namespace uds
} // namespace ecu
