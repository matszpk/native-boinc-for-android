/* hashtable.h -- (C) Geoffrey Reynolds, April 2006.

   Hash table implementation of the baby steps giant steps lookup table.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdint.h>
#include "config.h"
#include "arithmetic.h"
#include "memset_fast32.h"

/*
  The interface:
    init_hashtable(M) :- Allocate resources for a maximum of M elements.
    clear_hashtable() :- Delete all elements. Call before first use.
    insert32/64(j,bj) :- Insert an element (j,bj), bj is a 32/64 bit value.
    lookup32/64(bj)   :- Return j if (j,bj) is in the hashtable, bj is 32/64
                         bits. Return HASH_NOT_FOUND otherwise.
    fini_hashtable()  :- Free all resources ready for a new call to init.

  The following assumptions are valid for the current bsgs implementation:
    1. Every element (j,bj) is such that both j and bj are unique,
       0 <= j < M, and 0 <= bj < p for some 32/64 bit prime p.
    2. All insertions will be completed before the first lookup is made.
*/


/* Set SHORT_HASHTABLE to limit hashtable elements to 0 <= j < 2^15-1, which
   allows the following information to fit in a short (uint16_t):
   bit 15 clear: single element j in bits 0-14.
   bit 15 set: first element j in bits 0-14 and more in the overflow list.
*/
#define SHORT_HASHTABLE 1

/* Set CONST_EMPTY_SLOT=1 to use a constant value for empty_slot. This
   requires one extra 16-bit register/constant comparison to be performed
   during lookup(), but avoids a 64-bit register/memory comparison in the
   case that the slot is empty. Probably only worthwhile on 32-bit machines.
*/
#define CONST_EMPTY_SLOT 0


#define HASH_MINIMUM_ELTS 8
#define HASH_MINIMUM_SHIFT 8

#if SHORT_HASHTABLE
#define HASH_T uint16_t
#define HASH_BITS 16
#define HASH_DESC "short"
#else
#define HASH_T uint32_t
#define HASH_BITS 32
#define HASH_DESC "long"
#endif

#define HASH_NOT_FOUND UINT32_MAX
#define HASH_MASK1 ((HASH_T)1<<(HASH_BITS-1))
#define HASH_MASK2 (((HASH_T)1<<(HASH_BITS-1))-1)
#if !CONST_EMPTY_SLOT
#define HASH_MAX_ELTS (HASH_MASK2-1)
#else
#define HASH_MAX_ELTS HASH_MASK2
#endif

extern HASH_T *htable;
extern HASH_T *olist;
extern uint32_t hsize;
extern uint32_t hsize_minus1;
extern uint64_t *BJ64;

#if CONST_EMPTY_SLOT
#define empty_slot HASH_MASK2
#else
extern HASH_T empty_slot;
#endif

void init_hashtable(uint32_t M);
void fini_hashtable(void);

#if USE_ASM && __x86_64__ && _WIN64
static inline
void memset64(uint64_t *dst, uint64_t x, uint64_t count)
{
  asm ("rep"    "\n\t"
       "stosq"
       : "+c" (count), "+D" (dst),
         "=m" (*(struct { uint64_t dummy[count]; } *)dst)
       : "a" (x)
       : "cc" );
}
#endif

static inline void clear_hashtable(uint32_t size)
{
#if SHORT_HASHTABLE
# if USE_ASM && __x86_64__ && _WIN64
  uint64_t pattern = (uint64_t)empty_slot << 16 | empty_slot;
  memset64((uint64_t *)htable, pattern << 32 | pattern, size/4);
# elif (UINT_FAST32_MAX==UINT64_MAX)
  uint64_t pattern = (uint64_t)empty_slot << 16 | empty_slot;
  memset_fast32((uint_fast32_t *)htable, pattern << 32 | pattern, size/4);
# else
  memset_fast32((uint_fast32_t *)htable,
                (uint_fast32_t)empty_slot << 16 | empty_slot, size/2);
# endif
#else
# if (UINT_FAST32_MAX==UINT64_MAX)
  memset_fast32((uint_fast32_t *)htable,
                (uint64_t)empty_slot << 32 | empty_slot, size/2);
# else
  memset_fast32((uint_fast32_t *)htable,empty_slot,size);
# endif
#endif
#if !(CONST_EMPTY_SLOT)
  BJ64[empty_slot] = UINT64_MAX;
#endif
}

static inline void insert64(uint32_t j, uint64_t bj)
{
  uint32_t slot;

  BJ64[j] = bj;
#if USE_ASM && ARM_CPU
  slot = (bj>>(mod64_init_data.clzp-2)) & hsize_minus1;
#else
  slot = bj & hsize_minus1;
#endif
  
  if (htable[slot] == empty_slot)
    htable[slot] = j;
  else
  {
    olist[j] = htable[slot] ^ HASH_MASK1;
    htable[slot] = j | HASH_MASK1;
  }
}

