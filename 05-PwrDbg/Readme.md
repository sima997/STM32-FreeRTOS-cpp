# Power, Debugging, Reliability
## 1. Tickless Idle & Low
- FreeRTOS normally runs SysTick interrupt every 1 ms -> keeps CPU awake
- **Tickless Idle** stops SysTick when no tasks are ready, and lets MCU enter `WFI` (Wait For Interrupt)
- On STM32G4 (Cortex-M4F):
  - **Sleep mode:** CPU stopped, peripherals keep running, wake on interrupt
  - **Stop/Standby modes:** deeper sleep, RAM retention may be lost (Stop 2), slower wake
- Tickless idle is configured with `configUSE_TICKLESS_IDLE` and `vPortSupressTicksAndSleep()` in FreeRTOS

## 2. Stack Overflow and Malloc Failure Hooks
- FreeRTOS provides **two weak hook functions** you can implement:
```cpp
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationMallocFailedHook(void);
```
- Enable with `configCHECK_FOR_STACK_OVERFLOW` and `configUSE_MALLOC_FAILED_HOOK`
- Useful to catch runaway tasks or out-of-memory situations

## 3. Watchdog Integration
  - STM32G4231 has **IWDG (Independent Watchdog),** running from LSI clock (~32 kHz)
  - Software must periodically "kick" the watchdog ot MCU resets
  - Best practice: run a **dedicated watchdog task** in FreeRTOS that checks all other tasks are allive (via task notifications or heartbeat counter)

## 4. Deebugging Tools
- **Segger SystemView**: Real-time tracing of FreeRTOS events (context switches, ISR, queues)
  - Requires `SEGGER_SYSVIEW_Conf.h` and trace hooks (`configUSE_TRACE_FACILITY`)
  - Connect via SWO pin (STM32G4 supports 4MHz + SWO)
- **Trace Macros**: Lightweight debug hooks inside FreeRTOS (can toggle GPIO or log over UART).

# Practice
## 1. Enable Stack Overflow Detection
1. In `FreeRTOSConfig.h`:
```cpp
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1
```
2. Implement hooks in `main.cpp` (C linkage):
```cpp
extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName) {
  //Blink LED fast or send UART error
  (void)xTask;
  (void)pcTaskName;
  for(;;);
}

extern "C" void vApplicationMallocFailedHook(void) {
  //Blink LED slow or reset
  for(;;);
}
```

## 2. Tickless Idle on STM32G431
1. in `FreeRTOSConfig.h`:
```cpp
#define configUSE_TICKLESS_IDLE 1
```
2. Cortex-M4 `port.h` already provides `vPortSupressTickAndSleep()
3. In idle task hook, enable `__WFI()` instruction:
```cpp
extern "C" void vApplicationIdleHook(void) {
  __WFI();
}   
```
4. Test: setup a periodic task(e.g. 1 Hz LED blink), system should sleep in between ticks. Measure current draw on Nucleo board.

### 4.1 Enable Tickledd Idle
In `FreeRTOSConfig.h`:
```cpp
#define configUSE_TICKLEDD_IDLE   1
#define configEXPECT_IDLE_TIME_BEFORE_SLEEP 2
```
This tells FreeRTOS: if the idle task expect at least 2 ticks od ifle time, call `vPortSupressTickAndSleep()` insteas of normal SysTick

### 4.2 Implement Idle Hook
In `main.cpp`
```cpp
extern "C" void vApplicationIdleHook(void) {
  //Wait For Interrupt - CPU goes to sleep until IRQ (SysTick, Uart, etc.)
  __WFI();
}
```

### 4.3. Heartbeat Task
Add a **1 Hz blink task:**
```cpp

void HeartbeatTask(void *arg) {
  (void)arg;
  for(;;) {
    Led::toggle();
    vTaskDelay(pdMS_TO_TICK(1000)); //Sleep 1 s
  }
}
```
In `main()`:
```cpp
xTaskCreate(HeartbeatTask, "Heartbeat", 128, NULL, 1, NULL);
```

### 4.4 Expected Behavior
1. Every second LED toggles -> proves FreeRTOS scheduler is alive
2. In between, **all tasks are blocked** -> only idle task runs -> idle hook executes `__WFI()`
3. CPU enters **sleep mode** between SysTick interrupts

### 4.5 Measurements
**Without Tickless Idle (`configUSE_TICKLESS_IDLE = 0`):**
- CPU wakes every 1 ms due to SysTick -> current draw ~6-8 mA on Nucleo board (cock dep.)
- Power trace looks like **constant activity** with small idle gaps

**With Tickless Idle (`configUSE_TICKLESS_IDLE = 1`):**
- SysTick is supressed for long idle periods (1 second here)
- CPU sleeps until **next tick or UART IRQ**
- Current drops to ~1-2 mA in sleep
- Power trace shows **long flat low-power lines** between LED toggles

### 4.6 Optional: Deeper Sleep
If you want to save more power, replace `__WFI()` with **STOP mode**:
```c
extern "C" void vApplicationIdleHook(void) {
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}
```
Then you need to reconfigure clocks on wakeup (PLL, SysTick). This is more advance.

## 3. Watchdog Task
1. Enable IWDG in RCC (32 kHz LSI)
```cpp
enum class IWDG_KR : uint32_t {RELOAD = 0xAAAA,
                                UNLOCK = 0x5555,
                                START = 0xCCCC
};

