#pragma once

template<size_t WindowSize>
class MovingAverageFilter {
public:
    MovingAverageFilter() : index(0), count(0), sum(0.0f) {
        for(auto &i : buffer) i = 0.0f; //Initialize buffer
    }

    float update(float& new_value) {
        //Substract the oldest
        sum -= buffer[index];

        // Store new value
        buffer[index] = new_value;
        sum += new_value;

        // Move index
        index = (index + 1) % WindowSize;

        //increas count until full
        if (count < WindowSize) count++;

        return sum / count;
    }

private:
    float buffer[WindowSize];
    size_t index;
    size_t count;
    float sum;
};