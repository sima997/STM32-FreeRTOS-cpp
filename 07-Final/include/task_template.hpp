#pragma once
#include <cstdint>

/**
 * @brief Error codes for tasks
 * 
 * Used to indicate task-specific errors such as peripheral failures or queue issues.
 */
enum class TaskError {
    None = 0, ///< No error
    Adc,      ///< ADC error
    Uart,     ///< UART error
    I2c,      ///< I2C error
    Queue     ///< Queue operation error
};

// Forward declaration of WatchdogSupervisor
class WatchdogSupervisor;

/**
 * @brief Base class for all FreeRTOS tasks
 * 
 * Provides a common interface for task creation, watchdog integration, and
 * error monitoring. Derived classes must implement the run() method.
 */
class Task {
public:
    /**
     * @brief Construct a new Task
     * 
     * @param name Name of the task (for identification/logging)
     */
    Task(const char* name) : name_(name), aliveBit_(0) {}

    /**
     * @brief Pure virtual method implementing the task logic
     * 
     * Derived classes must override this method to define task behavior.
     */
    virtual void run() = 0;

    /**
     * @brief Optional task initialization
     * 
     * Can be overridden by derived classes for pre-run setup.
     */
    virtual void init() {}

    /**
     * @brief Static FreeRTOS task entry point
     * 
     * Casts pvParams to Task* and calls init() and run().
     * Suitable for use with xTaskCreate().
     * 
     * @param pvParams Pointer to the Task instance
     */
    static void taskEntry(void* pvParams) {
        Task* t = static_cast<Task*>(pvParams);
        t->init();
        t->run();
    }

    /**
     * @brief Register this task with the watchdog supervisor
     * 
     * @param bit Bitmask assigned to this task for watchdog monitoring
     */
    void registerToWatchdog(uint32_t bit);

    /**
     * @brief Notify the watchdog that this task is alive
     */
    void notifyAlive();

    /**
     * @brief Get the alive bit assigned to this task
     * 
     * @return uint32_t Alive bit
     */
    uint32_t getAliveBit() const { return aliveBit_; }

private:
    const char* name_;   ///< Name of the task
    uint32_t aliveBit_;  ///< Bit used to signal liveness to the watchdog
};
