#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "cipher.h"
#include "dict.h"
#include "error.h"
#include "global.h"
#include "hillclimb.h"
#include "key.h"
#include "result.h"
#include "resume_out.h"
#include "score.h"
#include "stecker.h"
#include "state.h"


extern int tridict[][26][26];
extern int path_lookup[][26];
struct sigaction sigact;
volatile sig_atomic_t do_shutdown = 0;

void save_state(State state)
{
  FILE *fp;

  if ((fp = fopen("00hc.resume", "w")) == NULL)
    err_open_fatal("00hc.resume");

  print_state(fp, &state);
  if (ferror(fp) != 0)
    err_stream_fatal("00hc.resume");

  fclose(fp);
}

void save_state_exit(State state, int retval)
{
  FILE *fp;

  if ((fp = fopen("00hc.resume", "w")) == NULL)
    err_open_fatal("00hc.resume");

  print_state(fp, &state);
  if (ferror(fp) != 0)
    err_stream_fatal("00hc.resume");

  free(state.ciphertext);
  
  exit(retval);
}

void handle_signal(int signum)
{
  do_shutdown = 1;
}

void install_sighandler(void)
{
  int catchsig[3] = {SIGINT, SIGQUIT, SIGTERM};
  int i;

  sigact.sa_handler = handle_signal;
  for (i = 0; i < 3; i++)
    if (sigaction(catchsig[i], &sigact, NULL) != 0)
      err_sigaction_fatal(catchsig[i]);
}


