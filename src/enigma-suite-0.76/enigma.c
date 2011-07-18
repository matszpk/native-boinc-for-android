#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "charmap.h"
#include "cipher.h"
#include "ciphertext.h"
#include "dict.h"
#include "display.h"
#include "error.h"
#include "global.h"
#include "hillclimb.h"
#include "ic.h"
#include "input.h"
#include "key.h"
#include "result.h"
#include "resume_in.h"
#include "resume_out.h"
#include "scan.h"


int main(int argc, char **argv)
{
  Key key;
  Key from, to, ckey_res, gkey_res;
  int *ciphertext, len, clen;
  int model = H;
  int opt, first = 1, keyop = 0;
  int hc = 0, ic = 0;
  int sw_mode = SW_ONSTART;
  int max_pass = 1, firstpass = 1;
  int max_score = INT_MAX-1, resume = 0, maxargs;
  FILE *outfile = stdout;
  char *f = NULL, *t = NULL, *k = NULL;
  char *fmin[3] = {
    "A:123:AA:AAA",
    "B:123:AA:AAA",
    "B:B123:AA:AAAA"
  };
  char *tmax[3] = {
    "C:543:ZZ:ZZZ",
    "C:876:MM:ZZZ",
    "C:G876:MM:ZZZZ"
  };


  init_key_default(&key, model);
  init_charmap();

  opterr = 0;
  while ((opt = getopt(argc, argv, "hvicxaRM:w:r:m:u:s:f:t:k:n:z:o:")) != -1) {
    switch (opt) {
      case 'h': help(); break;
      case 'v': version(); break;
      case 'u': if (!set_ukw(&key, optarg, model)) usage(); keyop = 1; break;
      case 'w': if (!set_walze(&key, optarg, model)) usage(); keyop = 1; break;
      case 'r': if (!set_ring(&key, optarg, model)) usage(); keyop = 1; break;
      case 'm': if (!set_mesg(&key, optarg, model)) usage(); keyop = 1; break;
      case 's': if (!set_stecker(&key, optarg)) usage(); keyop = 1; break;
      case 'c': hc = 1; break;
      case 'i': ic = 1; break;
      case 'f': f = optarg; break;
      case 't': t = optarg; break;
      case 'k': k = optarg; break;
      case 'x': if (sw_mode != SW_ONSTART) usage(); sw_mode = SW_OTHER; break;
      case 'a': if (sw_mode != SW_ONSTART) usage(); sw_mode = SW_ALL; break;
      case 'R': resume = 1; hc = 1; break;
      case 'n': if ((max_pass = scan_posint(optarg)) == -1) usage(); break;
      case 'z': if ((max_score = scan_posint(optarg)) == -1) usage(); break;
      case 'o': if (!(outfile = open_outfile(optarg))) usage(); break;
      case 'M': if ((model = get_model(optarg)) == -1 || !first) usage();
                if (!init_key_default(&key, model)) usage(); break;
      default: usage();
    }
    first = 0;
  }


  if (hc == 0 && ic == 0) {
    en_deciph_stdin_ALL(outfile, &key);
    return 0;
  }


  if (argc-optind != 3) usage();
  load_tridict(argv[optind++]);
  load_bidict(argv[optind++]);
  ciphertext = load_ciphertext(argv[optind], &len, resume);
  if (len < 3) exit(EXIT_FAILURE);


  if (ic == 1) {
    if (keyop == 1) usage();
    if (resume == 1) usage();
    if (k != NULL) usage();
    if (f == NULL && t == NULL) {
      f = fmin[model];
      t = tmax[model];

      if (!set_range(&from, &to, f, t, model)) usage();

      /* no range given, first try fast noring option */
      ic_noring(&from, &to, NULL, NULL, SINGLE_KEY, 300, 1, max_score, resume, outfile, 0, ciphertext, len);
      ic_allring(&from, &to, NULL, NULL, SINGLE_KEY, 300, 1, max_score, resume, outfile, 0, ciphertext, len);
    }
    else {
      if (f == NULL) f = fmin[model];
      if (t == NULL) t = tmax[model];

      if (!set_range(&from, &to, f, t, model)) usage();

      /* 300 passes hard wired */
      ic_allring(&from, &to, NULL, NULL, SINGLE_KEY, 300, 1, max_score, resume, outfile, 0, ciphertext, len);
    }
  }

  if (hc == 1) {
    if (keyop == 1) usage();
    if (!resume) {
      if (k != NULL) {
        if (f != NULL || t != NULL) usage();
        if (!set_key(&from, k, model, 1)) usage();
        to = from;
        sw_mode = SINGLE_KEY;
      }
      else {
        if (f == NULL) f = fmin[model];
        if (t == NULL) t = tmax[model];
        if (!set_range(&from, &to, f, t, model)) usage();
      }
    }
    else {
      /* only -o option is allowed in addition to -R */
      maxargs = (outfile == stdout) ? 5 : 7;
      if (argc != maxargs) usage();
      if (!set_state(&from, &to, &ckey_res, &gkey_res, &sw_mode, &max_pass, &firstpass, &max_score)) {
        fputs("enigma: error: resume file is not in the right format\n", stderr);
        exit(EXIT_FAILURE);
      }
    }

    clen = (len < CT) ? len : CT;

    hillclimb( &from, &to, &ckey_res, &gkey_res, sw_mode, max_pass, firstpass,
                max_score, resume, outfile, 1, ciphertext, clen );

  }

  if (outfile != stdout)
    fclose(outfile);
  free(ciphertext);
  return 0;

}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
