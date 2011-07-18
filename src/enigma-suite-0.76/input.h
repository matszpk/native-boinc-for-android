#ifndef INPUT_H
#define INPUT_H

#include "key.h"

int get_model(char *s);
int set_ukw(Key *key, char *s, int model);
int set_walze(Key *key, char *s, int model);
int set_ring(Key *key, char *s, int model);
int set_mesg(Key *key, char *s, int model);
int set_stecker(Key *key, char *s);
int get_sw_mode(char *s);
int get_firstpass(char *s);
int set_key(Key *key, const char *keystring, int model, int adjust);
int set_range(Key *from, Key *to, const char *kf, const char *kt, int model);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
