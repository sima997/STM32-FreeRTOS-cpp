#pragma once

#include "FreeRTOS.h"
#include "gpio_template.hpp"
#include <cstdint>
#include "task_template.hpp"
#include "event_groups.h"
#include "CommonEnums.hpp"

#include "task.h"



/**
 * @brief Template task for blinking an LED based on an event group signal.
 * 
 * This task periodically checks an event group for a specific command bit
 * and toggles or clears the associated LED accordingly. It also provides
 * basic monitoring of task health via alive notifications and stack usage checks.
 * 
 * @tparam LedPin Type of the LED GPIO abstraction. Must provide toggle() and clear().
 */
template<typename LedPin>
class BlinkTask : public Task {
public:
        /**
     * @brief Construct a new Blink Task
     * 
     * @param led Reference to the LED GPIO object.
     * @param eg FreeRTOS event group handle used to control LED behavior.
     */    
    BlinkTask(LedPin& led, EventGroupHandle_t eg) : Task("BlinkTask"), led_(led), eg_(eg) {}

    /**
     * @brief Main task loop
     * 
     * Runs indefinitely. Checks the event group for the LED command bit:
     * - If set, toggle the LED.
     * - If not set, turn the LED off.
     * 
     * Notifies that the task is alive and monitors stack usage on each iteration.
     */
    void run() override {
        for(;;) {
            // Read the event group bits (non-blocking)
            EventBits_t control_bits = xEventGroupGetBits(eg_);
            // Check if the LED command bit is set
            if (static_cast<uint32_t>(control_bits) & static_cast<uint32_t>(CommandType::LED)) {
                led_.toggle();  // toggle LED if command is active
            } else {
                led_.clear();   // ensure LED is off otherwise
            }
            
            notifyAlive(); // notify the system watchdog that this task is running
            checkStackUsage(); // monitor stack high-water mark to detect potential overflow
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

private:
    LedPin& led_;            ///< Reference to the LED GPIO abstraction
    EventGroupHandle_t eg_; ///< FreeRTOS event group controlling the LED

    /**
     * @brief Checks remaining stack space for the task
     * 
     * Uses FreeRTOS uxTaskGetStackHighWaterMark() to detect potential stack overflow.
     * Threshold of 50 words (~200 bytes on 32-bit MCU) is considered low.
     */
    void checkStackUsage() {
        UBaseType_t minFreeStack = uxTaskGetStackHighWaterMark(nullptr); // nullptr = current task
        if (minFreeStack < 50) { 
             // TODO: Implement logging or system alert for low stack if required
        }
    }
};