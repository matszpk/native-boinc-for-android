/* bitmap.c -- (C) Geoffrey Reynolds, April 2006.

   Bitmap functions set_bit(B,n), clear_bit(B,n), test_bit(B,n),
   first_bit(B), next_bit(B,n), fill_bits(B,first,last). B is a
   pointer to a bitmap, n is the bit number counting from zero.
   first_bit()/next_bit() require at least one bit to be set.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifdef __GNUC__
/* Get prototype for ffs */
#define _GNU_SOURCE 1
#include <string.h>
#endif

#include "sr5sieve.h"
#include "bitmap.h"
#include "memset_fast32.h"

#if HAVE_FFS
# if (UINT_FAST32_MAX==UINT_MAX)
#  define ffs_fast32 ffs
# elif (UINT_FAST32_MAX==ULONG_MAX)
#  define ffs_fast32 ffsl
# elif (UINT_FAST32_MAX==ULONG_LONG_MAX)
#  define ffs_fast32 ffsll
# endif
#else
static int ffs_fast32(uint_fast32_t x)
{
  int i;

  if (x == 0)
    return 0;

  i = 1;
#if (UINT_FAST32_MAX==UINT64_MAX)
  if ((x & ((1<<32)-1)) == 0)
    x >>= 32, i += 32;
#endif
  if ((x & ((1<<16)-1)) == 0)
    x >>= 16, i += 16;
  if ((x & ((1<<8)-1)) == 0)
    x >>= 8, i += 8;
  if ((x & ((1<<4)-1)) == 0)
    x >>= 4, i += 4;
  if ((x & ((1<<2)-1)) == 0)
    x >>= 2, i += 2;
  if ((x & ((1<<1)-1)) == 0)
    i += 1;

  return i;
}
#endif

/* Create a new bitmap of at least n clear bits.
*/
uint_fast32_t *make_bitmap(uint_fast32_t n)
{
  uint_fast32_t size, *bitmap;

  size = bitmap_size(n);
  bitmap = xmalloc(size*sizeof(uint_fast32_t));
  memset_fast32(bitmap,0,size);
  bitmap[size-1] = UINT_FAST32_MAX; /* ~0; sentinel */

  return bitmap;
}

/* Resize a bitmap B of m bits to a bitmap of n bits. If the new bitmap is
   larger, clear the extra bits.
*/
#if 0
uint_fast32_t *resize_bitmap(uint_fast32_t *B,uint_fast32_t m,uint_fast32_t n)
{
  uint_fast32_t old_size, new_size;

  old_size = bitmap_size(m);
  new_size = bitmap_size(n);
  if (old_size != new_size)
  {
    B = xrealloc(B,new_size*sizeof(uint_fast32_t));
    if (old_size < new_size)
      memset_fast32(&B[old_size-1],0,new_size-old_size);
    B[new_size-1] = UINT_FAST32_MAX; /* ~0; Sentinel */
  }

  return B;
}
#endif

/* Return first set bit in B counting from bit 0 (inclusive). At least one
   bit must be set for first_bit to find.
 */
uint_fast32_t first_bit(const uint_fast32_t *B)
{
  uint_fast32_t i;

  for (i = 0; B[i] == 0; i++)
    ;

  return i*UINT_FAST32_BIT + ffs_fast32(B[i]) - 1;
}

/* Return the next set bit in B counting from bit (inclusive). At least one
   bit must be set for next_bit to find.
 */
uint_fast32_t next_bit(const uint_fast32_t *B, uint_fast32_t bit)
{
  uint_fast32_t i, mask;

  i = bit / UINT_FAST32_BIT;
  mask = UINT_FAST32_MAX << (bit % UINT_FAST32_BIT);

  if (B[i] & mask)
    return i*UINT_FAST32_BIT + ffs_fast32(B[i] & mask) - 1;

  for (i++; B[i] == 0; i++)
    ;

  return i*UINT_FAST32_BIT + ffs_fast32(B[i]) - 1;
}

/* Set all bits from first to last inclusive.
 */
void fill_bits(uint_fast32_t *B, uint_fast32_t first, uint_fast32_t last)
{
  uint_fast32_t fmask, lmask, *ptr;

  fmask = UINT_FAST32_MAX << (first % UINT_FAST32_BIT);
  lmask = UINT_FAST32_MAX >> (UINT_FAST32_BIT - 1 - last % UINT_FAST32_BIT);
  ptr = B + first / UINT_FAST32_BIT;
  if (first / UINT_FAST32_BIT == last / UINT_FAST32_BIT)
    *ptr |= (fmask & lmask);
  else
  {
    *ptr |= fmask;
    while (++ptr < B + last / UINT_FAST32_BIT)
      *ptr = UINT_FAST32_MAX; /* ~0; */
    *ptr |= lmask;
  }
}
