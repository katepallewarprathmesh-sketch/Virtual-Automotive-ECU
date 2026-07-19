#include "../common/can/CanAuthenticator.h"
#include "../common/hsm/HsmClient.h"
#include "../common/hsm/HsmService.h"
#include "../common/uds/UdsDispatcher.h"
#include "../common/uds/UdsProtocol.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

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
