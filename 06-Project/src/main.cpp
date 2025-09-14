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
#include "watchdog.hpp"

//Drivers templates (RAII)
using Uart2 = Uart<2, 115200>;
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Adc_temp = Adc12<AdcInstance::Adc1,
                     AdcPsc::div16, 
                     AdcClkMode::SynchronousDiv4, 
                     AdcSpecialChannel::TEMP,
                     AdcChannel::CH16,
                     AdcSamplTime::CYCLES_640_5>;



int main(void) {
    //Queue
    auto adc_queue = xQueueCreate(5,sizeof(uint16_t));
    auto process_queue = xQueueCreate(5, sizeof(float));

    //Initialize drivers
    static Uart2 uart_log;
    static Led led_green;
    static Adc_temp adc_temp;

    //Initialize tasks
    static SensorTask<Adc_temp> sensor(adc_temp, adc_queue);
    static LoggerTask<Uart2> logger(uart_log, process_queue);
    static ProcessingTask processing(adc_queue, process_queue);
    static BlinkTask<Led> heartbeat(led_green);

    //Initialize Watchdog and register tasks
    WatchdogSupervisor::init(1000);
    WatchdogSupervisor::registerTask(&sensor);
    WatchdogSupervisor::registerTask(&logger);
    WatchdogSupervisor::registerTask(&processing);
    WatchdogSupervisor::registerTask(&heartbeat);

    //Create tasks
    xTaskCreate(Task::taskEntry, "SensorTask", 512, &sensor, 1, nullptr);
    xTaskCreate(Task::taskEntry, "LoggerTask", 256, &logger, 1, nullptr);
    xTaskCreate(Task::taskEntry, "ProcessingTask",1024, &processing, 2, nullptr);
    xTaskCreate(Task::taskEntry, "HeartBeatTask", 256, &heartbeat, 1, nullptr);

    
    //Create watchdog task
    xTaskCreate(WatchdogSupervisor::taskEntry, "Watchdog",256, nullptr, 3, nullptr);

    __enable_irq();
    vTaskStartScheduler();
    while(1) {}
}

