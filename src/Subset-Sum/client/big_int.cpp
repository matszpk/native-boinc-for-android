#include "big_int.hpp"


static inline big_int new_big_int(const unsigned int length) {
    big_int n = (big_int)malloc(sizeof(unsigned int) * length);

    for (unsigned int i = 0; i < length; i++) n[i] = 0;

    return n;
}

static inline big_int new_big_int(const unsigned int length, const unsigned int value) {
    big_int n = (big_int)malloc(sizeof(unsigned int) * length);

    for (unsigned int i = 0; i < length - 1; i++) n[i] = 0;
    n[i] = value;

    return n;
}

static inline bool less_than(const unsigned int length, const big_int n1, const big_int n2) {
}

static inline big_int add(const unsigned int length, const big_int n1, const big_int n2) {
}

static inline big_int add(const unsigned int length, const big_int n1, const unsigned int n2) {
}

static inline void create_pascals_triangle(const unsigned int length, const unsigned int n, const unsigned int k) {
    pascals_triangle = (big_int**)malloc(sizeof(big_int*) * (n + 1));
    for (unsigned int i = 0; i <= n; i++) {
        pascals_triangle[i] = (big_int*)malloc(sizeof(big_int) * (k + 1));

        for (unsigned int j = 0; j <= k; j++) {
            pascals_triangle[i][j] = new_big_int(length);
        }
    }

    for (unsigned int i = 0; i <= n; i++) {
        pascals_triangle[i][0] = new_big_int(i, length);
    }

    for (unsigned int i = 1; i <= n; i++) {
        for (unsigned int j = 1; j <= k; j++) {
            pascals_triangle[i][j] = add(length, pascals_triangle[i - 1][j - 1], pascals_triangle[i - 1][j]);
        }
    }
}

static inline big_int n_choose_k(unsigned int n, unsigned int k) {
    return pascals_triangle[n][k];
}

static inline string to_string(const unsigned int length, big_int n) {
}
