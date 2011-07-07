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

typedef uint64_t v128_t attribute ((vector_size(16)));

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

/* Old FPU rounding mode (bits 10 and 11) saved here before switching to
   round-to-zero mode. This is also used as a dummy dependancy for those
   functions which actually depend on the state of the FPU stack.
*/
extern uint16_t mod64_rnd;

/* a*b (mod p), where a,b < p < 2^62.
 */
uint64_t mulmod64_i386(uint64_t a, uint64_t b, uint64_t p) attribute ((pure));
#define mulmod64(a,b,p) mulmod64_i386(a,b,p)

/* Pushes 1.0/p onto the FPU stack and changes the FPU into round-to-zero
   mode. Saves the old rounding mode in mod64_rnd.
*/
void mod64_init_i386(uint64_t p);
#define mod64_init(p) mod64_init_i386(p)

/* Pops 1.0/p off the FPU stack and restores the previous FPU rounding mode
   from mod64_rnd.
*/
void mod64_fini_i386(void);
#define mod64_fini() mod64_fini_i386()

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
#endif


#if defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64)
#define HAVE_sqrmod64
extern uint16_t mod64_rnd;

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
#endif


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
/*
  b^n (mod p)
*/
#ifdef __SSE2__
/* Use the code in powmod-sse2.S
 */
