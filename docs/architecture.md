# Architecture Overview

This repository contains a starter implementation for a virtual automotive ECU simulator.

## Components

- `common/can`: SocketCAN wrapper and CAN frame abstraction.
- `common/crypto`: Secure Boot simulation using SHA256 and RSA signature verification.
- `common/hsm`: Simple HSM emulator for in-memory key management and signing.
- `ecus/sensor`: A sensor ECU process that performs Secure Boot and sends CAN frames.
- `ecus/gateway`: A gateway ECU process that listens on the CAN bus and logs traffic.
- `scripts`: Helper scripts for creating a virtual CAN interface and generating firmware/signature files.

## Runtime Flow

1. Sensor ECU performs Secure Boot validation during startup.
2. If validation succeeds, it transmits periodic CAN frames on `vcan0`.
3. Gateway ECU receives CAN frames and prints them.

## Extension Ideas

- Add RTOS-style task scheduling using threads, condition variables, and timerfd.
- Implement a shared `HsmEmulator` service for signing and verification across ECUs.
- Expand the CAN protocol with UDS or application-layer authentication.
- Add attack simulations in Python for injection, replay, and unauthorized firmware updates.
