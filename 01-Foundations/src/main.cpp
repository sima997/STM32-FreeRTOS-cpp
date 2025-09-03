extern "C" {
#include "stm32g431xx.h"
#include "FreeRTOS.h"
#include "task.h"
}

#include "gpio_template.hpp"
#include "uart_template.hpp"

using Led1 = Gpio<'A',5,GpioMode::Output>;
using Led2 = Gpio<'A',6,GpioMode::Output>;
using Led3 = Gpio<'A',7,GpioMode::Output>;
using Uart_log = Uart<2, 115200>;



// ---------- Task 1 and 2 ----------
template<typename LedPin, TickType_t DelayMS>
class TaskLed {
public :
    static void start(const char* name, uint16_t stack_size, BaseType_t priority) {
        xTaskCreate(taskFun, name, stack_size, nullptr, priority, nullptr);
        LedPin::init();
    }

private :
    static void taskFun(void* pvParameters) {
        (void) pvParameters;
        while(1) {
            LedPin::toggle();
            vTaskDelay(pdMS_TO_TICKS(DelayMS));
        }
    }
};


// ---------- Task 3 ----------
template<typename UartLog, TickType_t DelayMs>
class TaskUart {
public :
    static void start(const char* name, uint16_t stack_size,BaseType_t priotity) {
        xTaskCreate(taskFun, name, stack_size, nullptr, priotity, nullptr);
        UartLog::init(128000000);
    }
private :
    static void taskFun(void* pvParameters) {
        (void) pvParameters;
        while(1) {
            UartLog::send("TicTac\r\n");
            vTaskDelay(pdMS_TO_TICKS(DelayMs));
        }
        
    }
};

// ---------- Task 4 ----------
template<typename LedPin, TickType_t DelayMS>
class TaskLedPrecise {
public :
    static void start(const char* name, uint16_t stack_size, BaseType_t priority) {
        xTaskCreate(taskFun, name, stack_size, nullptr, priority, nullptr);
        LedPin::init();
    }

private :
    

    static void taskFun(void* pvParameters) {
        (void) pvParameters;
        TickType_t xLastWakeupTime = xTaskGetTickCount();
        while(1) {
            LedPin::toggle();
            vTaskDelayUntil(&xLastWakeupTime, pdMS_TO_TICKS(DelayMS));
        }
    }
};


// ---------- Main ----------
int main(void) {
    SystemCoreClockUpdate();
    //Create Task 1
    TaskLed<Led3,500>::start("Led1",256,1);
    //Create Task 2
    TaskLed<Led2,250>::start("Led2",256,1);
    //Create Task 3
    TaskUart<Uart_log, 1000>::start("UartLog",512,2);
    //Create Task 4
    TaskLedPrecise<Led1,500>::start("LedPrecise",256,1);

    //Run Scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (1) { }
}
