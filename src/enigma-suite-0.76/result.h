#ifndef RESULT_H
#define RESULT_H

#include <stdio.h>
#include "key.h"

FILE *open_outfile(char *s);
void print_plaintext(FILE *file, const int *stbrett, const int *ciphertext, int len);
void print_key(FILE *file, const Key *key);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
