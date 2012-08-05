/* bsgs.c -- (C) Geoffrey Reynolds, April 2006.

   Implementation of a baby step giant step algorithm for finding all
   n in the range nmin <= n <= nmax satisfying b^n=d_i (mod p) where b
   and each d_i are relatively prime to p.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr5sieve.h"
#include "arithmetic.h"
#include "bitmap.h"
#include "hashtable.h"
#include "lookup-ind.h"

/*
  Giant step baby step algorithm, from Wikipaedea:

  input: A cyclic group G of order n, having a generator a and an element b.

  Output: A value x satisfying a^x = b (mod n).

  1. m <-- ceiling(sqrt(n))
  2. For all j where 0 <= j < m:
     1. Compute a^j mod n and store the pair (j, a^j) in a table.
  3. Compute a^(-m).
  4. g <-- b.
  5. For i = 0 to (m - 1):
     1. Check if g is the second component (a^j) of any pair in the table.
     2. If so, return im + j.
     3. If not, g <-- ga^(-m) mod n.
*/

/* Info for ARM version:
 * D64 and BJ64 are shifted by clzp-2 for speed improvements (no shift required
 * during mulmods computations). hash operations honors this rule.
 * Unfortunatelly this rule introduces limit p<=2^56
 */

static uint32_t m;            /* Current number of baby steps */
static uint32_t M;            /* Current number of giant steps */
static uint32_t sieve_low;    /* Start of sieve n range, n_min/Q */

#ifdef HAVE_lookup_ind
static void *lookup_ind_data;
#endif

#ifdef DUMP_ITER_STATE
static uint32_t dumps_count = 10000;
static FILE* dump_file = NULL;

static void dump_init(void)
{
  dump_file = fopen("/mnt/sdcard/dump.out","wb");
}

static void dump_fini(void)
{
  if (dump_file != NULL)
  {
    fclose(dump_file);
    dump_file = NULL;
  }
}
#endif

/* Use functions defined in hash-i386.S or hash-x86-64.S.
 */
#if SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && ARM_CPU
uint32_t build_hashtable_arm(uint32_t m);
#define build_hashtable build_hashtable_arm
#elif SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __i386__
//#if SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __i386__
uint32_t build_hashtable_i386(uint32_t m) attribute ((regparm(1)));
#define build_hashtable build_hashtable_i386
#elif SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __x86_64__
uint32_t build_hashtable_x86_64(uint32_t m);
#define build_hashtable build_hashtable_x86_64
#else
static uint32_t build_hashtable(uint32_t m)
{
  uint64_t bj0;
  uint32_t j;

  bj0 = BJ64[0];
  BJ64[m] = bj0;
  clear_hashtable(hsize);
  j = 0;
  do { post_insert64(j); }
  while (BJ64[++j] != bj0);

  return (j < m) ? j : 0;
}
#endif

#ifndef HAVE_powseq64
static int baby_vec_len;
static void
#if USE_ASM && defined(__i386__) && defined(__GNUC__)
__attribute__((regparm(3)))
#endif
     (*baby_vec_mulmod64)(const uint64_t *,uint64_t *,int);
#endif

static uint32_t baby_steps(uint64_t b, uint64_t p)
{
#ifdef DUMP_ITER_STATE
  uint32_t i,ret;
#if USE_ASM && ARM_CPU
  uint32_t clza;
  clza = mod64_init_data.clzp-2;
#endif
#endif
#ifdef HAVE_powseq64
  BJ64[0] = 1ULL<<(mod64_init_data.clzp-2);
  powseq64_shifted(BJ64, m, powmod64(b,subseq_Q,p));
#else
  /* b <- b^Q (mod p) */
  b = powmod64(b,subseq_Q,p);

  BJ64[0] = powmod64(b,sieve_low,p);
  BJ64[1] = mulmod64(BJ64[0],b,p);
  b = sqrmod64(b,p);
  if (baby_vec_len > 2)
  {
    vec_mulmod64_initb(b);
#ifdef HAVE_vector_sse2
    vec2_mulmod64_sse2(BJ64,BJ64+2,baby_vec_len-2);
#else
    vec2_mulmod64(BJ64,BJ64+2,baby_vec_len-2);
#endif
    vec_mulmod64_finib();
    b = powmod64(b,baby_vec_len/2,p);
  }

  /* Don't bother to check whether m > baby_vec_len. This could result in a
     total overrun of up to 2*baby_vec_len-1.
   */
  vec_mulmod64_initb(b);
  baby_vec_mulmod64(BJ64, BJ64+baby_vec_len, m-baby_vec_len);
  vec_mulmod64_finib();
#endif

#ifdef DUMP_ITER_STATE
  fprintf(dump_file,"BJ64:m=%u\n",m);
  for (i = 0; i < m; i++)
#if USE_ASM && ARM_CPU
    fprintf(dump_file,"  %u:%llu\n", i, BJ64[i]>>clza);
#else
    fprintf(dump_file,"  %u:%llu\n", i, BJ64[i]);
#endif
  
  ret = build_hashtable(m);
  
  fprintf(dump_file, "HashTable:hsize=%u\n", hsize);
    for (i = 0; i < hsize; i++)
      fprintf(dump_file,"  %u:%u,\n", i, htable[i]);
  
  return ret;
#else
  return build_hashtable(m);
#endif
}

