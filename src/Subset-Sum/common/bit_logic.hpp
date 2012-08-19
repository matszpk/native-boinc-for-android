#ifndef SSS_BIT_LOGIC_HPP
#define SSS_BIT_LOGIC_HPP

#include "stdint.h"

#include <limits>
#include <climits>

//#include "../common/output.hpp"


const uint32_t ELEMENT_SIZE = sizeof(uint32_t) * 8;                      

#ifdef HTML_OUTPUT                                                               
extern double max_digits;                                                        
extern double max_set_digits;                                                    
#endif                                                                           

extern unsigned long int max_sums_length;                                        
extern uint32_t *sums;                                                       
extern uint32_t *new_sums;                                                   

using std::numeric_limits;

//void shift_left(uint32_t *dest, const uint32_t length, const uint32_t *src, const uint32_t shift);
//void or_equal(uint32_t *dest, const uint32_t length, const uint32_t *src);
//void or_single(uint32_t *dest, const uint32_t length, const uint32_t number);

/**
 *  Shift all the bits in an array of to the left by shift. src is unchanged, and the result of the shift is put into dest.
 *  length is the number of elements in dest (and src) which should be the same for both
 *
 *  Performs:
 *      dest |= dest << shift
 */
static inline void shift_left_or_equal(uint32_t *dest, const uint32_t length, const uint32_t shift) {
    uint32_t full_element_shifts = shift / ELEMENT_SIZE;
    uint32_t sub_shift = shift % ELEMENT_SIZE;

//    printf("shift: %u, full_element_shifts: %u, sub_shift: %u\n", shift, full_element_shifts, sub_shift);

    /**
     *  Note that the shift may be more than the length of an uint32_t (ie over 32), this needs to be accounted for, so the element
     *  we're shifting from may be ahead a few elements in the array.  When we do the shift, we can do this quickly by getting the target bits
     *  shifted to the left and doing an or with a shift to the right.
     *  ie (if our elements had 8 bits):
     *      00011010 101111101
     *  doing a shift of 5, we could update the first one to:
     *     00010111         // src[i + (full_element_shifts = 0) + 1] >> ((ELEMENT_SIZE = 8) - (sub_shift = 5)) // shift right 3
     *     |
     *     01000000
     *  which would be:
     *     01010111
     *  then the next would just be the second element shifted to the left by 5:
     *     10100000
     *   which results in:
     *     01010111 10100000
     *   which is the whole array shifted to the left by 5
     */
    uint32_t i;
    if ((ELEMENT_SIZE - sub_shift) == 32) {
        for (i = 0; i < (length - full_element_shifts) - 1; i++) {
            dest[i] |= dest[i + full_element_shifts] << sub_shift;
        }
    } else {
        for (i = 0; i < (length - full_element_shifts) - 1; i++) {
            dest[i] |= dest[i + full_element_shifts] << sub_shift | dest[i + full_element_shifts + 1] >> (ELEMENT_SIZE - sub_shift);
        }
    }

    dest[i] |= dest[length - 1] << sub_shift;
    i++;

    /*for (; i < length; i++) {
        dest[i] = 0;
    }*/
}

/**
 *  Shift all the bits in an array of to the left by shift. src is unchanged, and the result of the shift is put into dest.
 *  length is the number of elements in dest (and src) which should be the same for both
 *
 *  Performs:
 *      dest = src << shift
 */
