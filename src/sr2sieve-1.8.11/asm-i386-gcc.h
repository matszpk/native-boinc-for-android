/* asm-i386-gcc.h -- (C) Geoffrey Reynolds, November 2006.

   Inline i386 assember routines for GCC.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _ASM_I386_GCC_H
#define _ASM_I386_GCC_H

#include "config.h"

/* Transparent unions are a GCC extension. When a function is declared to
   take transparant union arguments the argument is actually passed as the
   type of its first element. TU64_t types declared below are used to pass
   64 bit integers to low level functions that operate on their parts.
*/
typedef union
{
  uint64_t u;
  struct { uint32_t l, h; };
} __attribute__ ((transparent_union)) TU64_t;

/* Old FPU precision (bits 8,9) and rounding (bits 10,11) mode saved here
   before switching to round-to-zero mode. This is also used as a dummy
   dependency for those functions which actually depend on the state of the
   FPU registers.
*/
extern uint16_t mod64_rnd;

#endif /* _ASM_I386_GCC_H */


/* This file may be included from a number of different headers. It should
   only define FEATURE if NEED_FEATURE is defined but HAVE_FEATURE is not.
*/


#if defined(NEED_mulmod64) && !defined(HAVE_mulmod64)
#define HAVE_mulmod64

/* All intermediate values are kept on the FPU stack, which gives correct
   results for all primes up to 2^62.
*/
#define MOD64_MAX_BITS 62

#ifdef __SSE2__
/* mulmod-sse2.S
 */
extern uint16_t mulmod64_init_sse2(const uint64_t *p)
     __attribute__ ((regparm(1)));
extern void mulmod64_fini_sse2(uint16_t mode)
     __attribute__ ((regparm(1)));

#define mod64_init(p) { mod64_rnd = mulmod64_init_sse2(&p); }
#define mod64_fini() { mulmod64_fini_sse2(mod64_rnd); }
#else
/* mulmod-i386.S
 */
extern uint16_t mulmod64_init_i386(const uint64_t *p)
     __attribute__ ((regparm(1)));
extern void mulmod64_fini_i386(uint16_t mode)
     __attribute__ ((regparm(1)));

#define mod64_init(p) { mod64_rnd = mulmod64_init_i386(&p); }
#define mod64_fini() { mulmod64_fini_i386(mod64_rnd); }
#endif

/* a*b (mod p), where a,b < p < 2^62.
   Assumes %st(0) contains 1.0/p and FPU is in round-to-zero mode.
*/
#if USE_INLINE_MULMOD
static inline uint64_t mulmod64(uint64_t a, uint64_t b, TU64_t p)
{
  uint32_t t1;
  uint32_t t2;
  uint64_t tmp;
  register uint64_t ret;

  asm("fildll  %5"                  "\n\t"
      "fildll  %4"                  "\n\t"
      "mov     %5, %%eax"           "\n\t"
      "mov     %5, %%ecx"           "\n\t"
      "mov     4+%5, %%ebx"         "\n\t"
      "mull    %4"                  "\n\t"
      "fmulp   %%st(0), %%st(1)"    "\n\t"
      "imul    %4, %%ebx"           "\n\t"
      "fmul    %%st(1), %%st(0)"    "\n\t"
      "imul    4+%4, %%ecx"         "\n\t"
      "mov     %%eax, %2"           "\n\t"
      "add     %%ebx, %%ecx"        "\n\t"
      "fistpll %1"                  "\n\t"
      "add     %%ecx, %%edx"        "\n\t"
      "mov     %%edx, %3"           "\n\t"
      "mov     %1, %%eax"           "\n\t"
      "mov     4+%1, %%edx"         "\n\t"
      "mov     %7, %%ebx"           "\n\t"
      "mov     %6, %%ecx"           "\n\t"
      "imul    %%eax, %%ebx"        "\n\t"
      "imul    %%edx, %%ecx"        "\n\t"
      "mull    %6"                  "\n\t"
      "add     %%ebx, %%ecx"        "\n\t"
      "mov     %3, %%ebx"           "\n\t"
      "add     %%ecx,%%edx"         "\n\t"
      "mov     %2, %%ecx"           "\n\t"
      "sub     %%eax, %%ecx"        "\n\t"
      "sbb     %%edx, %%ebx"        "\n\t"
      "mov     %%ecx, %%eax"        "\n\t"
      "mov     %%ebx, %%edx"        "\n\t"
      "sub     %6, %%ecx"           "\n\t"
      "sbb     %7, %%ebx"           "\n\t"
      "jl      0f"                  "\n\t"
      "mov     %%ecx, %%eax"        "\n\t"
      "mov     %%ebx, %%edx"        "\n\t"
      "0:"
      : "=&A" (ret), "=m" (tmp), "=m" (t1), "=m" (t2)
      : "m" (a), "m" (b), "rm" (p.l), "rm" (p.h), "m" (mod64_rnd)
      : "%ebx", "%ecx", "cc" );

  return ret;
}


