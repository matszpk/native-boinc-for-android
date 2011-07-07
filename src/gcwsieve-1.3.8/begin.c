/* begin.c -- (C) Geoffrey Reynolds, Mark Rodenkirch, December 2007.

   Begin a new sieve from scratch.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/



#ifdef SMALL_P

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include "gcwsieve.h"
#include "arithmetic.h"
#include "bitmap.h"

#define WOODALL 0
#define CULLEN 1

static uint_fast32_t *cw_map[2];

static int eliminate_phase1_term(uint32_t n, int cw, uint32_t p)
{
  assert(n <= n_max);

  if (test_bit(cw_map[cw],n-n_min))
  {
    if (n < 32 && is_equal(n,cw,p))
    {
      /* p is an improper factor, p == n*b^n+c */
      logger(1,"%s is prime.",term_str(n,cw));
    }
    else
    {
      /* p is a proper new factor. */
      clear_bit(cw_map[cw],n-n_min);
      report_factor(n,cw,p);
      return 1;
    }
  }

  return 0;
}

static void init_phase1_sieve(void)
{
  uint32_t range = n_max - n_min;
  uint32_t i, count;
  int cw;

  smallp_phase = 1;
  for (count = cw = 0; cw <= 1; cw++)
    if (cw_opt[cw])
    {
      cw_map[cw] = make_bitmap(range+1,"init_phase1_sieve");
      fill_bits(cw_map[cw],0,range);
      if (b_term % 2)
      {
        /* b is odd ==> n*b^n+/-1 is divisible by 2 whenever n is odd.
           Don't bother to report these factors. */

        for (i = (n_min+1) % 2; i <= range; i += 2)
          if (clear_bit(cw_map[cw],i))
            count++;
      }
    }

  factor_count += count;
  if (verbose_opt && count > 0)
    report(1,"Removed %"PRIu32" even terms without logging factors.", count);

  /* Code for removing algebraic factors was supplied by Mark Rodenkirch.
   */
  // Generalized Woodalls and Cullens might have some easy to find large
  // factors. This function will find factors based upon the following:
  // Given a*b^a-1, if a=c^x*b^y and (a+y)%x = 0 for integers c, x, and y,
  // then a*b^a-1 has a factor of c*b^((a+y)/x)-1
  // Given a*b^a+1, if a=c^x*b^y and (a+y)%x = 0 and x is odd for integers
  // c, x, and y, a*b^a+1 has a factor of c*b^((a+y)/x)+1
  {
    uint32_t  b, maxk, mink, k, kbase, bpow, kpow;
    uint64_t  bexp, kexp;

    b = b_term;
    maxk = n_max;
    mink = n_min;
    count = 0;

    // Start with nexp=n^0, going to n^1, n^2, etc.
    for (bpow=0, bexp=1; bexp*bexp<=maxk; bpow++, bexp*=b)
    {
      // Start with kbase=2, going to 3, 4, etc.
      for (kbase=2; kbase*kbase*bexp<=maxk; kbase++)
      {
        // Start with kexp=kbase^2, going to kbase^3, etc.
        for (kpow=2, kexp=kbase*kbase; kexp*bexp<=maxk; kpow++, kexp*=kbase)
        {
          k = kexp * bexp;
          if (cw_opt[WOODALL] && (k+bpow)%kpow == 0 && k >= mink)
            if (clear_bit(cw_map[WOODALL],k-mink))
            {
              logger(0,"(%d*%d^%d-1)%%(%d*%d^%d-1)",
                     k, b, k, kbase, b, (k+bpow)/kpow);
              count++;
            }

          if (cw_opt[CULLEN] && kpow%2 == 1 && (k+bpow)%kpow == 0 && k >= mink)
            if (clear_bit(cw_map[CULLEN],k-mink))
            {
              logger(0,"(%d*%d^%d+1)%%(%d*%d^%d+1)",
                     k, b, k, kbase, b, (k+bpow)/kpow);
              count++;
            }
        }
      }
    }
  }

  factor_count += count;
  if (verbose_opt && count > 0)
    report(1,"Removed %"PRIu32" terms with algebraic factors.", count);
}

static void fini_phase1_sieve(void)
{
  int cw;

  for (cw = 0; cw <= 1; cw++)
    if (cw_opt[cw])
      free(cw_map[cw]);
}

/* Return (a/p) if gcd(a,p)==1, 0 otherwise.
 */
static int jacobi32(uint32_t a, uint32_t p)
{
  uint32_t x, y, t;
  int sign;

  for (sign = 1, x = a, y = p; x > 0; x %= y)
  {
    for ( ; x % 2 == 0; x /= 2)
      if (y % 8 == 3 || y % 8 == 5)
        sign = -sign;

    t = x, x = y, y = t;

    if (x % 4 == 3 && y % 4 == 3)
      sign = -sign;
  }

  return (y == 1) ? sign : 0;
}

static void phase1_sieve(uint64_t p64)
{
  uint32_t b, p, k, t, x, y, i;

  assert(p64 > 2);
  assert(p64 < (1 << 15));

  p = p64;
  b = b_term % p;
  if (b == 0)
    return;

  /* If p divides n*b^n+/-1 then p also divides (n+t)*b^(n+t)+/-1.
  */
  t = p*(p-1);
  if (jacobi32(b,p) == 1)
    t /= 2;

  mod64_init(p);
  for (x = k = 1; k <= t; k++)
  {
    x = mulmod64(x,b,p);
    y = mulmod64(x,k,p);

    if (y == 1 && cw_opt[WOODALL])
    {
      for (i = k; i <= n_max; i += t)
        if (i >= n_min)
          eliminate_phase1_term(i,WOODALL,p);
    }
    else if (y == p-1 && cw_opt[CULLEN])
    {
      for (i = k; i <= n_max; i += t)
        if (i >= n_min)
          eliminate_phase1_term(i,CULLEN,p);
    }
  }
  mod64_fini();
}

void begin_sieve(void)
{
  int i, cw, tmp;
  uint32_t phase1_max;

  b_term = base_opt;

  factor_count = 0;
  set_accumulated_time(0.0);
  set_accumulated_cpu(0.0);

  tmp = quiet_opt;
  quiet_opt = 1;
  phase1_max = MIN(p_max,sqrt(n_max));
  smallp_phase = 1;

  init_prime_sieve(phase1_max);
  init_phase1_sieve();
  if (verbose_opt)
    report(1,"Removing tiny factors (3 <= p <= %"PRIu32") from new sieve ...",
           phase1_max);
  smallp_phase = 2;
  prime_sieve(3,phase1_max,phase1_sieve);

  for (cw = 0; cw <= 1; cw++)
    if (cw_opt[cw])
      for (i = n_min; i <= n_max; i++)
        if (test_bit(cw_map[cw],i-n_min))
          add_term(i, cw? +1 : -1);

  fini_phase1_sieve();
  fini_prime_sieve();

  if (verbose_opt)
    report(1,"Removed %"PRIu32" terms from new sieve in %.2f sec.",
           factor_count,get_accumulated_time());

  quiet_opt = tmp;
  p_min = MIN(p_max,phase1_max+1);
}

#endif /* SMALL_P */
