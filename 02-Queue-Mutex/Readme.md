# Tasks and Communication
## 1. Tasks in FreeRTOS + C++
- Tasks must have signature `void(*)(void*)`.
- In C++, we wrap this with a `static` entry function that casts the pointer back to `this`

```cpp
class Task {
public :
    virtual void run() = 0;

    static void taskEntry(void* pvParams) {
      static_cast<Task*>(pvParams)->run();
    }

};
```

Derived tasks (Blink, UART, etc.) override `run()`.

## 2. Delays
- `vTaskDelay(ticks)` suspend task for N ticks.
- `vTaksDelayUntil(&lastWakeTime, ticks)` -> periodic timing, avoids drift
- Ticks depend on `configTICK_RATE_HZ` (e.g., 1000 -> 1 ms)

## 3. Queues
- Thread-safe FIFO
- FreeRTOS API:
  - `xQueueCreate(len, item_size)`
  - `xQueueSend` / `xQueueReceive` 
- In C++, wrap with templates:

```cpp
template<typename T>
class Queue {
  QueueHandle_t q;
public :
  Queue(size_t length) {
      q = xCreateQueue(length, sizeof(T));
  }

  bool send(const T& item, TickType_t wait = 0) {
    return xQueueSend(q, &item, wait) == pdPass;
  }

  bool receive(T& item, TickType_t wait = portMAX_DELAY) {
    return xQueueReceive(q, &item, wait) == pdPass;
  }
};
```

## 4. Mutex and Semaphore
- **Mutex** -> resource protection (e.g. UART). Supports **priority inheritance**
- **Binary semaphore** -> signal from ISR to task
- Use `xSemaphoreGiveFromISR()` in interrupts

# Practice
## 1. Implement Task Wrapper
```cpp

class BlinkTask : public Task {
  void run() override {
      while(1) {
        GPIOA->ODR ^= (1 << 5); //Toggle PA5
        vTaskDelay(pdMS_TO_TICKS(500));
      }
  }
};
```

Startup:
```cpp
BlinkTask blink;
xTaskCreate(Task::taskEntry, "Blink", 256, &blink, 1, nullptr);
vTaskStartScheduler();
```

## 2. Two Tasks Exchanging Messages

```cpp
Queue<int> q(5);

class Producer : public Task {
  void run() override {
    int counter = 0;
    while(1) {
      q.send(counter++);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
};


class Consumer : public Task {
  void run() override {
    int value;
    while(1) {
      if (q.receive(value)) {
        GPIO->ODR ^= (1 << 5);
        vTaskDelay(pdMS_TO_TICKS(100));
      }
    }
  }
};
```
## 3. Protect Shared UART with Mutex

```cpp
Queue<char> uartQ(32);
SemaphoreHandle_t uartMutex;

class UARTTask : public Task {
  void run() override {
    char c;
    while(1) {
      if (uartQ.receive(c)) {
        xSemaphoreTake(uartMutex, portMAX_DELAY);
        while (!(USART2->ISR & USART_ISR_TXE)) {}
        USART2->TDR = c;
        xSemaphoreGive(uartMutex);
      }
    }
  }
};
```