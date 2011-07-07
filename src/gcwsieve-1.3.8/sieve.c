/* sieve.c -- (C) Geoffrey Reynolds, March 2007.

   Main sieve routine.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

/* Algorithm for sieving Cullen and Woodall numbers n*b^n+/-1.

   [Input]
   Input a sieve range P0,P1
   Input a list N+ of positive Cullen exponents in increasing order
   Input a list N- of positive Woodall exponents in increasing order
   Input a base integer b > 1

   [Initialise table of gaps]
   Let G+ and  G- be lists of positive integers
   For c in {+,-}
     For i from 2 to |Nc|
       Gc[i] <-- Nc[i]-Nc[i-1]

   [Sieve]
   For each prime p in P0 <= p <= P1
     [Build a table of small powers]
     Let T be a table of integers modulo p
     For each g in {z | z in G+ or z in G-}
       T[g] <-- b^g mod p
     Let x and y be integers modulo p
     Let i be a positive integer
     y <-- 1/b mod p
     For c in {+,-}
       i <-- |Nc|
       x <-- c1*y^Nc[i] mod p
       If x = Nc[i]
         Report that p divides Nc[i]*b^Nc[i]c1
       [Main loop]
       While i > 2
         x <-- x*T[Gc[i]] mod p
         i <-- i-1
         If x = Nc[i]
           Report that p divides Nc[i]*b^Nc[i]c1

   The actual implementation below has become a bit more complicated.
*/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#ifndef NDEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include "gcwsieve.h"
#include "arithmetic.h"
#include "swizzle.h"
#include "btop.h"

/* This file is compiled seperately for each code path, and so any global
   symbol SYM must be declared and referenced as CODE_PATH_NAME(SYM).
*/


static uint32_t lmin;     /* gmax/2 <= lmin <= gmax */
                          /* Second part of table begins at lmin+1 */

#ifdef ARM_T_T
static struct T_t* T;
#else
static uint64_t *T;       /* T[j] = (b^gmul)^j for 0 <= j <= gmax. */
#endif

static struct loop_data_t *D[2]; /* Swizzled data. See swizzle.h */

#if USE_LADDER
static uint32_t *L;       /* L[i] = i-th entry in second part of G[] */
static uint32_t llen;     /* llen = length of L[] */

/* If the second part of the table is shorter than MIN_LADDER_SPAN then
   don't use a ladder.
*/
#define MIN_LADDER_SPAN (2*VECTOR_LENGTH)
#endif

#ifdef HAVE_novfp_loop_data
uint32_t oldclzp;
#endif

#ifdef HAVE_novfp_loop_data
static void reshift_loop_data(uint32_t shift)
{
  int cw, i, j, k;
  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
    {
      k = tcount[cw]/SWIZZLE;

      for (i = k; i-- > 0; )
        for (j = 0; j < SWIZZLE; j++)
          D[cw]->R[i].N[j] <<= shift;
    }
}
#endif

