/* arithmetic.h -- (C) Geoffrey Reynolds, April 2006.

   arithmetic functions: mulmod, invmod, powmod.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
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
# if defined(ARM_CPU)
#  include "asm-arm.h"
# elif defined(__i386__) && defined(__GNUC__)
#  include "asm-i386-gcc.h"
# elif defined(__i386__) && defined(_MSC_VER)
#  include "asm-i386-msc.h"
# elif (defined(__ppc64__) || defined(__powerpc64__)) && defined(__GNUC__)
#  include "asm-ppc64.h"
# elif defined(__x86_64__) && defined(__GNUC__)
#  include "asm-x86-64-gcc.h"
# elif defined(__x86_64__) && defined(_MSC_VER)
#  include "asm-x86-64-msc.h"
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

/*
  a * b (mod p)
*/

/* This is based on code by Phil Carmody posted in sci.crypt. It should
   work for a,b,p up to 52 bits.
*/
#define MOD64_MAX_BITS 52

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

static inline void mod64_init(uint64_t p)
{
  one_over_p = 1.0/p;
}

static inline void mod64_fini(void)
{
}

extern double b_over_p;

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

static inline void PRE2_MULMOD64_INIT(uint64_t b)
{
  b_over_p = (double)((int64_t)b) * one_over_p;
}

static inline void PRE2_MULMOD64_FINI(void)
{
}
#endif


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
#endif


#ifndef HAVE_powmod64
/*
  b^n (mod p)
*/
uint64_t powmod64(uint64_t b, uint64_t n, uint64_t p) attribute ((pure));
#endif


#ifndef HAVE_invmod64
/*
  1/a (mod p). Assumes 1 < a < 2^32, a < p.
*/
uint64_t invmod32_64(uint32_t a, uint64_t p) attribute ((const));
#endif

#endif /* _ARITHMETIC_H */
