/* asm-x86-64-gcc.h -- (C) Geoffrey Reynolds, July 2006.

   Inline x86-64 assember routines for GCC.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

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
uint64_t mulmod64_x86_64(uint64_t a, uint64_t b, uint64_t p) attribute((pure));
#define mulmod64(a,b,p) mulmod64_x86_64(a,b,p)

/* Set one_over_p to 1.0/p and SSE rounding mode to round-to-zero.
   Saves the old rounding mode in mod64_rnd.
*/
void mod64_init_x86_64(uint64_t p);
#define mod64_init(p) mod64_init_x86_64(p)

/* Restore the previous SSE rounding mode from mod64_rnd.
*/
void mod64_fini_x86_64(void);
#define mod64_fini() mod64_fini_x86_64()

/* a*b (mod p), where a,b < p < 2^51.
   Assumes b_over_p = b/p and SSE operations round to zero.
*/
#if USE_CMOV
#define PRE2_MULMOD64(a,b,p) ({ \
  register uint64_t ret, tmp; \
  register double x; \
  asm ("cvtsi2sdq %0, %2"    "\n\t" \
       "mulsd     %6, %2"    "\n\t" \
       "cvtsd2siq %2, %1"    "\n\t" \
       "imul      %4, %0"    "\n\t" \
       "imul      %5, %1"    "\n\t" \
       "sub       %1, %0"    "\n\t" \
       "mov       %0, %1"    "\n\t" \
       "sub       %5, %0"    "\n\t" \
       "cmovl     %1, %0" \
       : "=r" (ret), "=&r" (tmp), "=&x" (x) \
       : "0" (a), "r" (b), "r" (p), "x" (b_over_p), "m" (mod64_rnd) \
       : "cc" ); \
  ret; })
#else
#define PRE2_MULMOD64(a,b,p) ({ \
  register uint64_t ret, tmp; \
  register double x; \
  asm ("cvtsi2sdq %0, %2"    "\n\t" \
       "mulsd     %6, %2"    "\n\t" \
       "cvtsd2siq %2, %1"    "\n\t" \
       "imul      %4, %0"    "\n\t" \
       "imul      %5, %1"    "\n\t" \
       "sub       %1, %0"    "\n\t" \
       "mov       %0, %1"    "\n\t" \
       "sub       %5, %1"    "\n\t" \
       "jl        0f"        "\n\t" \
       "mov       %1, %0"    "\n" \
       "0:" \
       : "=r" (ret), "=&r" (tmp), "=&x" (x) \
       : "0" (a), "r" (b), "r" (p), "x" (b_over_p), "m" (mod64_rnd) \
       : "cc" ); \
  ret; })
#endif

/* Set b_over_p to b/p. Assumes one_over_p contains 1.0/p.
 */
#define PRE2_MULMOD64_INIT(b) \
  asm ("cvtsi2sdq %2, %0"    "\n\t" \
       "mulsd     %3, %0" \
       : "=&x" (b_over_p), "+m" (mod64_rnd) \
       : "rm" ((uint64_t)b), "xm" (one_over_p) \
       : "cc" )

#define PRE2_MULMOD64_FINI() {}

#endif /* defined(NEED_mulmod64) && !defined(HAVE_mulmod64) */


#if defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64)
#define HAVE_sqrmod64
extern uint32_t mod64_rnd;

