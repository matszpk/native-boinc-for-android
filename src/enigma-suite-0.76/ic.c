#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "cipher.h"
#include "global.h"
#include "hillclimb.h"
#include "ic.h"
#include "key.h"


void ic_noring( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
                int sw_mode, int max_pass, int firstpass, int max_score, int resume,
                FILE *outfile, int act_on_sig, int *ciphertext, int len )
{
  Key ckey;
  Key gkey;
  Key lo;
  int hi[3][12] = {
    {H, 2,0,5,5,5,25,25,0,25,25,25},
    {M3,2,0,8,8,8,25,25,0,25,25,25},
    {M4,4,10,8,8,8,25,25,25,25,25,25}
  };
  int m;
  double a, bestic;
  int firstloop = 1, clen;


  m = from->model;


  /* iterate thru all but ring settings */
  ckey = gkey = lo = *from;
  ckey.m_ring = ckey.r_ring = 0;
  bestic = 0;
  for (ckey.ukwnum=lo.ukwnum; ckey.ukwnum<=hi[m][1]; ckey.ukwnum++) {
   for (ckey.g_slot=lo.g_slot; ckey.g_slot<=hi[m][2]; ckey.g_slot++) {
    for (ckey.l_slot=lo.l_slot; ckey.l_slot<=hi[m][3]; ckey.l_slot++) {
     for (ckey.m_slot=lo.m_slot; ckey.m_slot<=hi[m][4]; ckey.m_slot++) {
       if (ckey.m_slot == ckey.l_slot) continue;
      for (ckey.r_slot=lo.r_slot; ckey.r_slot<=hi[m][5]; ckey.r_slot++) {
        if (ckey.r_slot == ckey.l_slot || ckey.r_slot == ckey.m_slot) continue;
       for (ckey.g_mesg=lo.g_mesg; ckey.g_mesg<=hi[m][8]; ckey.g_mesg++) {
        for (ckey.l_mesg=lo.l_mesg; ckey.l_mesg<=hi[m][9]; ckey.l_mesg++) {
         for (ckey.m_mesg=lo.m_mesg; ckey.m_mesg<=hi[m][10]; ckey.m_mesg++) {
          for (ckey.r_mesg=lo.r_mesg; ckey.r_mesg<=hi[m][11]; ckey.r_mesg++) {

             a = dgetic_ALL(&ckey, ciphertext, len);
             if (a-bestic > DBL_EPSILON) {
               bestic = a;
               gkey = ckey;
             }
             
             if (firstloop) {
               firstloop = 0;
               init_key_low(&lo, m);
             }

          }
         }
        }
       }
      }
     }
    }
   }
  }

  /* recover ring settings */
  ckey = gkey;
  for (ckey.r_ring = 0; ckey.r_ring < 26; ckey.r_ring++) {
    a = dgetic_ALL(&ckey, ciphertext, len);
    if (a-bestic > DBL_EPSILON) {
      bestic = a;
      gkey = ckey;
    }
    ckey.r_mesg = (ckey.r_mesg+1) % 26;
  }
  ckey = gkey;
  for (ckey.m_ring = 0; ckey.m_ring < 26; ckey.m_ring++) {
    a = dgetic_ALL(&ckey, ciphertext, len);
    if (a-bestic > DBL_EPSILON) {
      bestic = a;
      gkey = ckey;
    }
    ckey.m_mesg = (ckey.m_mesg+1) % 26;
  }

  /* try to recover stecker of the best key (gkey) */
  clen = (len < CT) ? len : CT;
  hillclimb( &gkey, &gkey, ckey_res, gkey_res, sw_mode, max_pass, firstpass,
             max_score, resume, outfile, act_on_sig, ciphertext, clen );

}


void ic_allring( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
                 int sw_mode, int max_pass, int firstpass, int max_score, int resume,
                 FILE *outfile, int act_on_sig, int *ciphertext, int len )
{
  Key ckey;
  Key gkey;
  Key lo;
  int hi[3][12] = {
    {H, 2,0,5,5,5,25,25,0,25,25,25},
    {M3,2,0,8,8,8,25,25,0,25,25,25},
    {M4,4,10,8,8,8,25,25,25,25,25,25}
  };
  int m;
  double a, bestic;
  int firstloop = 1, clen;


  m = from->model;


  /* iterate thru all settings */
  ckey = gkey = lo = *from;
  bestic = 0;
  for (ckey.ukwnum=lo.ukwnum; ckey.ukwnum<=hi[m][1]; ckey.ukwnum++) {
   for (ckey.g_slot=lo.g_slot; ckey.g_slot<=hi[m][2]; ckey.g_slot++) {
    for (ckey.l_slot=lo.l_slot; ckey.l_slot<=hi[m][3]; ckey.l_slot++) {
     for (ckey.m_slot=lo.m_slot; ckey.m_slot<=hi[m][4]; ckey.m_slot++) {
       if (ckey.m_slot == ckey.l_slot) continue;
      for (ckey.r_slot=lo.r_slot; ckey.r_slot<=hi[m][5]; ckey.r_slot++) {
        if (ckey.r_slot == ckey.l_slot || ckey.r_slot == ckey.m_slot) continue;
       for (ckey.m_ring=lo.m_ring; ckey.m_ring<=hi[m][6]; ckey.m_ring++) {
        for (ckey.r_ring=lo.r_ring; ckey.r_ring<=hi[m][7]; ckey.r_ring++) {
         for (ckey.g_mesg=lo.g_mesg; ckey.g_mesg<=hi[m][8]; ckey.g_mesg++) {
          for (ckey.l_mesg=lo.l_mesg; ckey.l_mesg<=hi[m][9]; ckey.l_mesg++) {
           for (ckey.m_mesg=lo.m_mesg; ckey.m_mesg<=hi[m][10]; ckey.m_mesg++) {
            for (ckey.r_mesg=lo.r_mesg; ckey.r_mesg<=hi[m][11]; ckey.r_mesg++) {

             a = dgetic_ALL(&ckey, ciphertext, len);
             if (a-bestic > DBL_EPSILON) {
               bestic = a;
               gkey = ckey;
             }
             
             if (firstloop) {
               firstloop = 0;
               init_key_low(&lo, m);
             }
             if (keycmp(&ckey, to) == 0)
               goto HILLCLIMB;

            }
           }
          }
         }
        }
       }
      }
     }
    }
   }
  }

  /* try to recover stecker of the best key (gkey) */
  HILLCLIMB:
  clen = (len < CT) ? len : CT;
  hillclimb( &gkey, &gkey, ckey_res, gkey_res, sw_mode, max_pass, firstpass,
             max_score, resume, outfile, act_on_sig, ciphertext, clen );

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
