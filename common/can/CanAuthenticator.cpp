#include "CanAuthenticator.h"
#include <algorithm>
#include <cstring>

namespace ecu {

bool CanAuthenticator::createAuthenticatedFrame(uint32_t id,
                                                uint8_t sequence,
                                                uint8_t messageType,
                                                const std::vector<uint8_t>& payload,
                                                const std::vector<uint8_t>& mac,
                                                CanFrame& frame) {
    if (payload.size() > 4 || mac.size() < 2) {
        return false;
    }
    frame.id = id;
    frame.dlc = 8;
    std::memset(frame.data, 0, sizeof(frame.data));
    frame.data[0] = sequence;
    frame.data[1] = messageType;
    std::memcpy(frame.data + 2, payload.data(), payload.size());
    frame.data[6] = mac[0];
    frame.data[7] = mac[1];
    return true;
}

bool CanAuthenticator::parseAuthenticatedFrame(const CanFrame& frame,
                                               AuthenticatedCanMessage& message) {
    if (frame.dlc != 8) {
        return false;
    }
    message.sequence = frame.data[0];
    message.messageType = frame.data[1];
    std::memcpy(message.data, frame.data + 2, sizeof(message.data));
    message.mac = static_cast<uint16_t>(frame.data[6]) << 8 | frame.data[7];
    message.crc = 0;
    return true;
}

std::vector<uint8_t> CanAuthenticator::rawAuthData(const AuthenticatedCanMessage& message) {
    std::vector<uint8_t> raw;
    raw.reserve(6);
    raw.push_back(message.sequence);
    raw.push_back(message.messageType);
    raw.insert(raw.end(), std::begin(message.data), std::end(message.data));
    return raw;
}

uint8_t CanAuthenticator::computeCrc(const std::vector<uint8_t>& payload) {
    uint8_t crc = 0xFF;
    for (uint8_t byte : payload) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if ((crc & 0x80U) != 0U) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07U);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool CanAuthenticator::validateFrameIntegrity(const CanFrame& frame, const AuthenticatedCanMessage& message) {
    if (frame.dlc != 8) {
        return false;
    }
    std::vector<uint8_t> payload;
    payload.push_back(message.sequence);
    payload.push_back(message.messageType);
    payload.insert(payload.end(), std::begin(message.data), std::end(message.data));
    const uint8_t expectedCrc = computeCrc(payload);
    return expectedCrc == message.crc;
}

} // namespace ecu
