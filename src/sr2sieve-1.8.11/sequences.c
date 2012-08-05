/* sequences.c -- (C) Geoffrey Reynolds, April 2006.

   Routines for creating and manipulating candidate sequences.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "sr5sieve.h"

seq_t   *SEQ = NULL;          /* global array of candidate sequences */
uint32_t seq_count = 0;       /* current number of sequences */

int16_t div_shift[POWER_RESIDUE_LCM/2];
uint8_t divisor_index[POWER_RESIDUE_LCM+1];

uint64_t p_min;
uint64_t p_max;
uint32_t n_min;               /* Min n for all k*b^n+c */
uint32_t n_max;               /* Max n for all k*b^n+c */
uint32_t k_max;               /* Max k for all k*b^n+c */

#if (BASE == 0)
uint32_t b_term;              /* Fixed b in k*b^n+c */
#endif

static uint32_t seq_alloc = 0;

#define SEQ_GROW_SIZE 64
uint32_t new_seq(uint32_t k, int32_t c)
{
  uint32_t new;

  if (seq_count == seq_alloc)
  {
    seq_alloc += SEQ_GROW_SIZE;
    SEQ = xrealloc(SEQ, seq_alloc*sizeof(seq_t));
  }

  new = seq_count++;
  SEQ[new].k = k;
  SEQ[new].c = c;
  SEQ[new].ncount = 0;
  SEQ[new].N = NULL;
  SEQ[new].nsize = 0;

  return new;
}

static char seq_buf[44];
const char *kbnc_str(uint32_t k, uint32_t n, int32_t c)
{
#if (BASE == 0)
#if DUAL
  if (dual_opt)
    sprintf(seq_buf,"%"PRIu32"^%"PRIu32"%c%"PRIu32,b_term,n,(c>0)?'+':'-',k);
  else
#endif
    sprintf(seq_buf,"%"PRIu32"*%"PRIu32"^%"PRIu32"%+"PRId32,k,b_term,n,c);
#else
#if DUAL
  if (dual_opt)
    sprintf(seq_buf,XSTR(BASE)"^%"PRIu32"%c%"PRIu32,n,(c>0)?'+':'-',k);
  else
#endif
    sprintf(seq_buf,"%"PRIu32"*"XSTR(BASE)"^%"PRIu32"%+"PRId32,k,n,c);
#endif

  return seq_buf;
}

const char *seq_str(uint32_t seq)
{
  assert (seq < seq_count);

#if (BASE == 0)
#if DUAL
  if (dual_opt)
    sprintf(seq_buf,"%"PRIu32"^n%c%"PRIu32,b_term,(SEQ[seq].c>0)?'+':'-',SEQ[seq].k);
  else
#endif
    sprintf(seq_buf,"%"PRIu32"*%"PRIu32"^n%+"PRId32,SEQ[seq].k,b_term,SEQ[seq].c);
#else
#if DUAL
  if (dual_opt)
    sprintf(seq_buf,XSTR(BASE)"^n%c%"PRIu32,(SEQ[seq].c>0)?'+':'-',SEQ[seq].k);
  else
#endif
    sprintf(seq_buf,"%"PRIu32"*"XSTR(BASE)"^n%+"PRId32,SEQ[seq].k,SEQ[seq].c);
#endif

  return seq_buf;
}

#define SEQ_N_GROW_SIZE 4096
void add_seq_n(uint32_t seq, uint32_t n)
{
  uint32_t count;

  assert(seq < seq_count);

  count = SEQ[seq].ncount;
  if (count == SEQ[seq].nsize)
  {
    SEQ[seq].nsize += SEQ_N_GROW_SIZE;
    SEQ[seq].N = xrealloc(SEQ[seq].N, SEQ[seq].nsize*sizeof(uint32_t));
  }

  SEQ[seq].N[count] = n;
  SEQ[seq].ncount++;
}

/* Set n_min, n_max, k_max from the current candidate sequences.
 */
static void calculate_k_n_range(void)
{
  uint32_t i;

  n_min = UINT32_MAX;
  n_max = 0;
  k_max = 0;

  for (i = 0; i < seq_count; i++)
  {
    n_min = MIN(n_min, SEQ[i].N[0]);
    n_max = MAX(n_max, SEQ[i].N[SEQ[i].ncount-1]);
    k_max = MAX(k_max, SEQ[i].k);
  }
}

