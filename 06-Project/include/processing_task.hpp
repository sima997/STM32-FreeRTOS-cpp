#pragma once
extern "C" {
    #include "FreeRTOS.h"
    #include "queue.h"
}
#include <cstdint>
#include "task_template.hpp"
#include "moving_avg_template.hpp"
#include "device_defines.hpp"

enum class ProcessingState : uint8_t {Idle = 0, Receive, Process, Send, Error};


class ProcessingTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;

    ProcessingTask(QueueHandle_t sensor_queue, QueueHandle_t process_queue) : 
    Task("ProcessingTask"), sensor_queue_(sensor_queue), process_queue_(process_queue) {}

    void run() override {
        uint16_t raw_data;
        float temperature;
        float temperature_filt;
        ProcessingState state = ProcessingState::Idle;

        for(;;) {
            if(sensor_queue_ != nullptr) {
                
            }

            switch(state) {
                case ProcessingState::Idle:
                    if(sensor_queue_ != nullptr && process_queue_ != nullptr) {
                        // Read calibration coefficients
                        t_cal1 = TS_CAL1;
                        t_cal2 = TS_CAL2;
                        state = ProcessingState::Receive;
                    } else {
                        ErrorStatus = TaskError::Queue;
                        state = ProcessingState::Error;
                    }
                    break;
                case ProcessingState::Receive:
                    if(xQueueReceive(sensor_queue_, &raw_data, portMAX_DELAY) == pdPASS) state = ProcessingState::Process;
                    break;
                case ProcessingState::Process:
                    temperature = raw_to_temp(raw_data);
                    temperature_filt = mavg_filt.update(temperature);
                    state = ProcessingState::Send;
                    break;
                case ProcessingState::Send:
                    if(xQueueSendToBack(process_queue_, &temperature_filt, 0) == pdPASS) state = ProcessingState::Receive;
                    break;
                case ProcessingState::Error:
                    //TODO: Retrial
                    break;
            }

        }
    }

private:
    QueueHandle_t sensor_queue_;
    QueueHandle_t process_queue_;

    MovingAverageFilter<TEMP_MA_BUF_SIZE> mavg_filt;

    uint16_t t_cal1 = 0, t_cal2 = 0; 

    float raw_to_temp(uint16_t raw_data) {
        float raw_adj = raw_data * (3.3f / 3.0f);
        float temp = ((float)(raw_adj - t_cal1)) * (130.0f - 30.0f);
        temp /= (float)(t_cal2 - t_cal1);
        temp += 30.0f;

        return temp;
    }

  
};