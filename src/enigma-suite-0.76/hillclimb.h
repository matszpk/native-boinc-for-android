#ifndef HILLCLIMB_H
#define HILLCLIMB_H

#include <stdio.h>
#include "key.h"


typedef struct {
        int s1;         /* positions of letters to be swapped */
        int s2;
        int u1;         /* positions of letters to be unswapped */
        int u2;
} Change;


enum { NONE, KZ_IK, KZ_IZ, IX_KI, IX_KX, IXKZ_IK, IXKZ_IZ, IXKZ_IKXZ,
       IXKZ_IZXK, RESWAP };


void hillclimb( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
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
