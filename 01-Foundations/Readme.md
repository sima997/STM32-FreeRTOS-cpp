# RTOS Fundamentals
## 1. Why RTOS instead of bare-metal
- Bare metal: single loop, sequential execution, blocking delays
- ROTOS advantages:
  - Multitasking with concurent tasks
  - Deterministic timing and deadlines
  - Better CPU utilization for periodic/interrupt-driven tasks

## 2. Key RTOS concepts
- **Task:** a thread of execution
- **Scheduler:** decides which task runs based on priority and state
- **States:** Running, Ready, Blocked, Suspended
- **Context switching** saving CPU registers to switch between tasks
- **Priorities:** Preemptive vs cooperative scheduling

# FreeRTOS Kernel Overview
## 1. FreeRTOS components
- **Kernel:** manages tasks, scheduling, and system tick
- **Tick rate:** frequency of system interrupts (`configTICK_RATE_HZ`)
- **Heap options:** `heap_1`...`heap_5` (dynamic vs static memory)
- Idle and timer tasks

## 2. Configuration (`FreeRTOSConfig.h`)
- Define max priorities, stack size, tick rate
- Enable/disable features like software timers, mutexes

# FreeRTOS + C++ Integration
## 1. Calling C FreeRTOS APIs from C++
- `extern "C"` avoids name mangling for C headers
- Example
    ```cpp
    extern "C" {
        #include "FreeRTOS.h"
        #include "task.h"
    }
    ```
## 2. Using member functions as tasks
- Task require static functions:
    ```cpp
    class MyTask {
    public:
        void run();

        static void taskAdapter(void* pvParameters) {
            static_cast<MyTask*>(pvParameters)->run();
        }
    };
    ```

## 3. Global/static constructors
- Avoid heavy constructors before scheduler starts
- Prefer `init()` methods for initialization

# Project: Blinker Scheduler
- Implement 3 tasks with different behavior:
  - **Task A:** Blink LED at 1 Hz
  - **Task B:** Blink LED at 2 Hz
  - **Task C:** Print debug message to UART/console every 1 second 
- Extra:
  - Add a 4th task that toggles LED at irregular interval using `vTaskDelayUntil()` for precise periodic timing
  - Measure CPU idle time with `vTaskGetRunTimeStats()` (if supported)