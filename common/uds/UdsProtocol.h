#pragma once

#include "../can/CanSocket.h"
#include <cstdint>
#include <vector>

namespace ecu {

namespace uds {

static constexpr uint8_t kSessionControl = 0x10;
static constexpr uint8_t kEcuReset = 0x11;
static constexpr uint8_t kReadDtcInformation = 0x19;
static constexpr uint8_t kReadDataByIdentifier = 0x22;
static constexpr uint8_t kSecurityAccess = 0x27;
static constexpr uint8_t kTesterPresent = 0x3E;

static constexpr uint8_t kSessionDefault = 0x01;
static constexpr uint8_t kSessionProgramming = 0x02;
static constexpr uint8_t kSessionExtended = 0x03;

static constexpr uint8_t kReadDataByIdentifierResponse = 0x62;
static constexpr uint8_t kNegativeResponse = 0x7F;
static constexpr uint16_t kDataIdentifierVehicleSpeed = 0xF190;

static constexpr uint8_t kNrcServiceNotSupported = 0x11;
static constexpr uint8_t kNrcInvalidFormat = 0x13;
static constexpr uint8_t kNrcRequestOutOfRange = 0x31;
static constexpr uint8_t kNrcSecurityDenied = 0x33;

struct UdsMessage {
    uint8_t service = 0;
    std::vector<uint8_t> payload;
};

bool encodeRequest(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId = 0x7DF);
bool encodeResponse(uint8_t service, const std::vector<uint8_t>& payload, CanFrame& frame, uint32_t canId = 0x7E8);
bool parseUdsMessage(const CanFrame& frame, UdsMessage& message);

} // namespace uds

} // namespace ecu
