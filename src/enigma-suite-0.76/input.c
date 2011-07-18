#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "charmap.h"
#include "error.h"
#include "global.h"
#include "key.h"
#include "stecker.h"
#include "input.h"


/* determine model */
int get_model(char *s)
{
  if (strcmp(s, "H") == 0 || strcmp(s, "h") == 0)
    return H;
  if (strcmp(s, "M3") == 0 || strcmp(s, "m3") == 0)
    return M3;
  if (strcmp(s, "M4") == 0 || strcmp(s, "m4") == 0)
    return M4;

  return -1;
}

/* set UKW */
int set_ukw(Key *key, char *s, int model)
{
  if (strcmp(s, "A") == 0 || strcmp(s, "a") == 0) {
    switch (model) {
      case H: key->ukwnum = 0; break;
      default: return 0;
    }
    return 1;
  }
  if (strcmp(s, "B") == 0 || strcmp(s, "b") == 0) {
    switch (model) {
      case M4: key->ukwnum = 3; break;
      default: key->ukwnum = 1; break;
    }
    return 1;
  }
  if (strcmp(s, "C") == 0 || strcmp(s, "c") == 0) {
    switch (model) {
      case M4: key->ukwnum = 4; break;
      default: key->ukwnum = 2; break;
    }
    return 1;
  }

  return 0;
}

/* set walzen */
int set_walze(Key *key, char *s, int model)
{
  char *x;

  switch (model) {
    case M4: if (strlen(s) != 4) return 0; break;
    default: if (strlen(s) != 3) return 0; break;
  }

  x = s;
  if (model == M4) { /* greek wheel */
    if ( !(*x == 'B' || *x == 'b' || *x == 'G' || *x == 'g') )
      return 0;
    x++;
  }
  while (*x != '\0') {
    switch (model) {
      case H: /* digits 1-5, no repetitions */
        if ( !isdigit((unsigned char)*x) || *x < '1' || *x > '5'
           || strrchr(x, *x) != x )
             return 0;
        break;
      case M3: case M4: /* digits 1-8, no repetitions */
        if ( !isdigit((unsigned char)*x) || *x < '1' || *x > '8'
           || strrchr(x, *x) != x )
             return 0;
        break;
      default:
        return 0;
    }
    x++;
  }

  x = s;
  if (model == M4) {
    if (*x == 'B' || *x == 'b')
      key->g_slot = 9;
    if (*x == 'G' || *x == 'g')
      key->g_slot = 10;
    x++;
  }
  key->l_slot = *x++ - '0';
  key->m_slot = *x++ - '0';
  key->r_slot = *x - '0';

  return 1;
}

/* set rings */
int set_ring(Key *key, char *s, int model)
{
  char *x;

  switch (model) {
    case M4: if (strlen(s) != 4) return 0; break;
    default: if (strlen(s) != 3) return 0; break;
  }

  x = s;
  while (*x != '\0') {
    if (code[(unsigned char)*x] == 26)
      return 0;
    x++;
  }

  x = s;
  if (model == M4)
    key->g_ring = code[(unsigned char)*x++];
  key->l_ring = code[(unsigned char)*x++];
  key->m_ring = code[(unsigned char)*x++];
  key->r_ring = code[(unsigned char)*x];

  return 1;
}

/* set message keys */
int set_mesg(Key *key, char *s, int model)
{
  char *x;

  switch (model) {
    case M4: if (strlen(s) != 4) return 0; break;
    default: if (strlen(s) != 3) return 0; break;
  }

  x = s;
  while (*x != '\0') {
    if (code[(unsigned char)*x] == 26)
      return 0;
    x++;
  }

  x = s;
  if (model == M4)
    key->g_mesg = code[(unsigned char)*x++];
  key->l_mesg = code[(unsigned char)*x++];
  key->m_mesg = code[(unsigned char)*x++];
  key->r_mesg = code[(unsigned char)*x];

  return 1;
}

/* set steckerbrett */
int set_stecker(Key *key, char *s)
{
  int len;
  char *x;

  /* max 26 chars, even number */
  if ((len = strlen(s)) > 26 || len%2 != 0)
        return 0;

  x = s;
  while (*x != '\0') {
    *x = tolower((unsigned char)*x);
    x++;
  }

  x = s;
  while (*x != '\0') {
    /* alphabetic, no repetitions */
    if (code[(unsigned char)*x] == 26 || strrchr(x, *x) != x)
      return 0;
    x++;
  }

  /* swap appropriate letters */
  x = s;
  while (*x != '\0') {
    swap(key->stbrett, code[(unsigned char)*x], code[(unsigned char)*(x+1)]);
    x += 2;
  }

  return 1;
}

/* determine mode for slow ring */
int get_sw_mode(char *s)
{
  if (strcmp(s, "0") == 0)
    return SW_ONSTART;
  if (strcmp(s, "1") == 0)
    return SW_OTHER;
  if (strcmp(s, "2") == 0)
    return SW_ALL;

  return -1;
}

/* get firstpass */
int get_firstpass(char *s)
{
  if (strcmp(s, "0") == 0)
    return 0;
  if (strcmp(s, "1") == 0)
    return 1;

  return -1;
}

/* set *key according to *keystring, model */
int set_key(Key *key, const char *keystring, int model, int adjust)
{
    int i, d;
    unsigned int len;
    char s[15];
    char ring[5];
    char *x;


    if (!init_key_low(key, model)) return 0;

    switch (model) {
      case H: case M3: len = 12; d = 4; break;
      case M4: len = 14; d = 5; break;
      default: return 0;
    }

    if (strlen(keystring) != len) return 0;
    strcpy(s, keystring);

    i = 1;
    if (s[i] != ':') return 0;
    s[i] = '\0';
    i += d;
    if (s[i] != ':') return 0;
    s[i] = '\0';
    i += 3;
    if (s[i] != ':') return 0;
    s[i] = '\0';

    x = s;
    if (!set_ukw(key, x, model)) return 0;

    x += 2;
    if (!set_walze(key, x, model)) return 0;

    x += d;
    if (model == M4)
      sprintf(ring, "AA");
    else
      sprintf(ring, "A");
    strcat(ring, x);
    if (!set_ring(key, ring, model)) return 0;

    x += 3;
    if (!set_mesg(key, x, model)) return 0;


    /* error checking for rings */
    if ( key->m_slot > 5 && key->m_ring > 12 ) {
      if (adjust) {
        key->m_ring = (key->m_ring + 13) % 26;
        key->m_mesg = (key->m_mesg + 13) % 26;
      }
      else
        err_input_fatal(ERR_RING_SHORTCUT);
    }
    if ( key->r_slot > 5 && key->r_ring > 12 ) {
      if (adjust) {
        key->r_ring = (key->r_ring + 13) % 26;
        key->r_mesg = (key->r_mesg + 13) % 26;
      }
      else
        err_input_fatal(ERR_RING_SHORTCUT);
    }
 
    return 1;
}

/* set keys *from, *to according to [keystrings kf, kt], model */
int set_range(Key *from, Key *to, const char *kf, const char *kt, int model)
{
  if (!set_key(from, kf, model, 0)) return 0;
  if (!set_key(to, kt, model, 0)) return 0;
  if (keycmp(from, to) == 1) return 0;

  return 1;
  
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
