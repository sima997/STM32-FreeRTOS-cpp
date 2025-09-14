#pragma once
#include "task_template.hpp"
#include "uart_template.hpp"
extern "C" {
    #include "FreeRTOS.h"
    #include "queue.h"
}


template<typename UartDriver>
class LoggerTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;

    LoggerTask(UartDriver& uart, QueueHandle_t queue) : Task("LoggerTask"), uart_(uart), queue_(queue) {

    }

    void run() override {
        float processed_data;
        for(;;) {
            char buffer[64] = {};
            if (queue_ != nullptr) {
                if(xQueueReceive(queue_, &processed_data, portMAX_DELAY) == pdPASS) {
                    snprintf(buffer,sizeof(buffer),"Core temperature: %.1f deg C\n", processed_data);
                    uart_.send(buffer);
                }
            }else {
                ErrorStatus = TaskError::Queue;
            }  
        }        
    }
private:
    UartDriver& uart_;
    QueueHandle_t queue_;

    static TaskError map_uart_error(UartError error) {
        if(error == UartError::None) return TaskError::None;
        else return TaskError::Adc;
    }
};