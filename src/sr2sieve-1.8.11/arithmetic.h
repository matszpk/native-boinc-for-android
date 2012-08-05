/* arithmetic.h -- (C) Geoffrey Reynolds, April 2006.

   arithmetic functions: mulmod, sqrmod, invmod, powmod.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

/* The reason for the complicated structure of this file is to allow
   the the critical functions to be implemented incrementally: At a
   minimum it should be possible to implement a fast mulmod function,
   and the generic implementation of the other critical functions will
   employ it. At a later date a fast powmod function can be added, etc.
*/

#ifndef _ARITHMETIC_H
#define _ARITHMETIC_H

#include <stdint.h>
#include "config.h"

#if USE_ASM
# define NEED_mulmod64
# define NEED_sqrmod64
# define NEED_powmod64
# define NEED_invmod64
# define NEED_vector
# define NEED_powseq64
# define NEED_powmod64_shifted
# if defined(ARM_CPU)
#  include "asm-arm.h"
# elif (__i386__ && __GNUC__)
#  include "asm-i386-gcc.h"
# elif ((__ppc64__ || __powerpc64__) && __GNUC__)
#  include "asm-ppc64.h"
# elif (__x86_64__ && __GNUC__)
#  include "asm-x86-64-gcc.h"
# endif
#endif

/*
  Use generic code for any functions not defined above in assembler.
*/


/* arithmetic64.c */

#ifndef HAVE_mulmod64
#define HAVE_mulmod64
#define GENERIC_mulmod64

extern double one_over_p;

static inline void mod64_init(uint64_t p)
{
  one_over_p = 1.0/p;
}

static inline void mod64_fini(void)
{
}

/* This is based on code by Phil Carmody posted in sci.crypt. It should
   work for a,b,p up to 52 bits.
*/
#define MOD64_MAX_BITS 52

/*
  a * b (mod p)
*/
static inline uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t p)
{
  int64_t tmp, ret;
  register double x, y;

  x = (int64_t)a;
  y = (int64_t)b;
  tmp = (int64_t)p * (int64_t)(x*y*one_over_p);
  ret = a*b - tmp;

  if (ret < 0)
    ret += p;
  else if (ret >= p)
    ret -= p;

  return ret;
}


extern double b_over_p;

static inline void PRE2_MULMOD64_INIT(uint64_t b)
{
  b_over_p = (double)((int64_t)b) * one_over_p;
}

static inline void PRE2_MULMOD64_FINI(void)
{
}

static inline uint64_t PRE2_MULMOD64(uint64_t a, uint64_t b, uint64_t p)
{
  int64_t tmp, ret;
  register double x;

  x = (int64_t)a;
  tmp = (int64_t)p * (int64_t)(x*b_over_p);
  ret = a*b - tmp;

  if (ret < 0)
    ret += p;
  else if (ret >= p)
    ret -= p;

  return ret;
}
#endif /* HAVE_mulmod64 */


#ifndef HAVE_sqrmod64
#define HAVE_sqrmod64
/*
  b^2 (mod p)
*/
#ifdef GENERIC_mulmod64
static inline uint64_t sqrmod64(uint64_t b, uint64_t p)
{
  int64_t tmp, ret;
  register double x;

  x = (int64_t)b;
  tmp = (int64_t)p * (int64_t)(x*x*one_over_p);
  ret = b*b - tmp;

  if (ret < 0)
    ret += p;
  else if (ret >= p)
    ret -= p;

  return ret;
}
#else
/* Assume the custom mulmod is faster than a generic sqrmod. */
static inline uint64_t sqrmod64(uint64_t b, uint64_t p)
{
  return mulmod64(b,b,p);
}
#endif
#endif /* HAVE_sqrmod64 */


#ifndef HAVE_vector
#define HAVE_vector

#define GENERIC_vector
void vec_mulmod64_initp(uint64_t p);
static inline void vec_mulmod64_finip(void) { mod64_fini(); }
void vec_mulmod64_initb(uint64_t b);
static inline void vec_mulmod64_finib(void) { PRE2_MULMOD64_FINI(); }
void vec2_mulmod64(const uint64_t *X, uint64_t *Y, int count);
void vec4_mulmod64(const uint64_t *X, uint64_t *Y, int count);
void vec8_mulmod64(const uint64_t *X, uint64_t *Y, int count);
#endif /* HAVE_vector */


#ifndef HAVE_powmod64
#define HAVE_powmod64

#define GENERIC_powmod64
/*
  b^n (mod p)
*/
uint64_t powmod64(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#endif /* HAVE_powmod64 */


#ifdef __GNUC__
#ifndef HAVE_vec_powmod64
#define HAVE_vec_powmod64

#define GENERIC_vec_powmod64
/*
  B[i] <-- B[i]^n (mod p) for 0 <= i < len.
*/
void vec_powmod64(uint64_t *B, int len, uint64_t n, uint64_t p);
#endif /* HAVE_vec_powmod64 */
#endif /* __GNUC__ */


#ifndef HAVE_invmod64
#define HAVE_invmod64

#define GENERIC_invmod64
/*
  1/a (mod p) if sign is negative or -1/a (mod p) otherwise.
  Assumes 1 < a < p, p prime.
*/
uint64_t invmod32_64(uint32_t a, int32_t sign, uint64_t p) attribute ((const));
#endif /* HAVE_invmod64 */

#endif /* _ARITHMETIC_H */
