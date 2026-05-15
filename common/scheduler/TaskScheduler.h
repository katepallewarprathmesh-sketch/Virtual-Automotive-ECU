#pragma once

#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace ecu {

class TaskScheduler {
public:
    using TaskFunction = std::function<void()>;

    TaskScheduler();
    ~TaskScheduler();

    void addPeriodicTask(int intervalMs, TaskFunction task, const std::string& name);
    bool start();
    void stop();

private:
    struct TaskEntry {
        int intervalMs;
        TaskFunction task;
        std::string name;
        int timerFd;
        std::thread thread;
        bool running;
    };

    std::vector<TaskEntry> tasks_;
    bool started_;
};

} // namespace ecu
