#pragma once

/**
 * @brief Bitmask commands used for inter-task signaling
 * 
 * Each command corresponds to a single bit in an EventGroup.
 * Tasks can set or clear these bits to trigger actions in other tasks.
 */
enum class CommandType : uint32_t {
    LED = (1UL << 0),
    TEMP = (1UL << 1),
    HUMID = (1UL << 2),
    STATUS = (1UL << 3),
    HELP = (1UL << 4)
};

