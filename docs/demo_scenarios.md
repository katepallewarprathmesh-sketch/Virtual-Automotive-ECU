# Demo Scenarios

## Normal Operation

1. Configure the virtual CAN bus with `scripts/setup_vcan.sh`.
2. Generate the firmware and signature with `scripts/generate_firmware.sh`.
3. Build the project and start `ecus/gateway/gateway_ecu`.
4. Start `ecus/sensor/sensor_ecu` and observe the simulated CAN traffic.

## Secure Boot Demo

- The sensor ECU verifies `firmware/ecu_firmware.bin` using the embedded public key.
- If the signature is invalid, the ECU refuses to start.
- This simulates an ECU bootloader verifying firmware authenticity.

## Attack Simulation Ideas

- Modify `firmware/ecu_firmware.bin` without updating `firmware/ecu_firmware.sig` to show boot failure.
- Record CAN frames and replay them with a Python script to simulate a replay attack.
- Add a CAN message authentication tag to protect the sensor payload from injection.