/* b^2 (mod p), where b < p < 2^51.
   Assumes one_over_p = 1.0/p and SSE operations round to zero.
*/
static inline uint64_t sqrmod64(uint64_t b, uint64_t p)
{
  register uint64_t ret, tmp;
  register double x;

  asm ("cvtsi2sdq %0, %2"    "\n\t"
       "mulsd     %2, %2"    "\n\t"
       "mulsd     %5, %2"    "\n\t"
       "cvtsd2siq %2, %1"    "\n\t"
       "imul      %0, %0"    "\n\t"
       "imul      %4, %1"    "\n\t"
       "sub       %1, %0"    "\n\t"
       "mov       %0, %1"    "\n\t"
#if USE_CMOV
       "sub       %4, %0"    "\n\t"
       "cmovl     %1, %0"
#else
       "sub       %4, %1"    "\n\t"
       "jl        0f"        "\n\t"
       "mov       %1, %0"    "\n"
       "0:"
#endif
       : "=r" (ret), "=&r" (tmp), "=&x" (x)
       : "0" (b), "rm" (p), "xm" (one_over_p), "m" (mod64_rnd)
       : "cc" );

  return ret;
}
#endif /* defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64) */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
/*
  b^n (mod p)
*/
uint64_t powmod64_x86_64(uint64_t b, uint64_t n, uint64_t p, double invp)
     attribute ((const));
#define powmod64(b,n,p) powmod64_x86_64(b,n,p,one_over_p)
#endif


#if defined(NEED_memset_fast32) && !defined(HAVE_memset_fast32)
#define HAVE_memset_fast32
/* store count copies of x starting at dst.
 */
static inline
void memset_fast32(uint_fast32_t *dst, uint_fast32_t x, uint_fast32_t count)
{
  register uint_fast32_t tmp;
  register uint_fast32_t *ptr;

  asm ("rep"    "\n\t"
#if (UINT_FAST32_MAX==UINT64_MAX)
       "stosq"
#else
       "stosl"
#endif
       : "=c" (tmp), "=D" (ptr),
         "=m" (*(struct { uint_fast32_t dummy[count]; } *)dst)
       : "a" (x), "0" (count), "1" (dst)
       : "cc" );
}
#endif


#if defined(NEED_vector) && !defined(HAVE_vector)
#define HAVE_vector

/* Given uint64_t X[2], b, p, where 0 <= X[i],b < p < 2^51:
   Assign X[i+2] <-- X[i] * b (mod p), for 0 <= i < 2.
   Assumes b_over_p = b/p and SSE rounding mode is round-to-zero.
*/
#if USE_CMOV
#define VEC2_MULMOD64_NEXT(X,b,p) { \
  register uint64_t r0, r1, t0, t1; \
  register double x0, x1; \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %6"    "\n\t" \
       "mov       %5, %7"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "cmovl     %6, %4"    "\n\t" \
       "sub       %9, %5"    "\n\t" \
       "cmovl     %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1" \
       : "=m" ((X)[2]), "=m" ((X)[3]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[0]), "m" ((X)[1]), \
         "m" (mod64_rnd) \
       : "cc" ); \
}
#else
#define VEC2_MULMOD64_NEXT(X,b,p) { \
  register uint64_t r0, r1, t0, t1; \
  register double x0, x1; \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "jl        0f"        "\n\t" \
       "mov       %4, %0"    "\n"   \
       "0:"                  "\t"   \
       "sub       %9, %5"    "\n\t" \
       "jl        1f"        "\n\t" \
       "mov       %5, %1"    "\n"   \
       "1:"                         \
       : "=m" ((X)[2]), "=m" ((X)[3]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[0]), "m" ((X)[1]), \
         "m" (mod64_rnd) \
       : "cc" ); \
}
#endif

