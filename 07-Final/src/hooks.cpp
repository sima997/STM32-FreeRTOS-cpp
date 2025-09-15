#include "stm32g431xx.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Hook called by FreeRTOS when a stack overflow is detected
 * 
 * This implementation uses PA5 (onboard LED) to indicate the error.
 * The system halts in an infinite loop to prevent undefined behavior.
 * 
 * @param xTask Handle of the task that overflowed
 * @param pcTaskName Name of the task that overflowed
 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // Enable GPIOA clock if not already enabled
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // Configure PA5 as output (push-pull, no pull-up/down, low speed)
    GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);
    GPIOA->MODER |= (1U << GPIO_MODER_MODE5_Pos); // 01 = output mode

    // Infinite loop to indicate stack overflow via LED
    while (1) {
        GPIOA->ODR |= GPIO_ODR_OD5; // Turn LED on
        for (volatile int i = 0; i < 10000; i++); // crude delay for blinking
    }
}

/**
 * @brief Hook called by FreeRTOS when memory allocation fails
 * 
 * This implementation uses PA5 (onboard LED) to indicate the error.
 * The system halts in an infinite loop to prevent further allocation attempts.
 */
extern "C" void vApplicationMallocFailedHook(void) {
    // Enable GPIOA clock if not already enabled
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // Configure PA5 as output (push-pull, no pull-up/down, low speed)
    GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);
    GPIOA->MODER |= (1U << GPIO_MODER_MODE5_Pos); // 01 = output mode

    // Infinite loop to indicate malloc failure via LED
    while (1) {
        GPIOA->ODR |= GPIO_ODR_OD5; // Turn LED on
        for (volatile int i = 0; i < 100000; i++); // longer crude delay for blinking
    }
}