static void sieve64(uint64_t p)
{
  uint64_t y;
  int i, cw;
  uint32_t j, k;
#ifdef HAVE_novfp_loop_data
  uint32_t clzb;
#endif

  mod64_init(p);

#ifdef HAVE_novfp_loop_data
  if (mod64_init_data.clzp!=oldclzp)
    reshift_loop_data(mod64_init_data.clzp-oldclzp);
  clzb = mod64_init_data.clzp-2;
#endif

  /* Pre-compute table of powers T[j] = (b^gmul)^(j+1) (mod p).
   */
#ifdef ARM_T_T
  T[0].T = powmod64(b_term,gmul,p);
  premod64_init(T,p);
#else
  T[0] = powmod64(b_term,gmul,p);
#endif
  
#if USE_LADDER
  build_table_of_powers(L,T,lmin,llen,p);
#else
  build_table_of_powers(NULL,T,lmin,0,p);
#endif

#ifdef SMALL_P
  if (p <= n_max/2)
  {
    const uint32_t cw_res[2] = {1,p-1};

    for (cw = 0; cw < 2; cw++)
      if (ncount[cw])
      {
        j = powmod64(b_term,N[cw][0],p);
        k = mulmod64(j,N[cw][0],p);
        if (k == cw_res[cw])
          eliminate_term(0,cw,p);
        for (i = 1; i < tcount[cw]; i++)
        {
#ifdef ARM_T_T
          uint64_t B = T[G[cw][i-1]].T-p;
          B = (B>=p) ? B-p : B;
          j = mulmod64(j,B,p);
#else
          j = mulmod64(j,T[G[cw][i-1]],p);
#endif
          k = mulmod64(j,N[cw][i],p);
          if (k == cw_res[cw])
            eliminate_term(i,cw,p);
        }
      }
    mod64_fini();
    return;
  }
  smallp_phase = 4;
#endif

  /* y <-- 1/b (mod p).
   */
  y = (b_term == 2) ? (p+1)/2 : invmod32_64(b_term,p);

  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
    {
      /* If tcount is not a multiple of SWIZZLE add the extra terms to
         X[SWIZZLE-1] */
      j = tcount[cw]-1;
      D[cw]->X[SWIZZLE-1] = powmod64(y,N[cw][j],p);
      if (cw)
        D[cw]->X[SWIZZLE-1] = p - D[cw]->X[SWIZZLE-1];

      /* Test the extra terms.
       */
      while (j%SWIZZLE != (SWIZZLE-1))
      {
#ifdef SMALL_P
        if (D[cw]->X[SWIZZLE-1]==N[cw][j] || D[cw]->X[SWIZZLE-1]+p==N[cw][j])
#else
        if (D[cw]->X[SWIZZLE-1] == N[cw][j])
#endif
        {
          eliminate_term(j,cw,p);
        }
        j--;
#ifdef ARM_T_T
        {
          uint64_t B = T[G[cw][j]].T-p;
          B = (B>=p) ? B-p : B;
          D[cw]->X[SWIZZLE-1] = mulmod64(D[cw]->X[SWIZZLE-1],B,p);
        }
#else
        D[cw]->X[SWIZZLE-1] = mulmod64(D[cw]->X[SWIZZLE-1],T[G[cw][j]],p);
#endif
      }

      /* In iteration i (decreasing to 0) we process terms N[j*k+i],
         0<=j<SWIZZLE. */

      /* X[SWIZZLE-1] was computed above.
         Now compute remaining initial values X[0] ... X[SWIZZLE-2].
      */
      k = tcount[cw]/SWIZZLE;
      i = k-1;
      for (j = 0; j < (SWIZZLE-1); j++)
      {
        D[cw]->X[j] = powmod64(y,N[cw][j*k+i],p);
        if (cw)
          D[cw]->X[j] = p - D[cw]->X[j];
      }

#ifdef HAVE_novfp_loop_data
      for (j = 0; j < SWIZZLE; j++)
        D[cw]->X[j] = (D[cw]->X[j])<<clzb;
#endif

      /* Main loop.
       */
      do
      {
        /* Check for factor */
        for (j = 0; j < SWIZZLE; j++)
#ifdef SMALL_P
          if (D[cw]->X[j]==D[cw]->R[i].N[j] || D[cw]->X[j]+p==D[cw]->R[i].N[j])
#else
          if (D[cw]->X[j] == D[cw]->R[i].N[j])
#endif
          {
            eliminate_term(j*k+i,cw,p);
          }

#if PREFETCH_OPT
        if (prefetch_opt > 0)
          i = swizzle_loop_prefetch(T,D[cw],i,p);
        else
#endif
          i = swizzle_loop(T,D[cw],i,p);
#ifndef NDEBUG
#ifdef HAVE_novfp_loop_data
        printf("cw=%d,i=%d:X0=%llu:X1=%llu,p=%llu\n", cw, i,
               D[cw]->X[0]>>clzb, D[cw]->X[1]>>clzb, p);
#else
        printf("cw=%d,i=%d:X0=%llu:X1=%llu,p=%llu\n", cw, i,
               D[cw]->X[0], D[cw]->X[1], p);
#endif
#endif
      }
      while (i >= 0);
    }

#ifdef HAVE_novfp_loop_data
  oldclzp = mod64_init_data.clzp;
#endif
  mod64_fini();
}

