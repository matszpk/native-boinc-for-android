#include <stdio.h>
#include <math.h>
#include "charmap.h"
#include "error.h"
#include "dict.h"


int tridict[26][26][26];
int bidict[26][26];
int unidict[26];


int load_tridict(const char *filename)
{
  unsigned char tri[4];
  int log;
  FILE *fp;

  if ( (fp = fopen(filename, "r")) == NULL )
    err_open_fatal(filename);

  while (fscanf(fp, "%3s%d", tri, &log) != EOF) {
    if (  code[tri[0]] == 26
       || code[tri[1]] == 26
       || code[tri[2]] == 26  ) 
         err_illegal_char_fatal(filename);
    tridict[code[tri[0]]][code[tri[1]]][code[tri[2]]] = log;
  }

  fclose(fp);
  return 0;
}

int load_bidict(const char *filename)
{
  unsigned char bi[3];
  int log;
  FILE *fp;

  if ( (fp = fopen(filename, "r")) == NULL )
    err_open_fatal(filename);

  while (fscanf(fp, "%2s%d", bi, &log) != EOF) {
    if (  code[bi[0]] == 26
       || code[bi[1]] == 26 )
         err_illegal_char_fatal(filename);
    bidict[code[bi[0]]][code[bi[1]]] = log;
  }

  fclose(fp);
  return 0;
}


int load_unidict(const char *filename)
{
  unsigned char uni[2];
  int log;
  FILE *fp;

  if ( (fp = fopen(filename, "r")) == NULL )
    err_open_fatal(filename);

  while (fscanf(fp, "%1s%d", uni, &log) != EOF) {
    if (  code[uni[0]] == 26 )
      err_illegal_char_fatal(filename);
    unidict[code[uni[0]]] = log;
  }

  fclose(fp);
  return 0;
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
