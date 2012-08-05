/* factors.c -- (C) Geoffrey Reynolds, May 2006.

   Factors file routines and misc factoring related functions.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "sr5sieve.h"
#if CHECK_FACTORS
#include "arithmetic.h"
#endif

/* Append a factor to file_name.
 */
void save_factor(const char *file_name, uint32_t k, int32_t c, uint32_t n,
                 uint64_t p)
{
  FILE *file;

  file = xfopen(file_name,"a",error);

  if (fprintf(file,"%"PRIu64" | %s\n",p,kbnc_str(k,n,c)) < 0)
    error("Could not write factor to file `%s'.",file_name);

  xfclose(file,file_name);
}

#if CHECK_FACTORS
/* Return 1 if p is a factor of k*b^n+c, 0 otherwise.
 */
int is_factor(uint32_t k, int32_t c, uint32_t n, uint64_t p)
{
  uint64_t res;

  assert(k < p);
  assert(ABS(c) < p);

#if (USE_ASM && __GNUC__ && (__i386__ || (__x86_64__ && USE_FPU_MULMOD)))
  /* We don't know whether b/p or 1.0/p is on the FPU stack, or even whether
     the FPU is being used at all, so save the old mode and initialise.
  */
  uint16_t rnd_save = mod64_rnd;
  mod64_init(p);
#elif HAVE_FORK
  /* Parent thread also reports factors found by children, in which case the
     mod64_init() will not have been called.
   */
# if (USE_ASM && __GNUC__ && (__i386__ || __x86_64__))
  uint16_t rnd_save = mod64_rnd;
# endif
  if (num_children > 0)
    mod64_init(p);
#endif

#if (BASE == 0)
  res = mulmod64(powmod64(b_term,n,p),k,p);
#else
  res = mulmod64(powmod64(BASE,n,p),k,p);
#endif

#if (USE_ASM && __GNUC__ && (__i386__ || (__x86_64__ && USE_FPU_MULMOD)))
  mod64_fini();
  mod64_rnd = rnd_save;
#elif HAVE_FORK
# if (USE_ASM && __GNUC__ && (__i386__ || __x86_64__))
  mod64_rnd = rnd_save;
# endif
  if (num_children > 0)
    mod64_fini();
#endif

  return (c < 0) ? (res == -c) : (res == p-c);
}
#endif
