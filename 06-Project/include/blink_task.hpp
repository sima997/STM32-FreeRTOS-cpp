#pragma once
extern"C" {
    #include "FreeRTOS.h"
}
#include "gpio_template.hpp"
#include <cstdint>
#include "task_template.hpp"

template<typename LedPin>
class BlinkTask : public Task {
public:
    BlinkTask(LedPin& led) : Task("BlinkTask"), led_(led) {}

    void run() override {

        for(;;) {
            led_.toggle();
            notifyAlive();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

private:
    LedPin& led_;
};