static uint32_t *C32;         /* List of candidate subsequences for BSGS */
static uint64_t *D64;         /* D64[j] holds -c/(k*b^(im+d)) (mod p) for
                                 subsequence C[j] at giant step i. */
static uint32_t *F32;         /* Temp storage for seq_count elements */
static uint64_t *G64;         /* Temp storage for seq_count elements */

/* Use functions defined in hash-i386.S or hash-x86-64.S.
 */
#if !(USE_ASM && ARM_CPU)
#if SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __i386__
uint32_t search_hashtable_i386(const uint64_t *D64, uint32_t cc)
     attribute ((pure));
#define search_hashtable search_hashtable_i386
#elif SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __x86_64__
uint32_t search_hashtable_x86_64(const uint64_t *D64, uint32_t cc)
     attribute ((pure));
#define search_hashtable search_hashtable_x86_64
#else
static uint32_t search_hashtable(const uint64_t *D64, uint32_t cc)
{
  uint32_t k;

  for (k = 0; k < cc; k++)
    if (lookup64(D64[k]) != HASH_NOT_FOUND)
      break;

  return k;
}
#endif
#endif

#if SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && ARM_CPU
void search_elim_arm(const uint64_t *D64, uint32_t cc, uint32_t i,
        void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t), uint64_t p);
#define search_elim search_elim_arm
#define HAVE_search_elim
#endif

#if SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && ARM_CPU
#ifdef ARM_NEON
void giant4_neon(const uint64_t *D64, uint32_t cc, uint32_t M,
                   void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t),
                   uint64_t b, uint64_t p);
#define giant_steps giant4_neon
#else
void giant4_arm(const uint64_t *D64, uint32_t cc, uint32_t M,
                   void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t),
                   uint64_t b, uint64_t p);
#define giant_steps giant4_arm
#endif
#define HAVE_new_giant_steps
#elif SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __i386__ && 0
/* Not yet implemented for i386 */
void giant_i386(const uint64_t *D64, uint32_t cc, uint32_t M,
                void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t),
                uint64_t b, uint64_t p);
#define giant_steps giant_i386
#define HAVE_new_giant_steps
#elif SHORT_HASHTABLE && !CONST_EMPTY_SLOT && USE_ASM && __x86_64__
#if USE_FPU_MULMOD
void giant4_x87_64(const uint64_t *D64, uint32_t cc, uint32_t M,
                   void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t),
                   uint64_t b, uint64_t p);
#define giant_steps giant4_x87_64
#else
void giant4_x86_64(const uint64_t *D64, uint32_t cc, uint32_t M,
                   void (*fun)(uint32_t,uint32_t,uint32_t,uint64_t),
                   uint64_t b, uint64_t p);
#define giant_steps giant4_x86_64
#endif
#define HAVE_new_giant_steps
#endif

static void eliminate_1(uint32_t i, uint32_t j, uint32_t k, uint64_t p)
{
#ifdef DUMP_ITER_STATE
  fprintf(dump_file, "Eliminate: %u,%u,%u,%llu\n",i,j,k,p);
#endif
  eliminate_term(C32[k],sieve_low+i*m+j,p);
}

#if !(USE_ASM && ARM_CPU)
static void eliminate_hashtable_terms(uint32_t i, uint32_t cc, uint64_t p)
{
  uint32_t j, k;

  for (k = 0; k < cc; k++)
    if ((j = lookup64(D64[k])) != HASH_NOT_FOUND)
      eliminate_1(i,j,k,p);
}
#endif

#ifndef HAVE_new_giant_steps
static void
#if USE_ASM && defined(__i386__) && defined(__GNUC__)
__attribute__((regparm(3)))
#endif
     (*giant_vec_mulmod64)(const uint64_t *,uint64_t *,int);

static void giant_steps(uint32_t cc, uint64_t b, uint64_t p)
{
  uint32_t i;

  assert (M > 1);
  assert (cc > 0);

  vec_mulmod64_initb(b);
  i = 1;
  do
  {
    giant_vec_mulmod64(D64,D64,cc);
    if (search_hashtable(D64,cc) < cc)
      eliminate_hashtable_terms(i,cc,p);
  } while (++i < M);
  vec_mulmod64_finib();
}
#endif

#ifndef HAVE_powseq64
static int ladder_vec_len;
static void
#if USE_ASM && defined(__i386__) && defined(__GNUC__)
__attribute__((regparm(3)))
#endif
     (*ladder_vec_mulmod64)(const uint64_t *,uint64_t *,int);
#endif

