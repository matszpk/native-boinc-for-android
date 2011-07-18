#ifndef RESUME_IN_H
#define RESUME_IN_H

#include "key.h"

char *getline_resume(char *dest, int n, FILE *fp);
int set_state( Key *from, Key *to, Key *ckey_res, Key *gkey_res, int *sw_mode,
               int *max_pass, int *firstpass, int *max_score );

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
