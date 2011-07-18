#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "charmap.h"
#include "global.h"
#include "key.h"
#include "state.h"
#include "resume_out.h"


void print_state(FILE *fp, const State *state)
{
  char stecker[27];
  int i;
  int ofd;

  const Key *from = state->from;
  const Key *to = state->to;
  Key *ckey = state->ckey;
  Key *gkey = state->gkey;
  int *sw_mode = state->sw_mode;
  int *pass = state->pass;
  int *firstpass = state->firstpass;
  int *max_score = state->max_score;


  /* general state */
  if (from->model == H) fprintf(fp, "H=");
  else if (from->model == M3) fprintf(fp, "M3=");
  else if (from->model== M4) fprintf(fp, "M4=");
 
  if (from->model != M4) {
    fprintf(fp,
    "%c:%d%d%d:%c%c:%c%c%c=%c:%d%d%d:%c%c:%c%c%c=%c:%d%d%d:%c%c:%c%c%c=",
    toupper(alpha[from->ukwnum]),
    from->l_slot, from->m_slot, from->r_slot,
    toupper(alpha[from->m_ring]), toupper(alpha[from->r_ring]),
    toupper(alpha[from->l_mesg]), toupper(alpha[from->m_mesg]),
    toupper(alpha[from->r_mesg]),
    toupper(alpha[to->ukwnum]),
    to->l_slot, to->m_slot, to->r_slot,
    toupper(alpha[to->m_ring]), toupper(alpha[to->r_ring]),
    toupper(alpha[to->l_mesg]), toupper(alpha[to->m_mesg]),
    toupper(alpha[to->r_mesg]),
    toupper(alpha[ckey->ukwnum]),
    ckey->l_slot, ckey->m_slot, ckey->r_slot,
    toupper(alpha[ckey->m_ring]), toupper(alpha[ckey->r_ring]),
    toupper(alpha[ckey->l_mesg]), toupper(alpha[ckey->m_mesg]),
    toupper(alpha[ckey->r_mesg]));
  }
  else {
    fprintf(fp,
    "%c:%c%d%d%d:%c%c:%c%c%c%c=%c:%c%d%d%d:%c%c:%c%c%c%c=%c:%c%d%d%d:%c%c:%c%c%c%c=",
    from->ukwnum == 3 ? 'B' : 'C',
    from->g_slot == 9 ? 'B' : 'G', from->l_slot, from->m_slot, from->r_slot,
    toupper(alpha[from->m_ring]), toupper(alpha[from->r_ring]),
    toupper(alpha[from->g_mesg]), toupper(alpha[from->l_mesg]),
    toupper(alpha[from->m_mesg]), toupper(alpha[from->r_mesg]),
    to->ukwnum == 3 ? 'B' : 'C',
    to->g_slot == 9 ? 'B' : 'G', to->l_slot, to->m_slot, to->r_slot,
    toupper(alpha[to->m_ring]), toupper(alpha[to->r_ring]),
    toupper(alpha[to->g_mesg]), toupper(alpha[to->l_mesg]),
    toupper(alpha[to->m_mesg]), toupper(alpha[to->r_mesg]),
    ckey->ukwnum == 3 ? 'B' : 'C',
    ckey->g_slot == 9 ? 'B' : 'G', ckey->l_slot, ckey->m_slot, ckey->r_slot,
    toupper(alpha[ckey->m_ring]), toupper(alpha[ckey->r_ring]),
    toupper(alpha[ckey->g_mesg]), toupper(alpha[ckey->l_mesg]),
    toupper(alpha[ckey->m_mesg]), toupper(alpha[ckey->r_mesg]));
  }

  fprintf(fp, "%d=", *sw_mode);
  fprintf(fp, "%d=", *pass);
  fprintf(fp, "%d=", *firstpass);
  fprintf(fp, "%d\n", *max_score); 


  /* global key */
  if (from->model == H) fprintf(fp, "H=");
  else if (from->model == M3) fprintf(fp, "M3=");
  else if (from->model== M4) fprintf(fp, "M4=");

  if (from->model != M4) {
    fprintf(fp,
    "%c:%d%d%d:%c%c:%c%c%c=",
    toupper(alpha[gkey->ukwnum]),
    gkey->l_slot, gkey->m_slot, gkey->r_slot,
    toupper(alpha[gkey->m_ring]), toupper(alpha[gkey->r_ring]),
    toupper(alpha[gkey->l_mesg]), toupper(alpha[gkey->m_mesg]),
    toupper(alpha[gkey->r_mesg]));
  }
  else {
    fprintf(fp,
    "%c:%c%d%d%d:%c%c:%c%c%c%c=",
    gkey->ukwnum == 3 ? 'B' : 'C',
    gkey->g_slot == 9 ? 'B' : 'G', gkey->l_slot, gkey->m_slot, gkey->r_slot,
    toupper(alpha[gkey->m_ring]), toupper(alpha[gkey->r_ring]),
    toupper(alpha[gkey->g_mesg]), toupper(alpha[gkey->l_mesg]),
    toupper(alpha[gkey->m_mesg]), toupper(alpha[gkey->r_mesg]));
  }

  for (i = 0; i < gkey->count; i++)
    stecker[i] = toupper(alpha[gkey->sf[i]]);
  stecker[i] = '\0';
  fprintf(fp, "%s=", stecker);

  fprintf(fp, "%d\n", gkey->score);


  fflush(fp);
  ofd = fileno(fp);
  fsync(ofd);
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
