/* hashtable.c -- (C) Geoffrey Reynolds, April 2006.

   Hash table implementation of the baby steps giant steps lookup table.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "sr5sieve.h"
#include "hashtable.h"

HASH_T *htable;
HASH_T *olist;
uint32_t hsize;
uint32_t hsize_minus1;
uint64_t *BJ64;
#if !CONST_EMPTY_SLOT
HASH_T empty_slot;
#endif

#if USE_ASM && (__i386__ || __x86_64__) && SHORT_HASHTABLE
#define HASH_DATA 1
#else
#define HASH_DATA 0
#endif

/* Return the optimal size for a hashtable containing up to elts elements.
   This is just guesswork. There are other things to consider besides L1
   cache size, such as the relative speed of L2 cache. On my Coppermine P3
   it can be better to use a low-density hashtable in L2 cache than a high
   density hashtable in L1 cache. On my P2 (which has an external L2 cache)
   the hashtable must fit in L1 cache to get best performance.

   TODO: It can sometimes be better to reduce hashtable density by limiting
   the number of baby-steps rather than just increasing hashtable size.
 */
static uint32_t choose_hsize(uint32_t elts)
{
  uint32_t size;

  elts = MAX(elts,HASH_MINIMUM_ELTS);

  for (size = 1<<HASH_MINIMUM_SHIFT; size*HASH_MAX_DENSITY < elts; )
    size *= 2;

#ifdef ANDROID
  if (elts >= L1_cache_size*512 && elts < L1_cache_size*1024)
    size >>= 1;
#endif
  if (size*sizeof(HASH_T) < L1_cache_size*1024 && size <= elts/HASH_MIN_DENSITY/2)
    size *= 2; /* Double hashtable size if it will fit in L1 cache */
  if (size*sizeof(HASH_T)*2 < L1_cache_size*1024 && size <= elts/HASH_MIN_DENSITY/2)
    size *= 2; /* Double again if it will fit in half L1 cache */

#if USE_SETUP_HASHTABLE
  size = MAX(size,SMALL_HASH_SIZE);
#endif
  size = MIN(size,HASH_MASK1); /* Don't exceed maximum size for HASH_T */

  return size;
}

void init_hashtable(uint32_t M)
{
  uint32_t bj_len;

  assert(hsize == 0);
  assert(M <= HASH_MAX_ELTS);

#if HASHTABLE_SIZE_OPT
  if (hashtable_size > 0)
  {
    hsize = hashtable_size/sizeof(HASH_T);
    if (hsize > HASH_MASK1)
      error("Hashtable size %"PRIu32"Kb is too large"
#if SHORT_HASHTABLE
            ", recompile with SHORT_HASHTABLE=0"
#endif
            , hashtable_size/1024);
  }
  else
#endif
    hsize = choose_hsize(M);
  hsize_minus1 = hsize - 1;

  htable = xmalloc(hsize*sizeof(HASH_T));
#if USE_SETUP_HASHTABLE
  M = MAX(M,POWER_RESIDUE_LCM);
#endif
  olist = xmalloc(M*sizeof(HASH_T));

  /* BJ64 is used in setup64() as scratch storage for subseq_Q+1 +
     POWER_RESIDUE_LCM+8 elements. */
#if (!CONST_EMPTY_SLOT)
  /* We need an extra element to represent the empty object. */
  bj_len = MAX(M+1,subseq_Q+1+POWER_RESIDUE_LCM+8);
#else
  bj_len = MAX(M,subseq_Q+1+POWER_RESIDUE_LCM+8);
#endif

  /* Allow room for vector operations to overrun the end of the array. */
  bj_len += 15;

#if HASH_DATA
  /* Reserve an extra 64 bytes below the array. */
  BJ64 = xmemalign(64,(bj_len+8)*sizeof(uint64_t));
  BJ64 += 8;
#else
  BJ64 = xmemalign(64,bj_len*sizeof(uint64_t));
#endif

#if !CONST_EMPTY_SLOT
  /* The j values are all in the range 0 <= j < M, and j=M may be used in
     the baby steps as an end-of-array marker, so we can use M+1 as an
     empty slot marker as long as we fill BJ[M+1] with a value that will
     never match a real b^j value. Since b^j is always in the range
     0 <= b^j < p for some prime p, any value larger than all 64 bit primes
     will do.
  */
  empty_slot = M+1;
  /* Since we use the BJ64[] array for temporary storage when the hashtable
     is not being used, the empty slot object needs to be initialised by
     clear_hashtable() in case it was overwritten. */
  /* BJ64[empty_slot] = UINT64_MAX; */
#endif

#if HASH_DATA
  /* make a copy of the global variables so that they can all be indexed
     from the same pointer. */
#if __i386__
  ((uint32_t *)BJ64)[-1] = (uint32_t)htable;
  ((uint32_t *)BJ64)[-2] = (uint32_t)olist;
  ((uint32_t *)BJ64)[-3] = (uint32_t)hsize/2;
  ((uint32_t *)BJ64)[-4] = (uint32_t)hsize_minus1;
#if !CONST_EMPTY_SLOT
  ((uint32_t *)BJ64)[-5] = ((uint32_t)empty_slot << 16) | empty_slot;
#endif
#elif  __x86_64__
  BJ64[-1] = (uint64_t)htable;
  BJ64[-2] = (uint64_t)olist;
  BJ64[-3] = (uint64_t)hsize/4;
  BJ64[-4] = (uint64_t)hsize_minus1;
#if !CONST_EMPTY_SLOT
  uint64_t pattern = ((uint32_t)empty_slot << 16) | empty_slot;
  pattern = (pattern << 32) | pattern;
  BJ64[-5] = pattern;
#endif
#endif
#endif /* HASH_DATA */

  if (verbose_opt)
    report(1,"Using %"PRIu32"Kb for the baby-steps giant-steps hashtable, "
           "maximum density %.2f.",
           hsize*(uint32_t)sizeof(HASH_T)/1024,(double)M/hsize);
}

void fini_hashtable(void)
{
  assert (hsize > 0);

#if HASH_DATA
  xfreealign(BJ64-8);
#else
  xfreealign(BJ64);
#endif
  free(olist);
  free(htable);
#ifndef NDEBUG
  hsize = 0;
#endif
}
