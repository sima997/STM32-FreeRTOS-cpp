#include "watchdog.hpp"
#include "stm32g431xx.h"
#include <cstdint>
#include "task_template.hpp"

/**
 * @brief Initialize the watchdog supervisor and hardware IWDG
 * 
 * Configures the hardware watchdog and creates the event group to track task liveness.
 * 
 * @param periodMs Supervisor task period in milliseconds
 */
void WatchdogSupervisor::init(uint32_t periodMs) {
    // Initialize hardware IWDG with prescaler and timeout
    initHw(IWDG_PR::DIV32, 4000);

    // Convert supervisor period to FreeRTOS ticks
    periodTicks = pdMS_TO_TICKS(periodMs);

    // Create event group for tracking alive signals from tasks
    aliveFlags = xEventGroupCreate();
}

/**
 * @brief Register a task with the watchdog
 * 
 * Assigns a unique alive bit to the task and updates the ALL_ALIVE bitmask.
 * 
 * @param task Pointer to the task to register
 */
void WatchdogSupervisor::registerTask(Task* task) {
    uint32_t bit = (1 << registredTasks);  // assign next free bit
    task->registerToWatchdog(bit);
    ALL_ALIVE |= bit;                       // update ALL_ALIVE mask
    registredTasks++;
}

/**
 * @brief Unregister a task from the watchdog
 * 
 * Removes the task's bit from ALL_ALIVE. Note: does not handle bit fragmentation.
 * 
 * @param task Pointer to the task to unregister
 */
void WatchdogSupervisor::unregisterTask(Task* task) {
    uint32_t bit = task->getAliveBit();
    ALL_ALIVE &= ~bit;
    // TODO: Handle potential fragmentation if tasks unregister dynamically
}

/**
 * @brief Supervisor FreeRTOS task entry point
 * 
 * Periodically checks that all registered tasks have reported alive.
 * Kicks the hardware watchdog if all tasks are alive, otherwise MCU reset is implied.
 * 
 * @param pvParams Unused
 */
void WatchdogSupervisor::taskEntry(void *pvParams) {
    (void)pvParams;
    TickType_t last = 0;

    for(;;) {
        // Wait for next period
        xTaskDelayUntil(&last, periodTicks);

        // Read alive flags from tasks
        EventBits_t flag = xEventGroupGetBits(aliveFlags);

        if ((flag & ALL_ALIVE) == ALL_ALIVE) {
            // All tasks reported alive, reload hardware watchdog
            kick();

            // Clear flags for next period
            xEventGroupClearBits(aliveFlags, ALL_ALIVE);
        } else {
            // At least one task did not report alive
            // TODO: implement MCU reset or recovery procedure
        }
    }
}

/**
 * @brief Notify supervisor that a task is alive
 * 
 * Sets the corresponding bit in the aliveFlags event group.
 * 
 * @param bit Task's alive bit
 */
void WatchdogSupervisor::notify(uint32_t bit) {
    xEventGroupSetBits(aliveFlags, bit);
}

/**
 * @brief Initialize the hardware IWDG peripheral
 * 
 * Configures prescaler, reload value, and starts the watchdog.
 * 
 * @param pr Prescaler value
 * @param periodMsIWDG Watchdog timeout in milliseconds
 */
void WatchdogSupervisor::initHw(IWDG_PR pr, uint32_t periodMsIWDG) {
    // Start and unlock IWDG registers
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::START);
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::UNLOCK);

    // Set prescaler and reload value
    IWDG->PR = static_cast<uint32_t>(pr);
    IWDG->RLR = periodMsIWDG;  // approximate timeout

    // Reload counter to start watchdog
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
}

/**
 * @brief Kick (reload) the hardware watchdog counter
 */
inline void WatchdogSupervisor::kick() {
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
}
