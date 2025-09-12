#pragma once

#include <cstdint>

//Forward declaration of Task
class Task;

extern "C" {
    #include "FreeRTOS.h"
    #include "event_groups.h"
    
}

//Class Enums
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


class WatchdogSupervisor {
public:
    /*Initialize IWDG*/
    static void init(uint32_t periodMs);
    /*Register task to check*/
    static void registerTask(Task* task);
    /*Unregister task to check*/
    static void unregisterTask(Task* task);
    /*Notify Watchdog*/
    static void notify(uint32_t bit);

    /*Task entry function*/
    static void taskEntry(void *pvParams);


private:
    inline static EventGroupHandle_t  aliveFlags;
    inline static uint32_t registredTasks = 0;
    inline static uint32_t ALL_ALIVE = 0;
    inline static TickType_t periodTicks;

    static inline void kick();
    static void initHw(IWDG_PR pr, uint32_t periodMs);
};