static uint64_t attribute ((noinline))
climb_ladder_gen(uint64_t *X, uint64_t b, uint64_t p)
{
#ifdef DUMP_ITER_STATE
  uint32_t i;
#if USE_ASM && ARM_CPU
  uint32_t clza;
  clza = mod64_init_data.clzp-2;
#endif
#endif
#ifdef HAVE_powseq64
  X[0] = 1ULL<<(mod64_init_data.clzp-2);
  powseq64_shifted(X, subseq_Q+1, b);
#else
  X[0] = 1;
  X[1] = b;
  X[2] = sqrmod64(b,p);
  if (ladder_vec_len > 2)
  {
    vec_mulmod64_initb(X[2]);
#ifdef HAVE_vector_sse2
    vec2_mulmod64_sse2(X+1,X+3,ladder_vec_len-2);
#else
    vec2_mulmod64(X+1,X+3,ladder_vec_len-2);
#endif
    vec_mulmod64_finib();
  }

  if (subseq_Q > ladder_vec_len)
  {
    vec_mulmod64_initb(X[ladder_vec_len]);
    ladder_vec_mulmod64(X+1, X+1+ladder_vec_len, subseq_Q-ladder_vec_len);
    vec_mulmod64_finib();
  }
#endif
#ifdef DUMP_ITER_STATE
  fprintf(dump_file,"ClimbLadder: %u\n",subseq_Q);
  for (i = 0; i <= subseq_Q; i++)
#if USE_ASM && ARM_CPU
    fprintf(dump_file,"  %u: %llu\n", i, X[i]>>clza);
#else
    fprintf(dump_file,"  %u: %llu\n", i, X[i]);
#endif
#endif
  return X[subseq_Q];
}

static uint32_t *setup_ladder;

#if !(USE_ASM && ARM_CPU)
static uint64_t attribute ((noinline))
 climb_ladder_add_1(uint64_t *X, uint64_t b, uint64_t p)
{
  const uint32_t *ladder;
  uint32_t i;

  assert (setup_ladder != NULL);

  X[0] = 1;
  X[1] = b;
  X[2] = sqrmod64(b,p);
  for (i = 2, ladder = setup_ladder; *ladder > 0; i += *ladder++)
    X[i+*ladder] = mulmod64(X[i],X[*ladder],p);

  return X[subseq_Q];
}
#endif

static uint64_t (*climb_ladder)(uint64_t *,uint64_t,uint64_t);

/* BD64 is a temporary array of Q+1 elements. For SSE2 operations BD64[1]
   must be at least 16-aligned, but 64-aligned is better.
*/
#define BD64 (BJ64+((POWER_RESIDUE_LCM+1)|7))

static unsigned char *sym = NULL;

static uint32_t get_symbols(uint64_t p)
{
  uint32_t i, j;
#if NO_LOOKUP_OPT
  int b_sym = 0, kc_sym;
#endif

#if NO_LOOKUP_OPT
  if (no_lookup_opt)
  {
# if (BASE==0)
    b_sym = legendre32_64(b_term,p);
# else
    b_sym = legendre32_64(BASE,p);
# endif
  }
  else
#endif /* NO_LOOKUP_OPT */
  {
#ifdef HAVE_lookup_ind
    gen_lookup_ind(F32,lookup_ind_data,seq_count,p/2);
#else
    for (i = 0; i < seq_count; i++)
      F32[i] = (p/2)%SEQ[i].mod;
#endif /* HAVE_lookup_ind */
  }

#if NO_LOOKUP_OPT
  if (no_lookup_opt)
    for (i = j = 0; i < seq_count; i++)
    {
      kc_sym = legendre32_64(SEQ[i].kc_core,p);
      switch (SEQ[i].parity)
      {
        default:
        case 0: /* even n */
          sym[i] = (kc_sym == 1);
          break;
        case 1: /* odd n */
          sym[i] = (kc_sym == b_sym);
          break;
#if ALLOW_MIXED_PARITY_SEQUENCES
        case 2: /* even and odd n */
          sym[i] = (kc_sym == 1 || kc_sym == b_sym);
          break;
#endif
      }
      j += sym[i];
    }
  else
#endif /* NO_LOOKUP_OPT */
    for (i = j = 0; i < seq_count; i++)
    {
      sym[i] = (test_bit(SEQ[i].map,F32[i]) != 0);  /* (-ckb^n/p) == +1 */
      j += sym[i];
    }

  return j;
}

#if USE_ASM && ARM_CPU
uint32_t setup1_mulmod_arm_shifted(uint32_t i,
        uint32_t* C32, uint64_t* D64, uint64_t p);
#define setup1_mulmod_shifted setup1_mulmod_arm_shifted

uint32_t setup2_mulmod_arm_shifted(uint64_t powval, uint64_t p);
#define setup2_mulmod_shifted setup2_mulmod_arm_shifted

uint32_t setup3_mulmod_arm_shifted(uint32_t f, const uint32_t* list,
        uint32_t* C32, uint64_t* D64, uint64_t p);
#define setup3_mulmod_shifted setup3_mulmod_arm_shifted
#define HAVE_setup64_shifted
#endif