uint32_t *make_setup_ladder(double max_density)
{
  uint32_t i, j, k, a;
  uint32_t *ladder;
  uint32_t D[LIMIT_BASE+1];

  for (i = 0; i < subseq_Q; i++)
    D[i] = 0;
  D[subseq_Q] = 1;
  for (i = 0, a = 1; i < subseq_count; i++)
    if (D[subseq_d[i]] == 0)
      D[subseq_d[i]] = 1, a++;

  for (i = 0; i < 3; i++)
  {
    if (D[i] == 1)
      a--;
    D[i] = 2;
  }

  while (a > 0)
  {
    for (i = 3, j = 2; i <= subseq_Q; i++)
    {
      if (D[i] == 2)
        j = i;
      else if (D[i] == 1)
        break;
    }
    assert(i <= subseq_Q);

    if (D[i-j] == 2)
      D[i] = 2, a--; /* We can use an existing rung */
    else
    {
      k = MIN(i-j,(i+1)/2); /* Need to create a new rung */
      assert(D[k]==0);
      D[k] = 1;
      a++;
      for (k++; k <= j; k++) /* Need to re-check rungs above the new one */
        if (D[k] == 2)
          D[k] = 1, a++;
    }
  }

  for (i = 3, a = 2; i <= subseq_Q; i++)
    if (D[i] == 2)
      a++;

  if (a > subseq_Q*max_density)
    return NULL; /* Too many rungs, faster not to use a ladder. */

  ladder = xmalloc((a-1)*sizeof(uint32_t));
  for (i = 3, j = 2, k = 0; i <= subseq_Q; i++)
    if (D[i] == 2)
    {
      assert(D[i-j]==2);
      ladder[k] = i-j;
      j = i;
      k++;
    }
  assert(k+2 == a);
  ladder[k] = 0;

#ifndef NDEBUG
    report(1,"Using a %"PRIu32" rung addition ladder, density %.2f.",
           a, (double)a/subseq_Q);
#endif

  return ladder;
}

void finish_candidate_seqs(void)
{
  uint32_t i;

  for (i = 0; i < seq_count; i++)
  {
    if (SEQ[i].ncount == 0)
      error("%s: Empty sequence.",seq_str(i));
#if DUAL
    if (dual_opt)
    {
      if (SEQ[i].k <= 1)
        error("%s: b^n+/-k must satisfy |k|>1.", seq_str(i));
    }
    else
#endif
    {
      if (SEQ[i].k == 0)
        error("%s: k*b^n+c must satisfy k>0.", seq_str(i));
      if (ABS(SEQ[i].c) != 1)
        error("%s: k*b^n+c must satisfy |c|>1.", seq_str(i));
    }

#if DUAL
    if (dual_opt)
    {
# if (BASE == 0)
      if (b_term % 2 == SEQ[i].k % 2)
        error("%s: Every term is divisible by 2.", seq_str(i));
# else
      if (BASE % 2 == SEQ[i].k % 2)
        error("%s: Every term is divisible by 2.", seq_str(i));
# endif
    }
    else /* |c| == 1 */
#endif /* DUAL */
    {
#if (BASE == 0)
      if (b_term % 2 == 1 && SEQ[i].k % 2 == 1)
        error("%s: Every term is divisible by 2.", seq_str(i));
#else
      if (BASE % 2 == 1 && SEQ[i].k % 2 == 1)
        error("%s: Every term is divisible by 2.", seq_str(i));
#endif
    }
  }

  calculate_k_n_range();
  make_subseqs();

  /* Precompute the div_shift table. Powers 2^x are represented as -x. */ 
  {
    uint32_t divide, shift, r;

    for (i = 0; i < POWER_RESIDUE_LCM/2; i++)
    {
      r = gcd32(2*i,POWER_RESIDUE_LCM);
      for (shift = 0, divide = r; divide % 2 == 0; shift++)
        divide /= 2;
      if (divide > 1)
        div_shift[i] = r;
      else if (shift > 1)
        div_shift[i] = -shift;
      else
        div_shift[i] = 0;
    }

    /* Build table divisor_index[r], for divisors r of POWER_RESIDUE_LCM.
     */
    for (i = 1, r = 0; i <= POWER_RESIDUE_LCM; i++)
      if (POWER_RESIDUE_LCM % i == 0)
        divisor_index[i] = r++;

    assert(r == POWER_RESIDUE_DIVISORS);
  }
}
