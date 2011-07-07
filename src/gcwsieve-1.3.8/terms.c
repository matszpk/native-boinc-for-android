/* terms.c -- (C) Geoffrey Reynolds, March 2007.

   Routines for creating and manipulating candidate terms n*b^n+c.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "gcwsieve.h"
#include "bitmap.h"

/* The maximum number of terms allowed in the sieve. */
#if USE_ASM && defined(__i386__)
#define MAX_TERMS ((1U<<28)-1)
#else
#define MAX_TERMS ((1U<<31)-1)
#endif

/* The minimum number of terms allowed in the sieve.
   We must have MIN_TERMS <= SWIZZLE, but SWIZZLE depends on which code
   path is used and that has not been decided yet. Thus MIN_TERMS must
   be at least equal to the greatest possible value of SWIZZLE.
*/
#if USE_ASM && defined(__x86_64__)
#define MIN_TERMS 6
#else
#define MIN_TERMS 4
#endif

uint64_t p_min;     /* lower bound on primes p */
uint64_t p_max;     /* upper bound on primes p */
uint32_t *N[2];     /* list of Cullen|Woodall n terms */
uint32_t ncount[2]; /* number of Cullen|Woodall n terms */
uint32_t tcount[2]; /* Total number of Cullen|Woodall terms (incl. dummies) */
uint32_t b_term;    /* b in n*b^n+c */
uint32_t n_min;     /* least n term */
uint32_t n_max;     /* greatest n term */
uint32_t gmul;      /* gap multiple */
uint32_t *G[2];     /* Gaps G[x][i] = (N[x][i+1]-N[x][i])/gmul - 1 */
uint32_t gmax;      /* Maximum G[x][i] */

uint32_t dummy_opt;
int benchmarking;   /* Don't report factors if set. */

static const int32_t cw_val[2] = {-1,+1};

static uint32_t nsize[2];          /* N[x] has room for nsize[x] terms */
static uint_fast32_t *elim_map[2]; /* Bit i set if N[][i] has been eliminated */

/* A large gap is one that is more than LARGE_GAP_FACTOR times the size of
   the average gap between terms. They will be filled with dummy terms.
*/
#define LARGE_GAP_FACTOR 6.0

static char seq_buf[40];
const char *term_str(uint32_t n, int cw)
{
  sprintf(seq_buf,"%"PRIu32"*%"PRIu32"^%"PRIu32"%+"PRId32,
          n,b_term,n,cw_val[cw]);

  return seq_buf;
}

#define TERM_GROW_SIZE 4096
void add_term(uint32_t n, int32_t c)
{
  assert (c == -1 || c == 1);

  if (n < n_min || n > n_max)
    return;

  int cw = (c > 0);

  if (!cw_opt[cw])
    return;

  if (ncount[cw] == nsize[cw])
  {
    nsize[cw] += TERM_GROW_SIZE;
    N[cw] = xrealloc(N[cw],nsize[cw]*sizeof(uint32_t));
  }

  N[cw][ncount[cw]] = n;
  ncount[cw]++;
}

static int compare_uint32(const void *A, const void *B)
{
  uint32_t a = *(const uint32_t *)A;
  uint32_t b = *(const uint32_t *)B;

  return (a > b) - (a < b);
}

