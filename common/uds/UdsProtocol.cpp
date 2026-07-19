#include "UdsProtocol.h"
#include <cstring>

namespace ecu {
    namespace uds {

    bool encodeRequest(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId) {
        if (payload.size() > 7) {
            return false;
        }
        frame.id = canId;
        frame.dlc = static_cast<uint8_t>(1 + payload.size());
        std::memset(frame.data, 0, sizeof(frame.data));
        frame.data[0] = service;
        std::memcpy(frame.data + 1, payload.data(), payload.size());
        return true;
    }

    bool encodeResponse(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId) {
        if (payload.size() > 7) {
            return false;
        }
        frame.id = canId;
        frame.dlc = static_cast<uint8_t>(1 + payload.size());
        std::memset(frame.data, 0, sizeof(frame.data));
        frame.data[0] = service;
        std::memcpy(frame.data + 1, payload.data(), payload.size());
        return true;
    }

    bool parseUdsMessage(const CanFrame& frame, UdsMessage& message) {
        if (frame.dlc < 1) {
            return false;
        }
        message.service = frame.data[0];
        message.payload.assign(frame.data + 1, frame.data + frame.dlc);
        return true;
    }

    } // namespace uds
} // namespace ecu
