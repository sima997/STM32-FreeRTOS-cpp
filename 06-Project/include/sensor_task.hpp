#pragma once
#include <cstdio>
#include "task_template.hpp"
#include "adc12_template.hpp"
extern "C" {
    #include "FreeRTOS.h"
    #include "queue.h"
}



template<typename AdcDriver>
class SensorTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;

    SensorTask(AdcDriver& adc, QueueHandle_t queue) : Task("SensorTask"), adc_(adc), queue_(queue) {

    }

    void run() override {
        uint16_t raw_data;
        static TickType_t last = xTaskGetTickCount();
        for(;;) {
            auto error = adc_.start_conversion(raw_data);
            ErrorStatus = map_adc_error(error);
            if (queue_ != nullptr) {
                xQueueSend(queue_, &raw_data, 0);
            }else {
                ErrorStatus = TaskError::Queue;
            }
            vTaskDelayUntil(&last, pdMS_TO_TICKS(1000)); //For precise timing
        }
        
    }
private:
    AdcDriver& adc_;
    QueueHandle_t queue_;

    static TaskError map_adc_error(AdcError error) {
        if(error == AdcError::None) return TaskError::None;
        else return TaskError::Adc;
    }
};