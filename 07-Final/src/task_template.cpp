#include "task_template.hpp"
#include "watchdog.hpp"

/**
 * @brief Register this task with the watchdog supervisor
 * 
 * Stores the task-specific alive bit which will be used when
 * notifying the watchdog that this task is alive.
 * 
 * @param bit Unique bit assigned to this task for watchdog monitoring
 */
void Task::registerToWatchdog(uint32_t bit) {
    aliveBit_ = bit;
}

/**
 * @brief Notify the watchdog that this task is alive
 * 
 * Sends the stored alive bit to the WatchdogSupervisor.
 * Only notifies if a valid bit has been registered.
 */
void Task::notifyAlive() {
    if (aliveBit_ != 0) {
        WatchdogSupervisor::notify(aliveBit_);
    }
}
