/* asm-i386-msc.h -- (C) Geoffrey Reynolds, September 2007.

   i386 assember routines for MSC.

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

/* All intermediate values are kept on the FPU stack, which gives correct
   results for all primes up to 2^62.
*/
#define MOD64_MAX_BITS 62

/* Old FPU rounding mode (bits 10 and 11) saved here before switching to
   round-to-zero mode. This is also used as a dummy dependancy for those
   functions which actually depend on the state of the FPU stack.
*/
extern uint16_t mod64_rnd;

/* a*b (mod p), where a,b < p < 2^62.
 */
uint64_t mulmod64_i386(uint64_t a, uint64_t b, uint64_t p);
#define mulmod64(a,b,p) mulmod64_i386(a,b,p)

/* Pushes 1.0/p onto the FPU stack and changes the FPU into round-to-zero
   mode. Saves the old rounding mode in mod64_rnd.
*/
EXTERN void mod64_init_i386(uint64_t p);
#define mod64_init(p) mod64_init_i386(p)

/* Pops 1.0/p off the FPU stack and restores the previous FPU rounding mode
   from mod64_rnd.
*/
EXTERN void mod64_fini_i386(void);
#define mod64_fini() mod64_fini_i386()

#endif /* defined(NEED_mulmod64) && !defined(HAVE_mulmod64) */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
/*
  b^n (mod p)
*/
#ifdef __SSE2__
/* Use the code in powmod-sse2.S
 */
EXTERN uint64_t powmod64_sse2(uint64_t b, uint64_t n, uint64_t p);
#define powmod64(b,n,p) powmod64_sse2(b,n,p)
#else
/* Use the code in powmod-i386.S
 */
EXTERN uint64_t powmod64(uint64_t b, uint64_t n, uint64_t p);
#endif
#endif


#if defined(NEED_btop) && !defined(HAVE_btop)

#ifdef __SSE2__
#define VECTOR_LENGTH 4
/* In btop-x86-sse2.S */
EXTERN void btop_x86_sse2(const uint32_t *L, uint64_t *T, uint32_t lmin,
                          uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86_sse2(L,T,lmin,llen,p)
#define HAVE_btop
#else /* !__SSE2__ */
#define VECTOR_LENGTH 4
/* In btop-x86.S */
EXTERN void btop_x86(const uint32_t *L, uint64_t *T, uint32_t lmin,
                     uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86(L,T,lmin,llen,p)
#define HAVE_btop
#endif /* __SSE2__ */

#endif /* NEED_btop && !HAVE_btop */


#if defined(NEED_swizzle_loop) && !defined(HAVE_swizzle_loop)

#ifdef __SSE2__
#if (SWIZZLE==4)
/* In loop-x86-sse2.S */
EXTERN int swizzle_loop4_x86_sse2(int i, const uint64_t *T,
                                  struct loop_data_t *D, uint64_t p);
EXTERN int swizzle_loop4_x86_sse2_prefetch(int i, const uint64_t *T,
                                           struct loop_data_t *D, uint64_t p);
/* Note permuted arguments */
#define swizzle_loop(a,b,c,d) swizzle_loop4_x86_sse2(a,b,d,c)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop4_x86_sse2_prefetch(a,b,d,c)
#define HAVE_swizzle_loop
#endif
#else /* !__SSE2__ */
#if (SWIZZLE==2)
/* In loop-x86.S */
EXTERN int swizzle_loop2_x86(const uint64_t *T, struct loop_data_t *D,
                             int i, uint64_t p);
EXTERN int swizzle_loop2_x86_prefetch(const uint64_t *T, struct loop_data_t *D,
                                      int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop2_x86(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop2_x86_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#endif
#endif /* __SSE2__ */

#endif /* NEED_swizzle_loop && !HAVE_swizzle_loop */
