#ifndef KEY_H
#define KEY_H

typedef struct {
        int model;
        int ukwnum;
        int g_slot;
        int l_slot;     /* greek, left, middle, right slot */
        int m_slot;
        int r_slot;
        int g_ring;
        int l_ring;     /* ringstellungen */
        int m_ring;
        int r_ring;
        int g_mesg;
        int l_mesg;     /* message settings */
        int m_mesg;
        int r_mesg;
        int stbrett[26];
        int sf[26];     /* swapped/free letters */
        int count;      /* number of swapped letters */
        int score;      /* hillclimbing score */
} Key;

int init_key_default(Key *key, int model);
int init_key_low(Key *key, int model);
int keycmp(const Key *k1, const Key *k2);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
