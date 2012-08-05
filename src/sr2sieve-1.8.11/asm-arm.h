/*
 * asm-arm.h
 * Author: Mateusz Szpakowski
 * License: LGPL v2.0
 */

#include "config.h"

#if defined(NEED_mulmod64) && !defined(HAVE_mulmod64)
#define HAVE_mulmod64
#define HAVE_sqrmod64

/* All intermediate values are kept on the FPU stack, which gives correct
   results for all primes up to 2^56.
*/
#define MOD64_MAX_BITS 56

extern void mod64_arm_init(uint64_t p);
extern uint64_t mulmod64_arm(uint64_t a, uint64_t b, uint64_t p);
extern uint64_t sqrmod64_arm(uint64_t a, uint64_t p);

struct mod64_init_data_t {
    uint64_t onep;      // 0
    uint64_t bbyp;      // 8
    uint64_t p;         // 16
    uint64_t b;         // 24
    uint32_t clzp;      // 32
    uint32_t clzbbyp;   // 36
    // p*4
    uint64_t ptimes4;   // 40
    uint64_t pshifted;  // 48
    uint64_t bbyp2;     // 56
    uint32_t pmask;     // 64
    uint64_t b2;        // 72
};

extern struct mod64_init_data_t mod64_init_data;

#ifdef ARM_NEON
extern void mod64_neon_init(uint64_t p);
#define mod64_init mod64_neon_init

struct mod64_neon_init_data_t {
  uint64_t ptimes4_1, ptimes4_2;
  uint64_t pshifted1, pshifted2;
  uint64_t pmask1, pmask2;
  uint64_t b_l, b_h;           // 48
  uint64_t bbyp_l, bbyp_h;        // 64
};

extern struct mod64_neon_init_data_t mod64_neon_init_data;
#else
#define mod64_init mod64_arm_init
#endif

#define mulmod64 mulmod64_arm
#define sqrmod64 sqrmod64_arm

static inline void mod64_fini(void)
{
}

#ifdef ARM_NEON
extern void premulmod64_neon_init_shifted(uint64_t b);
#endif
extern void premulmod64_arm_init(uint64_t b);
extern uint64_t premulmod64_arm(uint64_t a, uint64_t b, uint64_t p);

extern void premulmod64_arm_init_shifted(uint64_t b);

#define PRE2_MULMOD64_INIT premulmod64_arm_init
#define PRE2_MULMOD64 premulmod64_arm
#define PRE2_MULMOD64_FINI mod64_fini

#endif

#if defined(NEED_powseq64) && !defined(HAVE_powseq64)
#define HAVE_powseq64
#ifdef ARM_NEON
void powseq64_neon_shifted(uint64_t *X, int count, uint64_t b);
#define powseq64_shifted powseq64_neon_shifted
#else
void powseq64_arm_shifted(uint64_t *X, int count, uint64_t b);
#define powseq64_shifted powseq64_arm_shifted
#endif
#endif

#if defined(NEED_powmod64_shifted) && !defined(HAVE_powmod64_shifted)
#define HAVE_powmod64_shifted
uint64_t powmod64_arm_shifted(uint64_t a, uint64_t n, uint64_t p);
#ifdef ARM_NEON
uint64_t vec_powmod64_neon_shifted(uint64_t* A, uint32_t len,
                                  uint64_t n, uint64_t p);
#define vec_powmod64_shifted vec_powmod64_neon_shifted
#else
uint64_t vec_powmod64_arm_shifted(uint64_t* A, uint32_t len,
                                  uint64_t n, uint64_t p);
#define vec_powmod64_shifted vec_powmod64_arm_shifted
#endif
#define powmod64_shifted powmod64_arm_shifted
#endif
/*
#ifndef HAVE_vector
#define HAVE_vector

void vec_mulmod64_arm_initp(uint64_t p);
static inline void vec_mulmod64_arm_finip(void) { mod64_fini(); }
void vec_mulmod64_arm_initb(uint64_t b);
static inline void vec_mulmod64_arm_finib(void) { PRE2_MULMOD64_FINI(); }
void vec2_mulmod64_arm(const uint64_t *X, uint64_t *Y, int count);
void vec4_mulmod64_arm(const uint64_t *X, uint64_t *Y, int count);
void vec8_mulmod64_arm(const uint64_t *X, uint64_t *Y, int count);

#define vec_mulmod64_initp vec_mulmod64_arm_initp
#define vec_mulmod64_finip vec_mulmod64_arm_finip
#define vec_mulmod64_initb vec_mulmod64_arm_initb
#define vec_mulmod64_finib vec_mulmod64_arm_finib
#define vec2_mulmod64 vec2_mulmod64_arm
#define vec4_mulmod64 vec4_mulmod64_arm
#define vec8_mulmod64 vec8_mulmod64_arm

#endif
*/