/* This function builds the list C32[] of subsequences (k*b^d)*(b^Q)^m+c for
   which p may be a factor (-ckb^d is a quadratic/cubic/quartic/quintic
   residue with respect to p) and initialises the table D64[] with the
   values -c/(k*b^d) (mod p). Returns the number of subsequences in C32[].
*/
static uint32_t attribute ((noinline))
 setup64(uint64_t inv_b, uint64_t p)
{
#ifdef DUMP_ITER_STATE
  uint32_t di;
#endif
#ifdef HAVE_powmod64_shifted
  uint64_t p_s;
#else
  uint64_t neg_ck, p_s;
#endif
  uint32_t div_ind;
  const uint32_t *list;
  uint32_t r, f, g, h, i, j;
  int16_t s;
#ifdef HAVE_powmod64_shifted
  uint32_t clza;
  clza = mod64_init_data.clzp-2;
#endif

#if SKIP_CUBIC_OPT
  if (skip_cubic_opt)
    s = 0;
  else
#endif
  s = div_shift[(p/2)%(POWER_RESIDUE_LCM/2)];

  if (s == 0)
  {
    /* p = 1 (mod 2) is all we know, check for quadratic residues only.
     */
    for (i = j = 0; i < seq_count; i++)
    {
      if (sym[i])  /* (-ckb^n/p) == +1 */
      {
        /* For each subsequence (k*b^d)*(b^Q)^(n/Q)+c, compute
           -c/(k*b^d) (mod p) and add the subsequence to the bsgs list.
        */
#ifdef HAVE_setup64_shifted
        j += setup1_mulmod_shifted(i, C32+j, D64+j, p);
#else
        neg_ck = (SEQ[i].c < 0) ? SEQ[i].k : p - SEQ[i].k;
        PRE2_MULMOD64_INIT(neg_ck);
        for (h = SEQ[i].first; h <= SEQ[i].last; h++)
        {
          C32[j] = h;
          D64[j] = PRE2_MULMOD64(BD64[subseq_d[h]],neg_ck,p); /* -c/(k*b^d) */
          j++;
        }
        PRE2_MULMOD64_FINI();
#endif
      }
    }
#ifdef DUMP_ITER_STATE
    fprintf(dump_file,"D64 S1: %u\n", j);
    for (di = 0; di < j; di++)
#if USE_ASM && ARM_CPU
      fprintf(dump_file,"  %u: %u,%llu\n", di, C32[di], D64[di]>>clza);
#else
      fprintf(dump_file,"  %u: %u,%llu\n", di, C32[di], D64[di]);
#endif
#endif
    return j;
  }
  else if (s > 0)
  {
    /* p = 1 (mod s), where s is not a power of 2. Check for r-th power
       residues for each prime power divisor r of s.
    */
    p_s = p/s;
  }
  else /* s < 0 */
  {
    /* p = 1 (mod 2^s), where s > 1. Check for r-th power residues for each
       divisor r of s. We handle this case seperately to avoid computing p/s
       using plain division.
    */
    p_s = p >> (-s);
#ifndef NDEBUG
    s = 1 << (-s);
#endif
  }

  /* For 0 <= r < s, BJ64[r] <- 1/(b^r)^((p-1)/s) */
#ifdef HAVE_setup64_shifted
#ifdef HAVE_powmod64_shifted
  r = setup2_mulmod_shifted(powmod64_shifted(inv_b<<clza,p_s,p)>>clza, p);
#else
  setup2_mulmod_shifted(powmod64(inv_b,p_s,p), p);
#endif
#else
  BJ64[0] = 1;
  BJ64[1] = powmod64(inv_b,p_s,p);
  PRE2_MULMOD64_INIT(BJ64[1]);
  for (r = 1; BJ64[r] != 1; r++)
    BJ64[r+1] = PRE2_MULMOD64(BJ64[r],BJ64[1],p);
  PRE2_MULMOD64_FINI();
#endif
#ifdef DUMP_ITER_STATE
  fprintf(dump_file,"BJ64 S2: %u\n", r);
  for (di = 0; di <= r; di++)
#if USE_ASM && ARM_CPU
    fprintf(dump_file,"  %u: %llu\n", di, BJ64[di]>>clza);
#else
    fprintf(dump_file,"  %u: %llu\n", di, BJ64[di]);
#endif
#endif
  assert(s%r == 0);
  /* 1/(b^r)^((p-1)/s)=1 (mod p) therefore (1/(b^r)^((p-1)/s))^y=1 (mod p)
     for 0 <= y < s/r. (Could we do more with this?)
  */
  div_ind = divisor_index[r];

#if USE_SETUP_HASHTABLE
  if (r > SMALL_HASH_THRESHOLD)
  {
    clear_hashtable(SMALL_HASH_SIZE);
    for (j = 0; j < r; j++)
      insert64_small(j);
  }
#endif

  for (i = g = 0; i < seq_count; i++)
  {
    if (sym[i])  /* (-ckb^n/p) == +1 */
    {
      G64[g] = (SEQ[i].c < 0) ? SEQ[i].k : p - SEQ[i].k; /* -k/c (mod p) */
#ifdef HAVE_powmod64_shifted
      G64[g] <<= clza;
#endif
      F32[g] = i;
      g++;
    }
  }

#ifdef HAVE_powmod64_shifted
  vec_powmod64_shifted(G64,g,p_s,p);
#else
#ifdef HAVE_vec_powmod64
  if (g > 1)
    vec_powmod64(G64,g,p_s,p);
  else
#endif
    for (i = 0; i < g; i++)
      G64[i] = powmod64(G64[i],p_s,p);
#endif

  for (i = j = 0; i < g; i++)
  {
    /* BJ64[r] <-- (-k/c)^((p-1)/s) */
    f = F32[i];
    BJ64[r] = G64[i];

    /* Find h such that BJ64[h]=BJ64[r], i.e. (-ckb^h)^((p-1)/r)=1 (mod p),
       or h >= r if not found. */
#if USE_SETUP_HASHTABLE
    if (r > SMALL_HASH_THRESHOLD)
      h = lookup64_small(BJ64[r]);
    else
#endif
    { /* Linear search */
      for (h = 0; BJ64[h] != BJ64[r]; h++)
        ;
    }
    if (h < r && (list = SCL[f].sc_lists[div_ind][h]) != NULL)
    {
      /* -c/(k*b^n) is an r-power residue for at least one term k*b^n+c
         of this sequence.
      */
#ifdef HAVE_setup64_shifted
      j += setup3_mulmod_shifted(f, list, C32+j, D64+j, p);
#else
      neg_ck = (SEQ[f].c < 0) ? SEQ[f].k : p - SEQ[f].k;
      PRE2_MULMOD64_INIT(neg_ck);
      while ((h = *list++) < SUBSEQ_MAX)
      {
        /* -ckb^d is an r-th power residue for at least one term
           (k*b^d)*(b^Q)^(n/Q)+c of this subsequence.
        */
        C32[j] = h;
        D64[j] = PRE2_MULMOD64(BD64[subseq_d[h]],neg_ck,p); /* -c/(k*b^d) */
        j++;
      }
      PRE2_MULMOD64_FINI();
#endif
    }
  }
#ifdef DUMP_ITER_STATE
  fprintf(dump_file,"D64 S3: %u\n", j);
  for (di = 0; di < j; di++)
#if USE_ASM && ARM_CPU
    fprintf(dump_file,"  %u: %u,%llu\n", di, C32[di], D64[di]>>clza);
#else
    fprintf(dump_file,"  %u: %u,%llu\n", di, C32[di], D64[di]);
#endif
#endif

  return j;
}

