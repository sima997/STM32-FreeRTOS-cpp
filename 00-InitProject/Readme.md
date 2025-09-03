# How to setup a FreeRTOS C++ project
## Step 1: Install Toolchain & Tools
- **ARM GCC compiler:** `gcc-arm-none-eabi`
- **CMake or Make:**
- **OpenOCD:** for flashing/debugging with ST-LINK

## Step 2: Project Structure
Create a minimal structure
```
freertos-stm32-cpp/
|-- CmakeList.txt    (or Makefile)
|-- src/
|   |-- main.cpp
|   |-- startup_stm32g431xx.s
|   |-- system_stm32g4xx.c
|-- include/
|   |-- stm32g4xx.h
|-- freertos/
|   |-- FreeRTOSConfig.h
|   |-- ...
|   |-- (FreeRTOS-Kernel sources)
|-- linker/
|   |-- stm32g431.ld
```
## Step 3: Get FreeRTOS
Clone FreeRTOS kernel:
```bash
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos/FreeRTOS-Kernel
```

For Cortex-M4F (G431 has FPU), include:
- `tasks.c`
- `queue.c`
- `list.c`
- `timers.c`
- `event_groups.c`
- `portable/GCC/ARM_CM4F/port.c`
- `portable/MemMang/heap_4.c`

## Step 4: Linker script and Startup
Linker script could define explicitly heap section in RAM or it can be allocated by FreeRTOS automatically  in `.bss` section. Depends on requirements for the system.

```ld
/* Explicit FreeRTOS heap section (optimal) */
    .heap (NOLOAD) :
    {
        __heap_start__ = .;
        . = . +8K;      /* Reserve 8 KB for FreeRTOS heap*/
        __heap_end__ = .;
    } > RAM
```

Used startup is generic one from CMSIS. Followind modifications are necesary for FreeRTOS to work
### Supervisor Call (aka Software Interrupt)
- On Cortex-M, an `SVC` triggers this handler
- When you call `vTaskStartScheduler()`, FreeRTOS executes an `svc` instruction to switch from the initial `main()` context into the first task context
- `vPortSVCHandler` sets up the first task's stack frame and starts the scheduler

Think of it as the **entry point into multitasking.**

In startup file replace
```asm
.weak   SVC_Handler
.thumb_set SVC_Handler, Default_Handler
```
with
```asm
.weak   vPortSVCHandler
.thumb_set SVC_Handler, vPortSVCHandler
```

### Pended Supervisor Call
- This is a special exception designed for context switching -- it has the lowest priority so it only runs when nothing more urgent is pending.
- Whenever FreeRTOS decides to switch tasks (e.g. tick interrupt, yield, semaphore release), it sets the PendSV bit in the NVIC. Then, when interrupts finish, the CPU triggers PendSV.
- `xPortPendSVHandler` does the **actual context switch**:
  - Saves the registers of the current task
  - Loads the registers of the current task
  - Returns to thread mode, now running the new task

So PendSV = **the context switch engine**

In startup file replace
```asm
.weak   PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
```
with
```asm
.weak   xPortPendSVHandler
.thumb_set PendSV_Handler, xPortPendSVHandler
```

### SysTick Handler
- A built-in timer in Cortex-M that fires at a fixed rate (usually 1 kHz for RTOS)
- The SysTick interrupt is the **heartbeat** of the OS. Every tick (e.g. every 1 ms):
  - Increments the RTOS tick counter
  - Wakes up delayed tasks
  - Triggers a context switch if a higher-priority task is now ready

So SysTick = **the scheduler's clock**

In startup file replace
```asm
.weak   SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
```
with
```asm
.weak   xPortSysTickHandler
.thumb_set SysTick_Handler, xPortSysTickHandler
```

**Note: What is `.thumb_set` does?**
- It is a **GNU assembler directive** (not an instruction for the CPU)
- It tells the assembler: "Make symbol **A** an alias for symbol **B**. Both names refer to the same address"

So:
```asm
.thumb_set SVC_Handler, vPortSVCHandler
```
means:

`SVC_Handler` is just another name for `vPortSVCHandler`

## Step 5: FreeRTOSConfig.h
Example minimal config (for STM32G4, 128 Mhz max, 1 kHz tick)

```C
#define configCPU_CLOCK_HZ         ( ( unsigned long ) 128'000'000 )
#define configTICK_RATE_HZ         ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES       ( 5 )
#define configMINIMAL_STACK_SIZE   ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE      ( ( size_t ) ( 10 * 1024 ) )
#define configUSE_MUTEXES          1
#define configUSE_TIMERS           1
#define configTIMER_TASK_PRIORITY  3
#define configUSE_PREEMPTION       1
#define configUSE_IDLE_HOOK        0
#define configUSE_TICK_HOOK        0
```

## Step 6: Minimal C++ FreeRTOS Example
`src/main.cpp`
```cpp
cpp

extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "stm32g4xx.h"
}

class BlinkTask {
public :
    BlinkTask(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {}

    void run() {
        while (1) {
            port->ODR ^= pin; // toggle pin
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    static void adapter(void* pv) {
        static_cast<BlinkTask>(pv)->run();
    }

private :
    GPIO_TypeDef* port;
    uint16_t pin;
};

extern "C" void SystemInit(void) {
    // Enable GPIOB clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    // PB0 output
    GPIOB->MODER &= ~(3U << (0 * 2));
    GPIOB->MODER |=  (1U << (0 * 2));    
}

int main(void) {
    BlinkTask task(GPIOB, 1U << 0);

    xTaskCreate(BlinkTask::adapter, "Blink", 256, &task, 1, nullptr);

    vTaskStartScheduler();

    while(1);
}
```

This:
- Configures PB0 (LED)
- Creates FreeRTOS task in C++ class style
- Toggles LED every 500ms


## Step 7: CMakeList.txt (example)


