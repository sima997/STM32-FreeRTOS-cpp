# Synchronization & Resource Management
## 1. Event groups
- **What:** A FreeRTOS object where each bit in a word is a flag
- **Why:** Lets multiple tasks wait for multiple conditions simultaneously
- **API:**
  - `xEventGroupCreate()`
  - `xEventGroupSetBits()` / `xEventGroupClearBits()`
  - `xEventGroupWaitBits()`
Example: `BIT_RED_READY | BIT_GREEN_READY` -> task waits until both lights are ready

## 2. Software timers vs. Hardware timers
- **Hardware Timer** = STM32 peripheral (TIM1, TIM2, ...). Runs independently of FreeRTOS, Good for precise PWM or interrupts.
- **Software Timer** = Managed by FreeRTOS in the timer task. Runs after tick ISR wakes timer task. Good for periodic scheduling without consuming hardware resources.
- **API:**
  - `xTimerCreate()`
  - `xTimerStart()` / `xTimerStop()`
  - Callback -> executed in **timer task context**, not interrupt

## 3. Static vs Dynamic Allocation
- Dynamic: `xTaskCreate()` / `xQueueCreate()` -> memory comes from FreeRTOS heap
- Static: `xTaskCreateStatic()`, `xQueueCreateStatic()` -> you provide the buffer/stack
- Use static if:
  - System with limited RAM
  - Safety-critical (no fragmentation allowed)
- Use dynamic if:
  - Flexibility, tasks created/destroyed on demand

## 4. C++ `new`/`delete` with FreeRTOS
- By default, `new`/`delete` use `malloc`/`free`
- If you use FreeRTOS heap (`heap_4.c`), you must redirect them
- Override global operators:
```cpp
void* operator new(size_t size) {
  return pvPortMalloc(size);
}

void operator delete(void* ptr) noexcept {
  vPortFree(ptr);
}
```
Now `new` and `delete` are FreeRTOS-aware

# Practice
## 1. Event Group Demo
Create two tasks: one sets event bits, snother waits
```cpp

EventGroupHandle_t eg;

#define BIT_TASK1     (1 << 0)
#define BIT_TASK2     (1 << 1)

class Task1 : public Task {
  void run() override {
    for(;;) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      xEventGroupSetBit(eg, BIT_TASK1);
    }
  }
};

class Task2 : public Task {
  void run() override {
    for(;;) {
      vTaskDelay(pdMS_TO_TICKS(1500));
      xEventGroupSetBit(eg, BIT_TASK2);
    }
  }
};

class Coordinator : public Task {
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
```

## 2. Software Timer Demo
```cpp

void vTimerCallback(TimerHandle_t xTimer) {
  GPIO->ODR ^= (1 << 5); //Blink LED every 500ms
}

TimerHandle_t ledTimer;

ledTimer = xTimerCreate("LED", pdMS_TO_TICKS(500),
                        pdTRUE,
                        nullptr,
                        vTimerCallback);

xTimerStart(ledTimer, 0);
```

## 3. Heap Schemes Experiment
- Compile with different `heap_x.c` files:
  - `heap_1.c` -> tasks cannot be deleted (no free)
  - `heap_2.c` -> free works, but fragmentation possible
  - `heap_4.c` -> most flexible, recomended.
  - `heap_5.c` -> muti-region (useful if RAM is split)
- Measure memory with `xPortGetFreeHeapSize()` and `xPortGetMinimumEverFreeHeapSize()`

```cpp
printf("Free: %u, Min Ever: %u\r\n",
      xPortGetFreeHeapSize(),
      xPortGetMinimumEverFreeHeapSize());
```

# Project: Trafic Light Controller
**Goal:** Simulate a trafic light using FreeRTOS tasks, event groupt and software timer
- **LEDs:**
  - PA5 = Green
  - PB0 - Yellow
  - PB7 = Red
- **Event group bits**:
  - `BIT_RED`, `BIT_GREEN`, `BIT_YELLOW`
- **Software timer:** Fires every second -> sets event for next state