static inline void shift_left(uint32_t *dest, const uint32_t length, const uint32_t *src, const uint32_t shift) {
    uint32_t full_element_shifts = shift / ELEMENT_SIZE;
    uint32_t sub_shift = shift % ELEMENT_SIZE;

//    printf("shift: %u, full_element_shifts: %u, sub_shift: %u\n", shift, full_element_shifts, sub_shift);

    /**
     *  Note that the shift may be more than the length of an uint32_t (ie over 32), this needs to be accounted for, so the element
     *  we're shifting from may be ahead a few elements in the array.  When we do the shift, we can do this quickly by getting the target bits
     *  shifted to the left and doing an or with a shift to the right.
     *  ie (if our elements had 8 bits):
     *      00011010 101111101
     *  doing a shift of 5, we could update the first one to:
     *     00010111         // src[i + (full_element_shifts = 0) + 1] >> ((ELEMENT_SIZE = 8) - (sub_shift = 5)) // shift right 3
     *     |
     *     01000000
     *  which would be:
     *     01010111
     *  then the next would just be the second element shifted to the left by 5:
     *     10100000
     *   which results in:
     *     01010111 10100000
     *   which is the whole array shifted to the left by 5
     */
    uint32_t i;
    for (i = 0; i < length; i++) dest[i] = 0;

    if ((ELEMENT_SIZE - sub_shift) == 32) {
        for (i = 0; i < (length - full_element_shifts) - 1; i++) {
            dest[i] = src[i + full_element_shifts] << sub_shift;
        }
    } else {
        for (i = 0; i < (length - full_element_shifts) - 1; i++) {
            dest[i] = src[i + full_element_shifts] << sub_shift | src[i + full_element_shifts + 1] >> (ELEMENT_SIZE - sub_shift);
        }
    }

    dest[i] = src[length - 1] << sub_shift;
    i++;

    for (; i < length; i++) {
        dest[i] = 0;
    }
}

/**
 *  updates dest to:
 *      dest != src
 *
 *  Where dest and src are two arrays with length elements
 */
static inline void or_equal(uint32_t *dest, const uint32_t length, const uint32_t *src) {
    for (uint32_t i = 0; i < length; i++) dest[i] |= src[i];
}

/**
 *  Adds the single bit (for a new set element) into dest:
 *
 *  dest |= 1 << number
 */
static inline void or_single(uint32_t *dest, const uint32_t length, const uint32_t number) {
    uint32_t pos = number / ELEMENT_SIZE;
    uint32_t tmp = number % ELEMENT_SIZE;

    dest[length - pos - 1] |= 1 << tmp;
}

/**
 *  Tests to see if all the bits are 1s between min and max
 */
static inline bool all_ones(const uint32_t *subset, const uint32_t length, const uint32_t min, const uint32_t max) {
    uint32_t min_pos = min / ELEMENT_SIZE;
    uint32_t min_tmp = min % ELEMENT_SIZE;
    uint32_t max_pos = max / ELEMENT_SIZE;
    uint32_t max_tmp = max % ELEMENT_SIZE;

    /*
     * This will print out all the 1s that we're looking for.

    if (min_pos < max_pos) {
        if (max_tmp > 0) {
            print_bits(numeric_limits<uint32_t>::max() >> (ELEMENT_SIZE - max_tmp));
        } else {
            print_bits(0);
        }
        for (uint32_t i = min_pos + 1; i < max_pos - 1; i++) {
            print_bits(numeric_limits<uint32_t>::max());
        }
        print_bits(numeric_limits<uint32_t>::max() << (min_tmp - 1));
    } else {
        print_bits((numeric_limits<uint32_t>::max() << (min_tmp - 1)) & (numeric_limits<uint32_t>::max() >> (ELEMENT_SIZE - max_tmp)));
    }
    */

    if (min_pos == max_pos) {
        uint32_t against = (numeric_limits<uint32_t>::max() >> (ELEMENT_SIZE - max_tmp)) & (numeric_limits<uint32_t>::max() << (min_tmp - 1));
        return against == (against & subset[length - max_pos - 1]);
    } else {
        uint32_t against = numeric_limits<uint32_t>::max() << (min_tmp - 1);
        if (against != (against & subset[length - min_pos - 1])) {
            return false;
        }
//        fprintf(output_target, "min success\n");

        for (uint32_t i = (length - min_pos - 2); i > (length - max_pos); i--) {
            if (numeric_limits<uint32_t>::max() != (numeric_limits<uint32_t>::max() & subset[i])) {
                return false;
            }
        }
//        fprintf(output_target, "mid success\n");

        if (max_tmp > 0) {
            against = numeric_limits<uint32_t>::max() >> (ELEMENT_SIZE - max_tmp);
            if (against != (against & subset[length - max_pos - 1])) {
                return false;
            }
        }
//        fprintf(output_target, "max success\n");

    }

    return true;
}


#endif
