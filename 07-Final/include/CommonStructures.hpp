#pragma once
#include <cstdint>

/**
 * @brief Raw sensor readings
 * 
 * Contains raw 16-bit values read directly from temperature and humidity sensors.
 */
struct SensorData {
    uint16_t temp_raw;      ///< Raw temperature reading
    uint16_t humid_raw;     ///< Raw temperature reading
};

/**
 * @brief Processed sensor data
 * 
 * Contains floating-point values converted from raw sensor readings
 * for easier interpretation and logging.
 */
struct ProcessedData {
    float temperature;      ///< Temperature in degrees Celsius
    float humidity;         ///< Relative humidity in percentage
};

/**
 * @brief Mapping between human-readable messages and command strings
 * 
 * Used to send commands to the CommandHandlerTask or log status messages.
 */
struct Message {
    const char* key;        ///< Human-readable description of the command
    const char* value;      ///< Command string sent to the system
};


/**
 * @brief Predefined list of system messages and corresponding command strings
 * 
 * This array can be used to display available commands or send
 * specific instructions programmatically.
 */
constexpr Message messages[] = {
    {"Status LED - ON", "LED ON"},
    {"Status LED - OFF", "LED OFF"},
    {"Temperature log start",    "LOG TEMP"},
    {"Temperature log stop",    "STOP TEMP"},
    {"Humidity log start",    "LOG HUMID"},
    {"Humidity log stop",    "STOP HUMID"},
    {"Sensors status",       "STATUS"},
    {"List commands",       "-help"}
};