#include "task_template.hpp"
#include "watchdog.hpp"

void Task::registerToWatchdog(uint32_t bit) {
        aliveBit_ = bit;
    }

void Task::notifyAlive() {
    if(aliveBit_ != 0) {
        WatchdogSupervisor::notify(aliveBit_);
    }
}