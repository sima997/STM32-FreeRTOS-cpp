extern "C" {
#include "stm32g431xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include "gpio_template.hpp"
#include "task_template.hpp"
#include "uart_template.hpp"

// ---------- GPIO typedefs ----------
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Button = Gpio<GpioPort::C, 13, GpioMode::Input>;
// ----------- UART -------------------
using UartLog = Uart<2,115200>;
// ---------- Global objects ----------
QueueHandle_t que_toggle;
Led led;
Button btn;
UartLog uart;

// ---------- EXTI ISR ----------
extern "C" void EXTI15_10_IRQHandler() {
    BaseType_t higher = pdFALSE;
    bool pressed = true;

    if (EXTI->PR1 & (1U << 13)) {
        xQueueSendFromISR(que_toggle, &pressed, &higher);
        EXTI->PR1 = (1U << 13); // clear pending
        portYIELD_FROM_ISR(higher);
    }
}

// ---------- LED Task ----------
class LedTask : public Task {
public:
    void init() override {
        led.init();
        led.clear();
        uart.init(128000000);
    }

private:
    void run() override {
        bool event;
        for (;;) {
            if (xQueueReceive(que_toggle, &event, portMAX_DELAY) == pdPASS) {
                if (event) {
                    led.toggle();
                    uart.send("Event from queue. WOOW.\r\n");
                }
            }
        }
    }
};

// ---------- Main ----------
int main() {
    // 1. Initialize button
    btn.init();
    btn.setInterrupt(GpioIrqEdge::Falling);

    // 2. Create FreeRTOS queue
    que_toggle = xQueueCreate(1, sizeof(bool));

    // 3. NVIC priority and enable
    NVIC_SetPriority(EXTI15_10_IRQn, 10); // safe with FreeRTOS
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    __enable_irq();

    // 4. Create LED task
    static LedTask ledTask;   // static to ensure lifetime
    ledTask.init();
    xTaskCreate(Task::taskEntry, "LED Task", 256, &ledTask, 1, nullptr);

    // 5. Start scheduler
    vTaskStartScheduler();

    while (1) { }
}