enum class IWDG_PR :uint32_t { DIV4 = 0b000,
                              DIV8 = 0b001,
                              DIV16 = 0b010,
                              DIV32 = 0b011,
                              DIV64 = 0b100,
                              DIV128 = 0b101,
                              DIV256 = 0b110

};


extern "C" void IWDG_Init(void) {
  //Prescaler 32, reload ~1000 -> ~2 s timeout
  IWDG->KR = static_cast<uint32_t>(IWDG_KR::START);
  IWDG->KR = static_cast<uint32_t>(IWDG_KR::UNLOCK);
  IWDG->PR = static_cast<uint32_t>(IWDG_PR::DIV32);
  IWDG->RLR = 1000; //~1s timeout
  IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);

}

inline void IWDG_Kick() {
  IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
}
```
2. Create **a watchdog task** in FreeRTOS:
```cpp
class WatchdogTask : public Task {
  void run() override {
    for(;;) {
      //Check if all tasks signaled "alive"

      //Kick watchdog if OK
      IWDG_Kick();
      vTaskDelay(pdMS_TO_TICK(500));
    }
    
  }
};
```
3. Each critical task must "report alive" (e.g. via `xTaskNotify` or Event Group etc.)

**Example**
```cpp
#define NUM_TASKS 3
TaskHandle_t taskHandels[NUM_TASKS];
EventGroupHandle_t aliveFlags;

class CriticalTask : public Task {
  public:
    CriticalTask(uint16_t id) : id_(id) {}
    void run() override {
      for(;;) {
        // Do some work
        vTaskDelay(pdMS_TO_TICKS(500));

        //Mark alive
        xEventGroupSetBits(alliveFlags, (1 << id_));
      }
    }
  private:
    uint16_t id_;
};

class WatchdogTask : public Task {
  public:
    void init() override {
      //initialize IWDG
    }

    void run() override {
      const EventBits_t ALL_ALIVE = constexpr((1 << NUM_TASKS) - 1);
      for(;;) {
        //Wait one period
        vTaskDelay(pdMS_TO_TICKS(1000));

        EventBits_t flags = xEventGroupgetBits(alliveFlags);

        if((flags & ALL_ALIVE) == ALL_ALIVE) {
          WatchdogKick();
          xEventGroupClearBits(alliveFlags, ALL_ALIVE);
        } else {
          //Task failed -> IWD will reset MCU automatically
        }

        //Kick watchdog if OK
        IWDG_Kick();
        vTaskDelay(pdMS_TO_TICK(500));
      } 
    }
  private:
    static inline void WatchdogKick() {
      IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
    }
};
```

## 4. SystemView Trace
1. Download Segger SystemView
2. Add config to project:
```cpp
#define configUSE_TRACE_FACILITY  1
#define configUSE_STATS_FORMATTING_FUNCTIONS  1
#define configGENERATE_RUN_TIME_STATS 1
```
3. In `main.cpp`
```cpp
SEGGER_SYSVIEW_Conf();
SEGGER_SYSVIEW_Start();
```
4. Connect SWO (pin PB3 on Nucleo-G431)


# Project: System Monitor
**Tasks:**
1. **Heartbeat Task** -> Blink LED every 1s
2. **UART Logger Task** -> Send "System Alive"
3. **Sensor Reader Task** -> Simulate ADC read, push to queue
4. **Watchdog Task** -> Checks that all tasks "ping" it once per period
**Mechanism:**
- Each task call `TaskMonitor_Alive(TaskID)` macro
- Watchdog task waits for all tasks to report alive -> if one fails, watchdog is not kicked -> MCU resets
- Induce failure (disable one task's `TaskMonitor_Alive()`) -> Watchdog should reset MCU.
