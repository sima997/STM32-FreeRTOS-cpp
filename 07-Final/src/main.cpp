#include "stm32g431xx.h"
#include <cstdint>
#include "device_defines.hpp"
#include "gpio_template.hpp"
#include "uart_template.hpp"
#include "adc12_template.hpp"
#include "sensor_task.hpp"
#include "logger_task.hpp"
#include "processing_task.hpp"
#include "blink_task.hpp"
#include "command_handler_task.hpp"
#include "watchdog.hpp"
#include "CommonStructures.hpp"
#include <cstdio>

/**
 * @brief Compile-time debug message to display total heap size
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message ("HEAP SIZE = " STR(configTOTAL_HEAP_SIZE))

/**
 * @brief Global FreeRTOS queue for received commands
 */
QueueHandle_t command_queue;

// ------------------------- Driver type aliases -------------------------
using Uart2 = Uart<2, 115200, UartIRQ::RXFNE, 10>; ///< USART2 UART
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>; ///< Onboard green LED
using Adc_temp = Adc12<AdcInstance::Adc1,              ///< ADC1
                       AdcPsc::div16, 
                       AdcClkMode::SynchronousDiv4, 
                       AdcSpecialChannel::TEMP,
                       AdcChannel::CH16,
                       AdcSamplTime::CYCLES_640_5>; ///< Temperature channel

int main(void) {
    // ------------------------- FreeRTOS Queues -------------------------
    command_queue = xQueueCreate(64, sizeof(char));           // Command characters
    auto adc_queue = xQueueCreate(5, sizeof(SensorData));    // Raw sensor data
    auto process_queue = xQueueCreate(5, sizeof(ProcessedData)); // Processed sensor data

    // ------------------------- FreeRTOS Event Group -------------------------
    EventGroupHandle_t command_eg = xEventGroupCreate();      // Control flags

    // ------------------------- Initialize Drivers -------------------------
    static Uart2 uart_log;       // UART for logging
    static Led led_green;        // Heartbeat LED
    static Adc_temp adc_temp;    // Temperature ADC

    // ------------------------- Initialize Tasks -------------------------
    static SensorTask<Adc_temp> sensor(adc_temp, adc_queue);
    static LoggerTask<Uart2> logger(uart_log, process_queue, command_eg);
    static ProcessingTask processing(adc_queue, process_queue);
    static BlinkTask<Led> heartbeat(led_green, command_eg);
    static CommandHandlerTask command(command_queue, command_eg);

    // ------------------------- Initialize Watchdog -------------------------
    WatchdogSupervisor::init(1000); // 1s supervisor period

    WatchdogSupervisor::registerTask(&sensor);
    WatchdogSupervisor::registerTask(&logger);
    WatchdogSupervisor::registerTask(&processing);
    WatchdogSupervisor::registerTask(&heartbeat);

    // ------------------------- Task Creation Helper -------------------------
    auto createTaskOrHalt = [](TaskFunction_t taskFunc,
                                const char* name,
                                uint16_t stackSize,
                                void* params,
                                UBaseType_t priority) {
        if (xTaskCreate(taskFunc, name, stackSize, params, priority, nullptr) != pdPASS) {
            char msg[64] = {};
            snprintf(msg, sizeof(msg), "Creation of %s failed.", name);
            uart_log.send(msg);

            // Infinite blink to indicate error
            while (1) {
                for (uint32_t i = 0; i < 1000000; i++); // crude delay
                led_green.toggle();
            }
        }
    };

    // ------------------------- Create Tasks -------------------------
    createTaskOrHalt(Task::taskEntry, "SensorTask", 512, &sensor, 1);
    createTaskOrHalt(Task::taskEntry, "LoggerTask", 1024, &logger, 1);
    createTaskOrHalt(Task::taskEntry, "ProcessingTask", 1152, &processing, 2);
    createTaskOrHalt(Task::taskEntry, "HeartBeatTask", 256, &heartbeat, 1);
    createTaskOrHalt(Task::taskEntry, "CommandTask", 596, &command, 1);
    createTaskOrHalt(WatchdogSupervisor::taskEntry, "Watchdog", 1024, nullptr, 3);

    // ------------------------- Enable IRQs and Start Scheduler -------------------------
    __enable_irq();           // Enable global interrupts
    vTaskStartScheduler();    // Start FreeRTOS scheduler

    // ------------------------- Fallback infinite loop -------------------------
    while (1) {
        // Should never reach here unless scheduler fails
    }
}
