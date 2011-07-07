/* arithmetic.c -- (C) Geoffrey Reynolds, April 2006.

   64 bit arithmetic functions: mod, addmod, mulmod, invmod, powmod
   (see arithmetic.h for inline functions).

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <assert.h>
#include "arithmetic.h"

#if (defined(GENERIC_mulmod64) || (USE_ASM && __x86_64__))
double one_over_p;
double b_over_p;
#endif

#if (USE_ASM && __i386__)
uint16_t mod64_rnd; /* Saved FPU/SSE rounding mode */
#elif (USE_ASM && __x86_64__)
uint32_t mod64_rnd; /* Saved FPU/SSE rounding mode */
#endif


#ifndef HAVE_powmod64
#define HAVE_powmod64
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
#endif

#ifndef HAVE_invmod64
#define HAVE_invmod64
/*
  1/a (mod p). Assumes 1 < a < 2^32, a < p.
*/
uint64_t invmod32_64(uint32_t a, uint64_t p)
{
  /* Thanks to the folks at mersenneforum.org.
     See http://www.mersenneforum.org/showthread.php?p=58252. */

  uint64_t ps1, ps2, q;
  uint32_t r, t, dividend, divisor;
  int sign = -1;

  assert(1 < a);
  assert(a < p);

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
#endif