uint64_t powmod64_sse2(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#define powmod64(b,n,p) powmod64_sse2(b,n,p)
#else
/* Use the code in powmod-i386.S
 */
uint64_t powmod64(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#endif
#endif


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
#endif


#if defined(NEED_vector) && !defined(HAVE_vector)
#define HAVE_vector

#ifdef __SSE2__
#define HAVE_vector_sse2

/* Given uint64_t X[4] and v128_t B={b,b}, P={p,p}:

   Assign X[i+4] <-- X[i] * b (mod p), for 0 <= i < 4.

   Assumes %st(0) contains b/p and FPU is in round-to-zero mode.
*/
#define VEC4_MULMOD64_NEXT_SSE2(X,B,P) { \
  asm("fildll  %6"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %0"                  "\n\t" \
      "fildll  %7"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %1"                  "\n\t" \
 \
      "movdqa  %6, %%xmm2"          "\n\t" \
      "movdqa  %4, %%xmm5"          "\n\t" \
      "movdqa  %%xmm2, %%xmm7"      "\n\t" \
      "movdqa  %%xmm2, %%xmm4"      "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "pmuludq %4, %%xmm2"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %4, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm2"      "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
 \
      "fildll  %8"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %2"                  "\n\t" \
      "fildll  %9"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %3"                  "\n\t" \
 \
      "movdqa  %8, %%xmm3"          "\n\t" \
      "movdqa  %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %4, %%xmm3"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %4, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "paddq   %%xmm4, %%xmm3"      "\n\t" \
 \
      "movdqa  %0, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "movdqa  %%xmm6, %%xmm4"      "\n\t" \
      "movdqa  %5, %%xmm5"          "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "pmuludq %5, %%xmm4"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %5, %%xmm6"          "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm4, %%xmm4"      "\n\t" \
      "psubq   %%xmm6, %%xmm2"      "\n\t" \
      "psubq   %5, %%xmm2"          "\n\t" \
      "pcmpgtd %%xmm2, %%xmm4"      "\n\t" \
      "pshufd  $0xF5,%%xmm4,%%xmm4" "\n\t" \
      "pand    %5, %%xmm4"          "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
      "movdqa  %%xmm2, %0"          "\n\t" \
 \
      "movdqa  %2, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %5, %%xmm6"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %5, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm7, %%xmm7"      "\n\t" \
      "psubq   %%xmm6, %%xmm3"      "\n\t" \
      "psubq   %5, %%xmm3"          "\n\t" \
      "pcmpgtd %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm7" "\n\t" \
      "pand    %5, %%xmm7"          "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "movdqa  %%xmm3, %2" \
 \
      : "=m" ((X)[4]), "=m" ((X)[5]), "=m" ((X)[6]), "=m" ((X)[7]) \
      : "x" (B), "x" (P), "m" ((X)[0]), "m" ((X)[1]), "m" ((X)[2]), \
        "m" ((X)[3]), "m" (mod64_rnd) \
      : "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7" ); \
}

/* Given uint64_t X[8] and v128_t B={b,b}, P={p,p}:

   Assign X[i+8] <-- X[i] * b (mod p), for 0 <= i < 8.

   Assumes %st(0) contains b/p and FPU is in round-to-zero mode.
*/
#define VEC8_MULMOD64_NEXT_SSE2(X,B,P) { \
  asm("fildll  %10"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %0"                  "\n\t" \
      "fildll  %11"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %1"                  "\n\t" \
 \
      "fildll  %12"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %2"                  "\n\t" \
      "fildll  %13"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %3"                  "\n\t" \
 \
      "fildll  %14"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %4"                  "\n\t" \
      "fildll  %15"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %5"                  "\n\t" \
 \
      "fildll  %16"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %6"                  "\n\t" \
      "fildll  %17"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %7"                  "\n\t" \
 \
      "movdqa  %10, %%xmm2"         "\n\t" \
      "movdqa  %8, %%xmm5"          "\n\t" \
      "movdqa  %%xmm2, %%xmm7"      "\n\t" \
      "movdqa  %%xmm2, %%xmm4"      "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "pmuludq %8, %%xmm2"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %8, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm2"      "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
 \
      "movdqa  %12, %%xmm3"         "\n\t" \
      "movdqa  %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %8, %%xmm3"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %8, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "paddq   %%xmm4, %%xmm3"      "\n\t" \
 \
      "movdqa  %0, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "movdqa  %%xmm6, %%xmm4"      "\n\t" \
      "movdqa  %9, %%xmm5"          "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "pmuludq %9, %%xmm4"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %9, %%xmm6"          "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm4, %%xmm4"      "\n\t" \
      "psubq   %%xmm6, %%xmm2"      "\n\t" \
      "psubq   %9, %%xmm2"          "\n\t" \
      "pcmpgtd %%xmm2, %%xmm4"      "\n\t" \
      "pshufd  $0xF5,%%xmm4,%%xmm4" "\n\t" \
      "pand    %9, %%xmm4"          "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
      "movdqa  %%xmm2, %0"          "\n\t" \
 \
      "movdqa  %2, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %9, %%xmm6"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %9, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm7, %%xmm7"      "\n\t" \
      "psubq   %%xmm6, %%xmm3"      "\n\t" \
      "psubq   %9, %%xmm3"          "\n\t" \
      "pcmpgtd %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm7" "\n\t" \
      "pand    %9, %%xmm7"          "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "movdqa  %%xmm3, %2"          "\n\t" \
 \
 \
      "movdqa  %14, %%xmm2"          "\n\t" \
      "movdqa  %8, %%xmm5"          "\n\t" \
      "movdqa  %%xmm2, %%xmm7"      "\n\t" \
      "movdqa  %%xmm2, %%xmm4"      "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "pmuludq %8, %%xmm2"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %8, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm2"      "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
 \
      "movdqa  %16, %%xmm3"          "\n\t" \
      "movdqa  %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %8, %%xmm3"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %8, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "paddq   %%xmm4, %%xmm3"      "\n\t" \
 \
      "movdqa  %4, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "movdqa  %%xmm6, %%xmm4"      "\n\t" \
      "movdqa  %9, %%xmm5"          "\n\t" \
      "psrlq   $32, %%xmm4"         "\n\t" \
      "psrlq   $32, %%xmm5"         "\n\t" \
      "pmuludq %9, %%xmm4"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %9, %%xmm6"          "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm4, %%xmm4"      "\n\t" \
      "psubq   %%xmm6, %%xmm2"      "\n\t" \
      "psubq   %9, %%xmm2"          "\n\t" \
      "pcmpgtd %%xmm2, %%xmm4"      "\n\t" \
      "pshufd  $0xF5,%%xmm4,%%xmm4" "\n\t" \
      "pand    %9, %%xmm4"          "\n\t" \
      "paddq   %%xmm4, %%xmm2"      "\n\t" \
      "movdqa  %%xmm2, %4"          "\n\t" \
 \
      "movdqa  %6, %%xmm6"          "\n\t" \
      "movdqa  %%xmm6, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm4" "\n\t" \
      "pmuludq %9, %%xmm6"          "\n\t" \
      "pmuludq %%xmm5, %%xmm7"      "\n\t" \
      "pmuludq %9, %%xmm4"          "\n\t" \
      "psllq   $32, %%xmm7"         "\n\t" \
      "psllq   $32, %%xmm4"         "\n\t" \
      "paddq   %%xmm7, %%xmm6"      "\n\t" \
      "paddq   %%xmm4, %%xmm6"      "\n\t" \
 \
      "pxor    %%xmm7, %%xmm7"      "\n\t" \
      "psubq   %%xmm6, %%xmm3"      "\n\t" \
      "psubq   %9, %%xmm3"          "\n\t" \
      "pcmpgtd %%xmm3, %%xmm7"      "\n\t" \
      "pshufd  $0xF5,%%xmm7,%%xmm7" "\n\t" \
      "pand    %9, %%xmm7"          "\n\t" \
      "paddq   %%xmm7, %%xmm3"      "\n\t" \
      "movdqa  %%xmm3, %6" \
 \
      : "=m" ((X)[8]), "=m" ((X)[9]), "=m" ((X)[10]), "=m" ((X)[11]), \
        "=m" ((X)[12]), "=m" ((X)[13]), "=m" ((X)[14]), "=m" ((X)[15]) \
      : "x" (B), "x" (P), "m" ((X)[0]), "m" ((X)[1]), "m" ((X)[2]), \
        "m" ((X)[3]), "m" ((X)[4]), "m" ((X)[5]), "m" ((X)[6]), \
        "m" ((X)[7]), "m" (mod64_rnd) \
      : "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7" ); \
}


