/* subseq.c -- (C) Geoffrey Reynolds, June 2006.

   Base b sequences in n=qm+r where n0 < n < n1 are represented as a number
   of base b^Q subsequences in m where n0/Q < m < n1/Q.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "sr5sieve.h"
#include "bitmap.h"

subseq_t *SUBSEQ = NULL;
scl_t    *SCL = NULL;
#if LIMIT_BASE < UINT16_MAX
uint16_t *subseq_d = NULL;
#else
uint32_t *subseq_d = NULL;
#endif
uint32_t subseq_count = 0;
uint32_t subseq_Q = 0;
uint32_t factor_count;        /* candidate n eliminated so far */

static uint32_t m_low;
static uint32_t m_high;

static uint_fast32_t subseq_test_m(uint32_t subseq, uint32_t m)
{
  assert(subseq < subseq_count);

  if (m < m_low || m > m_high)
    return 0;

  return test_bit(SUBSEQ[subseq].M, m-m_low);
}

static uint_fast32_t subseq_clear_m(uint32_t subseq, uint32_t m)
{
  assert(subseq < subseq_count);

  if (m < m_low || m > m_high)
    return 0;

  return clear_bit(SUBSEQ[subseq].M, m-m_low);
}

/* Return 1 iff subsequence h of k*b^n+c has any terms with n%a==b.
 */ 
static int congruent_terms(uint32_t h, uint32_t a, uint32_t b)
{
  uint_fast32_t i, j;
  uint32_t g;

  assert(b < a);

  g = gcd32(a,subseq_Q);
  if (b % g == subseq_d[h] % g)
  {
    j = m_high-m_low;
    for (i = first_bit(SUBSEQ[h].M); i <= j; i = next_bit(SUBSEQ[h].M,i+1))
      if (((i+m_low)*subseq_Q+subseq_d[h]) % a == b)
        return 1;
  }

  return 0;
}
/* Build tables sc_lists[i][j] of pointers to lists of subsequences
   whose terms k*b^m+c satisfy m = j (mod r) for i = divisor_index[r].
*/
static void make_subseq_congruence_tables(void)
{
  uint32_t h, i, j, k, r, len, mem = 0;
  uint32_t subseq_list[POWER_RESIDUE_LCM], *tmp;

  SCL = xmalloc(seq_count*sizeof(SCL[0]));

  for (i = 0; i < seq_count; i++)
  {
    for (j = 1, r = 0; j <= POWER_RESIDUE_LCM; j++)
      if (POWER_RESIDUE_LCM % j == 0)
      {
        SCL[i].sc_lists[r] = xmalloc(j*sizeof(uint32_t *));
        mem += j*sizeof(uint32_t *);
        for (k = 0; k < j; k++)
        {
          for (len = 0, h = SEQ[i].first; h <= SEQ[i].last; h++)
            if (congruent_terms(h,j,k))
              subseq_list[len++] = h;
          if (len == 0)
            tmp = NULL;
          else
          {
            tmp = xmalloc((len+1)*sizeof(uint32_t));
            mem += (len+1)*sizeof(uint32_t);
            for (h = 0; h < len; h++)
              tmp[h] = subseq_list[h];
            tmp[h] = SUBSEQ_MAX;
          }
          SCL[i].sc_lists[r][k] = tmp;
        }
        r++;
      }
    assert(r == POWER_RESIDUE_DIVISORS);
  }

  if (verbose_opt)
    report(1,"Using %"PRIu32"Kb for subsequence congruence tables.",mem/1024);
}

/* Create subsequence data: SUBSEQ[] and subseq_d[] arrays, subseq_count,
   and set .first and .last fields in SEQ[]. Also set subseq_Q (if not set).
 */