static void bsgs64(uint64_t p)
{
  uint32_t i, j, k, cc;
  uint64_t b, inv_b;
  uint64_t bQ;
#ifdef HAVE_powmod64_shifted
  uint32_t clza;
#endif

#ifdef DUMP_ITER_STATE
  uint32_t di;
  fprintf(dump_file,"p=%llu\n",p);
#endif

#ifdef HAVE_powseq64
  mod64_init(p);
#else
  vec_mulmod64_initp(p);
#endif

#ifdef HAVE_powmod64_shifted
  clza = mod64_init_data.clzp-2;
#endif
  
  if (get_symbols(p) > 0)
  {
    /* inv_b <-- 1/base (mod p) */
#if (BASE == 2)
    inv_b = (p+1)/2;
#elif (BASE == 0)
    inv_b = (b_term == 2) ? (p+1)/2 : invmod32_64(b_term,-1,p);
#else
    inv_b = invmod32_64(BASE,-1,p);
#endif

  /* Swap b and inv_b for dual sieve */
#if DUAL
    if (dual_opt)
    {
      b = inv_b;
# if (BASE == 0)
      inv_b = b_term;
# else
      inv_b = BASE;
# endif
    }
    else
#endif
# if (BASE == 0)
      b = b_term;
# else
      b = BASE;
# endif

    bQ = climb_ladder(BD64,b,p); /* bQ <-- b^Q. */

    if ((cc = setup64(inv_b,p)) > 0)
    {
      m = steps[cc].m;
      M = steps[cc].M;

      /* Baby steps. */
      if ((i = baby_steps(inv_b,p)) > 0) /* Unlikely */
      {
        /* i is the order of b (mod p). This is all the information we need to
           determine every solution for this p, so no giant steps are needed.
        */
        for (k = 0; k < cc; k++)
          for (j = lookup64(D64[k]); j < m*M; j += i)
            eliminate_term(C32[k],sieve_low+j,p);
      }
      else
      {
        /* First giant step. */
#ifdef HAVE_search_elim
        search_elim(D64,cc, 0, eliminate_1,p);
#else
        if (search_hashtable(D64,cc) < cc)
          eliminate_hashtable_terms(0,cc,p);
#endif

        /* Remaining giant steps. */
        if (M > 1)
#ifdef HAVE_new_giant_steps
#ifdef HAVE_powmod64_shifted
          giant_steps(D64,cc,M,eliminate_1,powmod64_shifted(bQ,m,p)>>clza,p);
#else
          giant_steps(D64,cc,M,eliminate_1,powmod64(bQ,m,p),p);
#endif
#else
          giant_steps(cc,powmod64(bQ,m,p),p);
#endif
#ifdef DUMP_ITER_STATE
          fprintf(dump_file,"D64out: %u\n", cc);
          for (di = 0; di < cc; di++)
            fprintf(dump_file,"  %u, %llu\n", di, D64[di]>>clza);
#endif
      }
    }
  }

  vec_mulmod64_finip();
#ifdef DUMP_ITER_STATE
  if (--dumps_count == 0)
  {
    puts("Dump finished");
    dump_fini();
    exit(0);
  }
#endif
}


