#pragma once
#include <cstdint>

/**
 * @brief Simple linear congruential generator (LCG) for pseudo-random numbers
 * 
 * Lightweight RNG suitable for embedded systems. Provides deterministic pseudo-random
 * sequences based on an initial seed.
 */
class SimpleRNG {
public:
    /**
     * @brief Construct a new SimpleRNG
     * 
     * @param seed Initial seed for the RNG (default = 1)
     */
    SimpleRNG(uint32_t seed = 1) : state(seed) {}

    /**
     * @brief Generate the next random number in the sequence
     * 
     * Uses the Numerical Recipes LCG parameters:
     *   state = state * 1664525 + 1013904223
     * 
     * @return uint32_t Next pseudo-random number
     */
    uint32_t next() {
        state = state * 1664525UL + 1013904223UL;
        return state;
    }

    /**
     * @brief Generate a random number within a specified range [min, max]
     * 
     * @param min Lower bound (inclusive)
     * @param max Upper bound (inclusive)
     * @return uint32_t Random number in the range [min, max]
     */
    uint32_t nextInRange(uint32_t min, uint32_t max) {
        return min + (next() % (max - min + 1));
    }

private:
    uint32_t state; ///< Current internal state of the RNG
};
