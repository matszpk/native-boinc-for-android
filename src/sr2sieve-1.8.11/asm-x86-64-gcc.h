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

#if USE_FPU_MULMOD
/**************************************************************************
 *                                                                        *
 * x87-64 routines use the x87 FPU for double extended precision floating *
 * point operations. Modulus is limited to 2^62.                          *
 *                                                                        *
 **************************************************************************/
#define FPU_MODE_BITS "(0x0F00)"  /* 64-bit precision, round to zero. */
#define MOD64_MAX_BITS 62         /* 64-2 useful bits */

/* Saved copy of FPU mode bits. */
extern uint16_t mod64_rnd;

/* in mulmod-x87-64.S */
uint16_t mulmod64_init_x87_64(uint64_t p);
void mulmod64_fini_x87_64(uint16_t mode);
#define mod64_init(p) { mod64_rnd = mulmod64_init_x87_64(p); }
#define mod64_fini() { mulmod64_fini_x87_64(mod64_rnd); }

/* a*b (mod p), where a,b < p < 2^62.
   Assumes FPU is set to double extended precision and round-to-zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
*/
static inline uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t p)
{
  register uint64_t ret, r1;
  uint64_t tmp;

  asm ("fildll  %3"                 "\n\t"
       "fildll  %4"                 "\n\t"
       "fmulp   %%st(0), %%st(1)"   "\n\t"
       "fmul    %%st(1), %%st(0)"   "\n\t"
       "fistpll %2"                 "\n\t"
       "imul    %1, %0"             "\n\t"
       "mov     %2, %1"             "\n\t"
       "imul    %5, %1"             "\n\t"
       "sub     %1, %0"             "\n\t"
       "mov     %0, %1"             "\n\t"
       "sub     %5, %1"             "\n\t"
       "jl      0f"                 "\n\t"
       "mov     %1, %0"             "\n"
       "0:"
       : "=r" (ret), "=r" (r1), "=m" (tmp)
       : "m" (a), "m" (b), "rm" (p), "0" (a), "1" (b), "m" (mod64_rnd)
       : "cc" );

  return ret;
}

/* a*b (mod p), where a,b < p < 2^62.
   Assumes FPU is set to double extended precision and round-to-zero.
   Assumes %st(0) contains b/p computed with above settings.
*/
#define PRE2_MULMOD64(a,b,p) ({ \
  register uint64_t ret, r1; \
  uint64_t tmp; \
  asm ("fildll  %3"                 "\n\t" \
       "fmul    %%st(1), %%st(0)"   "\n\t" \
       "fistpll %2"                 "\n\t" \
       "imul    %1, %0"             "\n\t" \
       "mov     %2, %1"             "\n\t" \
       "imul    %5, %1"             "\n\t" \
       "sub     %1, %0"             "\n\t" \
       "mov     %0, %1"             "\n\t" \
       "sub     %5, %1"             "\n\t" \
       "jl      0f"                 "\n\t" \
       "mov     %1, %0"             "\n" \
       "0:" \
       : "=r" (ret), "=r" (r1), "=m" (tmp) \
       : "m" (a), "0" (b), "rm" (p), "1" (a), "m" (mod64_rnd) \
       : "cc" ); \
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

#else /* !USE_FPU_MULMOD */
/**************************************************************************
 *                                                                        *
 * x86-64 routines use the SSE2 FPU for double precision floating point   *
 * operations. Modulus is limited to 2^51.                                *
 *                                                                        *
 **************************************************************************/
#define SSE_MODE_BITS "(0x6000)"  /* 53-bit precision, round To Zero */
#define MOD64_MAX_BITS 51         /* 53-2 useful bits */

/* Saved copy of SSE rounding mode bits. */
extern uint16_t mod64_rnd;

extern double one_over_p;
extern double b_over_p;

/* in mulmod-x86-64.S */
uint16_t mulmod64_init_x86_64(uint64_t p);
void mulmod64_fini_x86_64(uint16_t mode);
#define mod64_init(p) { mod64_rnd = mulmod64_init_x86_64(p); }
#define mod64_fini() { mulmod64_fini_x86_64(mod64_rnd); }

