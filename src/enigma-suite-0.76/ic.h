#ifndef IC_H
#define IC_H

#include <stdio.h>
#include "key.h"

void ic_noring( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
                int sw_mode, int max_pass, int firstpass, int max_score, int resume,
                FILE *outfile, int act_on_sig, int *ciphertext, int len );
void ic_allring( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
                 int sw_mode, int max_pass, int firstpass, int max_score, int resume,
                 FILE *outfile, int act_on_sig, int *ciphertext, int len );

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
