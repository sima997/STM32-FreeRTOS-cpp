#pragma once

#include "task_template.hpp"
#include "adc12_template.hpp"
#include "CommonStructures.hpp"
#include "FreeRTOS.h"
#include "queue.h"
#include "simpleRNG.hpp"
#include "task.h"



/**
 * @brief Task responsible for acquiring sensor data
 * 
 * Reads raw temperature from an ADC driver and generates simulated humidity data.
 * Sends the combined sensor data to a FreeRTOS queue at fixed intervals.
 * 
 * @tparam AdcDriver Type of ADC driver used for temperature measurement.
 */
template<typename AdcDriver>
class SensorTask : public Task {
public:
    TaskError ErrorStatus = TaskError::None;    ///< Current error status of the task

    /**
     * @brief Construct a new SensorTask
     * 
     * @param adc Reference to the ADC driver
     * @param queue Queue to send SensorData objects
     */
    SensorTask(AdcDriver& adc, QueueHandle_t queue) : Task("SensorTask"), adc_(adc), queue_(queue) {

    }

    /**
     * @brief Main task loop
     * 
     * Periodically starts ADC conversions for temperature, generates simulated humidity,
     * sends the data to the queue, and monitors task health.
     * Uses vTaskDelayUntil for precise periodic execution.
     */
    void run() override {
        SensorData data;
        uint16_t adc_data_raw;
        static TickType_t last = xTaskGetTickCount(); // For precise periodic delay
        for(;;) {
            // Acquire temperature from ADC
            auto error = adc_.start_conversion(adc_data_raw);
            ErrorStatus = map_adc_error(error);

            // Send sensor data to queue if available
            if (queue_ != nullptr) {
                data.temp_raw = adc_data_raw;
                data.humid_raw = get_humidity();
                xQueueSend(queue_, &data, 0);
            }else {
                ErrorStatus = TaskError::Queue;
            }
            notifyAlive();       // Signal to watchgdog that task is running
            checkStackUsage();  // Monitor stack usage
            vTaskDelayUntil(&last, pdMS_TO_TICKS(1000)); // For precise periodic timing
        }
        
    }
private:
    AdcDriver& adc_;          ///< Reference to the ADC driver for temperature
    QueueHandle_t queue_;     ///< Queue to send processed sensor data
    SimpleRNG rng;            ///< Random number generator for simulated humidity

 /**
     * @brief Maps ADC driver errors to task error codes
     * 
     * @param error ADC driver error
     * @return TaskError Corresponding task error
     */
    static TaskError map_adc_error(AdcError error) {
        if(error == AdcError::None) return TaskError::None;
        else return TaskError::Adc;
    }

    /**
     * @brief Generates a simulated humidity value
     * 
     * Uses a simple RNG to produce a value in the range 60.0% – 80.0% (scaled as 600–800)
     * 
     * @return uint16_t Simulated humidity raw value
     */
    uint16_t get_humidity() {
        uint32_t rand_humid = rng.nextInRange(600, 800);
        return static_cast<uint16_t>(rand_humid);
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