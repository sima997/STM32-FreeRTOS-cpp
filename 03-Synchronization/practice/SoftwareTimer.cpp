extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "timers.h"
}

#include <cstdint>
#include "task_template.hpp"
#include "stm32g431xx.h"



void vTimerCallback(TimerHandle_t xTimer) {
  GPIOA->ODR ^= (1 << 5); //Blink LED every 500ms
}

TimerHandle_t ledTimer;



int main(void) {
    ledTimer = xTimerCreate("LED", pdMS_TO_TICKS(500),
                        pdTRUE,
                        nullptr,
                        vTimerCallback);

    xTimerStart(ledTimer, 0);

    //Enable clock to gpio A
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    //Set gpio 5 as output
    GPIOA->MODER &= ~(0b11 << (2*5));
    GPIOA->MODER |= (0b01 << (2*5));

    __enable_irq();
    vTaskStartScheduler();

    while(1){}
}
