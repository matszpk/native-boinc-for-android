#ifndef CIPHER_H
#define CIPHER_H

#include "stdio.h"
#include "key.h"

int scrambler_state(const Key *key, int len);
void init_path_lookup_H_M3(const Key *key, int len);
void init_path_lookup_ALL(const Key *key, int len);
double dgetic_ALL(const Key *key, const int *ciphertext, int len);
void en_deciph_stdin_ALL(FILE *file, const Key *key);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
