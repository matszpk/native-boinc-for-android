/* choose.c -- (C) Geoffrey Reynolds, August 2006.

   Choose the best Q for sieving in subsequence base b^Q.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include "sr5sieve.h"
#include "hashtable.h"

steps_t *steps = NULL;      /* steps[i] is the optimal number of */
                            /*  baby/giant steps for i subsequences. */

#define BABY_WORK    1.1    /* 1 mulmod, 1 insert */
#define GIANT_WORK   1.0    /* 1 mulmod, 1 lookup */
#define EXP_WORK     0.5    /* 1 mulmod at most */
#define SUBSEQ_WORK  1.0    /* 1 mulmod, 1 lookup (giant step 0). */
#define PRT_WORK     0.8    /* List traversal, linear search, etc. */


/* Set the number of baby/giant steps used in BSGS on s subsequences.
 */
static steps_t choose_steps(uint32_t Q, uint32_t s)
{
  steps_t S;
  uint32_t m, M, r;

  r = n_max/Q-n_min/Q+1;

  /*
    r = range of n, s = number of subsequences.
    In the worst case we will do do one table insertion and one mulmod
    for m baby steps, then s table lookups and s mulmods for M giant
    steps. The average case depends on how many solutions are found
    and how early in the loop they are found, which I don't know how
    to analyse. However for the worst case we just want to minimise
    m + s*M subject to m*M >= r, which is when m = sqrt(s*r).
  */

#if GIANT_OPT
  M = MAX(min_giant_opt,rint(sqrt((double)r/s)*scale_giant_opt));
  M = MIN(M,r);
#else
  M = MAX(1,rint(sqrt((double)r/s)));
#endif
  m = MIN(r,ceil((double)r/M));

  if (m > HASH_MAX_ELTS)
  {
    /* There are three ways to handle this undesirable case:
       1. Allow m > HASH_MAX_ELTS (undersize hash table).
       2. Restrict m <= HASH_MAX_ELTS (suboptimal baby/giant-step ratio).
       3. Recompile with SHORT_HASHTABLE=0.
    */
    M = ceil((double)r/HASH_MAX_ELTS);
    m = ceil((double)r/M);
  }

  assert(m <= HASH_MAX_ELTS);

  S.m = m;
  S.M = M;

  return S;
}

/* Return an estimate of the work needed to do one BSGS iteration on s
   subsequences in base b^Q with prime p%r=1.
*/
static double estimate_work(uint32_t Q, uint32_t s, uint32_t r)
{
  steps_t S;
  double work;
  uint32_t x;

  x = (s+r-1)/r; /* subsequences expected to survive power residue tests */
  S = choose_steps(Q,x);
  work = S.m*BABY_WORK + x*(S.M-1)*GIANT_WORK + Q*EXP_WORK + x*SUBSEQ_WORK;

  if (r > 2)
    work += x*PRT_WORK;

  return work;
}

/* Try to estimate the work needed to sieve s subsequences in base b^Q. This
   is a bit rough.
*/
static double rate_Q(uint32_t Q, uint32_t s)
{
  double work, W[POWER_RESIDUE_LCM+1];
  uint32_t i;

  assert (Q % 2 == 0);
  assert (Q % BASE_MULTIPLE == 0);
  assert (LIMIT_BASE % Q == 0);

#if SKIP_CUBIC_OPT
  if (skip_cubic_opt)
    work = estimate_work(Q,s,2);
  else
#endif
  for (i = 2, work = 0.0; i <= POWER_RESIDUE_LCM; i += 2)
  {
    if (POWER_RESIDUE_LCM % i == 0)
      W[i] = estimate_work(Q,s,i);

    if (gcd32(i+1,POWER_RESIDUE_LCM) == 1)
      work += W[gcd32(i,POWER_RESIDUE_LCM)];
  }

  if (verbose_opt > 1)
    report(1,"Q=%"PRIu32", s=%"PRIu32", %%=%.2f, w=%.0f.",
           Q, s, 100.0*s/(seq_count*Q), work);

#if SUBSEQ_Q_OPT
  if (subseq_Q == Q)
    return 1.0;
#endif

  return work;
}

static uint32_t count_residue_classes(uint32_t d, uint32_t Q, const uint8_t *R)
{
  uint32_t i, count;
  uint8_t R0[LIMIT_BASE];

  assert (Q % d == 0);

  for (i = 0; i < d; i++)
    R0[i] = 0;

  for (i = 0; i < Q; i++)
    if (R[i])
      R0[i%d] = 1;

  for (i = 0, count = 0; i < d; i++)
    if (R0[i])
      count++;

  return count;
}


uint32_t make_table_of_steps(void)
{
  uint32_t s, max_baby;

  steps = xmalloc((subseq_count+1)*sizeof(steps_t));

  for (s = 1, max_baby = 0; s <= subseq_count; s++)
  {
    steps[s] = choose_steps(subseq_Q,s);
    assert(n_max/subseq_Q < n_min/subseq_Q+(uint32_t)steps[s].m*steps[s].M);
    max_baby = MAX(max_baby,steps[s].m);
  }

  return max_baby;
}

void free_table_of_steps(void)
{
  free(steps);
}

/* Increase baby steps as far as the next multiple of vec_len if doing so
   will reduce the number of giant steps. This minimises the extra work due
   to doing mulmods in blocks of vec_len.
*/
void trim_steps(int vec_len, uint32_t max_baby_steps)
{
  uint32_t i, r, m, M;

  r = n_max/subseq_Q - n_min/subseq_Q + 1;

  for (i = 1; i <= subseq_count; i++)
  {
    m = steps[i].m;
    M = steps[i].M;
    if (m % vec_len) /* m is not a multiple of vec_len */
    {
      m += (vec_len - m%vec_len); /* next multiple of vec_len. */
      while (M > 1 && m*(M-1) >= r) /* minimise M */
        M--;
      while ((m-1)*M >= r) /* minimise m */
        m--;
      if (m <= max_baby_steps)
      {
        steps[i].m = m;
        steps[i].M = M;
      }
    }
  }
}

#define NDIVISORS (LIMIT_BASE/BASE_MULTIPLE)
uint32_t find_best_Q(uint32_t *subseqs)
{
  uint32_t i, j;
  uint32_t S[NDIVISORS];
  double W[NDIVISORS];
  uint8_t R[LIMIT_BASE];

  for (j = 0; j < NDIVISORS; j++)
    S[j] = 0;

  for (i = 0; i < seq_count; i++)
  {
    for (j = 0; j < LIMIT_BASE; j++)
      R[j] = 0;
    for (j = 0; j < SEQ[i].ncount; j++)
      R[SEQ[i].N[j]%LIMIT_BASE] = 1;
    for (j = 0; j < NDIVISORS; j++)
      if (NDIVISORS % (j+1) == 0)
        S[j] += count_residue_classes((j+1)*BASE_MULTIPLE,LIMIT_BASE,R);
  }


  for (i = 0, j = 0; j < NDIVISORS; j++)
    if (NDIVISORS % (j+1) == 0)
    {
      W[j] = rate_Q((j+1)*BASE_MULTIPLE,S[j]);
      if (W[j] < W[i])
        i = j;
    }

  *subseqs = S[i];
  return (i+1)*BASE_MULTIPLE;
}