/* a*b (mod p), where a,b < p < 2^51.
   Assumes SSE operations round to zero.
   Assumes one_over_p = 1.0/p computed in round-to-zero mode.
*/
static inline uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t p)
{
  register uint64_t ret, tmp;
  register double x, y;

  asm ("cvtsi2sdq %0, %2"    "\n\t"
       "cvtsi2sdq %5, %3"    "\n\t"
       "mulsd     %7, %2"    "\n\t"
       "mulsd     %2, %3"    "\n\t"
       "cvtsd2siq %3, %1"    "\n\t"
       "imul      %5, %0"    "\n\t"
       "imul      %6, %1"    "\n\t"
       "sub       %1, %0"    "\n\t"
       "mov       %0, %1"    "\n\t"
       "sub       %6, %1"    "\n\t"
       "jl        0f"        "\n\t"
       "mov       %1, %0"    "\n"
       "0:"
       : "=r" (ret), "=&r" (tmp), "=&x" (x), "=&x" (y)
       : "0" (a), "rm" (b), "rm" (p), "xm" (one_over_p), "m" (mod64_rnd)
       : "cc" );

  return ret;
}

/* a*b (mod p), where a,b < p < 2^51.
   Assumes SSE operations round to zero.
   Assumes b_over_p = b/p computed in round-to-zero mode.
*/
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

/* Set b_over_p to b/p, computed in round-to-zero mode.
   Assumes SSE operations round to zero.
   Assumes one_over_p = 1.0/p computed in round-to-zero mode.
 */
#define PRE2_MULMOD64_INIT(b) \
  asm ("cvtsi2sdq %2, %0"    "\n\t" \
       "mulsd     %3, %0" \
       : "=&x" (b_over_p), "+m" (mod64_rnd) \
       : "rm" ((uint64_t)b), "xm" (one_over_p) \
       : "cc" )

#define PRE2_MULMOD64_FINI() {}

#endif /* USE_FPU_MULMOD */
#endif /* defined(NEED_mulmod64) && !defined(HAVE_mulmod64) */


#if defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64)
#define HAVE_sqrmod64
#if USE_FPU_MULMOD
/**************************************************************************
 *                                                                        *
 * x87-64 routines use the x87 FPU for double extended precision floating *
 * point operations. Modulus is limited to 2^62.                          *
 *                                                                        *
 **************************************************************************/
extern uint16_t mod64_rnd;

/* b^2 (mod p), where b < p < 2^62.
   Assumes FPU is set to double extended precision and round-to-zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
*/
static inline uint64_t sqrmod64(uint64_t a, uint64_t p)
{
  register uint64_t ret, r1;
  uint64_t tmp;

  asm ("fildll  %3"                 "\n\t"
       "fmul    %%st(0), %%st(0)"   "\n\t"
       "fmul    %%st(1), %%st(0)"   "\n\t"
       "fistpll %2"                 "\n\t"
       "imul    %0, %0"             "\n\t"
       "mov     %2, %1"             "\n\t"
       "imul    %4, %1"             "\n\t"
       "sub     %1, %0"             "\n\t"
       "mov     %0, %1"             "\n\t"
       "sub     %4, %1"             "\n\t"
       "jl      0f"                 "\n\t"
       "mov     %1, %0"             "\n"
       "0:"
       : "=r" (ret), "=&r" (r1), "=m" (tmp)
       : "m" (a), "rm" (p), "0" (a), "m" (mod64_rnd)
       : "cc" );

  return ret;
}
#else /* !USE_FPU_MULMOD */
/**************************************************************************
 *                                                                        *
 * x86-64 routines use the SSE2 FPU for double precision floating point   *
 * operations. Modulus is limited to 2^51.                                *
 *                                                                        *
 **************************************************************************/
extern uint16_t mod64_rnd;

/* b^2 (mod p), where b < p < 2^51.
   Assumes SSE operations round to zero.
   Assumes one_over_p = 1.0/p computed in round-to-zero mode.
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
       "sub       %4, %1"    "\n\t"
       "jl        0f"        "\n\t"
       "mov       %1, %0"    "\n"
       "0:"
       : "=r" (ret), "=&r" (tmp), "=&x" (x)
       : "0" (b), "rm" (p), "xm" (one_over_p), "m" (mod64_rnd)
       : "cc" );

  return ret;
}
#endif /* USE_FPU_MULMOD */
#endif /* defined(NEED_sqrmod64) && !defined(HAVE_sqrmod64) */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64
#define HAVE_vec_powmod64
/*
  b^n (mod p)
*/
#if USE_FPU_MULMOD
/**************************************************************************
 *                                                                        *
 * x87-64 routines use the x87 FPU for double extended precision floating *
 * point operations. Modulus is limited to 2^62.                          *
 *                                                                        *
 **************************************************************************/
uint64_t powmod64_x87_64(uint64_t b, uint64_t n, uint64_t p) attribute((pure));
#define powmod64(b,n,p) powmod64_x87_64(b,n,p)

