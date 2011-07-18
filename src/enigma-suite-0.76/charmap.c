#include <ctype.h>
#include <limits.h>
#include "charmap.h"


int code[UCHAR_MAX+1];
unsigned char alpha[26] = "abcdefghijklmnopqrstuvwxyz";

void init_charmap(void)
{
  int i;

  for (i = 0; i < UCHAR_MAX+1; i++)
    code[i] = 26;

  for (i = 0; i < 26; i++) {
    code[alpha[i]] = i;
    code[toupper(alpha[i])] = i;
  }

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