/* Given uint64_t X[4], b, p, where 0 <= X[i],b < p < 2^51:
   Assign X[i+4] <-- X[i] * b (mod p), for 0 <= i < 4.
   Assumes b_over_p = b/p and SSE rounding mode is round-to-zero.
*/
#if USE_CMOV
#define VEC4_MULMOD64_NEXT(X,b,p) { \
  register uint64_t r0, r1, t0, t1; \
  register double x0, x1; \
 \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %6"    "\n\t" \
       "mov       %5, %7"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "cmovl     %6, %4"    "\n\t" \
       "sub       %9, %5"    "\n\t" \
       "cmovl     %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1" \
       : "=m" ((X)[4]), "=m" ((X)[5]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[0]), "m" ((X)[1]), \
         "m" (mod64_rnd) \
       : "cc" ); \
 \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %6"    "\n\t" \
       "mov       %5, %7"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "cmovl     %6, %4"    "\n\t" \
       "sub       %9, %5"    "\n\t" \
       "cmovl     %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1" \
       : "=m" ((X)[6]), "=m" ((X)[7]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[2]), "m" ((X)[3]), \
         "m" (mod64_rnd) \
       : "cc" ); \
}
#else
#define VEC4_MULMOD64_NEXT(X,b,p) { \
  register uint64_t r0, r1, t0, t1; \
  register double x0, x1; \
 \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "jl        0f"        "\n\t" \
       "mov       %4, %0"    "\n"   \
       "0:"                  "\t"   \
       "sub       %9, %5"    "\n\t" \
       "jl        1f"        "\n\t" \
       "mov       %5, %1"    "\n"   \
       "1:"                         \
       : "=m" ((X)[4]), "=m" ((X)[5]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[0]), "m" ((X)[1]), \
         "m" (mod64_rnd) \
       : "cc" ); \
 \
  asm ("cvtsi2sdq %11, %2"   "\n\t" \
       "cvtsi2sdq %12, %3"   "\n\t" \
       "mulsd     %10, %2"   "\n\t" \
       "mulsd     %10, %3"   "\n\t" \
       "cvtsd2siq %2, %6"    "\n\t" \
       "cvtsd2siq %3, %7"    "\n\t" \
 \
       "mov       %11, %4"   "\n\t" \
       "mov       %12, %5"   "\n\t" \
       "imul      %8, %4"    "\n\t" \
       "imul      %8, %5"    "\n\t" \
       "imul      %9, %6"    "\n\t" \
       "imul      %9, %7"    "\n\t" \
       "sub       %6, %4"    "\n\t" \
       "sub       %7, %5"    "\n\t" \
       "mov       %4, %0"    "\n\t" \
       "mov       %5, %1"    "\n\t" \
       "sub       %9, %4"    "\n\t" \
       "jl        0f"        "\n\t" \
       "mov       %4, %0"    "\n"   \
       "0:"                  "\t"   \
       "sub       %9, %5"    "\n\t" \
       "jl        1f"        "\n\t" \
       "mov       %5, %1"    "\n"   \
       "1:"                         \
       : "=m" ((X)[6]), "=m" ((X)[7]), "=&x" (x0), "=&x" (x1), \
         "=&r" (r0), "=&r" (r1), "=&r" (t0), "=&r" (t1) \
       : "rm" (b), "rm" (p), "x" (b_over_p), "m" ((X)[2]), "m" ((X)[3]), \
         "m" (mod64_rnd) \
       : "cc" ); \
}
#endif
#endif /* defined(NEED_vector) && !defined(HAVE_vector) */


#if defined(NEED_btop) && !defined(HAVE_btop)

#if USE_LADDER
/* In btop-x86-64.S */
#define VECTOR_LENGTH 4
void btop_x86_64(const uint32_t *L, uint64_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86_64(L,T,lmin,llen,p)
#define HAVE_btop
#endif

#endif /* NEED_btop && !HAVE_btop */


#if defined(NEED_swizzle_loop) && !defined(HAVE_swizzle_loop)
#if (SWIZZLE==4)
/* In loop-core2.S */
int swizzle_loop4_core2(const uint64_t *T, struct loop_data_t *D,
                        int i, uint64_t p);
int swizzle_loop4_core2_prefetch(const uint64_t *T, struct loop_data_t *D,
                                 int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop4_core2(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop4_core2_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#elif (SWIZZLE==6)
/* In loop-a64.S */
int swizzle_loop6_a64(const uint64_t *T, struct loop_data_t *D,
                      int i, uint64_t p);
int swizzle_loop6_a64_prefetch(const uint64_t *T, struct loop_data_t *D,
                               int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop6_a64(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop6_a64_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#endif
#endif /* NEED_swizzle_loop && !HAVE_swizzle_loop */
