#pragma once

#include "HsmEmulator.h"
#include <atomic>
#include <string>
#include <thread>

namespace ecu {

class HsmService {
public:
    explicit HsmService(const std::string& socketPath = "/tmp/ecu_hsm.sock");
    ~HsmService();

    bool start();
    void stop();

private:
    void run();
    void handleClient(int clientFd);
    std::string processRequest(const std::string& request);

    HsmEmulator emulator_;
    std::string socketPath_;
    int listenFd_;
    std::atomic<bool> running_;
    std::thread acceptThread_;
};

} // namespace ecu
