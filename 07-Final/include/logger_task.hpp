#pragma once
#include "task_template.hpp"
#include "uart_template.hpp"
#include "FreeRTOS.h"
#include "queue.h"
#include "CommonStructures.hpp"
#include "CommonEnums.hpp"
#include "event_groups.h"
#include "task.h"
#include <cstdio>


/**
 * @brief Task responsible for logging sensor data and system messages via UART
 * 
 * This task reads processed sensor data from a queue and prints temperature,
 * humidity, or status information to a UART interface based on event group bits.
 * It also handles printing available commands when requested.
 * 
 * @tparam UartDriver Type of the UART driver used for communication. Must provide send().
 */
template<typename UartDriver>
class LoggerTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;     ///< Current error status of the task

    /**
     * @brief Construct a new LoggerTask
     * 
     * @param uart Reference to the UART driver
     * @param queue FreeRTOS queue from which processed data is received
     * @param eg Event group for command/status signaling
     */
    LoggerTask(UartDriver& uart, QueueHandle_t queue, EventGroupHandle_t eg) : 
        Task("LoggerTask"), uart_(uart), queue_(queue), eg_(eg) {

    }

    /**
     * @brief Main task loop
     * 
     * Sends welcome message and command list on startup.
     * Continuously receives processed sensor data from the queue and prints
     * corresponding output based on active command bits in the event group.
     * Monitors stack usage and notifies task liveness.
     */
    void run() override {
        ProcessedData processed_data;

        // Startup banner and command list
        uart_.send("Sensor Smart Hub v1.1. Commands:\n");
        uart_.send("-------------------------------------\n");
        printAllMessages();
        
        
        for(;;) {
            char buffer[64] = {};
            EventBits_t control_bits;
            if (queue_ != nullptr) {
                // Wait indefinitely for new processed data
                if(xQueueReceive(queue_, &processed_data, portMAX_DELAY) == pdPASS) {                    
                     // Read control bits to determine which messages to print
                    control_bits = xEventGroupGetBits(eg_);
                    if( static_cast<uint32_t>(control_bits) & static_cast<EventBits_t>(CommandType::TEMP) ) {
                        snprintf(buffer,sizeof(buffer),"Core Temperature: %.1f deg C\n", processed_data.temperature);
                        uart_.send(buffer);
                    }
                    if( static_cast<uint32_t>(control_bits) & static_cast<EventBits_t>(CommandType::HUMID) ) {
                        snprintf(buffer,sizeof(buffer),"Simulated Humidity: %.1f %%\n", processed_data.humidity);
                        uart_.send(buffer);
                    }
                    if( static_cast<uint32_t>(control_bits) & static_cast<EventBits_t>(CommandType::STATUS) ) {
                        snprintf(buffer,sizeof(buffer),"Status T: %.1f degC, H: %.1f %%\n", 
                            processed_data.temperature, processed_data.humidity);
                        uart_.send(buffer);
                        xEventGroupClearBits(eg_,static_cast<EventBits_t>(CommandType::STATUS));
                    }
                    
                    
                    
                }
            }else {
                // Queue not initialized; set error status
                ErrorStatus = TaskError::Queue;
                uart_.send("Device Error\n.");
            } 

            // Print command list if HELP bit is set
            if( static_cast<uint32_t>(control_bits) & static_cast<EventBits_t>(CommandType::HELP) ) {
                printAllMessages();
                xEventGroupClearBits(eg_,static_cast<EventBits_t>(CommandType::HELP));
            }
            

            checkStackUsage();  // monitor stack usage
            notifyAlive();      // indicate task is running
        }        
    }
private:
    UartDriver& uart_;          ///< UART driver reference for sending messages
    QueueHandle_t queue_;       ///< Queue handle for receiving processed sensor data
    EventGroupHandle_t eg_;     ///< Event group for command/status signaling

    /**
     * @brief Maps UART driver errors to task error codes
     * 
     * @param error UART driver error
     * @return TaskError Mapped task error
     */
    static TaskError map_uart_error(UartError error) {
        if(error == UartError::None) return TaskError::None;
        else return TaskError::Adc;
    }

    /**
     * @brief Checks remaining stack space for the task
     * 
     * Uses FreeRTOS uxTaskGetStackHighWaterMark() to detect potential stack overflow.
     * Threshold of 50 words (~200 bytes on 32-bit MCU) is considered low.
     */
    void checkStackUsage() {
        UBaseType_t minFreeStack = uxTaskGetStackHighWaterMark(nullptr); // nullptr = current task
        if (minFreeStack < 50) { 
            // TODO: Implement warning/logging mechanism for low stack
        }
    }

    /**
     * @brief Sends all predefined messages/commands over UART
     * 
     * Iterates through the global messages array and prints key-value pairs.
     */
    void printAllMessages() {
        for (const auto& msg : messages) {
            char buffer[64] = {};
            snprintf(buffer, sizeof(buffer),"%s = %s\n", msg.key, msg.value);
            uart_.send(buffer);
        }
    }
};