/* a*b (mod p), where a,b < p < 2^62.
   Assumes %st(0) contains b/p and FPU is in round-to-zero mode.
*/
#define PRE2_MULMOD64(a,b,p) ({ \
  uint32_t r1, r2; \
  uint64_t tmp; \
  register uint64_t ret; \
  asm("fildll  %4"                  "\n\t" \
      "mov     %4, %%ebx"           "\n\t" \
      "mov     4+%4, %%ecx"         "\n\t" \
      "imul    %%edx, %%ebx"        "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "fistpll %1"                  "\n\t" \
      "mull    %4"                  "\n\t" \
      "mov     %%eax, %2"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %1, %%eax"           "\n\t" \
      "mov     %%edx, %3"           "\n\t" \
      "mov     4+%1, %%edx"         "\n\t" \
      "mov     %7, %%ebx"           "\n\t" \
      "mov     %6, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    %%edx, %%ecx"        "\n\t" \
      "mull    %6"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %3, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %2, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, %%eax"        "\n\t" \
      "mov     %%ebx, %%edx"        "\n\t" \
      "sub     %6, %%ecx"           "\n\t" \
      "sbb     %7, %%ebx"           "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, %%eax"        "\n\t" \
      "mov     %%ebx, %%edx"        "\n"   \
      "0:" \
      : "=&A" (ret), "=m" (tmp), "=m" (r1), "=m" (r2) \
      : "m" ((uint64_t)a), "0" ((uint64_t)b), \
        "rm" (((TU64_t)p).l), "rm" (((TU64_t)p).h), "m" (mod64_rnd) \
      : "%ebx", "%ecx", "cc" ); \
  ret; })

/* Pushes b/p onto the FPU stack. Assumes %st(0) contains 1.0/p.
 */
#define PRE2_MULMOD64_INIT(b) \
  asm volatile ("fildll  %1\n\t" \
                "fmul    %%st(1),%%st(0)" \
                : "+m" (mod64_rnd) \
                : "m" ((uint64_t)b) \
                : "cc" )

/* Pops b/p off the FPU stack.
 */
#define PRE2_MULMOD64_FINI() \
  asm volatile ("fstp    %%st(0)" \
                : "+m" (mod64_rnd) \
                : \
                : "cc" )

#else /* !USE_INLINE_MULMOD */
uint64_t mulmod64_i386(uint64_t a, uint64_t b, uint64_t p) attribute ((pure));
#define mulmod64(a,b,p) mulmod64_i386(a,b,p)
#define PRE2_MULMOD64(a,b,p) mulmod64_i386(a,b,p)
#define PRE2_MULMOD64_INIT(b)
#define PRE2_MULMOD64_FINI()
#endif /* USE_INLINE_MULMOD */

#endif /* defined(NEED_mulmod64) && !defined(HAVE_mulmod64) */


#if defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64) && USE_INLINE_MULMOD
#define HAVE_sqrmod64

/* b^2 (mod p), where b < p < 2^62.
   Assumes %st(0) contains 1.0/p and FPU is in round-to-zero mode.
*/
static inline uint64_t sqrmod64(uint64_t b, TU64_t p)
{
  uint32_t r1;
  uint32_t r2;
  uint64_t tmp;
  register uint64_t ret;

  asm("fildll  %4"                  "\n\t"
      "mov     %4, %%ecx"           "\n\t"
      "mov     %4, %%eax"           "\n\t"
      "mov     4+%4, %%ebx"         "\n\t"
      "mul     %%eax"               "\n\t"
      "fmul    %%st(0), %%st(0)"    "\n\t"
      "imul    %%ebx, %%ecx"        "\n\t"
      "fmul    %%st(1), %%st(0)"    "\n\t"
      "mov     %%eax, %2"           "\n\t"
      "fistpll %1"                  "\n\t"
      "lea     (%%edx,%%ecx,2), %%edx\n\t"
      "mov     %%edx, %3"           "\n\t"
      "mov     %1, %%eax"           "\n\t"
      "mov     4+%1, %%edx"         "\n\t"
      "mov     %6, %%ebx"           "\n\t"
      "mov     %5, %%ecx"           "\n\t"
      "imul    %%eax, %%ebx"        "\n\t"
      "imul    %%edx, %%ecx"        "\n\t"
      "mull    %5"                  "\n\t"
      "add     %%ebx, %%ecx"        "\n\t"
      "mov     %3, %%ebx"           "\n\t"
      "add     %%ecx,%%edx"         "\n\t"
      "mov     %2, %%ecx"           "\n\t"
      "sub     %%eax, %%ecx"        "\n\t"
      "sbb     %%edx, %%ebx"        "\n\t"
      "mov     %%ecx, %%eax"        "\n\t"
      "mov     %%ebx, %%edx"        "\n\t"
      "sub     %5, %%ecx"           "\n\t"
      "sbb     %6, %%ebx"           "\n\t"
      "jl      0f"                  "\n\t"
      "mov     %%ecx, %%eax"        "\n\t"
      "mov     %%ebx, %%edx"        "\n\t"
      "0:"
      : "=&A" (ret), "=m" (tmp), "=m" (r1), "=m" (r2)
      : "m" (b), "g" (p.l), "g" (p.h), "m" (mod64_rnd)
      : "%ebx", "%ecx", "cc" );

  return ret;
}
#endif /* defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64) */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
#define HAVE_vec_powmod64
/*
  b^n (mod p)
*/
#ifdef __SSE2__
/* Use the code in powmod-sse2.S
 */
uint64_t powmod64_sse2(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#define powmod64(b,n,p) powmod64_sse2(b,n,p)
void vec_powmod64_sse2(uint64_t *B, int l, uint64_t n, uint64_t p);
#define vec_powmod64(B,len,n,p) vec_powmod64_sse2(B,len,n,p)
#else
/* Use the code in powmod-i386.S
 */
uint64_t powmod64_i386(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#define powmod64(b,n,p) powmod64_i386(b,n,p)
void vec_powmod64_i386(uint64_t *B, int l, uint64_t n, uint64_t p);
#define vec_powmod64(B,len,n,p) vec_powmod64_i386(B,len,n,p)
#endif
#endif /* defined(NEED_powmod64) && !defined(HAVE_powmod64) */


#if defined(NEED_vector) && !defined(HAVE_vector)
#define HAVE_vector

#ifdef __SSE2__
/* mulmod-sse2.S
 */
extern uint16_t vec_mulmod64_initp_sse2(const uint64_t *p)
     __attribute__ ((regparm(1)));
extern void vec_mulmod64_finip_sse2(uint16_t mode)
     __attribute__ ((regparm(1)));

#define vec_mulmod64_initp(p) { mod64_rnd = vec_mulmod64_initp_sse2(&p); }
#define vec_mulmod64_finip() { vec_mulmod64_finip_sse2(mod64_rnd); }
#else
/* mulmod-i386.S
 */
extern uint16_t vec_mulmod64_initp_i386(const uint64_t *p)
     __attribute__ ((regparm(1)));
extern void vec_mulmod64_finip_i386(uint16_t mode)
     __attribute__ ((regparm(1)));

#define vec_mulmod64_initp(p) { mod64_rnd = vec_mulmod64_initp_i386(&p); }
#define vec_mulmod64_finip() { vec_mulmod64_finip_i386(mod64_rnd); }
#endif

/* mulmod-i386.S
 */
void vec_mulmod64_initb_i386(const uint64_t *b) __attribute__ ((regparm(1)));
void vec2_mulmod64_i386(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));
void vec4_mulmod64_i386(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));
void vec8_mulmod64_i386(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));

