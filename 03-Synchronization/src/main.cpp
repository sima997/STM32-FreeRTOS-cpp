extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "timers.h"
    #include "event_groups.h"
}

#include <cstdint>
#include "task_template.hpp"
#include "stm32g431xx.h"
#include "gpio_template.hpp"

#define BIT_RED     (1 << 0)
#define BIT_GREEN   (1 << 1)
#define BIT_ORANGE  (1 << 2)

//LEDs
/**
 * GREEN = PA5
 * ORANGE = PB0
 * RED = PB7
 */
using Led_green = Gpio<GpioPort::C, 8, GpioMode::Output>;
using Led_orange = Gpio<GpioPort::C, 6, GpioMode::Output>;
using Led_red = Gpio<GpioPort::C, 5, GpioMode::Output>;


//GLobal handlers
EventGroupHandle_t eg;
TimerHandle_t timer;

//Global variables
uint8_t state = 0;

//Timer callback
void timer_callback(TimerHandle_t tim) {
    switch(state) {
      case 0: 
        xEventGroupSetBits(eg, BIT_RED);
        break;
      case 1: 
        xEventGroupSetBits(eg, BIT_GREEN);
        break;
      case 2: 
        xEventGroupSetBits(eg, BIT_ORANGE);
        break;
    }

    state = (state + 1) % 3;
}

//Light task
template<typename Led>
class LightTask : public Task {
  public:
    LightTask(uint16_t bit) : bit_(bit) {}
    void init() override {
      Led::init();
    }

    void run() override {
      for(;;) {
        EventBits_t ebits = xEventGroupWaitBits(eg, bit_, pdTRUE, pdTRUE, portMAX_DELAY);
        Led::set();
        vTaskDelay(pdMS_TO_TICKS(800));
        Led::clear();
      }
      
    }

  private:
  uint16_t bit_;
};


int main(void) {
  //Create timer
  timer = xTimerCreate("Semaphore Timer", pdMS_TO_TICKS(1000),pdTRUE, nullptr, timer_callback);

  //Create event group
  eg = xEventGroupCreate();


  //Create and Init tasks
  static LightTask<Led_green> GreenTask(BIT_GREEN);
  static LightTask<Led_orange> OrangeTask(BIT_ORANGE);
  static LightTask<Led_red> RedTask(BIT_RED);

  xTaskCreate(Task::taskEntry,"Green Light", 256, &GreenTask, 1, nullptr);
  xTaskCreate(Task::taskEntry, "Orange Task", 256, &OrangeTask, 1, nullptr);
  xTaskCreate(Task::taskEntry, "Red Task", 256, &RedTask, 1, nullptr);


  GreenTask.init();
  OrangeTask.init();
  RedTask.init();

  //Start timer
  xTimerStart(timer,0);

  
  //Enable interrupts
   __enable_irq();
  //Start Scheduled
  vTaskStartScheduler();

}
