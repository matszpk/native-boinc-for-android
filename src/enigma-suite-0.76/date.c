#include <time.h>
#include "date.h"

void datestring(char *s)
{
  time_t tp;
  struct tm *t;

  time(&tp);
  t = localtime(&tp);
  strftime(s, DATELEN, "%Y-%m-%d %H:%M:%S", t);
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