void vec_powmod64_x87_64(uint64_t *B, int l, uint64_t n, uint64_t p);
#define vec_powmod64(B,len,n,p) vec_powmod64_x87_64(B,len,n,p)

#else /* !USE_FPU_MULMOD */
/**************************************************************************
 *                                                                        *
 * x86-64 routines use the SSE2 FPU for double precision floating point   *
 * operations. Modulus is limited to 2^51.                                *
 *                                                                        *
 **************************************************************************/
uint64_t powmod64_x86_64(uint64_t b, uint64_t n, uint64_t p, double invp)
     attribute ((const));
#define powmod64(b,n,p) powmod64_x86_64(b,n,p,one_over_p)

void vec_powmod64_x86_64(uint64_t *B,int l,uint64_t n,uint64_t p,double invp);
#define vec_powmod64(B,len,n,p) vec_powmod64_x86_64(B,len,n,p,one_over_p)

#endif /* USE_FPU_MULMOD */
#endif /* defined(NEED_powmod64) && !defined(HAVE_powmod64) */


#if defined(NEED_vector) && !defined(HAVE_vector)
#define HAVE_vector

#if USE_FPU_MULMOD
/**************************************************************************
 *                                                                        *
 * x87-64 routines use the x87 FPU for double extended precision floating *
 * point operations. Modulus is limited to 2^62.                          *
 *                                                                        *
 **************************************************************************/
/* in mulmod-x87-64.S */
uint16_t vec_mulmod64_initp_x87_64(uint64_t p);
void vec_mulmod64_finip_x87_64(uint16_t mode);
#define vec_mulmod64_initp(p) { mod64_rnd = vec_mulmod64_initp_x87_64(p); }
#define vec_mulmod64_finip() { vec_mulmod64_finip_x87_64(mod64_rnd); }

void vec_mulmod64_initb_x87_64(uint64_t b);
static inline void vec_mulmod64_finib_x87_64(void)
{
  asm volatile ("fstp    %%st(0)" : "+m" (mod64_rnd) /* Dummy */ : : "cc" );
}
void vec2_mulmod64_x87_64(const uint64_t *X, uint64_t *Y, int count);
void vec4_mulmod64_x87_64(const uint64_t *X, uint64_t *Y, int count);
void vec6_mulmod64_x87_64(const uint64_t *X, uint64_t *Y, int count);
void vec8_mulmod64_x87_64(const uint64_t *X, uint64_t *Y, int count);

#define vec_mulmod64_initb(b) vec_mulmod64_initb_x87_64(b);
#define vec_mulmod64_finib() vec_mulmod64_finib_x87_64();

#define vec2_mulmod64 vec2_mulmod64_x87_64
#define vec4_mulmod64 vec4_mulmod64_x87_64
#define vec6_mulmod64 vec6_mulmod64_x87_64
#define vec8_mulmod64 vec8_mulmod64_x87_64

#else /* !USE_FPU_MULMOD */
/**************************************************************************
 *                                                                        *
 * x86-64 routines use the SSE2 FPU for double precision floating point   *
 * operations. Modulus is limited to 2^51.                                *
 *                                                                        *
 **************************************************************************/

/* in mulmod-x86-64.S */
uint16_t vec_mulmod64_initp_x86_64(uint64_t p);
void vec_mulmod64_finip_x86_64(uint16_t mode);
#define vec_mulmod64_initp(p) { mod64_rnd = vec_mulmod64_initp_x86_64(p); }
#define vec_mulmod64_finip() { vec_mulmod64_finip_x86_64(mod64_rnd); }

void vec_mulmod64_initb_x86_64(uint64_t b);
static inline void vec_mulmod64_finib_x86_64(void) { };
void vec2_mulmod64_x86_64(const uint64_t *X, uint64_t *Y, int count);
void vec4_mulmod64_x86_64(const uint64_t *X, uint64_t *Y, int count);
void vec6_mulmod64_x86_64(const uint64_t *X, uint64_t *Y, int count);
void vec8_mulmod64_x86_64(const uint64_t *X, uint64_t *Y, int count);

#define vec_mulmod64_initb(b) vec_mulmod64_initb_x86_64(b);
#define vec_mulmod64_finib() vec_mulmod64_finib_x86_64();

#define vec2_mulmod64 vec2_mulmod64_x86_64
#define vec4_mulmod64 vec4_mulmod64_x86_64
#define vec6_mulmod64 vec6_mulmod64_x86_64
#define vec8_mulmod64 vec8_mulmod64_x86_64

#endif /* USE_FPU_MULMOD */
#endif /* defined(NEED_vector) && !defined(HAVE_vector) */


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