/* Return a v128_t with lower quadword x, upper quadword y. This is needed
   because of a bug in GCC 4.1 that causes X = (v128_t){x,y} to load both x
   and y into the low quadword of X!
 */
#define VEC2_SET(x,y) ({ \
  v128_t ret; \
  asm ("movq    %1, %0"    "\n\t" \
       "movhps  %2, %0" \
       : "=&x" (ret) : "m" ((uint64_t)x), "m" ((uint64_t)y) ); \
  ret; \
})

/* Return a v128_t with both quadwords set to x. This should be the same as
   X = (v128_t){x,x}, but avoids a potential bug in GCC 4.1.
 */
#define VEC2_SET2(x) ({ \
  v128_t ret; \
  asm ("movq    %1, %0"    "\n\t" \
       "punpcklqdq %0, %0" \
       : "=x" (ret) : "m" ((uint64_t)x) ); \
  ret; \
})

#endif /* __SSE2__ */


#define VEC2_MULMOD64_NEXT(X,b,p) { \
  uint32_t r1, r2; \
  asm("fildll  %6"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %0"                  "\n\t" \
      "fildll  %7"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %1"                  "\n\t" \
 \
      "mov     0+%6, %%ebx"         "\n\t" \
      "mov     4+%6, %%ecx"         "\n\t" \
      "mov     %4, %%eax"           "\n\t" \
      "imul    4+%4, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%6"                "\n\t" \
      "mov     %%eax, %2"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %0, %%eax"           "\n\t" \
      "mov     %%edx, %3"           "\n\t" \
      "mov     4+%5, %%ebx"         "\n\t" \
      "mov     %5, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%0, %%ecx"         "\n\t" \
      "mull    %5"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %3, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %2, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n\t" \
      "sub     %5, %%ecx"           "\n\t" \
      "sbb     4+%5, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%7, %%ebx"         "\n\t" \
      "mov     4+%7, %%ecx"         "\n\t" \
      "mov     %4, %%eax"           "\n\t" \
      "imul    4+%4, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%7"                "\n\t" \
      "mov     %%eax, %2"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %1, %%eax"           "\n\t" \
      "mov     %%edx, %3"           "\n\t" \
      "mov     4+%5, %%ebx"         "\n\t" \
      "mov     %5, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%1, %%ecx"         "\n\t" \
      "mull    %5"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %3, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %2, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n\t" \
      "sub     %5, %%ecx"           "\n\t" \
      "sbb     4+%5, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n"   \
      "0:" \
 \
      : "=m" ((X)[2]), "=m" ((X)[3]), "=rm" (r1), "=rm" (r2) \
      : "m" (b), "m" (p), "m" ((X)[0]), "m" ((X)[1]), "m" (mod64_rnd) \
      : "%eax", "%ebx", "%ecx", "%edx", "cc" ); \
}

