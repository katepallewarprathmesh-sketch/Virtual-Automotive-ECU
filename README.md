# Virtual Automotive ECU Simulator

A starter implementation of a virtual automotive ECU simulator with:

- SocketCAN-based CAN communication (`vcan0`)
- Secure Boot simulation with RSA signature verification
- HSM emulator skeleton for key management and signing
- Separate ECU executables for sensor and gateway behavior
- Build system using CMake

## Getting Started

### Prepare the environment

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Or run:

```bash
./scripts/setup_vcan.sh
```

### Generate firmware and signature

```bash
chmod +x scripts/generate_firmware.sh
./scripts/generate_firmware.sh
```

### Build the project

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Run the simulation

Start the shared HSM service first:

```bash
./build/ecus/hsm_service/hsm_service
```

Then start the gateway ECU:

```bash
./build/ecus/gateway/gateway_ecu
```

Finally start the sensor ECU:

```bash
./build/ecus/sensor/sensor_ecu
```

### Run the unit tests

```bash
cd build
ctest --output-on-failure
```

## Project Structure

- `common/can`: CAN bus abstraction and SocketCAN wrapper
- `common/crypto`: Secure Boot implementation with SHA256 and RSA
- `common/hsm`: HSM emulator skeleton for key generation, signing, and verification
- `ecus/sensor`: Sensor ECU with secure firmware boot and CAN transmit behavior
- `ecus/gateway`: Gateway ECU that receives CAN traffic and logs frames
- `scripts`: Helpers to create `vcan0` and to generate firmware signing artifacts
- `docs`: Architecture notes and demo scenario guidance

## Next Enhancements

- Add RTOS-style scheduler tasks with POSIX threads and timers
- Extend the HSM emulator into a shared secure service
- Add CAN authentication and replay protection
- Implement UDS or AUTOSAR-style application-layer messaging
