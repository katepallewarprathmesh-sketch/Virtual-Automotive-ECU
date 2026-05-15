#include "CanSocket.h"
#include <cstring>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ecu {

CanSocket::CanSocket(const std::string& interfaceName)
    : socketFd_(-1), interfaceName_(interfaceName) {
    socketFd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd_ < 0) {
        lastError_ = "failed to create CAN socket";
        return;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);
    if (ioctl(socketFd_, SIOCGIFINDEX, &ifr) < 0) {
        lastError_ = "failed to get interface index";
        close(socketFd_);
        socketFd_ = -1;
        return;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socketFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        lastError_ = "failed to bind CAN socket";
        close(socketFd_);
        socketFd_ = -1;
    }
}

CanSocket::~CanSocket() {
    if (socketFd_ >= 0) {
        close(socketFd_);
    }
}

bool CanSocket::isOpen() const {
    return socketFd_ >= 0;
}

bool CanSocket::send(const CanFrame& frame) {
    if (!isOpen()) {
        lastError_ = "CAN socket is not open";
        return false;
    }

    struct can_frame canFrame;
    std::memset(&canFrame, 0, sizeof(canFrame));
    canFrame.can_id = frame.id;
    canFrame.can_dlc = frame.dlc;
    std::memcpy(canFrame.data, frame.data, frame.dlc);

    ssize_t bytes = write(socketFd_, &canFrame, sizeof(canFrame));
    if (bytes != sizeof(canFrame)) {
        lastError_ = "failed to write CAN frame";
        return false;
    }
    return true;
}

bool CanSocket::receive(CanFrame& frame) {
    if (!isOpen()) {
        lastError_ = "CAN socket is not open";
        return false;
    }

    struct can_frame canFrame;
    ssize_t bytes = read(socketFd_, &canFrame, sizeof(canFrame));
    if (bytes != sizeof(canFrame)) {
        lastError_ = "failed to read CAN frame";
        return false;
    }

    frame.id = canFrame.can_id;
    frame.dlc = canFrame.can_dlc;
    std::memcpy(frame.data, canFrame.data, frame.dlc);
    return true;
}

std::string CanSocket::lastError() const {
    return lastError_;
}

} // namespace ecu