/* Choose which mulmod scheme to use for bsgs.
 */
#if !(USE_ASM && ARM_CPU)
typedef struct {
#if USE_ASM && defined(__i386__) && defined(__GNUC__)
  void __attribute__((regparm(3))) (*fun)(const uint64_t *,uint64_t *,int);
#else
  void (*fun)(const uint64_t *,uint64_t *,int);
#endif
  const char *desc;
  int vec_len;
} vec_fun_t;

static const vec_fun_t vec_funs[] = {
#ifdef HAVE_vector_sse2
  { vec2_mulmod64_sse2, "sse2/2", 2 },
  { vec4_mulmod64_sse2, "sse2/4", 4 },
  { vec8_mulmod64_sse2, "sse2/8", 8 },
  { vec16_mulmod64_sse2, "sse2/16", 16 },
#endif
  { vec2_mulmod64, "gen/2", 2 },
  { vec4_mulmod64, "gen/4", 4 },
#if USE_ASM && defined(__x86_64__) && defined(__GNUC__)
  { vec6_mulmod64, "gen/6", 6 },
#endif
  { vec8_mulmod64, "gen/8", 8 }
};

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

#define NUM_VEC_FUN sizeof(vec_funs)/sizeof(vec_fun_t)
static int choose_baby_method(int print)
{
  uint64_t t0, t1, best[NUM_VEC_FUN];
  uint32_t cc;
  int i, j;

  /* How many subsequences pass the power residue tests? Probably 1/4 or
     less pass, but the speed of the bsgs routine is more important when a
     larger number pass since then BSGS accounts for a larger proportion of
     the total work done. Benchmark assuming 1/2 have passed.
  */
  cc = (subseq_count+1)/2;
  m = steps[cc].m;
  for (i = 0; i < NUM_VEC_FUN; i++)
  {
    best[i] = UINT64_MAX;
    baby_vec_len = vec_funs[i].vec_len;
    baby_vec_mulmod64 = vec_funs[i].fun;
    for (j = 0; j < NUM_TEST_PRIMES; j++)
    {
      uint64_t inv_b, p;

      p = test_primes[j];
#if (BASE == 0)
      inv_b = b_term;
#else
      inv_b = BASE;
#endif

      vec_mulmod64_initp(p);
      t0 = timestamp();
      baby_steps(inv_b,p);
      t1 = timestamp();
      best[i] = MIN(best[i],t1-t0);
      vec_mulmod64_finip();
    }
  }

  if (print >= 2)
    for (i = 0; i < NUM_VEC_FUN; i++)
      report(1,"Best time for baby step method %s: %"PRIu64".",
             vec_funs[i].desc,best[i]);

#if USE_ASM && defined(__x86_64__) && defined(__GNUC__)
  /* Prevent gen/6 method being selected unless running on AMD. */
  if (!is_amd())
    for (i = 0; i < NUM_VEC_FUN; i++)
      if (vec_funs[i].vec_len == 6)
        best[i] = UINT64_MAX;
#endif

  for (i = 1, j = 0; i < NUM_VEC_FUN; i++)
    if (best[i] < best[j])
      j = i;

  return j;
}

#ifndef HAVE_new_giant_steps
static int choose_giant_method(int print)
{
  uint64_t t0, t1, best[NUM_VEC_FUN];
  uint32_t cc;
  int i, j;

  cc = (subseq_count+1)/2;
  m = steps[cc].m;
  M = MAX(2,steps[cc].M);

  mod64_init(test_primes[0]);
  D64[0] = 23;
  for (i = 1; i < cc; i++)
    D64[i] = mulmod64(D64[i],D64[0],test_primes[0]);
  mod64_fini();

  for (i = 0; i < NUM_VEC_FUN; i++)
  {
    best[i] = UINT64_MAX;
    giant_vec_mulmod64 = vec_funs[i].fun;
    for (j = 0; j < NUM_TEST_PRIMES; j++)
    {
      uint64_t inv_b, p;

      p = test_primes[j];
#if (BASE == 0)
      inv_b = b_term;
#else
      inv_b = BASE;
#endif
      vec_mulmod64_initp(p);
      baby_steps(inv_b,p);
      t0 = timestamp();
      giant_steps(cc,inv_b,p);
      t1 = timestamp();
      best[i] = MIN(best[i],t1-t0);
      vec_mulmod64_finip();
    }
  }

  if (print >= 2)
    for (i = 0; i < NUM_VEC_FUN; i++)
      report(1,"Best time for giant step method %s: %"PRIu64".",
             vec_funs[i].desc,best[i]);

#if USE_ASM && defined(__x86_64__) && defined(__GNUC__)
  /* Prevent gen/6 method being selected unless running on AMD. */
  if (!is_amd())
    for (i = 0; i < NUM_VEC_FUN; i++)
      if (vec_funs[i].vec_len == 6)
        best[i] = UINT64_MAX;
#endif

  for (i = 1, j = 0; i < NUM_VEC_FUN; i++)
    if (best[i] < best[j])
      j = i;

  return j;
}
#endif

