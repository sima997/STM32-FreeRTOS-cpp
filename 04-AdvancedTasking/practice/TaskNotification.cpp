extern "C" {
  #include "FreeRTOS.h"
  #include "task.h"
  #include "stm32g431xx.h"
}

#include <cstdint>
#include "task_template.hpp"
#include "gpio_template.hpp"

using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Button = Gpio<GpioPort::C, 13, GpioMode::Input>;


TaskHandle_t blinkHandle;


extern "C" void EXTI15_10_IRQHandler(void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if(blinkHandle != nullptr) vTaskNotifyGiveFromISR(blinkHandle, &xHigherPriorityTaskWoken);
  EXTI->PR1 = EXTI_PR1_PIF13;
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

template<typename LedPin>
class BlinkTask : public Task {
  public:
    void init() override {
      LedPin::init();
    }

    void run() override {
      for(;;) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        LedPin::toggle();
      }
    }
};

int main(void) {

  //Initialize button interrupt
  Button btn;
  btn.init();
  btn.setInterrupt(GpioIrqEdge::Falling);

  static BlinkTask<Led> blinkTask;

  xTaskCreate(Task::taskEntry, "Blink", 256, &blinkTask, 1, &blinkHandle);
  
  // 3. NVIC priority and enable
  NVIC_SetPriority(EXTI15_10_IRQn, 10); // safe with FreeRTOS
  NVIC_EnableIRQ(EXTI15_10_IRQn);
  __enable_irq();
  vTaskStartScheduler();
  while(1) {}
}