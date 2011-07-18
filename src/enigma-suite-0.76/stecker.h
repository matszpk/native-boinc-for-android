#ifndef STECKER_H
#define STECKER_H

#include "key.h"

void swap(int stbrett[], int i, int k);
void get_stecker(Key *key);
void rand_var(int var[]);
void set_to_ct_freq(int var[], const int *ciphertext, int len);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
