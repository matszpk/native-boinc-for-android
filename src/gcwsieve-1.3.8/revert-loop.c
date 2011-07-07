/*
 * invmod.c -
 * Mateusz Szpakowski
 * License: LGPL v2.0
 */

#include <stdint.h>
#include "gcwsieve.h"
#include "swizzle.h"
#include "arithmetic.h"

/* compute inverse of A, B - must be coprime */
uint64_t invmod64_64(uint64_t a, uint64_t b)
{
  int64_t xm2 = 1LL;
  int64_t ym2 = 0LL;
  uint64_t a0 = a;
  uint64_t b0 = b;
  uint64_t temp, div;
  uint64_t x0 = 0ULL;
  uint64_t y0 = 1ULL;
  while (b0 != 0ULL)
  {
    temp = b0;
    div = a0 / b0;
    b0 = a0 % b0;
    a0 = temp;
    
    temp = x0;
    x0 = xm2 - div*x0;
    xm2 = temp;
    temp = y0;
    y0 = ym2 - div*y0;
    ym2 = temp;
  }
  return (xm2>=0LL)?xm2:xm2+b;
}


uint64_t revert_swizzle_loop_0(uint64_t X, struct loop_rec_t* LR,
                         int reverts_n, uint64_t p)
{
  int i;
  uint32_t clzb = mod64_init_data.clzp-2;
  
  X >>= clzb;
  if (X>=p)
    X-=p;
  for (i = 4-reverts_n; i < 4; i++)
  {
    uint64_t b = LR[i].G[0]->T;
    uint64_t invb;
    b -= p;
    if (b >= p)
      b -= p;
    
    invb = invmod64_64(b,p);
    X = mulmod64(X,invb,p);
  }
  return (X+p)<<clzb;
}

uint64_t revert_swizzle_loop_1(uint64_t X, struct loop_rec_t* LR,
                         int reverts_n, uint64_t p)
{
  int i;
  uint32_t clzb = mod64_init_data.clzp-2;
  
  X >>= clzb;
  if (X>=p)
    X-=p;
  for (i = 4-reverts_n; i < 4; i++)
  {
    uint64_t b = LR[i].G[1]->T;
    uint64_t invb;
    b -= p;
    if (b >= p)
      b -= p;
    
    invb = invmod64_64(b,p);
    X = mulmod64(X,invb,p);
  }
  return (X+p)<<clzb;
}
