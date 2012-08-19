#ifndef SSS_N_CHOOSE_K_HPP
#define SSS_N_CHOOSE_K_HPP

#include <stdint.h>

/**
 *  This only works up 68 choose 34.  After that we need to use a big number library
 */
static inline uint64_t n_choose_k(uint32_t n, uint32_t k) {
    /**
     *  Create pascal's triangle and use it for look up
     *  implement for ints of arbitrary length (need to implement binary add and less than)
     */
    uint32_t numerator = n - (k - 1);
    uint32_t denominator = 1;

    uint64_t combinations = 1;

    while (numerator <= n) {
        combinations *= numerator;
        combinations /= denominator;

        numerator++;
        denominator++;
    }

    return combinations;
}

#endif
