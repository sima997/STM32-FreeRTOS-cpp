#pragma once

/**
 * @brief Simple fixed-size moving average filter
 * 
 * Maintains a running average of the last `WindowSize` values.
 * Suitable for smoothing noisy sensor readings in embedded systems.
 * 
 * @tparam WindowSize Number of samples to include in the moving average
 */
template<size_t WindowSize>
class MovingAverageFilter {
public:
    /**
     * @brief Construct a new MovingAverageFilter
     * 
     * Initializes internal buffer and state variables.
     */
    MovingAverageFilter() : index(0), count(0), sum(0.0f) {
        for(auto &i : buffer) i = 0.0f; //Initialize buffer
    }

    /**
     * @brief Update the filter with a new value
     * 
     * Adds the new value to the circular buffer, removes the oldest value from the sum,
     * and returns the current moving average.
     * 
     * @param new_value New input value to include in the average
     * @return float Current moving average
     */
    float update(float& new_value) {
        // Substract the oldest
        sum -= buffer[index];

        // Store new value
        buffer[index] = new_value;
        sum += new_value;

        // Move index
        index = (index + 1) % WindowSize;

        // Increas count until full
        if (count < WindowSize) count++;

        return sum / count;
    }

private:
    float buffer[WindowSize];       ///< Circular buffer storing recent samples
    size_t index;                   ///< Current write position in the buffer
    size_t count;                   ///< Number of valid entries in the buffer (≤ WindowSize)
    float sum;                      ///< Running sum of the buffer values for fast averaging
};