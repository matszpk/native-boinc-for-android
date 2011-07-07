/* asm-x86-64-msc.h -- (C) Geoffrey Reynolds, July 2006.

   x86-64 assember routines for MSC.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#ifndef EXTERN
# ifdef __cplusplus
#  define EXTERN extern "C"
# else
#  define EXTERN extern
# endif
#endif

/* This file may be included from a number of different headers. It should
   only define FEATURE if NEED_FEATURE is defined but HAVE_FEATURE is not.
*/

#if defined(NEED_mulmod64) && !defined(HAVE_mulmod64)
#define HAVE_mulmod64

#define MOD64_MAX_BITS 51

/* Saved copy of SSE rounding mode (bits 13,14). */
extern uint32_t mod64_rnd;

extern double one_over_p;
extern double b_over_p;


/* a*b (mod p), where a,b < p < 2^51.
*/
EXTERN uint64_t mulmod64_x86_64(uint64_t a, uint64_t b, uint64_t p);
#define mulmod64(a,b,p) mulmod64_x86_64(a,b,p)

/* Set one_over_p to 1.0/p and SSE rounding mode to round-to-zero.
   Saves the old rounding mode in mod64_rnd.
*/
EXTERN void mod64_init_x86_64(uint64_t p);
#define mod64_init(p) mod64_init_x86_64(p)

/* Restore the previous SSE rounding mode from mod64_rnd.
*/
EXTERN void mod64_fini_x86_64(void);
#define mod64_fini() mod64_fini_x86_64()

#endif /* defined(NEED_mulmod64) && !defined(HAVE_mulmod64) */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
/*
  b^n (mod p)
*/
EXTERN
uint64_t powmod64_x86_64(uint64_t b, uint64_t n, uint64_t p, double invp);
#define powmod64(b,n,p) powmod64_x86_64(b,n,p,one_over_p)
#endif


#if defined(NEED_btop) && !defined(HAVE_btop)

#if USE_LADDER
/* In btop-x86-64.asm */
#define VECTOR_LENGTH 4
EXTERN void btop_x86_64(const uint32_t *L, uint64_t *T, uint32_t lmin,
                        uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86_64(L,T,lmin,llen,p)
#define HAVE_btop
#endif

#endif /* NEED_btop && !HAVE_btop */


#if defined(NEED_swizzle_loop) && !defined(HAVE_swizzle_loop)
#if (SWIZZLE==4)
/* In loop-core2.asm */
EXTERN int swizzle_loop4_core2(const uint64_t *T, struct loop_data_t *D,
                        int i, uint64_t p);
EXTERN int swizzle_loop4_core2_prefetch(const uint64_t *T, struct loop_data_t *D,
                                        int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop4_core2(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop4_core2_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#elif (SWIZZLE==6)
/* In loop-a64.asm */
EXTERN int swizzle_loop6_a64(const uint64_t *T, struct loop_data_t *D,
                             int i, uint64_t p);
EXTERN int swizzle_loop6_a64_prefetch(const uint64_t *T, struct loop_data_t *D,
                                      int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop6_a64(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop6_a64_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#endif
#endif /* NEED_swizzle_loop && !HAVE_swizzle_loop */
