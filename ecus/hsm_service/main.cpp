#include "HsmService.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    ecu::HsmService service;
    if (!service.start()) {
        std::cerr << "Failed to start HSM service." << std::endl;
        return 1;
    }

    std::cout << "HSM service is running." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    service.stop();
    return 0;
}
