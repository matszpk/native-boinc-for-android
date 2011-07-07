/* btop-generic.c -- (C) Geoffrey Reynolds, September 2007.

   Generic build table of powers function.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "arithmetic.h"
#include "btop.h"

#ifndef HAVE_btop

#if (VECTOR_LENGTH==2)
#define VEC_MULMOD64_NEXT VEC2_MULMOD64_NEXT
#elif (VECTOR_LENGTH==4)
#define VEC_MULMOD64_NEXT VEC4_MULMOD64_NEXT
#define VEC_MULMOD64_NEXT_SSE2 VEC4_MULMOD64_NEXT_SSE2
#elif (VECTOR_LENGTH==8)
#define VEC_MULMOD64_NEXT VEC8_MULMOD64_NEXT
#define VEC_MULMOD64_NEXT_SSE2 VEC8_MULMOD64_NEXT_SSE2
#endif

/* Given T[0], assign T[j] <-- T[0]^(j+1) (mod p) for 0 < j <= gmax.
 */
void btop_generic(const uint32_t *L, uint64_t *T, uint32_t lmin, uint32_t llen, uint64_t p)
{
  uint32_t j;

  //printf("btop:lmin=%d,llen=%d\n",lmin,llen);
  
  /* First part of the table from T[0] ... T[lmin].
   */
#ifdef HAVE_vector
#if (VECTOR_LENGTH > 2)
  PRE2_MULMOD64_INIT(T[0]);
  for (j = 0; j < (VECTOR_LENGTH-1); j++)
    T[j+1] = PRE2_MULMOD64(T[j],T[0],p);
  PRE2_MULMOD64_FINI();
#else
  T[1] = sqrmod64(T[0],p);
#endif
#endif

#if defined(HAVE_vector_sse2)
  PRE2_MULMOD64_INIT(T[VECTOR_LENGTH-1]);
  v128_t B = VEC2_SET2(T[VECTOR_LENGTH-1]);
  v128_t P = VEC2_SET2(p);
  for (j = 0; j+VECTOR_LENGTH <= lmin; j += VECTOR_LENGTH)
    VEC_MULMOD64_NEXT_SSE2(T+j,B,P);
  PRE2_MULMOD64_FINI();

#elif defined(HAVE_vector)
  PRE2_MULMOD64_INIT(T[VECTOR_LENGTH-1]);
  for (j = 0; j+VECTOR_LENGTH <= lmin; j += VECTOR_LENGTH)
    VEC_MULMOD64_NEXT(T+j,T[VECTOR_LENGTH-1],p);
  PRE2_MULMOD64_FINI();

#else
  PRE2_MULMOD64_INIT(T[0]);
  for (j = 0; j < lmin; j++)
    T[j+1] = PRE2_MULMOD64(T[j],T[0],p);
  PRE2_MULMOD64_FINI();
#endif

  /* Second part of the table from T[lmin+1] ... T[gmax].
   */
#if USE_LADDER
  if (llen > 0)
  {
    uint64_t y = T[lmin];
    PRE2_MULMOD64_INIT(y);
    j = 0;
    do T[L[j]+lmin+1] = PRE2_MULMOD64(T[L[j]],y,p);
    while (++j < llen);
    PRE2_MULMOD64_FINI();
  }
#endif /* USE_LADDER */
}

#endif /* HAVE_btop */
