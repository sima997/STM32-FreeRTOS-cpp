extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "event_groups.h"

}

#include <cstdint>
#include "task_template.hpp"
#include "stm32g431xx.h"

//Global objects
EventGroupHandle_t eg;


#define BIT_TASK1     (1 << 0)
#define BIT_TASK2     (1 << 1)

class Task1 : public Task {
    void init() override {}

    void run() override {
        for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        xEventGroupSetBits(eg, BIT_TASK1);
        }
    }
};

class Task2 : public Task {
    void init() override {}

    void run() override {
        for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1500));
        xEventGroupSetBits(eg, BIT_TASK2);
        }
  }
};

class Coordinator : public Task {
    void init() override {}

    void run() override {
        for (;;) {
        EventBits_t bits = xEventGroupWaitBits(
            eg, BIT_TASK1 | BIT_TASK2,
            pdTRUE,   //clear on exit
            pdTRUE,   //wait for both
            portMAX_DELAY
        );
        GPIOA->ODR ^= (1 << 5); //toggle LED
        }
    }
};

int main(void) {
    //Create event group
    eg = xEventGroupCreate();

    //Enable clock to gpio A
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    //Set gpio 5 as output
    GPIOA->MODER &= ~(0b11 << (2*5));
    GPIOA->MODER |= (0b01 << (2*5));
    static Task1 task1;
    static Task2 task2;
    static Coordinator coordinator;

    __enable_irq();



    xTaskCreate(Task::taskEntry,"Task1",256,&task1,1,nullptr);
    xTaskCreate(Task::taskEntry,"Task2",256,&task2,1,nullptr);
    xTaskCreate(Task::taskEntry,"Coordinator",256,&coordinator,1,nullptr);

    vTaskStartScheduler();

    while(1){}
}