#if PREFETCH_OPT
#define NUM_TEST_PRIMES 10
static const uint64_t test_primes[NUM_TEST_PRIMES] = {
  UINT64_C(2250000000000023),
  UINT64_C(2250000000000043),
  UINT64_C(2250000000000059),
  UINT64_C(2250000000000061),
  UINT64_C(2250000000000079),
  UINT64_C(2250000000000089),
  UINT64_C(2250000000000113),
  UINT64_C(2250000000000163),
  UINT64_C(2250000000000191),
  UINT64_C(2250000000000209) };

static void choose_prefetch_opt(void)
{
  uint64_t times[2], t0, t1;
  int i, j;

  benchmarking = 1; /* Ignore any factors found while benchmarking. */

  for (i = 0; i < 2; i++)
  {
    prefetch_opt = i;
    times[i] = UINT64_MAX;
    for (j = 0; j < NUM_TEST_PRIMES; j++) /* Prime cache. */
      sieve64(test_primes[j]);
    for (j = 0; j < NUM_TEST_PRIMES; j++)
    {
      t0 = timestamp();
      sieve64(test_primes[j]);
      t1 = timestamp();
      times[i] = MIN(times[i],(t1-t0)); /* Record best time. */
    }

    if (verbose_opt >= 2)
      report(1,"Best time with%s software prefetch: %"PRIu64".",
             (i>0)? "" : "out", times[i]);
  }

  if (times[0] <= times[1])
    prefetch_opt = -1;
  else
    prefetch_opt = 1;

  benchmarking = 0;
}
#endif

