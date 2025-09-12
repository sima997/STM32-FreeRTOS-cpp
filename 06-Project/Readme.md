# Full System Project 
## 1. System achitecture in FreeRTOS C++
- **Task separation** -> one responsibility per task
- **Queues** -> transfer ownership of data (producer -> consumer)
- **Event groups** -> synchronization (multiple tasks must agree)
- **Timer/interrupts** -> Periodic scheduling
- **Watchdog integration** -> only kick if all tasks are alive

## 2. C++ design patterns for embedded FreeRTOS
- **RAII** -> resource init/deinit automatically
- **Template drivers** -> compile-time configuration (e.g., `Uart<2, 115200>`)
- **Abstract Task base class** -> derived task implement `run()`
- **Dependency injection** -> pass `Uart&` or `Sensor&` to task, instead of globals

### 2.1 RAII (Resource Acquisition Is Initialization)
**Idea:** When you create and object, it automatically initializes hardware. When it goes out of scope, it cleans up

**Example: GPIO pin**
```cpp
class ScopedGpio {
public :
    ScopedGpio(GPIO_TypeDef* port, uint32_t pin) : port_(port), pin_(pin) {
      port_->MODER &= ~(0x3 << (pin_ * 2));
      port_->MODER |= (0x1 << (pin_ * 2));
    }

    ~ScopedGpio() {
      //Reset pin
      port_->BSRR = static_cast<uint32_t>(pin_ << 16);
    }

    void set()  {port_->BSRR = static_cast<uint32_t>(pin_); }
    void clear() {port_->BSRR = static_cast<uint32_t> (pin_ <<16); }
    void toggle() {
      if (port_->ODR & pin_) clear();
      else set();
    }

private :
  GPIO_TypeDef* port_;
  uint32 pin_;
};
```

Usage:
```cpp
void blinkRAII() {
  ScopedGpio led(GPIOA, 5); //initializes pin automatically
  for(;;) {
    led.toggle();
    vTaskDelay(pdMS_TO_TICKS(500));
  }//When function exits, destructor cleans up
}
```
### 2.2 Template Drivers
**Idea:** Configure peripherals at compile time instead of runtime (zero-cost abstraction)

**Example: UART Driver**
```cpp
template<int Instance, uint32_t Baudrate>
class Uart {
public:
  void init(uint32_t sysClk) {
    USART_TypeDef* uart = getInstance();
    uint32_t usartdiv = sysclk / Baudrate;
    uart->BRR = usartdiv;
    uart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
  }

  void send(const char* msg) {
        USART_TypeDef* uart = getInstance();
        while (*msg) {
            while (!(uart->ISR & USART_ISR_TXE_TXFNF)) {}
            uart->TDR = *msg++;
        }
  }
private:
  static constexpr USART_TypeDef* getInstance() {
    if constexpr (Instance == 2) return USART2;
    else static_assert(Instance == 2, "Unsuported UART instance");
  }

};
```

Usage:
```cpp
using Uart2 = Uart<2, 115200>;

void uartExample() {
  static Uart2 uart; //Since static, lifetime will be entire program lifetime. If non static, desctructor at the end.
  uart.init(128000000);
  uart.send("Hello ...\r\n");
}
```

### 2.3 Abstract Task Base Class
**Idea:** All tasks inherit from a common `Task` class -> enforce consistent structure

```cpp
class Task {
public:
    Task(const char* name) : name_(name) {}
    virtual ~Task() {}

    virtual void init() {}
    virtual void run() = 0;

    static void taskEntry(void *arg) {
      Task* task = static_cast<Task*>(arg);
      task->init();
      task->run();
    }
protected:
    const char* name_;
};
```

**Example: LED Task**
```cpp
class LedTask : public Task {
public:
    LedTask() : Task("Led") {}

    void init() override {
      Led::init();
    }

    void run() override {
      for(;;) {
        Led::toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
      }

    }
};
```

Usage:
```cpp

static LedTask led;
xTaskCreate(Task::taskEntry, "LED", 128, &led, 1, nullptr);
```

### 2.4 Dependency injection
**Idea:** Don't use globals; pass dependencies (drivers) into tasks at construction. This makes testing easier and removes hidden couplings

**Example: LoggerTask with injected `Uart`**
```cpp
class LoggerTask : public Task {
public:
    LoggerTask(Uart2& uart) : Task("Logger"), uart_(uart) {}

    void run() override {
      for(;;) {
        uart_.send("System allive!\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

private:
    Uart2& uart_; //Dependency injected here
}
```

Usage:
```cpp
static Uart2 uart;
static LoggerTask logger(uart);

xTaskCreate(Task::taskEntry, "Logger", 256, &logger, 1, nullptr);
```

Now `LoggerTask` is **independent of which UART is used. We could replate `Uart2` with `MockUart` for testing, or `Uart1` on different board

### 2.5 Tempale RAII
Very commond and very powerful technique. Templates let you **parameterize hardware at compile time**, and RAII ensures it's initialized safely

**Example: RAII + Template UART**
```cpp
template<int Instance, uint32_t Baudrate>
class Uart {
public:
    Uart(uint32_t sysClk) {
      USART_TypeDef* uart = getInstance();
        uint32_t usartdiv = sysclk / Baudrate;
        uart->BRR = usartdiv;
        uart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    }

    ~Uart() {
      //Disable UART on destruction
      getInstance()->CR1 = 0;
    }

    void send(const char* msg) {/*Implementation*/}

private: 
    static constexpr USART_TypeDef* getInstance() {
      if constexpr(Instance == 2) return USART2;
      else static_assert(Instance == 2, "Unsuported UART")
    }
}
```

Usage:
```cpp
void app() {
  Uart<2,115200> uart(128000000); //RAII init
  uart.send("Hello\r\n");
}//destructor disables UART here
```

### 2.6 Template RAII + Dependency Injection
**Example: `LoggerTask` (depends on UART)**
```cpp
template<typename UartDriver>
class LoggerTask : public Task {
public:
    LoggerTask(UartDriver& uart) : uart_(uart) {}

    void init() override {
        uart_.send("LoggerTask started\r\n");
    }

    void run() override {
        int counter = 0;
        char buf[64];

        for (;;) {
            int n = snprintf(buf, sizeof(buf), "Tick %d\r\n", counter++);
            if (n > 0) {
                uart_.send(buf);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

private:
    UartDriver& uart_;
};
```
- **Dependency Injection** -> `LoggerTask` does not know which UART, it just uses a reference
- **No globals** -> safer and testable

Usage:
```cpp
int main(void) {
    // System clock ~128 MHz assumed
    static Uart<2, 115200> uart2(128000000);

    // Static task instance
    static LoggerTask<Uart<2,115200>> logger(uart2);

    // Create FreeRTOS task
    xTaskCreate(
        Task::taskEntry,   // function
        "Logger",          // name
        256,               // stack words
        &logger,           // parameter = this task
        1,                 // priority
        nullptr            // no handle
    );

    vTaskStartScheduler();

    while (1) {}
}
```

- `Uart` and `LoggerTask` are **statically allocated** -> no malloc
- RAII ensures 'Uart' initialized at startup
- Templates ensures **zero overhead config**
