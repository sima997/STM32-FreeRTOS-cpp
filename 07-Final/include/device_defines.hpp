#pragma once
#include <cstdint>

/**
 * @brief System clock frequency
 * 
 * Used for timing calculations, delays, and peripheral configurations.
 */
#define SYSTEM_CLOCK        (uint32_t)128000000 ///< System clock in Hz

/**
 * @brief Temperature calibration constants
 * 
 * Stored in system memory according to the microcontroller datasheet.
 * Used to convert raw temperature sensor readings to degrees Celsius.
 */
#define TS_CAL1             (*reinterpret_cast<volatile uint16_t*>(0x1FFF75A8)) ///< Calibration point 1 (low temperature)
#define TS_CAL2             (*reinterpret_cast<volatile uint16_t*>(0x1FFF75CA)) ///< Calibration point 2 (high temperature)

/**
 * @brief Moving average buffer sizes for sensor filtering
 * 
 * Defines the number of samples used to smooth raw temperature and humidity readings.
 */
#define TEMP_MA_BUF_SIZE    (uint8_t)6      ///< Temperature moving average window
#define HUMID_MA_BUF_SIZE    (uint8_t)8     ///< Humidity moving average window