void make_subseqs(void)
{
  uint32_t i, j, r, n, s;

#ifndef NDEBUG
  uint32_t t_count = 0, old_count = 0;
#endif

  subseq_Q = find_best_Q(&subseq_count);
  SUBSEQ = xmalloc(subseq_count*sizeof(SUBSEQ[0]));
  subseq_d = xmalloc(subseq_count*sizeof(subseq_d[0]));
  m_low = n_min/subseq_Q;
  m_high = n_max/subseq_Q;

  for (i = 0, s = 0; i < seq_count; i++)
  {
    uint32_t nmin[LIMIT_BASE], sub[LIMIT_BASE];
#if CHECK_FOR_GFN
    uint32_t g[LIMIT_BASE];
#endif

    SEQ[i].first = s;

    for (j = 0; j < subseq_Q; j++)
    {
#if CHECK_FOR_GFN
      g[j] = 0;
#endif
      nmin[j] = UINT32_MAX;
    }

    for (j = 0; j < SEQ[i].ncount; j++)
    {
      n = SEQ[i].N[j];
      r = n % subseq_Q;
      nmin[r] = MIN(nmin[r],n);
#if CHECK_FOR_GFN
      g[r] = gcd32(g[r],n-nmin[r]);
#endif
    }

    for (r = 0; r < subseq_Q; r++)
      if (nmin[r] != UINT32_MAX)
      {
        subseq_d[s] = r;
        SUBSEQ[s].seq = i;
        SUBSEQ[s].M = make_bitmap(m_high-m_low+1);
#ifndef NDEBUG
        SUBSEQ[s].mcount = 0;
#endif
#if CHECK_FOR_GFN
        SUBSEQ[s].a = MAX(1,g[r]);
        SUBSEQ[s].b = nmin[r]%SUBSEQ[s].a;
#endif
        sub[r] = s++;
      }

    assert (s <= subseq_count);

    SEQ[i].last = s-1;

    for (j = 0; j < SEQ[i].ncount; j++)
    {
      n = SEQ[i].N[j];
      r = n % subseq_Q;
      set_bit(SUBSEQ[sub[r]].M, n/subseq_Q-m_low);
#ifndef NDEBUG
      SUBSEQ[sub[r]].mcount++;
#endif
    }

    free(SEQ[i].N);
#ifndef NDEBUG
    old_count += SEQ[i].ncount;
#endif
  }

  assert (s == subseq_count);

#if (BASE == 0)
  report(1,"Split %"PRIu32" base %"PRIu32" sequence%s into %"PRIu32" base %"
         PRIu32"^%"PRIu32" subsequence%s.",seq_count,b_term,plural(seq_count),
         subseq_count,b_term,subseq_Q,plural(subseq_count));
#else
  report(1,"Split %"PRIu32" base %u sequence%s into %"PRIu32" base %u^%"PRIu32
         " subsequence%s.", seq_count, (unsigned int)BASE, plural(seq_count),
         subseq_count, (unsigned int)BASE, subseq_Q, plural(subseq_count));
#endif

  if (verbose_opt)
    report(1,"Using %"PRIu32"Kb for subsequence bitmaps.",
           subseq_count*(m_high-m_low)/8/1024);

#ifndef NDEBUG
  for (i = 0, t_count = 0; i < subseq_count; i++)
    t_count += SUBSEQ[i].mcount;
  assert(t_count == old_count);
#endif

#if SKIP_CUBIC_OPT
  if (!skip_cubic_opt)
#endif
    make_subseq_congruence_tables();
}

/* 0 if all terms have even n;
   1 if all terms have odd n;
   2 if some terms are even and some are odd.
*/
uint32_t seq_parity(uint32_t seq)
{
  uint32_t i, parity;

  assert(subseq_Q % 2 == 0);

  for (i = SEQ[seq].first, parity = subseq_d[i] % 2; i <= SEQ[seq].last; i++)
    if (subseq_d[i] % 2 != parity)
    {
#if ALLOW_MIXED_PARITY_SEQUENCES
      return 2;
#else
      error("Sequence %s has mixed parity terms.",seq_str(i));
#endif
    }

  return parity;
}

/* Return 1 iff p == k*b^n+c.
 */
static int is_equal(uint32_t k, uint32_t n, int32_t c, uint64_t p)
{
#if DUAL
  if (dual_opt)
  {
    c = (c > 0) ? k : -k;
    k = 1;
  }
#endif

  p -= c; /* Assumes |c| < p and p-c < 2^64. */

  if (p % k)
    return 0;
  p /= k;

#if (BASE==0)
  for ( ; p % b_term == 0; p /= b_term)
    n--;
#else
  for ( ; p % BASE == 0; p /= BASE)
    n--;
#endif

  return (n == 0 && p == 1);
}

int benchmarking = 0;

/* eliminate a single term n=Qm+d for this subsequence.
*/
void eliminate_term(uint32_t subseq, uint32_t m, uint64_t p)
{
  uint32_t k, n;
  int32_t c;

  assert(subseq < subseq_count);

  if (!benchmarking)
  {
    int duplicate = !subseq_test_m(subseq,m);

    /* Don't check duplicates unless --duplicates switch is used.
     */
    if (duplicate && duplicates_file_name == NULL)
      return;

#if HAVE_FORK
    if (child_num >= 0)
    {
      child_eliminate_term(subseq,m,p);
      return;
    }
#endif

    k = SEQ[SUBSEQ[subseq].seq].k;
    c = SEQ[SUBSEQ[subseq].seq].c;
    n = m*subseq_Q+subseq_d[subseq];

#if CHECK_FACTORS
#if DUAL
    if (dual_opt)
    {
      if (!is_factor(1,(c>0)?k:-k,n,p))
        error("%"PRIu64" DOES NOT DIVIDE %s.",p,kbnc_str(k,n,c));
    }
    else
#endif
      if (!is_factor(k,c,n,p))
        error("%"PRIu64" DOES NOT DIVIDE %s.",p,kbnc_str(k,n,c));
#endif

    if (!duplicate)
    {
      if (n <= 64 && is_equal(k,n,c,p))
      {
        /* p is an improper factor, p == k*b^n+c */
        logger(1,"%s is prime.",kbnc_str(k,n,c));
      }
      else
      {
        /* p is a proper new factor. */
        factor_count++;
        save_factor(factors_file_name,k,c,n,p);
        if (!quiet_opt)
        {
          report(1,"%"PRIu64" | %s",p,kbnc_str(k,n,c));
          notify_event(factor_found);
        }
#if LOG_FACTORS_OPT
        if (log_factors_opt)
          logger(0,"%"PRIu64" | %s",p,kbnc_str(k,n,c));
#endif
#if UPDATE_BITMAPS
        subseq_clear_m(subseq,m);
#endif
      }
    }
    else if (duplicates_file_name != NULL)
    {
      /* p is a duplicate factor. */
      save_factor(duplicates_file_name,k,c,n,p);
      if (!quiet_opt)
      {
        report(1,"%"PRIu64" | %s (duplicate)",p,kbnc_str(k,n,c));
        notify_event(factor_found);
      }
    }
  }
}
