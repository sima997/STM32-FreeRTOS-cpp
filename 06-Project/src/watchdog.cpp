#include "watchdog.hpp"
#include "stm32g431xx.h"
#include <cstdint>
#include "task_template.hpp"

void WatchdogSupervisor::init(uint32_t periodMs) {
    
    initHw(IWDG_PR::DIV32,4000);
    periodTicks = pdMS_TO_TICKS(periodMs);
    aliveFlags = xEventGroupCreate();
}

void WatchdogSupervisor::registerTask(Task* task) {
    uint32_t bit = (1 << registredTasks);
    task->registerToWatchdog(bit);
    ALL_ALIVE |= bit; //Add bit to whole dword
    registredTasks++;
}

void WatchdogSupervisor::unregisterTask(Task* task) {
    uint32_t bit = task->getAliveBit();
    ALL_ALIVE &= ~bit;
    //Not dealing with ALL_ALIVE dw fragmentation
}

void WatchdogSupervisor::taskEntry(void *pvParams) {
    (void)pvParams;
    TickType_t last = 0;
    for(;;) {
        xTaskDelayUntil(&last, periodTicks);

        EventBits_t flag = xEventGroupGetBits(aliveFlags);

        if((flag & ALL_ALIVE) == ALL_ALIVE) {
            kick();
            xEventGroupClearBits(aliveFlags, ALL_ALIVE);

        }else {
            //MCU reset
        }
    }

}

void WatchdogSupervisor::notify(uint32_t bit) {
    xEventGroupSetBits(aliveFlags, bit);
}

void WatchdogSupervisor::initHw(IWDG_PR pr, uint32_t periodMsIWDG) {
    //Prescaler 32, reload ~1000 -> ~2 s timeout
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::START);
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::UNLOCK);
    IWDG->PR = static_cast<uint32_t>(pr);
    IWDG->RLR = periodMsIWDG; //~1s timeout
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
}

inline void WatchdogSupervisor::kick() {
    IWDG->KR = static_cast<uint32_t>(IWDG_KR::RELOAD);
}