static int choose_ladder_method(int print)
{
  uint64_t t0, t1, best[NUM_VEC_FUN+1];
  int i, j;

  for (i = 0; i < NUM_VEC_FUN+1; i++)
    best[i] = UINT64_MAX;

  for (j = 0; j < NUM_TEST_PRIMES; j++)
  {
    vec_mulmod64_initp(test_primes[j]);
    for (i = 0; i < NUM_VEC_FUN; i++)
    {
      ladder_vec_len = vec_funs[i].vec_len;
      ladder_vec_mulmod64 = vec_funs[i].fun;
      t0 = timestamp();
      climb_ladder_gen(BD64,(test_primes[j]+1)/2,test_primes[j]);
      t1 = timestamp();
      best[i] = MIN(best[i],t1-t0);
    }

    t0 = timestamp();
    climb_ladder_add_1(BD64,(test_primes[j]+1)/2,test_primes[j]);
    t1 = timestamp();
    best[NUM_VEC_FUN] = MIN(best[NUM_VEC_FUN],t1-t0);

    vec_mulmod64_finip();
  }

  if (print >= 2)
  {
    for (i = 0; i < NUM_VEC_FUN; i++)
      report(1,"Best time for ladder method %s: %"PRIu64".",
             vec_funs[i].desc,best[i]);
   report(1,"Best time for ladder method add/1: %"PRIu64".",best[NUM_VEC_FUN]);
  }

#if USE_ASM && defined(__x86_64__) && defined(__GNUC__)
  /* Prevent gen/6 method being selected unless running on AMD. */
  if (!is_amd())
    for (i = 0; i < NUM_VEC_FUN; i++)
      if (vec_funs[i].vec_len == 6)
        best[i] = UINT64_MAX;
#endif

  for (i = 1, j = 0; i < NUM_VEC_FUN+1; i++)
    if (best[i] < best[j])
      j = i;

  return j;
}

/* Set baby_steps, giant_steps, climb_ladder functions.
   Return baby step vector size.
*/
static void choose_bsgs_methods(int print)
{
  benchmarking = 1; /* Don't check or report any factors. */

  int baby_method;
  if (baby_method_opt == NULL)
    baby_method = choose_baby_method(print);
  else
  {
    for (baby_method = 0; baby_method < NUM_VEC_FUN; baby_method++)
      if (strcmp(baby_method_opt,vec_funs[baby_method].desc)==0)
        break;
    if (baby_method >= NUM_VEC_FUN)
      error("Unknown baby step method `%s'.",baby_method_opt);
  }
  baby_vec_len = vec_funs[baby_method].vec_len;
  baby_vec_mulmod64 = vec_funs[baby_method].fun;

#ifndef HAVE_new_giant_steps
  int giant_method;
  if (giant_method_opt == NULL)
    giant_method = choose_giant_method(print);
  else
  {
    for (giant_method = 0; giant_method < NUM_VEC_FUN; giant_method++)
      if (strcmp(giant_method_opt,vec_funs[giant_method].desc)==0)
        break;
    if (giant_method >= NUM_VEC_FUN)
      error("Unknown giant step method `%s'.",giant_method_opt);
  }
  giant_vec_mulmod64 = vec_funs[giant_method].fun;
#endif

  int ladder_method;
  setup_ladder = make_setup_ladder(1.0);
  if (ladder_method_opt == NULL)
    ladder_method = choose_ladder_method(print);
  else if (strcmp(ladder_method_opt,"add/1")==0)
    ladder_method = NUM_VEC_FUN;
  else
  {
    for (ladder_method = 0; ladder_method < NUM_VEC_FUN; ladder_method++)
      if (strcmp(ladder_method_opt,vec_funs[ladder_method].desc)==0)
        break;
    if (ladder_method >= NUM_VEC_FUN)
      error("Unknown ladder method `%s'.",ladder_method_opt);
  }
  if (ladder_method < NUM_VEC_FUN)
  {
    climb_ladder = climb_ladder_gen;
    ladder_vec_len = vec_funs[ladder_method].vec_len;
    ladder_vec_mulmod64 = vec_funs[ladder_method].fun;
  }
  else /* ladder_method == NUM_VEC_FUN */
  {
    climb_ladder = climb_ladder_add_1;
  }

#ifndef HAVE_new_giant_steps
  if (print)
    report(1,"Baby step method %s, giant step method %s, ladder method %s.",
           vec_funs[baby_method].desc,vec_funs[giant_method].desc,
           (ladder_method<NUM_VEC_FUN)? vec_funs[ladder_method].desc:"add/1");
#else
  if (print)
    report(1,"Baby step method %s, giant step method new/4, ladder method %s.",
           vec_funs[baby_method].desc, (ladder_method<NUM_VEC_FUN)?
           vec_funs[ladder_method].desc : "add/1");
#endif

  benchmarking = 0;
}
#else
static void choose_bsgs_methods(int print)
{
  climb_ladder = climb_ladder_gen;
}
#endif // !(USE_ASM&&ARM_CPU)


