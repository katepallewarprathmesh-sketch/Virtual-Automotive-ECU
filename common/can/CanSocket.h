#pragma once

#include <cstdint>
#include <linux/can.h>
#include <string>
#include <vector>

namespace ecu {

struct CanFrame {
    uint32_t id;
    uint8_t data[8];
    uint8_t dlc;
};

class CanSocket {
public:
    explicit CanSocket(const std::string& interfaceName);
    ~CanSocket();

    bool isOpen() const;
    bool send(const CanFrame& frame);
    bool receive(CanFrame& frame);
    std::string lastError() const;

private:
    int socketFd_;
    std::string interfaceName_;
    std::string lastError_;
};

} // namespace ecu
