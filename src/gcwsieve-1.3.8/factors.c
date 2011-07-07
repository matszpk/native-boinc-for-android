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
#include "gcwsieve.h"
#if CHECK_FACTORS
#include "arithmetic.h"
#endif

uint32_t factor_count;

static void save_factor(uint32_t n, int cw, uint64_t p)
{
  FILE *factors_file;
  int res;

  if (factors_file_name != NULL)
  {
    factors_file = xfopen(factors_file_name,"a",error);

#if MULTISIEVE_OPT
    if (multisieve_opt)
      res = fprintf(factors_file,"(%s)%%%"PRIu64"\n",term_str(n,cw),p);
    else
#endif
      res = fprintf(factors_file,"%"PRIu64" | %s\n",p,term_str(n,cw));

    if (res < 0)
      error("Could not write to factors file `%s'.",factors_file_name);

    xfclose(factors_file,factors_file_name);
  }
}

#if CHECK_FACTORS
/*
  FIXME: All the bolted-on options have made this far too complicated.
*/
static int is_factor(uint32_t n, int cw, uint64_t p)
{
  uint64_t res;

  assert(p > 0);

#if HAVE_FORK
  /* Parent thread also reports factors found by children, in which case the
     mod64_init() will not have been called.
   */
# if (USE_ASM && __GNUC__ && (__i386__ || __x86_64__))
  uint16_t rnd_save = mod64_rnd;
# endif
# ifdef SMALL_P
  if (smallp_phase > 2)
# endif
    if (num_children > 0)
      mod64_init(p);
#endif

  res = mulmod64(powmod64(b_term,n,p),n,p);

#if HAVE_FORK
# if (USE_ASM && __GNUC__ && (__i386__ || __x86_64__))
  mod64_rnd = rnd_save;
# endif
# ifdef SMALL_P
  if (smallp_phase > 2)
# endif
    if (num_children > 0)
      mod64_fini();
#endif

  return cw? (res == p-1) : (res == 1);
}
#endif

void report_factor(uint32_t n, int cw, uint64_t p)
{
#if CHECK_FACTORS
  if (!is_factor(n,cw,p))
    error("%"PRIu64" DOES NOT DIVIDE %s.",p,term_str(n,cw));
#endif

  factor_count++;
  save_factor(n,cw,p);

  if (!quiet_opt)
  {
    report(1,"%"PRIu64" | %s",p,term_str(n,cw));
    notify_event(factor_found);
  }
}

#if KNOWN_FACTORS_OPT
static uint32_t get_factor(FILE *file, int *cw)
{
  uint64_t p;
  uint32_t k, b, n;
  int32_t c;
  int res;
  const char *line;

  while ((line = read_line(file)) != NULL)
  {
#if MULTISIEVE_OPT
    res = sscanf(line,"(%"SCNu32"*%"SCNu32"^%"SCNu32"%"SCNd32")%%%"SCNu64,
                 &k,&b,&n,&c,&p);
    if (res == 0)
#endif
      res = sscanf(line,"%"SCNu64" | %"SCNu32"*%"SCNu32"^%"SCNu32"%"SCNd32,
                   &p,&k,&b,&n,&c);

    if (res != 5 || k < 2 || k != n || b != b_term || (c != 1 && c != -1))
      warning("Ignoring unparsed line: %s",line);
    else
    {
#if CHECK_FACTORS
      if (p < 3 || p > UINT64_C(1) << MOD64_MAX_BITS)
        warning("Cannot check out of range factor: %s",line);
      else
      {
        mod64_init(p);
        res = is_factor(n,(c>0),p);
        mod64_fini();
        if (res == 0)
          error("Factor does not divide: %s",line);
      }
#endif
      *cw = (c > 0);
      return n;
    }
  }

  return 0;
}

void remove_known_factors(const char *file_name)
{
  FILE *file;
  uint32_t i, j, n, count;
  int cw;

  if (file_name == NULL)
    return;

  file = xfopen(file_name,"r",error);
  count = 0;
  while ((n = get_factor(file,&cw)) != 0)
    for (i = 0; i < ncount[cw]; i++)
      if (N[cw][i] == n)
        N[cw][i] = 0, count++;

  for (cw = 0; cw < 2; cw++)
  {
    for (i = 0, j = 0; i < ncount[cw]; i++)
      if (N[cw][i] != 0)
        N[cw][j++] = N[cw][i];
    ncount[cw] = j;
  }

  if (ferror(file))
    error("While reading factors from `%s'",file_name);
  fclose(file);

  report(1,"Removed %"PRIu32" term%s with factors in `%s'.",
         count,plural(count),file_name);
}
#endif
