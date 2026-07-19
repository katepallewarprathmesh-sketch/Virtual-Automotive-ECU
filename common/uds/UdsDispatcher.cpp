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
        if (request.payload.size() != 1) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        const uint8_t subFunction = request.payload[0];
        if (!sessionManager_.setSession(subFunction)) {
            return makeNegative(kNrcRequestOutOfRange, reqSid);
        }
        return makePositive(kSessionControl, {subFunction});
    }
    case kEcuReset: {
        if (request.payload.size() != 1) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        if (request.payload[0] != 0x01) {
            return makeNegative(kNrcRequestOutOfRange, reqSid);
        }
        return makePositive(kEcuReset, {0x01});
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
        if (payload.size() > 7) {
            payload.resize(7);
        }
        return makePositive(kReadDataByIdentifier, payload);
    }
    case kSecurityAccess: {
        if (request.payload.empty()) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        const uint8_t subFunction = request.payload[0];
        std::vector<uint8_t> payload;
        if (subFunction == 0x01) {
            if (!securityManager_.handleSecurityAccess(0x01, payload)) {
                return makeNegative(kNrcSecurityDenied, reqSid);
            }
            return makePositive(kSecurityAccess, payload);
        }
        if (subFunction == 0x02) {
            std::vector<uint8_t> key = request.payload.size() > 1 ? std::vector<uint8_t>(request.payload.begin() + 1, request.payload.end()) : std::vector<uint8_t>{};
            if (!securityManager_.validateKey(0x02, key)) {
                return makeNegative(kNrcSecurityDenied, reqSid);
            }
            payload = {0x00};
            return makePositive(kSecurityAccess, payload);
        }
        return makeNegative(kNrcRequestOutOfRange, reqSid);
    }
    case kReadDtcInformation: {
        if (!request.payload.empty()) {
            return makeNegative(kNrcInvalidFormat, reqSid);
        }
        auto entries = dtcManager_.listDtc();
        std::vector<uint8_t> payload;
        if (entries.empty()) {
            payload = {0x00};
        } else {
            const auto& first = entries.front();
            payload.push_back(static_cast<uint8_t>(entries.size()));
            payload.push_back(static_cast<uint8_t>(first.code.size()));
            for (char c : first.code) {
                payload.push_back(static_cast<uint8_t>(c));
            }
            payload.push_back(first.status);
            payload.push_back(static_cast<uint8_t>(first.timestamp & 0xFF));
            if (payload.size() > 7) {
                payload.resize(7);
            }
        }
        return makePositive(kReadDtcInformation, payload);
    }
    case kTesterPresent: {
        return makePositive(kTesterPresent, {0x00});
    }
    default:
        return makeNegative(kNrcServiceNotSupported, reqSid);
    }
}

} // namespace uds
} // namespace ecu
