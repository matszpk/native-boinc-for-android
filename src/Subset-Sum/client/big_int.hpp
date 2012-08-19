#ifndef SUBSET_SUM_BIG_INT
#define SUBSET_SUM_BIG_INT

const unsigned int ELEMENT_SIZE = sizeof(unsigned int) * 8;

typedef unsigned int* big_int;

big_int** pascals_triangle;


static inline big_int new_big_int(const unsigned int length);

static inline big_int new_big_int(const unsigned int length, const unsigned int value);

static inline bool less_than(const unsigned int length, const big_int n1, const big_int n2);

static inline big_int add(const unsigned int length, const big_int n1, const big_int n2);

static inline big_int add(const unsigned int length, const big_int n1, const unsigned int n2);

static inline void create_pascals_triangle(const unsigned int length, const unsigned int n, const unsigned int k);

#endif
