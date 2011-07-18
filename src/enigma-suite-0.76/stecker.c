#include <stdio.h>
#include <stdlib.h>
#include "key.h"
#include "stecker.h"


/* swaps letters */
void swap(int stbrett[], int i, int k)
{
  int store;

  store = stbrett[i];
  stbrett[i] = stbrett[k];
  stbrett[k] = store;
}

/* extracts stecker from key->stbrett to key->sf */
void get_stecker(Key *key)
{
  int i, k = 25;

  key->count = 0;
  for (i = 0; i < 26; i++) {
    if (key->stbrett[i] > i) {
      key->sf[key->count++] = i;
      key->sf[key->count++] = key->stbrett[i];
    }
    else if (key->stbrett[i] == i) {
      key->sf[k--] = i;
    }
  }

}

/* get new order for testing stecker */
void rand_var(int var[])
{
  int count;
  int store;
  int i;
  
  for (count = 25; count > 0; count--) {
    i = random() % (count+1);
    store = var[count];
    var[count] = var[i];
    var[i] = store;
  }

}

/* arrange var[] in order of frequency of letters in ciphertext */
void set_to_ct_freq(int var[], const int *ciphertext, int len)
{
  int f[26] = {0};
  int i, k, c;
  int max, pos = -1;
  int n = 0;
 
  for (i = 0; i < len; i++) {
    c = ciphertext[i];
    f[c]++;
  }

  for (i = 0; i < 26; i++) {
    max = 0;
    for (k = 0; k < 26; k++)
      if (f[k] >= max) {
        max = f[k];
        pos = k;
      }
    if (pos == -1)
      return;
    f[pos] = -1;
    var[n++] = pos;
  }

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