void finish_candidate_terms(void)
{
  uint32_t i, j, k, dcount[2], gap_lim, gap;
  int cw;

  if (ncount[0] + ncount[1] == 0)
    error("Empty sieve.");

  n_min = UINT32_MAX;
  n_max = 0;
  gmul = 0;
  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
    {
      if (ncount[cw] < MIN_TERMS)
        error("Sieve contains %"PRIu32" `%+"PRId32"' terms,"
              " at least %d are needed.",
              ncount[cw], cw_val[cw], (int)MIN_TERMS);

      /* Sort terms into ascending order and check for duplicates.
       */
      qsort(N[cw],ncount[cw],sizeof(uint32_t),compare_uint32);
      for (i = 1; i < ncount[cw]; i++)
      {
        assert(N[cw][i-1] <= N[cw][i]);

        if (N[cw][i-1] == N[cw][i])
          error("Duplicate term in input: %s",term_str(N[cw][i],cw));
      }

      /* Find combined n-range.
       */
      n_min = MIN(n_min,N[cw][0]);
      n_max = MAX(n_max,N[cw][ncount[cw]-1]);

      /* Calculate the greatest common divisor of all gaps between exponents.
       */
      for (i = 1; i < ncount[cw]; i++)
        gmul = gcd32(gmul,N[cw][i]-N[cw][i-1]);
    }

  if (dummy_opt)
    gap_lim = MAX(dummy_opt,gmul);
  else
  {
    /* Fill in gaps that are more than LARGE_GAP_FACTOR times larger than
       the average gap, or that would prevent the table of powers fitting in
       L1 cache.
    */
    gap_lim = 0;
    for (cw = 0; cw < 2; cw++)
      if (ncount[cw])
      {
        i = LARGE_GAP_FACTOR*(N[cw][ncount[cw]-1]-N[cw][0])/ncount[cw];
        gap_lim = MAX(gap_lim,i);
      }
    gap_lim = MIN(gap_lim,gmul*L1_cache_size*(1024/sizeof(uint64_t)));
  }
  gap_lim -= gap_lim % gmul;
  assert (gap_lim >= gmul);

  gmax = 0;
  dcount[0] = dcount[1] = 0;
  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
    {
      /* Count the number of dummy entries that need to be added.
       */
      if (gap_lim > gmul)
        for (i = 1; i < ncount[cw]; i++)
          if ((gap = N[cw][i]-N[cw][i-1]) > gap_lim)
            dcount[cw] += (gap-1)/gap_lim;

      tcount[cw] = ncount[cw] + dcount[cw];
      elim_map[cw] = make_bitmap(tcount[cw],"eliminated terms");
      N[cw] = xrealloc(N[cw],tcount[cw]*sizeof(uint32_t));

      /* Insert the dummy entries.
       */
      if (dcount[cw] > 0)
      {
        for (i = ncount[cw]-1, j = tcount[cw]-1; i > 0; i--, j--)
        {
          N[cw][j] = N[cw][i];
          gap = N[cw][i]-N[cw][i-1];
          if (gap > gap_lim)
          {
            /* TODO: minimise the number of new gap sizes. */
            k = (gap-1)/gap_lim; /* number of dummies to add */
            gap /= (k+1);
            gap -= gap % gmul;   /* new gap between dummies */
            while (k-- > 0)
            {
              N[cw][j-1] = N[cw][j] - gap;
              j--;
              set_bit(elim_map[cw],j);
            }
          }
        }
        assert (j == 0);
      }

      if (tcount[cw] > MAX_TERMS)
        error("Sieve has %"PRIu32" `%+"PRId32"' terms, maximum is %"PRIu32,
              tcount[cw],cw_val[cw],MAX_TERMS);

      /* Build table of gaps.
       */
      G[cw] = xmalloc((tcount[cw]-1)*sizeof(uint32_t));
      for (i = j = 0; i < tcount[cw]-1; i++)
      {
        G[cw][i] = (N[cw][i+1] - N[cw][i])/gmul - 1;
        j = MAX(j,G[cw][i]);
      }
      gmax = MAX(gmax,j);
    }

  if (verbose_opt && dcount[0]+dcount[1] > 0)
    report(1,"Added %"PRIu32" dummy term%s to fill gaps "
           "greater than %"PRIu32" between exponents.",
           dcount[0]+dcount[1],plural(dcount[0]+dcount[1]),gap_lim);
}

/* Return 1 iff p == n*b^n+c.
 */
int is_equal(uint32_t n, int cw, uint64_t p)
{
  p = cw? p-1 : p+1;

  if (p % n)
    return 0;
  p /= n;

  for ( ; p % b_term == 0; p /= b_term)
    n--;

  return (n == 0 && p == 1);
}

void eliminate_term(uint32_t i, int cw, uint64_t p)
{
  uint32_t n;

  if (!benchmarking)
  {
    if (!test_bit(elim_map[cw],i))
    {
#if HAVE_FORK
      if (child_num >= 0)
      {
        child_eliminate_term(i,cw,p);
        return;
      }
#endif
      n = N[cw][i];
      if (n < 64 && is_equal(n,cw,p))
      {
        /* p is an improper factor, p == n*b^n+c */
        logger(1,"%s is prime.",term_str(n,cw));
      }
      else
      {
        set_bit(elim_map[cw],i);
        report_factor(n,cw,p);
      }
    }
  }
}

/* Call fun(n,c,arg) for each term n*b^n+c remaining.
   Return the number of terms remaining.
 */
uint32_t for_each_term(void (*fun)(uint32_t,int32_t,void *), void *arg)
{
  uint32_t i, count;
  int cw;

  /* This is the simplest method, call in order of c then in order of n.
   */
  count = 0;
  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
      for (i = 0; i < tcount[cw]; i++)
        if (!test_bit(elim_map[cw],i))
        {
          fun(N[cw][i],cw_val[cw],arg);
          count++;
        }

  return count;
}