static void init_sieve(void)
{
  uint32_t i, j, k;
  int cw;
#if PREFETCH_OPT
  size_t mem;
#endif
#ifdef HAVE_novfp_loop_data
  uint32_t clzb = 0;
  oldclzp = __builtin_clzll(p_min)+1;     // clzp
  clzb = oldclzp-2;
#endif

  /* Allow room for vector operations to overrun end of array.
   */
#ifdef HAVE_novfp_loop_data
  T = xmemalign(64,(gmax+VECTOR_LENGTH)*sizeof(struct T_t));
#else
  T = xmemalign(64,(gmax+VECTOR_LENGTH)*sizeof(uint64_t));
#endif
#if PREFETCH_OPT
  mem = (gmax+VECTOR_LENGTH)*sizeof(uint64_t);
#endif

#if USE_LADDER
  {
#ifdef _MSC_VER
    uint8_t *B = xmalloc((gmax+1)*sizeof(uint8_t));
#else
    uint8_t B[gmax+1];
#endif

    for (i = 0; i <= gmax; i++)
      B[i] = 0;
    for (cw = 0; cw < 2; cw++)
      if (ncount[cw])
        for (i = 0; i < tcount[cw]-1; i++)
          B[G[cw][i]] = 1;
    for (i = gmax/2; i < gmax && B[i+1] == 1; i++)
      ;

#if (VECTOR_LENGTH >= 4)
    /* Try to extend the first part of table, which can be filled by vector
       operations. It is faster to do N vector mulmods than 3/4*N non-vector
       ones. Ensure i = N-1 (mod N) to make full use of any overruns.
     */
    for (i |= (VECTOR_LENGTH-1); i+VECTOR_LENGTH <= gmax; i += VECTOR_LENGTH)
    {
      for (j = 1, k = 0; j <= VECTOR_LENGTH; j++)
        k += B[i+j];
      if (k < (VECTOR_LENGTH+2)*3/4)
        break;
    }
#endif

    if (i+MIN_LADDER_SPAN <= gmax)
    {
      lmin = i;
      llen = 0;
      for (i = lmin + 1; i <= gmax; i++)
        if (B[i])
          llen++;
      L = xmemalign(64,llen*sizeof(uint32_t));
      for (j = 0, i = lmin + 1; i <= gmax; i++)
        if (B[i])
          L[j++] = i - lmin - 1;
    }
    else
      lmin = gmax;

    if (verbose_opt >= 2)
      report(1,"gmax=%"PRIu32",gmul=%"PRIu32",lmin=%"PRIu32",llen=%"PRIu32,
             gmax,gmul,lmin,llen);

#ifdef _MSC_VER
    free(B);
#endif
  }
#else /* !USE_LADDER */
  lmin = gmax;
  if (verbose_opt >= 2)
    report(1,"gmax=%"PRIu32",gmul=%"PRIu32,gmax,gmul);
#endif /* USE_LADDER */

  for (cw = 0; cw < 2; cw++)
    if (ncount[cw])
    {
      k = tcount[cw]/SWIZZLE;

      /* Space for loop_data_t structure with k records. See swizzle.h */
      D[cw] = xmemalign(64,sizeof_loop_data(k));
#if PREFETCH_OPT
      mem += sizeof_loop_data(k);
#endif

      for (i = k; i-- > 0; )
        for (j = 0; j < SWIZZLE; j++)
        {
#ifndef HAVE_novfp_loop_data
          D[cw]->R[i].N[j] = N[cw][j*k+i];
          D[cw]->R[i].G[j] = G[cw][j*k+i];
#else
          D[cw]->R[i].N[j] = ((uint64_t)N[cw][j*k+i])<<clzb;
          D[cw]->R[i].G[j] = T+G[cw][j*k+i];
#endif
        }
    }

#if PREFETCH_OPT
  if (prefetch_opt == 0)
  {
    /* Decide whether to use software prefetching. I don't know of any way
       to do this other than timing the relevent functions with and without
       prefetching. However this is not a perfect solution, as no matter how
       many benchmarks are run it seems that the results do not always
       reflect actual sieve times. */

    /* Don't run benchmarks if the data set will clearly fit in L2 cache.
     */
    if (mem + 1024 > L2_cache_size*1024)
      choose_prefetch_opt();
    else
      prefetch_opt = -1;
  }

  if (verbose_opt)
    report(1,"%s software prefetch.", (prefetch_opt>0) ? "Using":"Not using");
#endif
}

static void fini_sieve(void)
{
  xfreealign(T);
#if USE_LADDER
  if (llen > 0)
    xfreealign(L);
#endif
  if (ncount[0])
    xfreealign(D[0]);
  if (ncount[1])
    xfreealign(D[1]);
}

void CODE_PATH_NAME(sieve)(void)
{
  init_sieve();
#if HAVE_FORK
  /* Start children before initializing the Sieve of Eratosthenes, to
     minimise forked image size. */
  if (num_children > 0)
    init_threads(p_min,sieve64);
#endif
  init_prime_sieve(p_max);
  start_gcwsieve();

#ifdef SMALL_P
  smallp_phase = 3;
#endif

#if HAVE_FORK
  if (num_children > 0)
    prime_sieve(p_min,p_max,parent_thread);
  else
#endif
    prime_sieve(p_min,p_max,sieve64);

#if HAVE_FORK
  /* Check that all children terminated normally before assuming that
     the range is complete. */
  if (num_children > 0 && fini_threads(p_max))
    finish_gcwsieve("range is incomplete",get_lowtide());
  else
#endif
    finish_gcwsieve("range is complete",p_max);
  fini_prime_sieve();
  fini_sieve();
}
