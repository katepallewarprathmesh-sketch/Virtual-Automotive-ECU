#include "UdsDispatcher.h"
#include <algorithm>
#include <iostream>

namespace ecu {
namespace uds {

UdsDispatcher::UdsDispatcher(SessionManager& sessionManager,
                             SecurityManager& securityManager,
                             DidManager& didManager,
                             DtcManager& dtcManager)
    : sessionManager_(sessionManager),
      securityManager_(securityManager),
      didManager_(didManager),
      dtcManager_(dtcManager) {}

bool UdsDispatcher::dispatch(const UdsMessage& request, UdsResponse& response) const {
    response = {};
    const uint8_t reqSid = request.service;

    auto makePositive = [&](uint8_t service, const std::vector<uint8_t>& payload) {
        response.service = service + 0x40;
        response.payload = payload;
        response.isNegative = false;
        return true;
    };

    auto makeNegative = [&](uint8_t nrc, uint8_t reqSidForError) {
        response.service = kNegativeResponse;
        response.payload = {reqSidForError, nrc};
        response.isNegative = true;
        response.nrc = nrc;
        return false;
    };

    switch (reqSid) {
    case kSessionControl: {
        if (request.payload.empty()) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        if (!sessionManager_.setSession(request.payload[0])) {
            return makeNegative(kNrcRequestOutOfRange, reqSid);
        }
        return makePositive(kSessionControl, {request.payload[0]});
    }
    case kReadDataByIdentifier: {
        if (request.payload.size() != 2) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        uint16_t did = static_cast<uint16_t>(request.payload[0]) << 8 | request.payload[1];
        std::vector<uint8_t> value;
        if (!didManager_.readDid(did, value)) {
            return makeNegative(kNrcRequestOutOfRange, reqSid);
        }
        std::vector<uint8_t> payload = {static_cast<uint8_t>(did >> 8), static_cast<uint8_t>(did & 0xFF)};
        payload.insert(payload.end(), value.begin(), value.end());
        return makePositive(kReadDataByIdentifier, payload);
    }
    case kSecurityAccess: {
        if (request.payload.empty()) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        std::vector<uint8_t> payload;
        if (request.payload[0] == 0x01) {
            if (!securityManager_.handleSecurityAccess(0x01, payload)) {
                return makeNegative(kNrcSecurityDenied, reqSid);
            }
            return makePositive(kSecurityAccess, payload);
        }
        if (request.payload[0] == 0x02) {
            if (!securityManager_.validateKey(0x02, request.payload.size() > 1 ? std::vector<uint8_t>(request.payload.begin() + 1, request.payload.end()) : std::vector<uint8_t>{})) {
                return makeNegative(kNrcSecurityDenied, reqSid);
            }
            payload = {0x00};
            return makePositive(kSecurityAccess, payload);
        }
        return makeNegative(kNrcRequestOutOfRange, reqSid);
    }
    case kReadDtcInformation: {
        auto entries = dtcManager_.listDtc();
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(entries.size()));
        for (const auto& entry : entries) {
            payload.push_back(static_cast<uint8_t>(entry.code.size()));
            for (unsigned char c : entry.code) {
                payload.push_back(static_cast<uint8_t>(c));
            }
            payload.push_back(entry.status);
        }
        return makePositive(kReadDtcInformation, payload);
    }
    case kTesterPresent: {
        return makePositive(kTesterPresent, {0x00});
    }
    case kEcuReset: {
        return makeNegative(kNrcServiceNotSupported, reqSid);
    }
    default:
        return makeNegative(kNrcServiceNotSupported, reqSid);
    }
}

} // namespace uds
} // namespace ecu
