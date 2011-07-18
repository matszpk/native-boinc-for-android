#include <stdio.h>
#include <stdlib.h>
#include "date.h"
#include "error.h"


void err_alloc_fatal(const char *s)
{
  fprintf(stderr, "enigma: error while allocating %s\n", s);
  exit (EXIT_FAILURE);
}

void err_open_fatal(const char *s)
{
  fprintf(stderr, "enigma: error: could not open %s\n", s);
  exit(EXIT_FAILURE);
}

void err_open_fatal_resume(const char *s)
{
  char date[DATELEN];

  datestring(date);
  fprintf(stderr, "%s  enigma: need %s\n", date, s);
  exit(EXIT_FAILURE);
}

void err_stream_fatal(const char *s)
{
  fprintf(stderr, "enigma: error on open file %s\n", s);
  exit(EXIT_FAILURE);
}

void err_illegal_char_fatal(const char *s)
{
  fprintf(stderr, "enigma: error: illegal characters in %s.\n", s);
  exit(EXIT_FAILURE);
}

void err_sigaction_fatal(int signum)
{
  fprintf(stderr, "enigma: error: could not install signal handler for signal %d\n", signum);
  exit(EXIT_FAILURE);
}

void err_input_fatal(int type)
{
  switch (type) {
    case ERR_A_ONLY:
      fputs("enigma: error: use -a or -x if range is specified for middle ring\n", stderr);
      break;
    case ERR_EXCL_A:
      fputs("enigma: error: 'A' cannot be used as range for middle ring in combination with -x\n", stderr);
      break;
    case ERR_RING_SHORTCUT:
      fputs("enigma: error: in the keystring, wheels 6-8 cannot be combined with rings greater than 'M'\n", stderr);
      break;
    default:
      break;
  }

  exit(EXIT_FAILURE);
}


/* log messages */
void hillclimb_log(const char *s)
{
  char date[DATELEN];

  datestring(date);
  fprintf(stderr, "%s  %s\n", date, s);
  fflush(stderr);
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
