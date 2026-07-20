#pragma once

#include "CanSocket.h"
#include <cstdint>
#include <vector>

namespace ecu {

struct AuthenticatedCanMessage {
    uint8_t sequence;
    uint8_t messageType;
    uint8_t data[4];
    uint16_t mac;
    uint8_t crc;
};

class CanAuthenticator {
public:
    static bool createAuthenticatedFrame(uint32_t id,
                                         uint8_t sequence,
                                         uint8_t messageType,
                                         const std::vector<uint8_t>& payload,
                                         const std::vector<uint8_t>& mac,
                                         CanFrame& frame);

    static bool parseAuthenticatedFrame(const CanFrame& frame,
                                        AuthenticatedCanMessage& message);

    static std::vector<uint8_t> rawAuthData(const AuthenticatedCanMessage& message);
    static uint8_t computeCrc(const std::vector<uint8_t>& payload);
    static bool validateFrameIntegrity(const CanFrame& frame, const AuthenticatedCanMessage& message);
};

} // namespace ecu
