/* loop-generic.c -- (C) Geoffrey Reynolds, August 2007.

   Generic main loop.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "arithmetic.h"
#include "swizzle.h"

#ifndef HAVE_swizzle_loop

/* Prefetch data for this number of loop iterations in advance.
*/
#define PREFETCH_ITER 4


/* Let gmul be the greatest common divisor of all n in terms n*b^n+/-1,
   and let gmax+1 be the greatest gap between successive n.
   Let tcount be the number of terms in the sieve (including dummy terms).

   SWIZZLE is a constant multiple of 2.

   T[i] = b^(i*gmul+1) (mod p) for 0 <= i < gmax.

   D = {X[], N0[], G0[], N1[], G1[], ..., Ntcount[], Gtcount[]},
   where for 0 <= j < SWIZZLE, 0 <= k < tcount/SWIZZLE:
     X[j] = b^(-(j+1)*tcount/SWIZZLE),
     Nk[j] = exponent of term SWIZZLE*j+k,
     Gk[j]+1 = gap between exponent of terms SWIZZLE*j+k and SWIZZLE*j+k+1.
     (Declared in swizzle.h).

   It is safe to assume that D is aligned on a 64 byte boundary,
   and that i < 2^28 (so that i*8 will not suffer signed overflow).

   In iteration i, this function computes X[j] = X[j] * T[Gi[j]] (mod p),
   checks whether any X[j] == Ni[j], for 0 <= j < SWIZZLE, and returns i if
   true. A negative value is returned when complete.

   If a non-negative value is returned, then that value is the iteration on
   which a factor was found, and the function will be called again with i
   set equal to that value and with all other arguments unchanged once the
   caller has  eliminated the factor. X[j] must be updated before returning
   a non-negative value.
*/

#if USE_PREFETCH
int swizzle_loop_generic_prefetch(const uint64_t *T, struct loop_data_t *D,
                                  int i, uint64_t p)
#else
int swizzle_loop_generic(const uint64_t *T, struct loop_data_t *D,
                         int i, uint64_t p)
#endif
{
  /* Make a local copy of D->X[] so that the compiler knows that elements of
     D->X[] cannot alias elements of D->R[].
  */
#if (SWIZZLE==2)
  register uint64_t X0 = D->X[0], X1 = D->X[1];
#elif (SWIZZLE==4)
  register uint64_t X0 = D->X[0], X1 = D->X[1], X2 = D->X[2], X3 = D->X[3];
#elif (SWIZZLE==6)
  register uint64_t X0 = D->X[0], X1 = D->X[1], X2 = D->X[2];
  register uint64_t X3 = D->X[3], X4 = D->X[4], X5 = D->X[5];
#else
  uint64_t X[SWIZZLE];
  int j;

  for (j = 0; j < SWIZZLE; j++)
    X[j] = D->X[j];
#endif

  /* Assumes that mod64_init(p) has been called. */

  while (--i >= 0)
  {
#if defined(__GNUC__) && USE_PREFETCH
    __builtin_prefetch(&(D->R[i-PREFETCH_ITER]),0,0);
#endif

#if (SWIZZLE==2)
    X0 = mulmod64(X0,T[D->R[i].G[0]],p);
    X1 = mulmod64(X1,T[D->R[i].G[1]],p);

#ifdef SMALL_P
    if (X0 == D->R[i].N[0] || X0+p == D->R[i].N[0] ||
        X1 == D->R[i].N[1] || X1+p == D->R[i].N[1])
      break;
#else
    if (X0 == D->R[i].N[0] || X1 == D->R[i].N[1])
      break;
#endif

#elif (SWIZZLE==4)
    X0 = mulmod64(X0,T[D->R[i].G[0]],p);
    X1 = mulmod64(X1,T[D->R[i].G[1]],p);
    X2 = mulmod64(X2,T[D->R[i].G[2]],p);
    X3 = mulmod64(X3,T[D->R[i].G[3]],p);

#ifdef SMALL_P
    if (X0 == D->R[i].N[0] || X0+p == D->R[i].N[0] ||
        X1 == D->R[i].N[1] || X1+p == D->R[i].N[1] ||
        X2 == D->R[i].N[2] || X2+p == D->R[i].N[2] ||
        X3 == D->R[i].N[3] || X3+p == D->R[i].N[3])
      break;
#else
    if (X0 == D->R[i].N[0] || X1 == D->R[i].N[1] ||
        X2 == D->R[i].N[2] || X3 == D->R[i].N[3])
      break;
#endif

#elif (SWIZZLE==6)
    X0 = mulmod64(X0,T[D->R[i].G[0]],p);
    X1 = mulmod64(X1,T[D->R[i].G[1]],p);
    X2 = mulmod64(X2,T[D->R[i].G[2]],p);
    X3 = mulmod64(X3,T[D->R[i].G[3]],p);
    X4 = mulmod64(X4,T[D->R[i].G[4]],p);
    X5 = mulmod64(X5,T[D->R[i].G[5]],p);

#ifdef SMALL_P
    if (X0 == D->R[i].N[0] || X0+p == D->R[i].N[0] ||
        X1 == D->R[i].N[1] || X1+p == D->R[i].N[1] ||
        X2 == D->R[i].N[2] || X2+p == D->R[i].N[2] ||
        X3 == D->R[i].N[3] || X3+p == D->R[i].N[3] ||
        X4 == D->R[i].N[4] || X4+p == D->R[i].N[4] ||
        X5 == D->R[i].N[5] || X5+p == D->R[i].N[5])
      break;
#else
    if (X0 == D->R[i].N[0] || X1 == D->R[i].N[1] || X2 == D->R[i].N[2] ||
        X3 == D->R[i].N[3] || X4 == D->R[i].N[4] || X5 == D->R[i].N[5])
      break;
#endif

#else /* SWIZZLE > 6 */
    for (j = 0; j < SWIZZLE; j++)
      X[j] = mulmod64(X[j],T[D->R[i].G[j]],p);

#ifdef SMALL_P
    for (j = 0; j < SWIZZLE; j++)
      if (X[j] == D->R[i].N[j] || X[j]+p == D->R[i].N[j])
        goto out;
#else
    for (j = 0; j < SWIZZLE; j++)
      if (X[j] == D->R[i].N[j])
        goto out;
#endif
#endif
  }

#if (SWIZZLE==2)
  D->X[0] = X0, D->X[1] = X1;
#elif (SWIZZLE==4)
  D->X[0] = X0, D->X[1] = X1, D->X[2] = X2, D->X[3] = X3;
#elif (SWIZZLE==6)
  D->X[0] = X0, D->X[1] = X1, D->X[2] = X2;
  D->X[3] = X3, D->X[4] = X4, D->X[5] = X5;
#else
 out:
  for (j = 0; j < SWIZZLE; j++)
    D->X[j] = X[j];
#endif

  return i;
}
#endif /* HAVE_swizzle_loop */
