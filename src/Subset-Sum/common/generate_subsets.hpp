#ifndef SSS_GENERATE_SUBSETS
#define SSS_GENERATE_SUBSETS

#include "stdint.h"
#include "../common/n_choose_k.hpp"

void generate_ith_subset(uint64_t i, uint32_t *subset, uint32_t subset_size, uint32_t max_set_value);

void generate_next_subset(uint32_t *subset, uint32_t subset_size, uint32_t max_set_value);

#endif
