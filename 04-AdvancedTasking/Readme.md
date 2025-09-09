# Advanced Tasking
## 1. Task Notifications
- A **lightweight signaling mechanism** (each task has one 32-bit notification value)
- Faster and smaller than queues/semaphores
- APIs:
  - `xTaskNotifyGive()` / `xTaskNotifyTake()`
  - `xTaskNotify()` / `xTaskNotifyWait()`

Think of it like:
- **Semaphore replacement** -> `xTaskNotifyGive` (from ISR) + `ulTaskNotifyTake` (in task)
- **Event replacement** -> `xTaskNotify` with custom value

## 2. ISR in FreeRTOS
- ISR must use **FromISR version of API**:
  - `xQueueSendFromISR()`
  - `vTaskNotifyGiveFomISR()`
  - `xSemaphoreGiveFromISR()`
- ISR must check if a higher priority task was woken -> `portYIELD_FROM_ISR(xHigherPriorityTaskWoken);`
  - “Hey, after this ISR finishes, switch to the higher-priority task that just became ready.”
  ```cpp
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(taskHandle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  ```
  - What `portYIELD_FROM_ISR()` does?
    - If the flag is `pdTRUE`, it **triggers a PendSV interrupt** (on Cortex-M like STM32)
    - PendSV is the mechanism FreeRTOS uses to **request a context switch**
    - When the ISR exits, instead of returning to the old task, the scheduler switches to the inblocked higher-priority task


## 3. Combining ISRs and Tasks
- Rule: **Do the minimul in ISR**
- ISR should:
  - Read peripheral flag / clear it
  - Store data (or pointer to buffer)
  - Notify a task
- Task does the heavy work (parsing, printing, etc.)


# Practice
## 1. Notification Example
Task waits for ISR to notify it

```cpp

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
```

## 2. Compare Queue vs Notification
- Queue: `xQueueSendFromISR()` copies data (slower)
- Notification: updates 32-bit word (faster)

You can benchmark by measuring cycles inside ISR(with DWT (Data Watchpoint and Trace) cycle counter)
- It’s part of the Cortex-M debug/trace hardware block inside the ARM core.
- **What it does**
  - Provides **cycle counter** (`CYCCNT`) -> counts CPU cycles continously
  - Provides **comparators** for:
    - Watchpoints (break when an address is accessed)
    - PC sampling (program counter tracing)
  - Works with ITM (Instrumentation Trace Macrocell) and ETM (Embedded Trace Macrocell) for advanced debugging
- **Why you care in FreeRTOS projects**
  - The `DWT->CYCCNT` register is the **symples high-resolution timer** you get for free
  - Useful for:
    - Measuring ISR latency
    - Measuring task execution time
    - Benchmarking FreeRTOS primitives (queue vs notification, etc.)
    - Profiling without adding delay-heavy `printf`
- Included in `core_cm4.h`
  
```cpp
void dwt_init(void) {
  // Enable trace
  CoreDebug->DEMCR |= (1 << 24); //TRCENA, bit24
  // Reset counter
  DWT->CYCCNT = 0;
  // Enable counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
```

**Comparision**
- Queue = 1163 CPU cycles
- Notification = 717 CPU cycles

## 3. Wrap notification in C++

```cpp
class Notifier {
  public:
    Notifier(TaskHandle_t h) : handle(h) {}

    void giveFromISR(BaseType_t* woken) {
      vTaskNotifyGiveFromISR(handle, woken);
    }
    void wait() {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
  private:
    TaskHandle_t handle;
};
```

# Project: UART Command Parser
**Goal:**
- USART2 RX ISR -> push characters into buffer
- When newline received -> ISR notifies parser task
- Parser task reads buffer, parser command and responds