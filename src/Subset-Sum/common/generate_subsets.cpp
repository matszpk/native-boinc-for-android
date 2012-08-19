#include "stdint.h"

#include "generate_subsets.hpp"
#include "../common/n_choose_k.hpp"

void generate_ith_subset(uint64_t i, uint32_t *subset, uint32_t subset_size, uint32_t max_set_value) {
    uint32_t pos = 0;
    uint32_t current_value = 1;
    uint64_t nck;

    while (pos < subset_size - 1) {
        //TODO: this does not need to be recalcualted, there is a faster way to do this
        //Would be the fastest way to do it if we were using a table of n choose k values -- which we should for a big int representation
        nck = n_choose_k((max_set_value - 1) - current_value, (subset_size - 1) - (pos + 1));

        if (i < nck) {
            subset[pos] = current_value;
            pos++;
        } else {
            i -= nck;
        }
        current_value++;
    }

    subset[subset_size - 1] = max_set_value;
}

void generate_next_subset(uint32_t *subset, uint32_t subset_size, uint32_t max_set_value) {
    uint32_t current = subset_size - 2;
    subset[current]++;

//    *output_target << "subset_size: %u, max_set_value: %u\n", subset_size, max_set_value);

//    print_subset(subset, subset_size);
//    *output_target << "\n");

    while (current > 0 && subset[current] > (max_set_value - (subset_size - (current + 1)))) {
        subset[current - 1]++;
        current--;

//        print_subset(subset, subset_size);
//        *output_target << "\n");
    }

    while (current < subset_size - 2) {
        subset[current + 1] = subset[current] + 1;
        current++;

//        print_subset(subset, subset_size);
//        *output_target << "\n");
    }

    subset[subset_size - 1] = max_set_value;

//    print_subset(subset, subset_size);
//    *output_target << "\n");
}


