/* asm-ppc64.c -- (C) 2006 Mark Rodenkirch.

   Setup for PPC mulmod/expmod functions. See mul/expmod-ppc64.S.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "arithmetic.h"

/* See asm-ppc64.h for why unsigned long long int instead of uint64_t.
 */
unsigned long long int pMagic;
unsigned long long int pShift;
unsigned long long int bLO;
unsigned long long int bHI;

void getMagic(uint64_t d)
{
   uint64_t two63 = 0x8000000000000000;

   uint64_t t = two63;
   uint64_t anc = t - 1 - t%d;    /* Absolute value of nc. */
   uint64_t p = 63;            /* Init p. */
   uint64_t q1 = two63/anc;       /* Init q1 = 2**p/|nc|. */
   uint64_t r1 = two63 - q1*anc;  /* Init r1 = rem(2**p, |nc|). */
   uint64_t q2 = two63/d;        /* Init q2 = 2**p/|d|. */
   uint64_t r2 = two63- q2*d;      /* Init r2 = rem(2**p, |d|). */
   uint64_t delta, mag;

   do {
      p = p + 1;
      q1 = 2*q1;                  /* Update q1 = 2**p/|nc|. */
      r1 = 2*r1;                  /* Update r1 = rem(2**p, |nc|. */
      if (r1 >= anc) {      /* (Must be an unsigned */
         q1 = q1 + 1;        /* comparison here). */
         r1 = r1 - anc;
      }
      q2 = 2*q2;                  /* Update q2 = 2**p/|d|. */
      r2 = 2*r2;                  /* Update r2 = rem(2**p, |d|. */
      if (r2 >= d) {          /* (Must be an unsigned */
         q2 = q2 + 1;        /* comparison here). */
         r2 = r2 - d;
      }
      delta = d - r2;
   } while (q1 < delta || (q1 == delta && r1 == 0));

   mag = q2 + 1;

   pShift = p - 64;             /* shift amount to return. */

   pMagic = mag;
}
