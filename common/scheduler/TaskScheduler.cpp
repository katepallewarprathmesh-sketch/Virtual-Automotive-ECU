#include "TaskScheduler.h"
#include <chrono>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <sys/timerfd.h>
#include <unistd.h>

namespace ecu {

TaskScheduler::TaskScheduler()
    : started_(false) {
}

TaskScheduler::~TaskScheduler() {
    stop();
}

void TaskScheduler::addPeriodicTask(int intervalMs, TaskFunction task, const std::string& name) {
    if (started_) {
        throw std::runtime_error("Cannot add task after scheduler start");
    }
    tasks_.push_back({intervalMs, std::move(task), name, -1, std::thread(), false});
}

static int createTimerfd(int intervalMs) {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    struct itimerspec spec;
    spec.it_interval.tv_sec = intervalMs / 1000;
    spec.it_interval.tv_nsec = (intervalMs % 1000) * 1000000;
    spec.it_value = spec.it_interval;
    if (timerfd_settime(fd, 0, &spec, nullptr) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool TaskScheduler::start() {
    if (started_) {
        return false;
    }

    for (auto& task : tasks_) {
        task.timerFd = createTimerfd(task.intervalMs);
        if (task.timerFd < 0) {
            std::cerr << "Failed to create timerfd for task " << task.name << std::endl;
            return false;
        }
        task.running = true;
        task.thread = std::thread([this, &task]() {
            while (task.running) {
                struct pollfd pfd;
                pfd.fd = task.timerFd;
                pfd.events = POLLIN;
                int ret = poll(&pfd, 1, 1000);
                if (ret > 0 && (pfd.revents & POLLIN)) {
                    uint64_t expirations = 0;
                    read(task.timerFd, &expirations, sizeof(expirations));
                    if (task.running) {
                        task.task();
                    }
                }
            }
        });
    }

    started_ = true;
    return true;
}

void TaskScheduler::stop() {
    if (!started_) {
        return;
    }
    for (auto& task : tasks_) {
        task.running = false;
        if (task.timerFd >= 0) {
            close(task.timerFd);
            task.timerFd = -1;
        }
    }
    for (auto& task : tasks_) {
        if (task.thread.joinable()) {
            task.thread.join();
        }
    }
    started_ = false;
}

} // namespace ecu
