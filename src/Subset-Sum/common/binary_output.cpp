#include <cstdio>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "stdint.h"
#include "bit_logic.hpp"
#include "binary_output.hpp"


using namespace std;

/**
 *  Print the bits in an uint32_t.  Note this prints out from right to left (not left to right)
 */
void print_bits(ofstream *output_target, const uint32_t number) {
    uint32_t pos = 1 << (ELEMENT_SIZE - 1);
    while (pos > 0) {
        if (number & pos) *output_target << "1";
        else *output_target << "0";
        pos >>= 1;
    }
}

/**
 * Print out an array of bits
 */
void print_bit_array(ofstream *output_target, const uint32_t *bit_array, const uint32_t bit_array_length) {
    for (uint32_t i = 0; i < bit_array_length; i++) {
        print_bits(output_target, bit_array[i]);
    }
}

/**
 *  Print out all the elements in a subset
 */
void print_subset(ofstream *output_target, const uint32_t *subset, const uint32_t subset_size) {
    *output_target << "[";
#ifndef HTML_OUTPUT
    for (uint32_t i = 0; i < subset_size; i++) {
        *output_target << setw(4) << subset[i];
    }
#else
    for (uint32_t i = 0; i < subset_size; i++) {
        double whitespaces = (max_set_digits - floor(log10(subset[i]))) - 1;

        for (int j = 0; j < whitespaces; j++) *output_target << "&nbsp;";

        *output_target << setw(4) << subset[i];
    }
#endif
    *output_target << "]";
}

/**
 * Print out an array of bits, coloring the required subsets green, if there is a missing sum (a 0) it is colored red
 */
void print_bit_array_color(ofstream *output_target, const uint32_t *bit_array, unsigned long int max_sums_length, uint32_t min, uint32_t max) {
    uint32_t msl = max_sums_length * ELEMENT_SIZE;
    uint32_t number, pos;
    uint32_t count = 0;

//    fprintf(output_target, " - MSL: %u, MIN: %u, MAX: %u - ", msl, min, max);

    bool red_on = false;

//    fprintf(output_target, " msl - min [%u], msl - max [%u] ", (msl - min), (msl - max));

    for (uint32_t i = 0; i < max_sums_length; i++) {
        number = bit_array[i];
        pos = 1 << (ELEMENT_SIZE - 1);

        while (pos > 0) {
            if ((msl - min) == count) {
                red_on = true;
#ifndef HTML_OUTPUT
                *output_target << "\e[32m";
#else
                *output_target << "<b><span class=\"courier_green\">";
#endif
            }

            if (number & pos) *output_target << "1";
            else {
                if (red_on) {
#ifndef HTML_OUTPUT
                    *output_target << "\e[31m0\e[32m";
#else
                    *output_target << "<span class=\"courier_red\">0</span>";
#endif
                } else {
                    *output_target << "0";
                }
            }

            if ((msl - max) == count) {
#ifndef HTML_OUTPUT
                *output_target << "\e[0m";
#else
                *output_target << "</span></b>";
#endif
                red_on = false;
            }

            pos >>= 1;
            count++;
        }
    }
}
#ifndef ARM_OPT
void print_subset_calculation(ofstream *output_target, const uint64_t iteration, uint32_t *subset, const uint32_t subset_size, const bool success) {

    uint32_t M = subset[subset_size - 1];
    uint32_t max_subset_sum = 0;

    for (uint32_t i = 0; i < subset_size; i++) max_subset_sum += subset[i];
    for (uint32_t i = 0; i < max_sums_length; i++) {
        sums[i] = 0;
        new_sums[i] = 0;
    }


    for (uint32_t i = 0; i < max_sums_length; i++) {
        sums[i] = 0;
        new_sums[i] = 0;
    }

    uint32_t current;
#ifdef SHOW_SUM_CALCULATION
    *output_target << "\n";
#endif
    for (uint32_t i = 0; i < subset_size; i++) {
        current = subset[i];

        shift_left(new_sums, max_sums_length, sums, current);                    // new_sums = sums << current;
//            fprintf(output_target, "new_sums = sums << %2u                                          = ", current);
//            print_bit_array(new_sums, max_sums_length);
//            fprintf(output_target, "\n");

        or_equal(sums, max_sums_length, new_sums);                               //sums |= new_sums;
//            fprintf(output_target, "sums |= new_sums                                               = ");
//            print_bit_array(sums, max_sums_length);
//            fprintf(output_target, "\n");

        or_single(sums, max_sums_length, current - 1);                           //sums |= 1 << (current - 1);
#ifdef SHOW_SUM_CALCULATION
        *output_target << "sums != 1 << current - 1                                       = ";
        print_bit_array(sums, max_sums_length);
        *output_target << "\n";
#endif
    }

#ifdef HTML_OUTPUT
    double whitespaces;
    if (iteration == 0) {
        whitespaces = (max_digits - 1);
    } else {
        whitespaces = (max_digits - floor(log10(iteration))) - 1;
    }

    for (int i = 0; i < whitespaces; i++) *output_target << "&nbsp;";
#endif

#ifndef HTML_OUTPUT
    *output_target << setw(15) << iteration;
#else
    *output_target << setw(15) << iteration;
#endif
    print_subset(output_target, subset, subset_size);
    *output_target << " = ";

    uint32_t min = max_subset_sum - M;
    uint32_t max = M;
#ifdef ENABLE_COLOR
    print_bit_array_color(output_target, sums, max_sums_length, min, max);
#else 
    print_bit_array(output_target, sums, max_sums_length);
#endif

    *output_target << "  match " << setw(4) << min << " to " << setw(4) << max;
#ifndef HTML_OUTPUT
#ifdef ENABLE_COLOR
    if (success)    *output_target << " = \e[32mpass\e[0m" << endl;
    else            *output_target << " = \e[31mfail\e[0m" << endl;
#else
    if (success)    *output_target << " = pass" << endl;
    else            *output_target << " = fail" << endl;
#endif
#else
    if (success)    *output_target << " = <span class=\"courier_green\">pass</span><br>" << endl;
    else            *output_target << " = <span class=\"courier_red\">fail</span><br>" << endl;
#endif

    output_target->flush();
}
#endif


