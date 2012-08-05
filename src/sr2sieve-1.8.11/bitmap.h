/* bitmap.h -- (C) Geoffrey Reynolds, April 2006.

   Bitmap functions set_bit(B,n), clear_bit(B,n), test_bit(B,n),
   first_bit(B), next_bit(B,n), fill_bits(B,first,last). B is a
   pointer to a bitmap, n is the bit number counting from zero.
   first_bit()/next_bit() require at least one bit to be set.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include "config.h"

#define UINT_FAST32_BIT (sizeof(uint_fast32_t)*CHAR_BIT)

static inline uint_fast32_t set_bit(uint_fast32_t *B, uint_fast32_t bit)
{
  uint_fast32_t mask, ret;

  B += bit / UINT_FAST32_BIT;
  mask = (uint_fast32_t)1 << (bit % UINT_FAST32_BIT);
  ret = (mask & *B);
  *B |= mask;

  return ret;
}

static inline uint_fast32_t clear_bit(uint_fast32_t *B, uint_fast32_t bit)
{
  uint_fast32_t mask, ret;

  B += bit / UINT_FAST32_BIT;
  mask = (uint_fast32_t)1 << (bit % UINT_FAST32_BIT);
  ret = (mask & *B);
  *B &= ~mask;

  return ret;
}

static inline uint_fast32_t test_bit(const uint_fast32_t *B, uint_fast32_t bit)
{
  uint_fast32_t mask;

  B += bit / UINT_FAST32_BIT;
  mask = (uint_fast32_t)1 << (bit % UINT_FAST32_BIT);

  return (mask & *B);
}

#if 0
static inline void prefetch_bit(const uint_fast32_t *B, uint_fast32_t bit)
{
  __builtin_prefetch(B+bit/UINT_FAST32_BIT,0,0);
}
#endif

static inline uint_fast32_t bitmap_size(uint_fast32_t n)
{
  return ((n+UINT_FAST32_BIT-1)/UINT_FAST32_BIT+1);
}


uint_fast32_t *make_bitmap(uint_fast32_t n);
#if 0
uint_fast32_t
*resize_bitmap(uint_fast32_t *B, uint_fast32_t old_n, uint_fast32_t new_n);
#endif
uint_fast32_t first_bit(const uint_fast32_t *B) attribute ((pure));
uint_fast32_t
next_bit(const uint_fast32_t *B, uint_fast32_t bit) attribute ((pure));
void fill_bits(uint_fast32_t *B, uint_fast32_t first, uint_fast32_t last);

#endif /* _BITMAP_H */
