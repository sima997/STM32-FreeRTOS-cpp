#pragma once
#include <cstdint>

#define SYSTEM_CLOCK        (uint32_t)128000000

/**
 * Temperature calibration constants stored in System memory (Datasheet)
 */
#define TS_CAL1             (*reinterpret_cast<volatile uint16_t*>(0x1FFF75A8))
#define TS_CAL2             (*reinterpret_cast<volatile uint16_t*>(0x1FFF75CA))

/**
 * Temperature filtering window size
 */
#define TEMP_MA_BUF_SIZE    (uint8_t)10
