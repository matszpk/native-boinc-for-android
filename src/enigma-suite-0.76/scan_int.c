#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "scan.h"


/* scan unsigned integer, 10 digits or less.
 * accept interval [lb (inclusive), hb (exclusive) [. */
unsigned int scan_uint32(char *s, unsigned int lb, unsigned int hb)
{
  unsigned long int n;
  char *end;
  
  if (strchr(s, '-')) return UINT_MAX;
  if (strlen(s) > 10) return UINT_MAX;

  n = strtoul(s, &end, 10);
  if (n == ULONG_MAX || n < (unsigned long)lb || n >= (unsigned long)hb)
    return UINT_MAX;
  if ( !(*s != '\0' && *end == '\0') )
    return UINT_MAX;

  return (unsigned int) n;
}

/* scan integer, 10 digits or less + optional '-'.
 * accept interval [lb (inclusive), hb (exclusive) [.
 * lower bound must be greater than INT_MIN. */
int scan_int32(char *s, int lb, int hb)
{
  long int n;
  char *end;
  
  if (strlen(s) > 11) return INT_MAX;

  n = strtol(s, &end, 10);
  if (n == LONG_MAX || n  == LONG_MIN || n < (long)lb || n >= (long)hb)
    return INT_MAX;
  if ( !(*s != '\0' && *end == '\0') )
    return INT_MAX;

  return (int) n;
}

/* scan positive integer, 10 digits or less, smaller than INT_MAX */
int scan_posint(char *s)
{
  int n;

  if ((n = scan_int32(s, 0, INT_MAX)) == INT_MAX)
    return -1;

  return n;
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
