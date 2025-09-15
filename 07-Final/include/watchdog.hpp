#pragma once

#include <cstdint>

// Forward declaration of Task
class Task;

extern "C" {
    #include "FreeRTOS.h"
    #include "event_groups.h"
}

/**
 * @brief Independent Watchdog (IWDG) Key Register commands
 * 
 * Used to unlock, reload, or start the IWDG peripheral.
 */
enum class IWDG_KR : uint32_t {
    RELOAD = 0xAAAA, ///< Reload the watchdog counter
    UNLOCK = 0x5555, ///< Unlock write access to IWDG registers
    START  = 0xCCCC  ///< Start the watchdog
};

/**
 * @brief Independent Watchdog (IWDG) Prescaler values
 * 
 * Used to configure the IWDG clock prescaler.
 */
enum class IWDG_PR : uint32_t {
    DIV4   = 0b000,
    DIV8   = 0b001,
    DIV16  = 0b010,
    DIV32  = 0b011,
    DIV64  = 0b100,
    DIV128 = 0b101,
    DIV256 = 0b110
};

/**
 * @brief Supervisor for the Independent Watchdog (IWDG)
 * 
 * Monitors registered FreeRTOS tasks and ensures they are alive.
 * Kicks the hardware watchdog periodically if all tasks report activity.
 */
class WatchdogSupervisor {
public:
    /**
     * @brief Initialize the watchdog supervisor and hardware IWDG
     * 
     * @param periodMs Watchdog period in milliseconds
     */
    static void init(uint32_t periodMs);

    /**
     * @brief Register a task to be monitored by the watchdog
     * 
     * @param task Pointer to the Task instance
     */
    static void registerTask(Task* task);

    /**
     * @brief Unregister a previously registered task
     * 
     * @param task Pointer to the Task instance
     */
    static void unregisterTask(Task* task);

    /**
     * @brief Notify the watchdog that a task is alive
     * 
     * @param bit Alive bit corresponding to the task
     */
    static void notify(uint32_t bit);

    /**
     * @brief FreeRTOS task entry function for the watchdog supervisor
     * 
     * Periodically checks all registered tasks and kicks the hardware watchdog.
     * 
     * @param pvParams Pointer to task parameters (unused)
     */
    static void taskEntry(void *pvParams);

private:
    inline static EventGroupHandle_t aliveFlags; ///< Event group storing task alive signals
    inline static uint32_t registredTasks = 0;   ///< Number of registered tasks
    inline static uint32_t ALL_ALIVE = 0;       ///< Bitmask representing all tasks alive
    inline static TickType_t periodTicks;       ///< Watchdog checking period in ticks

    /**
     * @brief Kick (reload) the hardware watchdog
     */
    static inline void kick();

    /**
     * @brief Initialize the hardware IWDG peripheral
     * 
     * @param pr Prescaler value
     * @param periodMs Watchdog period in milliseconds
     */
    static void initHw(IWDG_PR pr, uint32_t periodMs);
};
