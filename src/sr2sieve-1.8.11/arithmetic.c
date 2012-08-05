/* arithmetic.c -- (C) Geoffrey Reynolds, April 2006.

   64 bit arithmetic functions: mod, addmod, mulmod, invmod, powmod
   (see arithmetic.h for inline functions).

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <assert.h>
#include <stdint.h>
#include "arithmetic.h"

#ifdef GENERIC_mulmod64
double one_over_p;
double b_over_p;
#elif (USE_ASM && __x86_64__ && (!USE_FPU_MULMOD || MULTI_PATH) && __GNUC__)
double one_over_p;
double b_over_p;
#endif

#if (USE_ASM && (__i386__ || __x86_64__) && __GNUC__)
uint16_t mod64_rnd; /* Saved FPU/SSE rounding mode */
#endif


#ifdef GENERIC_powmod64
/*
  return b^n (mod p)
*/
uint64_t powmod64(uint64_t b, uint64_t n, uint64_t p)
{
  uint64_t a;

  a = 1;
  goto tst;

 mul:
  a = mulmod64(a,b,p);

 sqr:
  b = sqrmod64(b,p);

  n >>= 1;

 tst:
  if (n & 1)
    goto mul;
  if (n > 0)
    goto sqr;

  return a;
}
#endif /* GENERIC_powmod64 */


#ifdef __GNUC__
#ifdef GENERIC_vec_powmod64
/*
  B[i] <-- B[i]^n (mod p) for 0 <= i < len.
*/
void vec_powmod64(uint64_t *B, int len, uint64_t n, uint64_t p)
{
  uint64_t A[len], x;
  int i;

  assert(len > 0);
  assert(n > 0);

  if (len == 1)
    return;

  for (i = 0; i < len; i++)
    A[i] = B[i];

  for (x = UINT64_C(1) << (62-__builtin_clzll(n)); x > 0; x >>= 1)
  {
    for (i = 0; i < len; i++)
      B[i] = sqrmod64(B[i],p);

    if (x & n)
      for (i = 0; i < len; i++)
        B[i] = mulmod64(B[i],A[i],p);
  }
}
#endif /* GENERIC_vec_powmod64 */
#endif /* __GNUC__ */


#ifdef GENERIC_invmod64
/*
  1/a (mod p) if sign is negative or -1/a (mod p) otherwise.
  Assumes 1 < a < p, p prime.
*/
uint64_t invmod32_64(uint32_t a, int32_t sign, uint64_t p)
{
  /* Thanks to the folks at mersenneforum.org.
     See http://www.mersenneforum.org/showthread.php?p=58252. */

  uint64_t ps1, ps2, q;
  uint32_t r, t, dividend, divisor;

  assert(1 < a);
  assert(a < p);
  assert(sign != 0);

  q = p / a;
  r = p % a;

  assert(r > 0);

  dividend = a;
  divisor = r;
  ps1 = q;
  ps2 = 1;
	
  while (divisor > 1)
  {
    r = dividend - divisor;
    t = r - divisor;
    if (r >= divisor) {
      q += ps1; r = t; t -= divisor;
      if (r >= divisor) {
        q += ps1; r = t; t -= divisor;
        if (r >= divisor) {
          q += ps1; r = t; t -= divisor;
          if (r >= divisor) {
            q += ps1; r = t; t -= divisor;
            if (r >= divisor) {
              q += ps1; r = t; t -= divisor;
              if (r >= divisor) {
                q += ps1; r = t; t -= divisor;
		if (r >= divisor) {
                  q += ps1; r = t; t -= divisor;
                  if (r >= divisor) {
                    q += ps1; r = t;
                    if (r >= divisor) {
                      q = dividend / divisor;
                      r = dividend % divisor;
                      q *= ps1;
                    } } } } } } } } }
    q += ps2;
    sign = -sign;
    dividend = divisor;
    divisor = r;
    ps2 = ps1;
    ps1 = q;
  }

  assert(0 < ps1);
  assert(ps1 < p);

  return (sign > 0) ? ps1 : p - ps1;
}
#endif /* GENERIC_invmod64 */


#ifdef GENERIC_vector
static uint64_t mulmod_b, mulmod_p;

void vec_mulmod64_initp(uint64_t p)
{
  mulmod_p = p;
  mod64_init(p);
}

void vec_mulmod64_initb(uint64_t b)
{
  mulmod_b = b;
  PRE2_MULMOD64_INIT(b);
}

void vec2_mulmod64(const uint64_t *X, uint64_t *Y, int count)
{
  uint64_t b, p;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  for (i = 0; i < count; i += 2)
  {
    Y[i+0] = PRE2_MULMOD64(X[i+0],b,p);
    Y[i+1] = PRE2_MULMOD64(X[i+1],b,p);
  }
}

void vec4_mulmod64(const uint64_t *X, uint64_t *Y, int count)
{
  uint64_t b, p;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  for (i = 0; i < count; i += 4)
  {
    Y[i+0] = PRE2_MULMOD64(X[i+0],b,p);
    Y[i+1] = PRE2_MULMOD64(X[i+1],b,p);
    Y[i+2] = PRE2_MULMOD64(X[i+2],b,p);
    Y[i+3] = PRE2_MULMOD64(X[i+3],b,p);
  }
}

void vec8_mulmod64(const uint64_t *X, uint64_t *Y, int count)
{
  uint64_t b, p;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  for (i = 0; i < count; i += 8)
  {
    Y[i+0] = PRE2_MULMOD64(X[i+0],b,p);
    Y[i+1] = PRE2_MULMOD64(X[i+1],b,p);
    Y[i+2] = PRE2_MULMOD64(X[i+2],b,p);
    Y[i+3] = PRE2_MULMOD64(X[i+3],b,p);
    Y[i+4] = PRE2_MULMOD64(X[i+4],b,p);
    Y[i+5] = PRE2_MULMOD64(X[i+5],b,p);
    Y[i+6] = PRE2_MULMOD64(X[i+6],b,p);
    Y[i+7] = PRE2_MULMOD64(X[i+7],b,p);
  }
}
#endif /* GENERIC_vector */