/* Once only initialization.
 */
static int init_done = 0;

static void init_sieve(void)
{
  uint32_t max_baby_steps;

  sieve_low = n_min / subseq_Q;

#ifdef DUMP_ITER_STATE
  dump_init();
#endif
  
  /* Allow room for vector operations to overrun the end of the array.
     For SSE2 operations D64 and G64 must be at least 16-aligned, but
     64-aligned is better.
  */
  D64 = xmemalign(64,(subseq_count+15)*sizeof(uint64_t));
  G64 = D64+((subseq_count-seq_count+7)&-8); /* 64-aligned */
  memset(D64,0,(subseq_count+15)*sizeof(uint64_t));
#ifdef HAVE_lookup_ind
  C32 = xmemalign(16,(subseq_count+2*LI_VECTOR_LEN-1)*sizeof(uint32_t));
  F32 = C32+((subseq_count-seq_count+LI_VECTOR_LEN-1)&-LI_VECTOR_LEN);
#else
  C32 = xmalloc(subseq_count*sizeof(uint32_t));
  F32 = C32+(subseq_count-seq_count);
#endif /* HAVE_lookup_ind */
  sym = xmalloc(seq_count);

  max_baby_steps = make_table_of_steps();
  init_hashtable(max_baby_steps);

#ifdef HAVE_lookup_ind
#if NO_LOOKUP_OPT
  if (!no_lookup_opt)
#endif
  {
    uint32_t i;

    lookup_ind_data = xmemalign(64,LI_STRUCT_SIZE*
                                ((seq_count+LI_VECTOR_LEN-1)&-LI_VECTOR_LEN));

    for (i = 0; i < seq_count; i++)
      F32[i] = SEQ[i].mod;
    for ( ; i < ((seq_count+LI_VECTOR_LEN-1)&-LI_VECTOR_LEN); i++)
      F32[i] = 1;

    init_lookup_ind(lookup_ind_data,F32,seq_count);
  }
#endif /* HAVE_lookup_ind */

#if !(USE_ASM && ARM_CPU)
  choose_bsgs_methods(0); /* Dummy run to prep cache. */
  choose_bsgs_methods(verbose_opt);
#else
  choose_bsgs_methods(0);
#endif

  /* Now that baby step method vector length has been chosen we can trim the
     step sizes to suit.
  */
#ifdef HAVE_powmod64_shifted
  trim_steps(8,max_baby_steps);
#else
  trim_steps(baby_vec_len,max_baby_steps);
#endif

  if (verbose_opt >= 2)
   report(1,"BSGS range: %"PRIu32"*%"PRIu32" - %"PRIu32"*%"PRIu32".",
          steps[1].m,steps[1].M,steps[subseq_count].m,steps[subseq_count].M);

  init_done = 1;
}

void CODE_PATH_NAME(fini_sieve)(void)
{
  if (init_done)
  {
    if (setup_ladder != NULL)
    {
      free(setup_ladder);
      setup_ladder = NULL;
    }

    xfreealign(D64);
#ifdef HAVE_lookup_ind
    xfreealign(C32);
#else
    free(C32);
#endif
    free(sym);
    free_table_of_steps();
    fini_hashtable();

#ifdef HAVE_lookup_ind
#if NO_LOOKUP_OPT
  if (!no_lookup_opt)
#endif
    xfreealign(lookup_ind_data);
#endif /* HAVE_lookup_ind */

    init_done = 0;
#ifdef DUMP_ITER_STATE
    dump_fini();
#endif
  }
}

void CODE_PATH_NAME(sieve)(void)
{
  if (!init_done)
    init_sieve();

#if HAVE_FORK
  /* Start children before initializing the Sieve of Eratosthenes, to
     minimise forked image size. */
  if (num_children > 0)
    init_threads(p_min,bsgs64);
#endif
  init_prime_sieve(p_max);
  start_srsieve();

#if HAVE_FORK
  if (num_children > 0)
    prime_sieve(p_min,p_max,parent_thread);
  else
#endif
    prime_sieve(p_min,p_max,bsgs64);

#if HAVE_FORK
  /* Check that all children terminated normally before assuming that
     the range is complete. */
  if (num_children > 0 && fini_threads(p_max))
    finish_srsieve("range is incomplete",get_lowtide());
  else
#endif
    finish_srsieve("range is complete",p_max);
  fini_prime_sieve();
}
