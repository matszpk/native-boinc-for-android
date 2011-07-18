#include "global.h"
#include "key.h"


/* initialize key to defaults */
int init_key_default(Key *key, int model)
{
  Key def_H  = {H, 1,0,1,2,3,0,0,0,0,0,0,0,0,{0},{0},0,0};
  Key def_M3 = {M3,1,0,1,2,3,0,0,0,0,0,0,0,0,{0},{0},0,0};
  Key def_M4 = {M4,3,9,1,2,3,0,0,0,0,0,0,0,0,{0},{0},0,0};

  int i;

  switch (model) {
    case H : *key = def_H; break;
    case M3: *key = def_M3; break;
    case M4: *key = def_M4; break;
    default: return 0;
  }
  
  for (i = 0; i < 26; i++)
    key->stbrett[i] = key->sf[i] = i;

  return 1;

}  

/* initializes each key element to the lowest possible value */
int init_key_low(Key *key, int model)
{
    Key low_H  = {H, 0,0,1,1,1,0,0,0,0,0,0,0,0,{0},{0},0,0};
    Key low_M3 = {M3,1,0,1,1,1,0,0,0,0,0,0,0,0,{0},{0},0,0};
    Key low_M4 = {M4,3,9,1,1,1,0,0,0,0,0,0,0,0,{0},{0},0,0};

    int i;


    switch (model) {
      case H : *key = low_H; break;
      case M3: *key = low_M3; break;
      case M4: *key = low_M4; break;
      default: return 0;
    }

    for (i = 0; i < 26; i++)
      key->stbrett[i] = key->sf[i] = i;

    return 1;

}

/* compares ukwnum thru r_mesg, omits g_ring, l_ring        */
/* returns -1 for k1 < k2, 0 for k1 == k2, 1 for k1 > k2    */
int keycmp(const Key *k1, const Key *k2)
{
  if (  k1->ukwnum != k2->ukwnum ) {
    if ( k1->ukwnum > k2->ukwnum ) return 1;
    else return -1;
  }
  if (  k1->g_slot != k2->g_slot ) {
    if ( k1->g_slot > k2->g_slot ) return 1;
    else return -1;
  }
  if (  k1->l_slot != k2->l_slot ) {
    if ( k1->l_slot > k2->l_slot ) return 1;
    else return -1;
  }
  if (  k1->m_slot != k2->m_slot ) {
    if ( k1->m_slot > k2->m_slot ) return 1;
    else return -1;
  }
  if (  k1->r_slot != k2->r_slot ) {
    if ( k1->r_slot > k2->r_slot ) return 1;
    else return -1;
  }
  if (  k1->m_ring != k2->m_ring ) {
    if ( k1->m_ring > k2->m_ring ) return 1;
    else return -1;
  }
  if (  k1->r_ring != k2->r_ring ) {
    if ( k1->r_ring > k2->r_ring ) return 1;
    else return -1;
  }
  if (  k1->g_mesg != k2->g_mesg ) {
    if ( k1->g_mesg > k2->g_mesg ) return 1;
    else return -1;
  }
  if (  k1->l_mesg != k2->l_mesg ) {
    if ( k1->l_mesg > k2->l_mesg ) return 1;
    else return -1;
  }
  if (  k1->m_mesg != k2->m_mesg ) {
    if ( k1->m_mesg > k2->m_mesg ) return 1;
    else return -1;
  }
  if (  k1->r_mesg != k2->r_mesg ) {
    if ( k1->r_mesg > k2->r_mesg ) return 1;
    else return -1;
  }

  return 0;

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
