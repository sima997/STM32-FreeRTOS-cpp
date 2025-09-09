#define QUEUE_OR_NOTIFICATION     1 //0 - QUEUE, 1- NOTIFICATION

extern "C" {
  #include "FreeRTOS.h"
  #include "task.h"
  #include "stm32g431xx.h"
  #include "core_cm4.h"

  #if QUEUE_OR_NOTIFICATION == 1
    #include "queue.h"
  #endif
}

#include <cstdint>
#include "task_template.hpp"
#include "gpio_template.hpp"

using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Button = Gpio<GpioPort::C, 13, GpioMode::Input>;




#if QUEUE_OR_NOTIFICATION == 1
  QueueHandle_t queH;
#endif

TaskHandle_t blinkHandle;

volatile uint32_t lastIsrCycles = 0;

extern "C" void EXTI15_10_IRQHandler(void) {
  uint32_t start = DWT->CYCCNT;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  #if QUEUE_OR_NOTIFICATION == 1
    bool pressed = true;
    if(queH != nullptr) xQueueSendFromISR(queH, &pressed, &xHigherPriorityTaskWoken);
  #else
    if(blinkHandle != nullptr) vTaskNotifyGiveFromISR(blinkHandle, &xHigherPriorityTaskWoken);
  #endif
  EXTI->PR1 = EXTI_PR1_PIF13;
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  uint32_t end = DWT->CYCCNT;
  uint32_t cycles = end - start;

  lastIsrCycles = cycles;

}

template<typename LedPin>
class BlinkTask : public Task {
  public:
    void init() override {
      LedPin::init();
    }

    void run() override {
      for(;;) {
        #if QUEUE_OR_NOTIFICATION == 1
        bool event = false;
          if(xQueueReceive(queH, &event, portMAX_DELAY) == pdPASS) {
            if(event) {
              LedPin::toggle();
            }
          }
        #else
          ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
          LedPin::toggle();
        #endif       
        
      }
    }
};

void dwt_init(void) {
  // Enable trace
  CoreDebug->DEMCR |= (1 << 24); //TRCENA, bit24
  // Reset counter
  DWT->CYCCNT = 0;
  // Enable counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

int main(void) {

  //Init DWT
  dwt_init();

  //Initialize button interrupt
  Button btn;
  btn.init();
  btn.setInterrupt(GpioIrqEdge::Falling);
  #if QUEUE_OR_NOTIFICATION == 1
    queH = xQueueCreate(1,sizeof(bool));
  #endif

  static BlinkTask<Led> blinkTask;

  xTaskCreate(Task::taskEntry, "Blink", 256, &blinkTask, 1, &blinkHandle);
  
  // 3. NVIC priority and enable
  NVIC_SetPriority(EXTI15_10_IRQn, 10); // safe with FreeRTOS
  NVIC_EnableIRQ(EXTI15_10_IRQn);
  __enable_irq();
  vTaskStartScheduler();
  while(1) {}
}