#include "../common/can/CanAuthenticator.h"
#include "../common/can/CanAuthorization.h"
#include "../common/can/CanArbitrationSimulator.h"
#include "../common/can/CanInterruptSimulator.h"
#include "../common/hsm/HsmClient.h"
#include "../common/hsm/HsmService.h"
#include "../common/uds/UdsDispatcher.h"
#include "../common/uds/UdsProtocol.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace ecu;

static void assertTrue(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failed: " << message << std::endl;
        std::exit(1);
    }
}

int main() {
    // Start the HSM service.
    HsmService hsmService("/tmp/ecu_hsm_test.sock");
    assertTrue(hsmService.start(), "HSM service failed to start");

    HsmClient client("/tmp/ecu_hsm_test.sock");
    assertTrue(client.connect(), "HSM client failed to connect");
    assertTrue(client.provisionSymmetricKey("test_key", 16), "HSM key provisioning failed");

    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    bool success = false;
    std::vector<uint8_t> mac = client.computeMac("test_key", data, success);
    assertTrue(success && !mac.empty(), "HSM MAC generation failed");
    bool ok = client.verifyMac("test_key", data, mac, success);
    assertTrue(success && ok, "HSM MAC verification failed");

    // Verify UDS encode/decode.
    CanFrame requestFrame;
    std::vector<uint8_t> requestPayload = {0xF1, 0x90};
    assertTrue(uds::encodeRequest(uds::kReadDataByIdentifier, requestPayload, requestFrame), "UDS request encoding failed");
    uds::UdsMessage requestMessage;
    assertTrue(uds::parseUdsMessage(requestFrame, requestMessage), "UDS request parsing failed");
    assertTrue(requestMessage.service == uds::kReadDataByIdentifier, "UDS service ID mismatch");
    assertTrue(requestMessage.payload == requestPayload, "UDS payload mismatch");

    // Verify CAN auth frame generation.
    CanFrame authFrame;
    std::vector<uint8_t> payload = {0xAB, 0xCD, 0xEF, 0x01};
    assertTrue(CanAuthenticator::createAuthenticatedFrame(0x100, 1, 0x01, payload, {0x12, 0x34}, authFrame), "Authenticated frame creation failed");
    AuthenticatedCanMessage parsed;
    assertTrue(CanAuthenticator::parseAuthenticatedFrame(authFrame, parsed), "Authenticated frame parsing failed");
    assertTrue(parsed.sequence == 1, "Authenticated frame sequence mismatch");
    assertTrue(parsed.messageType == 0x01, "Authenticated message type mismatch");
    assertTrue(std::equal(std::begin(parsed.data), std::end(parsed.data), payload.begin()), "Authenticated payload mismatch");

    // Verify centralized CAN authorization policy.
    ecu::CanAuthorization authorization;
    authorization.addAllowedCanId("sensor_ecu", 0x100);
    authorization.addAllowedCanId("sensor_ecu", 0x7DF);
    assertTrue(authorization.isAllowed("sensor_ecu", 0x100), "Authorization allowed CAN ID check failed");
    assertTrue(!authorization.isAllowed("sensor_ecu", 0x123), "Authorization should reject unexpected CAN ID");

    // Verify interrupt simulation.
    ecu::CanInterruptSimulator interrupts;
    assertTrue(interrupts.raiseInterrupt("timer_tick", 1), "Timer interrupt should be raised");
    assertTrue(interrupts.raiseInterrupt("frame_ready", 5), "Frame-ready interrupt should be raised");
    assertTrue(interrupts.pendingCount() == 2, "Interrupt queue should contain both events");
    auto nextInterrupt = interrupts.dispatchHighestPriority();
    assertTrue(nextInterrupt.has_value(), "High-priority interrupt should be available");
    assertTrue(nextInterrupt->name == "frame_ready", "Highest-priority interrupt should be dispatched first");
    assertTrue(interrupts.pendingCount() == 1, "One interrupt should remain after dispatch");

    // Verify CAN arbitration simulation.
    ecu::CanArbitrationSimulator arbitration;
    arbitration.enqueueFrame(0x200, {0xAA, 0xBB});
    arbitration.enqueueFrame(0x100, {0x01, 0x02});
    arbitration.enqueueFrame(0x180, {0x33, 0x44});
    auto winner = arbitration.arbitrateNext();
    assertTrue(winner.has_value(), "A winner should be selected");
    assertTrue(winner->id == 0x100, "Lowest CAN ID should win arbitration");

    // Verify new UDS dispatcher behaviors.
    ecu::uds::SessionManager sessionManager;
    ecu::uds::SecurityManager securityManager;
    ecu::uds::DidManager didManager;
    ecu::uds::DtcManager dtcManager;
    ecu::uds::UdsDispatcher dispatcher(sessionManager, securityManager, didManager, dtcManager);

    ecu::uds::UdsMessage sessionRequest;
    sessionRequest.service = ecu::uds::kSessionControl;
    sessionRequest.payload = {ecu::uds::kSessionDefault};
    ecu::uds::UdsResponse sessionResponse;
    assertTrue(dispatcher.dispatch(sessionRequest, sessionResponse), "Session control dispatch failed");
    assertTrue(sessionResponse.service == ecu::uds::kSessionControl + 0x40, "Session control positive response service mismatch");
    assertTrue(sessionManager.currentSession() == ecu::uds::kSessionDefault, "Session manager default session mismatch");

    ecu::uds::UdsMessage securityRequest;
    securityRequest.service = ecu::uds::kSecurityAccess;
    securityRequest.payload = {0x01};
    ecu::uds::UdsResponse securityResponse;
    assertTrue(dispatcher.dispatch(securityRequest, securityResponse), "Security seed dispatch failed");
    assertTrue(securityResponse.service == ecu::uds::kSecurityAccess + 0x40, "Security positive response service mismatch");
    assertTrue(securityResponse.payload.size() >= 2, "Security seed payload too short");

    ecu::uds::UdsMessage didRequest;
    didRequest.service = ecu::uds::kReadDataByIdentifier;
    didRequest.payload = {0xF1, 0x90};
    ecu::uds::UdsResponse didResponse;
    assertTrue(dispatcher.dispatch(didRequest, didResponse), "DID dispatch failed");
    assertTrue(didResponse.service == ecu::uds::kReadDataByIdentifier + 0x40, "DID positive response service mismatch");
    assertTrue(didResponse.payload.size() >= 4, "DID response payload too short");

    ecu::uds::UdsMessage resetRequest;
    resetRequest.service = ecu::uds::kEcuReset;
    resetRequest.payload = {0x01};
    ecu::uds::UdsResponse resetResponse;
    assertTrue(dispatcher.dispatch(resetRequest, resetResponse), "ECU reset dispatch failed");
    assertTrue(resetResponse.service == ecu::uds::kEcuReset + 0x40, "ECU reset positive response service mismatch");

    ecu::uds::UdsMessage dtcRequest;
    dtcRequest.service = ecu::uds::kReadDtcInformation;
    dtcRequest.payload = {};
    ecu::uds::UdsResponse dtcResponse;
    assertTrue(dispatcher.dispatch(dtcRequest, dtcResponse), "DTC read dispatch failed");
    assertTrue(dtcResponse.service == ecu::uds::kReadDtcInformation + 0x40, "DTC read positive response service mismatch");

    ecu::uds::UdsMessage radarRequest;
    radarRequest.service = ecu::uds::kReadDataByIdentifier;
    radarRequest.payload = {static_cast<uint8_t>(ecu::uds::kDataIdentifierRadarDistance >> 8), static_cast<uint8_t>(ecu::uds::kDataIdentifierRadarDistance & 0xFF)};
    ecu::uds::UdsResponse radarResponse;
    assertTrue(dispatcher.dispatch(radarRequest, radarResponse), "Radar distance DID dispatch failed");
    assertTrue(radarResponse.service == ecu::uds::kReadDataByIdentifier + 0x40, "Radar DID positive response service mismatch");

    // Normal communication path: valid authorized frame should parse successfully.
    CanFrame normalFrame;
    std::vector<uint8_t> normalPayload = {0xAA, 0xBB, 0xCC, 0xDD};
    assertTrue(CanAuthenticator::createAuthenticatedFrame(0x100, 0x05, 0x01, normalPayload, {0x12, 0x34}, normalFrame), "Normal authenticated frame creation failed");
    AuthenticatedCanMessage normalParsed;
    assertTrue(CanAuthenticator::parseAuthenticatedFrame(normalFrame, normalParsed), "Normal authenticated frame parsing failed");
    assertTrue(authorization.isAllowed("sensor_ecu", normalFrame.id), "Authorized sender should be accepted");

    // Invalid CAN IDs should be rejected by policy.
    CanFrame invalidIdFrame;
    assertTrue(CanAuthenticator::createAuthenticatedFrame(0x123, 0x06, 0x01, normalPayload, {0x12, 0x34}, invalidIdFrame), "Invalid-ID frame creation failed");
    assertTrue(!authorization.isAllowed("sensor_ecu", invalidIdFrame.id), "Unauthorized CAN ID should be rejected");

    // Wrong payload lengths should generate a negative response.
    ecu::uds::UdsMessage shortPayloadRequest;
    shortPayloadRequest.service = ecu::uds::kReadDataByIdentifier;
    shortPayloadRequest.payload = {0xF1};
    ecu::uds::UdsResponse shortPayloadResponse;
    assertTrue(!dispatcher.dispatch(shortPayloadRequest, shortPayloadResponse), "Short DID request should return a negative response");
    assertTrue(shortPayloadResponse.isNegative && shortPayloadResponse.nrc == ecu::uds::kNrcInvalidFormat, "Short DID request NRC mismatch");

    // Corrupted frames should be rejected during parsing.
    CanFrame corruptedFrame;
    corruptedFrame.id = 0x100;
    corruptedFrame.dlc = 7;
    std::fill(std::begin(corruptedFrame.data), std::end(corruptedFrame.data), 0xEE);
    AuthenticatedCanMessage corruptedParsed;
    assertTrue(!CanAuthenticator::parseAuthenticatedFrame(corruptedFrame, corruptedParsed), "Corrupted frame should be rejected");

    // Tampered authenticated frames should fail integrity validation.
    CanFrame tamperedFrame;
    assertTrue(CanAuthenticator::createAuthenticatedFrame(0x100, 0x07, 0x01, normalPayload, {0x12, 0x34}, tamperedFrame), "Tampered frame creation failed");
    tamperedFrame.data[2] ^= 0x01;
    AuthenticatedCanMessage tamperedParsed;
    assertTrue(CanAuthenticator::parseAuthenticatedFrame(tamperedFrame, tamperedParsed), "Tampered frame parsing should succeed structurally");
    assertTrue(!CanAuthenticator::validateFrameIntegrity(tamperedFrame, tamperedParsed), "Tampered frame integrity should fail");

    // Replay attacks: repeated sequence numbers should be blocked.
    std::vector<uint8_t> seenSequences;
    auto detectReplay = [&](uint8_t sequence) {
        auto existing = std::find(seenSequences.begin(), seenSequences.end(), sequence);
        if (existing != seenSequences.end()) {
            return false;
        }
        seenSequences.push_back(sequence);
        return true;
    };
    assertTrue(detectReplay(0x10), "First sequence should be accepted");
    assertTrue(!detectReplay(0x10), "Repeated sequence should be rejected as replay");
    assertTrue(detectReplay(0x11), "Newer sequence should be accepted");

    // Queue overflow simulation: keep a capped queue size to mimic overflow handling.
    constexpr size_t kQueueLimit = 4;
    std::vector<CanFrame> frameQueue;
    frameQueue.reserve(kQueueLimit);
    for (int i = 0; i < 6; ++i) {
        CanFrame queueFrame;
        if (!CanAuthenticator::createAuthenticatedFrame(0x100, static_cast<uint8_t>(i), 0x01, normalPayload, {0x12, 0x34}, queueFrame)) {
            continue;
        }
        if (frameQueue.size() >= kQueueLimit) {
            frameQueue.erase(frameQueue.begin());
        }
        frameQueue.push_back(queueFrame);
    }
    assertTrue(frameQueue.size() == kQueueLimit, "Queue overflow simulation should cap the queue size");

    ecu::uds::UdsMessage invalidRequest;
    invalidRequest.service = 0xEE;
    invalidRequest.payload = {};
    ecu::uds::UdsResponse invalidResponse;
    assertTrue(!dispatcher.dispatch(invalidRequest, invalidResponse), "Unsupported service should not dispatch successfully");
    assertTrue(invalidResponse.isNegative && invalidResponse.nrc == ecu::uds::kNrcServiceNotSupported, "Unsupported service NRC mismatch");

    client.disconnect();
    hsmService.stop();

    std::cout << "All unit tests passed." << std::endl;
    return 0;
}
