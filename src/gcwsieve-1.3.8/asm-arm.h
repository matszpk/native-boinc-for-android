/*
 * asm-arm.h
 * Author: Mateusz Szpakowski
 * License: LGPL v2.0
 */

#include "config.h"

/* This file may be included from a number of different headers. It should
   only define FEATURE if NEED_FEATURE is defined but HAVE_FEATURE is not.
*/

#if defined(NEED_mulmod64) && !defined(HAVE_mulmod64)
#define HAVE_mulmod64
#define HAVE_sqrmod64

#define MOD64_MAX_BITS 56

extern void mod64_arm_init(uint64_t p);
extern uint64_t mulmod64_arm(uint64_t a, uint64_t b, uint64_t p);
extern uint64_t sqrmod64_arm(uint64_t a, uint64_t p);


#define mulmod64 mulmod64_arm
#define sqrmod64 sqrmod64_arm

struct mod64_init_data_t {
  uint64_t onep;
  uint64_t ptimes4;
  uint64_t pshifted;
  uint32_t clzp;
  uint32_t pmask;
};

extern struct mod64_init_data_t mod64_init_data;

static inline void mod64_fini(void)
{
}

#if !defined(SWIZZLE) || (SWIZZLE==2)
#define mod64_init mod64_arm_init
#elif (SWIZZLE==6)
extern void mod64_neon_init(uint64_t p);
#define mod64_init mod64_neon_init

struct mod64_neon_init_data_t {
  uint64_t ptimes4_1, ptimes4_2;
  uint64_t pshifted1, pshifted2;
  uint64_t pmask1, pmask2;
};

extern struct mod64_neon_init_data_t mod64_neon_init_data;
#endif

#endif

#ifndef ARM_T_T
#define ARM_T_T
struct T_t { uint64_t T; uint64_t bbyp; };

#if (SWIZZLE==2)
extern void premod64_arm_init(struct T_t* T, uint64_t p);
#define premod64_init premod64_arm_init
#elif (SWIZZLE==6)
extern void premod64_neon_init(struct T_t* T, uint64_t p);
#define premod64_init premod64_neon_init
#endif
#endif

#if defined(NEED_loop_data) && !defined(HAVE_loop_data)
#define HAVE_loop_data
#define HAVE_novfp_loop_data
//struct loop_rec_t { uint64_t N[SWIZZLE]; uint32_t G[SWIZZLE]; };
struct loop_rec_t { uint64_t N[SWIZZLE]; struct T_t* G[SWIZZLE]; };
struct loop_data_t { uint64_t X[SWIZZLE]; struct loop_rec_t R[1]; };
#endif

#if defined(NEED_btop) && !defined(HAVE_btop)
#define HAVE_btop
#define VECTOR_LENGTH 4
#if (SWIZZLE==2)
void btop_arm(const uint32_t *L, struct T_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_arm(L,T,lmin,llen,p)
#elif (SWIZZLE==6)
void btop_neon(const uint32_t *L, struct T_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_neon(L,T,lmin,llen,p)
#endif
#endif

#if defined(NEED_swizzle_loop) && !defined(HAVE_swizzle_loop)
#define HAVE_swizzle_loop
#if (SWIZZLE==2)
int swizzle_loop2_arm(const struct T_t* T, struct loop_data_t *D,
                      int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop2_arm(a,b,c,d)
#elif (SWIZZLE==6)
extern int swizzle_loop6_neon(const struct T_t* T, struct loop_data_t *D,
                      int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop6_neon(a,b,c,d)
#endif
#endif