static inline void vec_mulmod64_finib_i386(void)
{
  asm volatile ("fstp    %%st(0)" : "+m" (mod64_rnd) /* Dummy */ : : "cc" );
}

#ifdef __SSE2__
#define HAVE_vector_sse2

/* mulmod-sse2.S
 */
void vec_mulmod64_initb_sse2(const uint64_t *b) __attribute__ ((regparm(1)));
void vec2_mulmod64_sse2(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));
void vec4_mulmod64_sse2(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));
void vec8_mulmod64_sse2(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));
void vec16_mulmod64_sse2(const uint64_t *X, uint64_t *Y, int count)
     __attribute__ ((regparm(3)));

static inline void vec_mulmod64_finib_sse2(void)
{
  asm volatile ("fstp    %%st(0)" : "+m" (mod64_rnd) /* Dummy */ : : "cc" );
}
#endif /* __SSE2__ */

#define vec2_mulmod64 vec2_mulmod64_i386
#define vec4_mulmod64 vec4_mulmod64_i386
#define vec8_mulmod64 vec8_mulmod64_i386

#ifdef __SSE2__
#define vec_mulmod64_initb(b) vec_mulmod64_initb_sse2(&b);
#define vec_mulmod64_finib() vec_mulmod64_finib_sse2();
#else
#define vec_mulmod64_initb(b) vec_mulmod64_initb_i386(&b);
#define vec_mulmod64_finib() vec_mulmod64_finib_i386();
#endif

#endif /* defined(NEED_vector) && !defined(HAVE_vector) */


#if defined(NEED_memset_fast32) && !defined(HAVE_memset_fast32)
#define HAVE_memset_fast32
/* store count copies of x starting at dst.
 */
static inline
void memset_fast32(uint_fast32_t *dst, uint_fast32_t x, uint_fast32_t count)
{
  register uint32_t tmp;
  register uint_fast32_t *ptr;

  asm ("rep; stosl"
       : "=c" (tmp), "=D" (ptr),
         "=m" (*(struct { uint_fast32_t dummy[count]; } *)dst)
       : "a" (x), "0" (count), "1" (dst)
       : "cc" );
}
#endif /* defined(NEED_memset_fast32) && !defined(HAVE_memset_fast32) */