static inline void post_insert64(uint32_t j)
{
  uint32_t slot;

#if USE_ASM && ARM_CPU
  slot = (BJ64[j]>>(mod64_init_data.clzp-2)) & hsize_minus1;
#else
  slot = BJ64[j] & hsize_minus1;
#endif
  if (htable[slot] == empty_slot)
    htable[slot] = j;
  else
  {
    olist[j] = htable[slot] ^ HASH_MASK1;
    htable[slot] = j | HASH_MASK1;
  }
}

static inline uint32_t lookup64(uint64_t bj)
{
  uint32_t slot;
  HASH_T elt;

#if USE_ASM && ARM_CPU
  slot = (bj>>(mod64_init_data.clzp-2)) & hsize_minus1;
#else
  slot = bj & hsize_minus1;
#endif
  elt = htable[slot];
#if CONST_EMPTY_SLOT
  if (elt == empty_slot)
    return HASH_NOT_FOUND;
#endif
  if (BJ64[elt & HASH_MASK2] == bj)
    return elt & HASH_MASK2;
  if ((elt & HASH_MASK1) == 0)
    return HASH_NOT_FOUND;

  elt &= HASH_MASK2;
  do
  {
    if (BJ64[olist[elt] & HASH_MASK2] == bj)
      return olist[elt] & HASH_MASK2;
    elt = olist[elt];
  } while ((elt & HASH_MASK1) == 0);

  return HASH_NOT_FOUND;
}

#if USE_SETUP_HASHTABLE

/* This is pure guesswork. */
#if (POWER_RESIDUE_LCM < 256*3/4)
# define SMALL_HASH_THRESHOLD 4
# define SMALL_HASH_SIZE 256
#elif (POWER_RESIDUE_LCM < 512*3/4)
# define SMALL_HASH_THRESHOLD 5
# define SMALL_HASH_SIZE 512
#elif (POWER_RESIDUE_LCM < 1024*3/4)
# define SMALL_HASH_THRESHOLD 6
# define SMALL_HASH_SIZE 1024
#elif (POWER_RESIDUE_LCM < 2048*3/4)
# define SMALL_HASH_THRESHOLD 10
# define SMALL_HASH_SIZE 2048
#elif (POWER_RESIDUE_LCM < 4096*3/4)
# define SMALL_HASH_THRESHOLD 16
# define SMALL_HASH_SIZE 4096
#elif (POWER_RESIDUE_LCM < 8192*3/4)
# define SMALL_HASH_THRESHOLD 24
# define SMALL_HASH_SIZE 8192
#elif (POWER_RESIDUE_LCM < 16384*3/4)
# define SMALL_HASH_THRESHOLD 36
# define SMALL_HASH_SIZE 16384
#elif (POWER_RESIDUE_LCM < 32768)
# define SMALL_HASH_THRESHOLD 64
# define SMALL_HASH_SIZE 32768
#endif

static inline void insert64_small(uint32_t j)
{
  uint32_t slot;

#if USE_ASM && ARM_CPU
  slot = (BJ64[j]>>(mod64_init_data.clzp-2)) & (SMALL_HASH_SIZE-1);
#else
  slot = BJ64[j] & (SMALL_HASH_SIZE-1);
#endif
  if (htable[slot] == empty_slot)
    htable[slot] = j;
  else
  {
    olist[j] = htable[slot] ^ HASH_MASK1;
    htable[slot] = j | HASH_MASK1;
  }
}

static inline uint32_t lookup64_small(uint64_t bj)
{
  uint32_t slot;
  HASH_T elt;

#if USE_ASM && ARM_CPU
  slot = (bj>>(mod64_init_data.clzp-2)) & (SMALL_HASH_SIZE-1);
#else
  slot = bj & (SMALL_HASH_SIZE-1);
#endif
  elt = htable[slot];
#if CONST_EMPTY_SLOT
  if (elt == empty_slot)
    return HASH_NOT_FOUND;
#endif
  if (BJ64[elt & HASH_MASK2] == bj)
    return elt & HASH_MASK2;
  if ((elt & HASH_MASK1) == 0)
    return HASH_NOT_FOUND;

  elt &= HASH_MASK2;
  do
  {
    if (BJ64[olist[elt] & HASH_MASK2] == bj)
      return olist[elt] & HASH_MASK2;
    elt = olist[elt];
  } while ((elt & HASH_MASK1) == 0);

  return HASH_NOT_FOUND;
}
#endif /* USE_SETUP_HASHTABLE */

#endif /* _HASHTABLE_H */
