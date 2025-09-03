extern "C" {
#include "stm32g431xx.h"
#include "FreeRTOS.h"
#include "task.h"
}

// ---------- Blink Task ----------
class BlinkTask {
public:
    BlinkTask(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {
        // Enable GPIOA clock
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

        // Configure PA5 as output
        port->MODER &= ~(3U << (5 * 2));  // clear mode
        port->MODER |=  (1U << (5 * 2));  // set to output
        port->OTYPER &= ~(1U << 5);       // push-pull
        port->OSPEEDR |= (3U << (5 * 2)); // high speed
        port->PUPDR &= ~(3U << (5 * 2));  // no pull-up/down
    }

    void run() {
        while (1) {
            port->ODR ^= pin; // toggle pin
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    static void adapter(void* pv) {
        static_cast<BlinkTask*>(pv)->run();
    }

private:
    GPIO_TypeDef* port;
    uint16_t pin;
};


// ---------- Main ----------
int main(void) {
    // Make sure SystemCoreClock is correct
    SystemCoreClockUpdate();

    static BlinkTask task(GPIOA, 1U << 5);

    xTaskCreate(BlinkTask::adapter, "Blink", 256, &task, 2, nullptr);

    vTaskStartScheduler();

    // Should never reach here
    while (1) { }
}