/* Given uint64_t X[4], b, p:

   Assign X[i+4] <-- X[i] * b (mod p), for 0 <= i < 4.

   Assumes %st(0) contains b/p and FPU is in round-to-zero mode.
*/
#define VEC4_MULMOD64_NEXT(X,b,p) { \
  uint32_t r1, r2; \
  asm("fildll  %8"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %0"                  "\n\t" \
      "fildll  %9"                  "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %1"                  "\n\t" \
 \
      "fildll  %10"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %2"                  "\n\t" \
      "fildll  %11"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %3"                  "\n\t" \
 \
      "mov     0+%8, %%ebx"         "\n\t" \
      "mov     4+%8, %%ecx"         "\n\t" \
      "mov     %6, %%eax"           "\n\t" \
      "imul    4+%6, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%8"                "\n\t" \
      "mov     %%eax, %4"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %0, %%eax"           "\n\t" \
      "mov     %%edx, %5"           "\n\t" \
      "mov     4+%7, %%ebx"         "\n\t" \
      "mov     %7, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%0, %%ecx"         "\n\t" \
      "mull    %7"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %5, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %4, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n\t" \
      "sub     %7, %%ecx"           "\n\t" \
      "sbb     4+%7, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%9, %%ebx"         "\n\t" \
      "mov     4+%9, %%ecx"         "\n\t" \
      "mov     %6, %%eax"           "\n\t" \
      "imul    4+%6, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%9"                "\n\t" \
      "mov     %%eax, %4"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %1, %%eax"           "\n\t" \
      "mov     %%edx, %5"           "\n\t" \
      "mov     4+%7, %%ebx"         "\n\t" \
      "mov     %7, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%1, %%ecx"         "\n\t" \
      "mull    %7"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %5, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %4, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n\t" \
      "sub     %7, %%ecx"           "\n\t" \
      "sbb     4+%7, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%10, %%ebx"        "\n\t" \
      "mov     4+%10, %%ecx"        "\n\t" \
      "mov     %6, %%eax"           "\n\t" \
      "imul    4+%6, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%10"               "\n\t" \
      "mov     %%eax, %4"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %2, %%eax"           "\n\t" \
      "mov     %%edx, %5"           "\n\t" \
      "mov     4+%7, %%ebx"         "\n\t" \
      "mov     %7, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%2, %%ecx"         "\n\t" \
      "mull    %7"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %5, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %4, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%2"         "\n\t" \
      "mov     %%ebx, 4+%2"         "\n\t" \
      "sub     %7, %%ecx"           "\n\t" \
      "sbb     4+%7, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%2"         "\n\t" \
      "mov     %%ebx, 4+%2"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%11, %%ebx"        "\n\t" \
      "mov     4+%11, %%ecx"        "\n\t" \
      "mov     %6, %%eax"           "\n\t" \
      "imul    4+%6, %%ebx"         "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%11"               "\n\t" \
      "mov     %%eax, %4"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %3, %%eax"           "\n\t" \
      "mov     %%edx, %5"           "\n\t" \
      "mov     4+%7, %%ebx"         "\n\t" \
      "mov     %7, %%ecx"           "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%3, %%ecx"         "\n\t" \
      "mull    %7"                  "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %5, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %4, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%3"         "\n\t" \
      "mov     %%ebx, 4+%3"         "\n\t" \
      "sub     %7, %%ecx"           "\n\t" \
      "sbb     4+%7, %%ebx"         "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%3"         "\n\t" \
      "mov     %%ebx, 4+%3"         "\n"   \
      "0:" \
 \
      : "=m" ((X)[4]), "=m" ((X)[5]), "=m" ((X)[6]), "=m" ((X)[7]), \
        "=rm" (r1), "=rm" (r2) \
      : "m" (b), "m" (p), \
        "m" ((X)[0]), "m" ((X)[1]), "m" ((X)[2]), "m" ((X)[3]), \
        "m" (mod64_rnd) \
      : "%eax", "%ebx", "%ecx", "%edx", "cc" ); \
}

/* Given uint64_t X[8], b, p:

   Assign X[i+8] <-- X[i] * b (mod p), for 0 <= i < 8.

   Assumes %st(0) contains b/p and FPU is in round-to-zero mode.
*/
#define VEC8_MULMOD64_NEXT(X,b,p) { \
  uint32_t r1, r2; \
  asm("fildll  %12"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %0"                  "\n\t" \
      "fildll  %13"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %1"                  "\n\t" \
 \
      "fildll  %14"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %2"                  "\n\t" \
      "fildll  %15"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %3"                  "\n\t" \
 \
      "fildll  %16"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %4"                  "\n\t" \
      "fildll  %17"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %5"                  "\n\t" \
 \
      "fildll  %18"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %6"                  "\n\t" \
      "fildll  %19"                 "\n\t" \
      "fmul    %%st(1), %%st(0)"    "\n\t" \
      "fistpll %7"                  "\n\t" \
 \
      "mov     0+%12, %%ebx"        "\n\t" \
      "mov     4+%12, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%12"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %0, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%0, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%0"         "\n\t" \
      "mov     %%ebx, 4+%0"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%13, %%ebx"        "\n\t" \
      "mov     4+%13, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%13"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %1, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%1, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%1"         "\n\t" \
      "mov     %%ebx, 4+%1"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%14, %%ebx"        "\n\t" \
      "mov     4+%14, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%14"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %2, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%2, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%2"         "\n\t" \
      "mov     %%ebx, 4+%2"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%2"         "\n\t" \
      "mov     %%ebx, 4+%2"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%15, %%ebx"        "\n\t" \
      "mov     4+%15, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%15"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %3, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%3, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%3"         "\n\t" \
      "mov     %%ebx, 4+%3"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%3"         "\n\t" \
      "mov     %%ebx, 4+%3"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%16, %%ebx"        "\n\t" \
      "mov     4+%16, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%16"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %4, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%4, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%4"         "\n\t" \
      "mov     %%ebx, 4+%4"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%4"         "\n\t" \
      "mov     %%ebx, 4+%4"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%17, %%ebx"        "\n\t" \
      "mov     4+%17, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%17"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %5, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%5, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%5"         "\n\t" \
      "mov     %%ebx, 4+%5"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%5"         "\n\t" \
      "mov     %%ebx, 4+%5"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%18, %%ebx"        "\n\t" \
      "mov     4+%18, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%18"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %6, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%6, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%6"         "\n\t" \
      "mov     %%ebx, 4+%6"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%6"         "\n\t" \
      "mov     %%ebx, 4+%6"         "\n"   \
      "0:"                          "\t"   \
 \
      "mov     0+%19, %%ebx"        "\n\t" \
      "mov     4+%19, %%ecx"        "\n\t" \
      "mov     %10, %%eax"          "\n\t" \
      "imul    4+%10, %%ebx"        "\n\t" \
      "imul    %%eax, %%ecx"        "\n\t" \
      "mull    0+%19"               "\n\t" \
      "mov     %%eax, %8"           "\n\t" \
      "add     %%ecx, %%ebx"        "\n\t" \
      "add     %%ebx, %%edx"        "\n\t" \
      "mov     %7, %%eax"           "\n\t" \
      "mov     %%edx, %9"           "\n\t" \
      "mov     4+%11, %%ebx"        "\n\t" \
      "mov     %11, %%ecx"          "\n\t" \
      "imul    %%eax, %%ebx"        "\n\t" \
      "imul    4+%7, %%ecx"         "\n\t" \
      "mull    %11"                 "\n\t" \
      "add     %%ebx, %%ecx"        "\n\t" \
      "mov     %9, %%ebx"           "\n\t" \
      "add     %%ecx,%%edx"         "\n\t" \
      "mov     %8, %%ecx"           "\n\t" \
      "sub     %%eax, %%ecx"        "\n\t" \
      "sbb     %%edx, %%ebx"        "\n\t" \
      "mov     %%ecx, 0+%7"         "\n\t" \
      "mov     %%ebx, 4+%7"         "\n\t" \
      "sub     %11, %%ecx"          "\n\t" \
      "sbb     4+%11, %%ebx"        "\n\t" \
      "jl      0f"                  "\n\t" \
      "mov     %%ecx, 0+%7"         "\n\t" \
      "mov     %%ebx, 4+%7"         "\n"   \
      "0:" \
      : "=m" ((X)[8]), "=m" ((X)[9]), "=m" ((X)[10]), "=m" ((X)[11]), \
        "=m" ((X)[12]), "=m" ((X)[13]), "=m" ((X)[14]), "=m" ((X)[15]), \
        "=rm" (r1), "=rm" (r2) \
      : "m" (b), "m" (p), \
        "m" ((X)[0]), "m" ((X)[1]), "m" ((X)[2]), "m" ((X)[3]), \
        "m" ((X)[4]), "m" ((X)[5]), "m" ((X)[6]), "m" ((X)[7]), "m" (mod64_rnd) \
      : "%eax", "%ebx", "%ecx", "%edx", "cc" ); \
}
#endif /* NEED_vector && !HAVE_vector */


#if defined(NEED_btop) && !defined(HAVE_btop)

#ifdef __SSE2__
#define VECTOR_LENGTH 4
/* In btop-x86-sse2.S */
void btop_x86_sse2(const uint32_t *L, uint64_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86_sse2(L,T,lmin,llen,p)
#define HAVE_btop
#else /* !__SSE2__ */
#define VECTOR_LENGTH 4
/* In btop-x86.S */
void btop_x86(const uint32_t *L, uint64_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_x86(L,T,lmin,llen,p)
#define HAVE_btop
#endif /* __SSE2__ */

#endif /* NEED_btop && !HAVE_btop */


#if defined(NEED_swizzle_loop) && !defined(HAVE_swizzle_loop)

#ifdef __SSE2__
#if (SWIZZLE==4)
/* In loop-x86-sse2.S */
int swizzle_loop4_x86_sse2(int i, const uint64_t *T, struct loop_data_t *D,
                           uint64_t p) attribute ((regparm(1)));
int swizzle_loop4_x86_sse2_prefetch(int i, const uint64_t *T,
                  struct loop_data_t *D, uint64_t p) attribute ((regparm(1)));
#define swizzle_loop(a,b,c,d) swizzle_loop4_x86_sse2(c,a,b,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop4_x86_sse2_prefetch(c,a,b,d)
#define HAVE_swizzle_loop
#endif
#else /* !__SSE2__ */
#if (SWIZZLE==2)
/* In loop-x86.S */
int swizzle_loop2_x86(const uint64_t *T, struct loop_data_t *D,
                      int i, uint64_t p);
int swizzle_loop2_x86_prefetch(const uint64_t *T, struct loop_data_t *D,
                               int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop2_x86(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop2_x86_prefetch(a,b,c,d)
#define HAVE_swizzle_loop
#endif
#endif /* __SSE2__ */

#endif /* NEED_swizzle_loop && !HAVE_swizzle_loop */
