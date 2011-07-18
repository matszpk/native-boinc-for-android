#ifndef STATE_H
#define STATE_H

typedef struct {
  const Key *from;
  const Key *to;
  Key *ckey;
  Key *gkey;
  int *sw_mode;
  int *pass;
  int *firstpass;
  int *max_score;
  int *ciphertext;
} State;

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
