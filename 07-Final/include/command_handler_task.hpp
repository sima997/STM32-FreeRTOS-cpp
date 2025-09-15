#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include <cstring>
#include "CommonEnums.hpp"
#include "task.h"

/**
 * @brief Command processing states for the CommandHandlerTask
 */
enum class CommandState : uint8_t {WaitingFor = 0, Process, Act};



/**
 * @brief Task responsible for handling incoming commands from a queue
 * 
 * This task reads bytes from a FreeRTOS queue, assembles them into commands,
 * and sets/clears corresponding bits in an event group to trigger system actions.
 */
class CommandHandlerTask : public Task {
public:

    /**
     * @brief Construct a new CommandHandlerTask
     * 
     * @param queue FreeRTOS queue handle from which to receive command bytes
     * @param eg FreeRTOS event group handle used to signal system actions
     */
    CommandHandlerTask(QueueHandle_t queue, EventGroupHandle_t eg) : 
    Task("CommandHandler"), queue_(queue), eg_(eg) {}

    /**
     * @brief Main task loop
     * 
     * Continuously reads bytes from the queue, builds commands, processes them,
     * and monitors stack usage. Uses a simple state machine:
     * - Idle -> WaitingFor: ready to receive bytes
     * - WaitingFor: collects bytes until newline, then moves to Process
     * - Process: interprets command and triggers actions
     */
    void run() override {
        size_t index = 0;
        char receive;
        char buffer[24];  // buffer for incoming command
        CommandState state = CommandState::WaitingFor;

        for(;;) {
            switch(state) {
                case CommandState::WaitingFor:
                    // Block task until a byte is received
                    if(xQueueReceive(queue_, &receive, portMAX_DELAY) == pdPASS) {
                        if(receive == '\n' || receive == '\r') {
                        buffer[index] = '\0'; // null-terminate command string
                        index = 0;
                        state = CommandState::Process;
                        } else {
                            buffer[index++] = receive; // append byte to buffer
                        }
                    }
                    
                    break;
                case CommandState::Process:
                    parse_command(buffer); // interpret command and update system
                    state = CommandState::WaitingFor; // wait for next command

                    break;
                default:
                    state = CommandState::WaitingFor; 
                    break;
            }
            checkStackUsage(); // monitor stack to prevent overflow
        }
    }


private:
    QueueHandle_t queue_;       ///< Queue from which command bytes are read
    EventGroupHandle_t eg_;     ///< Event group to signal command actions

        /**
     * @brief Parses a received command string and sets/clears event group bits
     * 
     * Supported commands:
     * - "LED ON"/"LED OFF": control LED
     * - "LOG TEMP"/"STOP TEMP": control temperature logging
     * - "LOG HUMID"/"STOP HUMID": control humidity logging
     * - "STATUS": trigger status report
     * - "-help": trigger help message
     * 
     * @param cmd Null-terminated command string
     */
    void parse_command(const char* cmd) {
        if (strcmp(cmd, "LED ON") == 0) {
           xEventGroupSetBits(eg_, static_cast<EventBits_t>(CommandType::LED));
        }else if (strcmp(cmd, "LED OFF") == 0) {
            xEventGroupClearBits(eg_, static_cast<EventBits_t>(CommandType::LED));
        }else if (strcmp(cmd, "LOG TEMP") == 0) {
            xEventGroupSetBits(eg_, static_cast<EventBits_t>(CommandType::TEMP));
        }else if (strcmp(cmd, "STOP TEMP") == 0) {
            xEventGroupClearBits(eg_, static_cast<EventBits_t>(CommandType::TEMP));
        }else if (strcmp(cmd, "LOG HUMID") == 0) {
            xEventGroupSetBits(eg_, static_cast<EventBits_t>(CommandType::HUMID));
        }else if (strcmp(cmd, "STOP HUMID") == 0) {
            xEventGroupClearBits(eg_, static_cast<EventBits_t>(CommandType::HUMID));
        }else if (strcmp(cmd, "STATUS") == 0) {
            xEventGroupSetBits(eg_, static_cast<EventBits_t>(CommandType::STATUS));  
        }else if (strcmp(cmd, "-help") == 0) {
            xEventGroupSetBits(eg_, static_cast<EventBits_t>(CommandType::HELP));    
        }else {

        }
    }

    /**
     * @brief Checks remaining stack space for the task
     * 
     * Uses FreeRTOS uxTaskGetStackHighWaterMark() to detect potential stack overflow.
     * Threshold of 50 words (~200 bytes on 32-bit MCU) is considered low.
     */
    void checkStackUsage() {
        UBaseType_t minFreeStack = uxTaskGetStackHighWaterMark(nullptr); // nullptr = current task
        if (minFreeStack < 50) { // warning threshold (words)
           // TODO: Implement logging or system alert for low stack if required
        }
    }

};