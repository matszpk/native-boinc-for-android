#include <stdio.h>
#include <string.h>
#include "error.h"
#include "global.h"
#include "input.h"
#include "key.h"
#include "scan.h"
#include "stecker.h"
#include "resume_in.h"


char *getline_resume(char *dest, int n, FILE *fp)
{
  char *x;

  if ((x = fgets(dest, n, fp)) == NULL)
    return NULL;

  if ((x = strchr(dest, '\n')) != NULL)
    *x = '\0';
  if ((x = strchr(dest, '\r')) != NULL)
    *x = '\0';

  return dest;
}

/* sets state according to 00hc.resume */
int set_state( Key *from, Key *to, Key *ckey_res, Key *gkey_res, int *sw_mode,
               int *max_pass, int *firstpass, int *max_score )
{
  FILE *fp;
  char *kf, *kt, *kc, *kg, *x; 
  char line1[NLINE1];
  char line2[NLINE2];
  int model;
  int eq;


  if ((fp = fopen("00hc.resume", "r")) == NULL)
    err_open_fatal("00hc.resume");

  if (getline_resume(line1, NLINE1, fp) == NULL)
    return 0;
  if (getline_resume(line2, NLINE2, fp) == NULL)
    return 0;
 
  eq = 0; x = line1;
  while((x = strchr(x, '=')) != NULL) {
    *x++ = '\0';
    eq++;
  }
  if (eq != 7) return 0;

  eq = 0; x = line2;
  while((x = strchr(x, '=')) != NULL) {
    *x++ = '\0';
    eq++;
  }
  if (eq != 3) return 0;


  /* line 1 */
  x = line1;
  if ((model = get_model(x)) == -1) return 0;

  while (*x++) ;
  kf = x;
  while (*x++) ;
  kt = x;
  while (*x++) ;
  kc = x;
  
  while (*x++) ;
  if ((*sw_mode = get_sw_mode(x)) == -1) return 0;

  while (*x++) ;
  if ((*max_pass = scan_posint(x)) == -1 ) return 0;

  while (*x++) ;
  if ((*firstpass = get_firstpass(x)) == -1) return 0;

  while (*x++) ;
  if ((*max_score = scan_posint(x)) == -1 ) return 0;
  

  if (!set_range(from, to, kf, kt, model)) return 0;
  
  if (!set_key(ckey_res, kc, model, 0)) return 0;

  if (keycmp(from, ckey_res) == 1) return 0;
  if (keycmp(ckey_res, to) == 1) return 0;


  /* line2 */
  x = line2;
  if ((model != get_model(x))) return 0;

  while (*x++) ;
  kg = x;
  if (!set_key(gkey_res, kg, model, 0)) return 0;

  while (*x++) ;
  if (!set_stecker(gkey_res, x)) return 0;
  get_stecker(gkey_res);

  while (*x++) ;
  if ((gkey_res->score = scan_posint(x)) == -1 ) return 0;
  
  fclose(fp);
  return 1;
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
