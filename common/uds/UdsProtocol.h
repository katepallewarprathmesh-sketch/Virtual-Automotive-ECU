#pragma once

#include "../can/CanSocket.h"
#include <cstdint>
#include <vector>

namespace ecu {

namespace uds {

static constexpr uint8_t kReadDataByIdentifier = 0x22;
static constexpr uint8_t kReadDataByIdentifierResponse = 0x62;
static constexpr uint16_t kDataIdentifierVehicleSpeed = 0xF190;

struct UdsMessage {
    uint8_t service;
    std::vector<uint8_t> payload;
};

bool encodeRequest(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId = 0x7DF);
bool encodeResponse(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId = 0x7E8);
bool parseUdsMessage(const CanFrame& frame, UdsMessage& message);

} // namespace uds

} // namespace ecu