void hillclimb( const Key *from, const Key *to, const Key *ckey_res, const Key *gkey_res,
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
  Change ch;
  State state;
  time_t lastsave;
  int m;
  int i, k, x, z;
  int var[26] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
  int pass, newtop, action;
  int bestscore, jbestscore, a, globalscore;
  //double bestic, ic;
  int bestic, ic;
  int firstloop = 1;
  struct timeval tv;
  unsigned int seed;
  
  gettimeofday(&tv, NULL);
  seed = (tv.tv_sec%1000)*1000000 + tv.tv_usec; 
  //seed = 123456789;
  srandom(seed);
  lastsave = time(NULL);

  if (resume)
    hillclimb_log("enigma: working on range ...");
  
  if (act_on_sig) {
    state.from = from;
    state.to = to;
    state.ckey = &ckey;
    state.gkey = &gkey;
    state.sw_mode = &sw_mode;
    state.pass = &pass;
    state.firstpass = &firstpass;
    state.max_score = &max_score;
    state.ciphertext = ciphertext;
  
    install_sighandler();
  }

  m = from->model;
  ckey = lo = (resume) ? *ckey_res : *from;
  gkey = (resume) ? *gkey_res : *from;
  globalscore = (resume) ? gkey_res->score : 0;


  if (firstpass)
    /* set testing order to letter frequency in ciphertext */
    set_to_ct_freq(var, ciphertext, len);
  else
    /* set random testing order */
    rand_var(var);


  /* with the rand_var() restarting method passes are independent
   * from each other
   */
  for (pass = max_pass; pass > 0; pass--, lo = *from) {

   firstloop = 1;

   for (ckey.ukwnum=lo.ukwnum; ckey.ukwnum<=hi[m][1]; ckey.ukwnum++) {
    for (ckey.g_slot=lo.g_slot; ckey.g_slot<=hi[m][2]; ckey.g_slot++) {
     for (ckey.l_slot=lo.l_slot; ckey.l_slot<=hi[m][3]; ckey.l_slot++) {
      for (ckey.m_slot=lo.m_slot; ckey.m_slot<=hi[m][4]; ckey.m_slot++) {
        if (ckey.m_slot == ckey.l_slot) continue;
       for (ckey.r_slot=lo.r_slot; ckey.r_slot<=hi[m][5]; ckey.r_slot++) {
         if (ckey.r_slot == ckey.l_slot || ckey.r_slot == ckey.m_slot) continue;
        for (ckey.m_ring=lo.m_ring; ckey.m_ring<=hi[m][6]; ckey.m_ring++) {
          if (ckey.m_slot > 5 && ckey.m_ring > 12) continue;
         for (ckey.r_ring=lo.r_ring; ckey.r_ring<=hi[m][7]; ckey.r_ring++) {
           if (ckey.r_slot > 5 && ckey.r_ring > 12) continue;
          for (ckey.g_mesg=lo.g_mesg; ckey.g_mesg<=hi[m][8]; ckey.g_mesg++) {
           for (ckey.l_mesg=lo.l_mesg; ckey.l_mesg<=hi[m][9]; ckey.l_mesg++) {
            for (ckey.m_mesg=lo.m_mesg; ckey.m_mesg<=hi[m][10]; ckey.m_mesg++) {
             for (ckey.r_mesg=lo.r_mesg; ckey.r_mesg<=hi[m][11]; ckey.r_mesg++) {

               if (do_shutdown)
                 save_state_exit(state, 111);
               if (difftime(time(NULL), lastsave) > 119) {
                 lastsave = time(NULL);
                 save_state(state);
               }


               /* avoid duplicate scrambler states */
               switch (sw_mode) {
                 case SW_ONSTART:
                   if (scrambler_state(&ckey, len) != SW_ONSTART)
                     goto ENDLOOP;
                   break;
                 case SW_OTHER:
                   if (scrambler_state(&ckey, len) != SW_OTHER)
                     goto ENDLOOP;
                   break;
                 case SW_ALL:
                   if (scrambler_state(&ckey, len) == SW_NONE)
                     goto ENDLOOP;
                   break;
                 default: /* includes SINGLE_KEY */
                   break;
               } 

               /* complete ckey initialization */
               for (i = 0; i < 26; i++)
                 ckey.sf[i] = ckey.stbrett[i] = i;
               ckey.count = 0;

               /* initialize path_lookup */
               switch (m) {
                 case H: case M3:
                   init_path_lookup_H_M3(&ckey, len);
                   break;
                 case M4:
                   init_path_lookup_ALL(&ckey, len);
                   break;
                 default:
                   break;
               }
             

               /* ic score */
               bestic = icscore(ckey.stbrett, ciphertext, len);
               for (i = 0; i < 26; i++) {
                 for (k = i+1; k < 26; k++) {
                   if ( (var[i] == ckey.stbrett[var[i]] && var[k] == ckey.stbrett[var[k]])
                      ||(var[i] == ckey.stbrett[var[k]] && var[k] == ckey.stbrett[var[i]]) ) {
                     swap(ckey.stbrett, var[i], var[k]);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       continue;
                     }
                     swap(ckey.stbrett, var[i], var[k]);
                   }
                   else if (var[i] == ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                     action = NONE;
                     z = ckey.stbrett[var[k]];
                     swap(ckey.stbrett, var[k], z);
                     
                     swap(ckey.stbrett, var[i], var[k]);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = KZ_IK;
                     }
                     swap(ckey.stbrett, var[i], var[k]);
                     
                     swap(ckey.stbrett, var[i], z);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = KZ_IZ;
                     }
                     swap(ckey.stbrett, var[i], z);

                     switch (action) {
                       case KZ_IK:
                         swap(ckey.stbrett, var[i], var[k]);
                         break;
                       case KZ_IZ:
                         swap(ckey.stbrett, var[i], z);
                         break;
                       case NONE:
                         swap(ckey.stbrett, var[k], z);
                         break;
                       default:
                         break;
                     }
                   }
                   else if (var[k] == ckey.stbrett[var[k]] && var[i] != ckey.stbrett[var[i]]) {
                     action = NONE;
                     x = ckey.stbrett[var[i]];
                     swap(ckey.stbrett, var[i], x);
                     
                     swap(ckey.stbrett, var[k], var[i]);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IX_KI;
                     }
                     swap(ckey.stbrett, var[k], var[i]);

                     swap(ckey.stbrett, var[k], x);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IX_KX;
                     }
                     swap(ckey.stbrett, var[k], x);

                     switch (action) {
                       case IX_KI:
                         swap(ckey.stbrett, var[k], var[i]);
                         break;
                       case IX_KX:
                         swap(ckey.stbrett, var[k], x);
                         break;
                       case NONE:
                         swap(ckey.stbrett, var[i], x);
                         break;
                       default:
                         break;
                     }
                   }
                   else if (var[i] != ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                     action = NONE;
                     x = ckey.stbrett[var[i]];
                     z = ckey.stbrett[var[k]];
                     swap(ckey.stbrett, var[i], x);
                     swap(ckey.stbrett, var[k], z);

                     swap(ckey.stbrett, var[i], var[k]);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IXKZ_IK;
                     }
                     swap(ckey.stbrett, x, z);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IXKZ_IKXZ;
                     }
                     swap(ckey.stbrett, x, z);
                     swap(ckey.stbrett, var[i], var[k]);

                     swap(ckey.stbrett, var[i], z);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IXKZ_IZ;
                     }
                     swap(ckey.stbrett, x, var[k]);
                     ic = icscore(ckey.stbrett, ciphertext, len);
                     if (ic-bestic >= 1) {
                       bestic = ic;
                       action = IXKZ_IZXK;
                     }
                     swap(ckey.stbrett, x, var[k]);
                     swap(ckey.stbrett, var[i], z);

                     switch (action) {
                       case IXKZ_IK:
                         swap(ckey.stbrett, var[i], var[k]);
                         break;
                       case IXKZ_IZ:
                         swap(ckey.stbrett, var[i], z);
                         break;
                       case IXKZ_IKXZ:
                         swap(ckey.stbrett, var[i], var[k]);
                         swap(ckey.stbrett, x, z);
                       break;
                       case IXKZ_IZXK:
                         swap(ckey.stbrett, var[i], z);
                         swap(ckey.stbrett, x, var[k]);
                       break;
                       case NONE:
                         swap(ckey.stbrett, var[i], x);
                         swap(ckey.stbrett, var[k], z);
                         break;
                       default:
                         break;
                     }
                   }
                 }
               }


               newtop = 1;
               jbestscore = triscore(ckey.stbrett, ciphertext, len) + biscore(ckey.stbrett, ciphertext, len);

               while (newtop) {

                 newtop = 0;
                 
                 bestscore = biscore(ckey.stbrett, ciphertext, len);
                 for (i = 0; i < 26; i++) {
                   for (k = i+1; k < 26; k++) {
                     if ( (var[i] == ckey.stbrett[var[i]] && var[k] == ckey.stbrett[var[k]])
                        ||(var[i] == ckey.stbrett[var[k]] && var[k] == ckey.stbrett[var[i]]) ) {
                       swap(ckey.stbrett, var[i], var[k]);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         continue;
                       }
                       swap(ckey.stbrett, var[i], var[k]);
                     }
                     else if (var[i] == ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                       action = NONE;
                       z = ckey.stbrett[var[k]];
                       swap(ckey.stbrett, var[k], z);

                       swap(ckey.stbrett, var[i], var[k]);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = KZ_IK;
                       }
                       swap(ckey.stbrett, var[i], var[k]);

                       swap(ckey.stbrett, var[i], z);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = KZ_IZ;
                       }
                       swap(ckey.stbrett, var[i], z);

                       switch (action) {
                         case KZ_IK:
                           swap(ckey.stbrett, var[i], var[k]);
                           break;
                         case KZ_IZ:
                           swap(ckey.stbrett, var[i], z);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[k], z);
                           break;
                         default:
                           break;
                       }
                     }
                     else if (var[k] == ckey.stbrett[var[k]] && var[i] != ckey.stbrett[var[i]]) {
                       action = NONE;
                       x = ckey.stbrett[var[i]];
                       swap(ckey.stbrett, var[i], x);

                       swap(ckey.stbrett, var[k], var[i]);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IX_KI;
                       }
                       swap(ckey.stbrett, var[k], var[i]);

                       swap(ckey.stbrett, var[k], x);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IX_KX;
                       }
                       swap(ckey.stbrett, var[k], x);

                       switch (action) {
                         case IX_KI:
                           swap(ckey.stbrett, var[k], var[i]);
                           break;
                         case IX_KX:
                           swap(ckey.stbrett, var[k], x);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[i], x);
                           break;
                         default:
                           break;
                       }
                     }
                     else if (var[i] != ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                       action = NONE;
                       x = ckey.stbrett[var[i]];
                       z = ckey.stbrett[var[k]];
                       swap(ckey.stbrett, var[i], x);
                       swap(ckey.stbrett, var[k], z);

                       swap(ckey.stbrett, var[i], var[k]);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IK;
                       }
                       swap(ckey.stbrett, x, z);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IKXZ;
                       }
                       swap(ckey.stbrett, x, z);
                       swap(ckey.stbrett, var[i], var[k]);

                       swap(ckey.stbrett, var[i], z);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IZ;
                       }
                       swap(ckey.stbrett, x, var[k]);
                       a = biscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IZXK;
                       }
                       swap(ckey.stbrett, x, var[k]);
                       swap(ckey.stbrett, var[i], z);

                       switch (action) {
                         case IXKZ_IK:
                           swap(ckey.stbrett, var[i], var[k]);
                           break;
                         case IXKZ_IZ:
                           swap(ckey.stbrett, var[i], z);
                           break;
                         case IXKZ_IKXZ:
                           swap(ckey.stbrett, var[i], var[k]);
                           swap(ckey.stbrett, x, z);
                           break;
                         case IXKZ_IZXK:
                           swap(ckey.stbrett, var[i], z);
                           swap(ckey.stbrett, x, var[k]);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[i], x);
                           swap(ckey.stbrett, var[k], z);
                           break;
                         default:
                           break;
                       }
                     }
                   }
                 }


                 bestscore = triscore(ckey.stbrett, ciphertext, len);
                 for (i = 0; i < 26; i++) {
                   for (k = i+1; k < 26; k++) {
                     if ( (var[i] == ckey.stbrett[var[i]] && var[k] == ckey.stbrett[var[k]])
                        ||(var[i] == ckey.stbrett[var[k]] && var[k] == ckey.stbrett[var[i]]) ) {
                       swap(ckey.stbrett, var[i], var[k]);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         continue;
                       }
                       swap(ckey.stbrett, var[i], var[k]);
                     }
                     else if (var[i] == ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                       action = NONE;
                       z = ckey.stbrett[var[k]];
                       swap(ckey.stbrett, var[k], z);

                       swap(ckey.stbrett, var[i], var[k]);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = KZ_IK;
                       }
                       swap(ckey.stbrett, var[i], var[k]);

                       swap(ckey.stbrett, var[i], z);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = KZ_IZ;
                       }
                       swap(ckey.stbrett, var[i], z);

                       switch (action) {
                         case KZ_IK:
                           swap(ckey.stbrett, var[i], var[k]);
                           break;
                         case KZ_IZ:
                           swap(ckey.stbrett, var[i], z);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[k], z);
                           break;
                         default:
                           break;
                       }
                     }
                     else if (var[k] == ckey.stbrett[var[k]] && var[i] != ckey.stbrett[var[i]]) {
                       action = NONE;
                       x = ckey.stbrett[var[i]];
                       swap(ckey.stbrett, var[i], x);

                       swap(ckey.stbrett, var[k], var[i]);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IX_KI;
                       }
                       swap(ckey.stbrett, var[k], var[i]);

                       swap(ckey.stbrett, var[k], x);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IX_KX;
                       }
                       swap(ckey.stbrett, var[k], x);

                       switch (action) {
                         case IX_KI:
                           swap(ckey.stbrett, var[k], var[i]);
                           break;
                         case IX_KX:
                           swap(ckey.stbrett, var[k], x);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[i], x);
                           break;
                         default:
                           break;
                       }
                     }
                     else if (var[i] != ckey.stbrett[var[i]] && var[k] != ckey.stbrett[var[k]]) {
                       action = NONE;
                       x = ckey.stbrett[var[i]];
                       z = ckey.stbrett[var[k]];
                       swap(ckey.stbrett, var[i], x);
                       swap(ckey.stbrett, var[k], z);

                       swap(ckey.stbrett, var[i], var[k]);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IK;
                       }
                       swap(ckey.stbrett, x, z);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IKXZ;
                       }
                       swap(ckey.stbrett, x, z);
                       swap(ckey.stbrett, var[i], var[k]);

                       swap(ckey.stbrett, var[i], z);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IZ;
                       }
                       swap(ckey.stbrett, x, var[k]);
                       a = triscore(ckey.stbrett, ciphertext, len);
                       if (a > bestscore) {
                         bestscore = a;
                         action = IXKZ_IZXK;
                       }
                       swap(ckey.stbrett, x, var[k]);
                       swap(ckey.stbrett, var[i], z);

                       switch (action) {
                         case IXKZ_IK:
                           swap(ckey.stbrett, var[i], var[k]);
                           break;
                         case IXKZ_IZ:
                           swap(ckey.stbrett, var[i], z);
                           break;
                         case IXKZ_IKXZ:
                           swap(ckey.stbrett, var[i], var[k]);
                           swap(ckey.stbrett, x, z);
                           break;
                         case IXKZ_IZXK:
                           swap(ckey.stbrett, var[i], z);
                           swap(ckey.stbrett, x, var[k]);
                           break;
                         case NONE:
                           swap(ckey.stbrett, var[i], x);
                           swap(ckey.stbrett, var[k], z);
                           break;
                         default:
                           break;
                       }
                     }
                   }
                 }


                 a = triscore(ckey.stbrett, ciphertext, len) + biscore(ckey.stbrett, ciphertext, len);
                 if (a > jbestscore) {
                   jbestscore = a;
                   newtop = 1;
                 }

               }


               get_stecker(&ckey);
               bestscore = triscore(ckey.stbrett, ciphertext, len);

               newtop = 1;

               while (newtop) {

                 newtop = 0;
                 action = NONE;

                 /* try reswapping each self-steckered with each pair,
                  * steepest ascent */
                 for (i = 0; i < ckey.count; i += 2) {
                   swap(ckey.stbrett, ckey.sf[i], ckey.sf[i+1]);
                   for (k = ckey.count; k < 26; k++) {
                     swap(ckey.stbrett, ckey.sf[i], ckey.sf[k]);
                     a = triscore(ckey.stbrett, ciphertext, len);
                     if (a > bestscore) {
                       newtop = 1;
                       action = RESWAP;
                       bestscore = a;
                       ch.u1 = i;
                       ch.u2 = i+1;
                       ch.s1 = k;
                       ch.s2 = i;
                     }
                     swap(ckey.stbrett, ckey.sf[i], ckey.sf[k]);
                     swap(ckey.stbrett, ckey.sf[i+1], ckey.sf[k]);
                     a = triscore(ckey.stbrett, ciphertext, len);
                     if (a > bestscore) {
                       newtop = 1;
                       action = RESWAP;
                       bestscore = a;
                       ch.u1 = i;
                       ch.u2 = i+1;
                       ch.s1 = k;
                       ch.s2 = i+1;
                     }
                     swap(ckey.stbrett, ckey.sf[i+1], ckey.sf[k]);
                   }
                   swap(ckey.stbrett, ckey.sf[i], ckey.sf[i+1]);
                 }
                 if (action == RESWAP) {
                   swap(ckey.stbrett, ckey.sf[ch.u1], ckey.sf[ch.u2]);
                   swap(ckey.stbrett, ckey.sf[ch.s1], ckey.sf[ch.s2]);
                   get_stecker(&ckey);
                 }
                 action = NONE;

               }


               /* record global max, if applicable */
               if (bestscore > globalscore) {
                 globalscore = bestscore;
                 gkey = ckey;
                 gkey.score = bestscore;
                 print_key(outfile, &gkey);
                 print_plaintext(outfile, gkey.stbrett, ciphertext, len);
                 if (ferror(outfile) != 0) {
                   fputs("enigma: error: writing to result file failed\n", stderr);
                   exit(EXIT_FAILURE);
                 }
               }
               /* abort if max_score is reached */
               if (globalscore > max_score)
                 goto FINISHED;
                

               ENDLOOP:
               if (firstloop) {
                 firstloop = 0;
                 init_key_low(&lo, m);
               }
               if (keycmp(&ckey, to) == 0)
                 goto RESTART;

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

   /* choose random order of testing stecker */
   RESTART:
   firstpass = 0;
   rand_var(var);

  }

  FINISHED:
  if (resume)
    hillclimb_log("enigma: finished range");
  if (act_on_sig)
    save_state_exit(state, EXIT_SUCCESS);

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
