#pragma once
extern "C" {
    #include "FreeRTOS.h"
    #include "queue.h"
}
#include <cstdint>
#include "task_template.hpp"
#include "moving_avg_template.hpp"
#include "device_defines.hpp"
#include "CommonStructures.hpp"
#include "task.h"

/**
 * @brief State machine for ProcessingTask
 */
enum class ProcessingState : uint8_t {Idle = 0, Receive, Process, Send, Error};

/**
 * @brief Task responsible for processing raw sensor data
 * 
 * Reads raw sensor data from a queue, applies calibration and optional filtering,
 * and forwards processed data to another queue. Implements a simple state machine:
 * Idle -> Receive -> Process -> Send -> Receive.
 */
class ProcessingTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;     ///< Current error status of the task

    /**
     * @brief Construct a new ProcessingTask
     * 
     * @param sensor_queue Queue containing raw sensor data
     * @param process_queue Queue for sending processed sensor data
     */
    ProcessingTask(QueueHandle_t sensor_queue, QueueHandle_t process_queue) : 
    Task("ProcessingTask"), sensor_queue_(sensor_queue), process_queue_(process_queue) {}

    /**
     * @brief Main task loop
     * 
     * Implements a state machine to:
     * 1. Initialize calibration (Idle)
     * 2. Receive raw sensor data (Receive)
     * 3. Process data (Process)
     * 4. Send processed data (Send)
     * 5. Handle errors (Error)
     * 
     * Uses moving average filtering for temperature.
     */
    void run() override {
        SensorData rec_data;
        ProcessedData proc_data;
        ProcessingState state = ProcessingState::Idle;

        for(;;) {
            switch(state) {
                case ProcessingState::Idle: {
                    // Initialize calibration coefficients from system memory
                    if(sensor_queue_ != nullptr && process_queue_ != nullptr) {
                        t_cal1 = TS_CAL1;
                        t_cal2 = TS_CAL2;
                        state = ProcessingState::Receive;
                    } else {
                        ErrorStatus = TaskError::Queue;
                        state = ProcessingState::Error;
                    }
                    break;
                }
                    
                case ProcessingState::Receive:{
                    // Wait for raw sensor data from queue
                    if(xQueueReceive(sensor_queue_, &rec_data, portMAX_DELAY) == pdPASS) state = ProcessingState::Process;
                    break;
                }
                    
                case ProcessingState::Process:{
                    // Convert raw sensor values to calibrated physical units
                    float temperature = raw_to_temp(rec_data.temp_raw);
                    float humidity = raw_to_humid(rec_data.humid_raw);
                    // Apply moving average filter to temperature
                    proc_data.temperature = mavg_filt_temp.update(temperature);
                    
                    // Humidity currently unfiltered
                    proc_data.humidity = humidity;
                    state = ProcessingState::Send;
                    break;
                }
                case ProcessingState::Send:{
                    // Send processed data to the next queue
                    if(xQueueSendToBack(process_queue_, &proc_data, 0) == pdPASS) state = ProcessingState::Receive;
                    
                    break;
                }
                case ProcessingState::Error:{
                    // TODO: Implement retry or recovery strategy
                    break;
                }
            }
            checkStackUsage(); // Monitor stack usage for safe
            notifyAlive(); // Indicate to watchdog task is active
            
        }
    }

private:
    QueueHandle_t sensor_queue_;        ///< Queue handle for raw sensor data
    QueueHandle_t process_queue_;       ///< Queue handle for processed data
    

    MovingAverageFilter<TEMP_MA_BUF_SIZE> mavg_filt_temp;       ///< Moving average filter for temperature
    MovingAverageFilter<HUMID_MA_BUF_SIZE> mavg_filt_humid;     ///< Moving average filter for humidity

    uint16_t t_cal1 = 0, t_cal2 = 0; ///< Temperature calibration constant 1 an 2

    /**
     * @brief Convert raw temperature ADC reading to degrees Celsius
     * 
     * @param raw_data Raw ADC value
     * @return float Temperature in Celsius
     */
    float raw_to_temp(uint16_t raw_data) {
        float raw_adj = raw_data * (3.3f / 3.0f);
        float temp = ((float)(raw_adj - t_cal1)) * (130.0f - 30.0f);
        temp /= (float)(t_cal2 - t_cal1);
        temp += 30.0f;

        return temp;
    }

    /**
     * @brief Convert raw humidity ADC reading to percentage
     * 
     * @param raw_data Raw ADC value
     * @return float Relative humidity in %
     */
    float raw_to_humid(uint16_t raw_data) {
        return static_cast<float>(raw_data / 10);
    }

        /**
     * @brief Checks remaining stack space for the task
     * 
     * Uses FreeRTOS uxTaskGetStackHighWaterMark() to detect potential stack overflow.
     * Threshold of 50 words (~200 bytes on 32-bit MCU) is considered low.
     */
    void checkStackUsage() {
        UBaseType_t minFreeStack = uxTaskGetStackHighWaterMark(nullptr); // nullptr = current task
        if (minFreeStack < 50) { 
            // TODO: Implement warning/logging mechanism for low stack
        }
    }

  
};