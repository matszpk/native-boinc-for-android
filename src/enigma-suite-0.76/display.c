#include <stdio.h>
#include <stdlib.h>


void usage(void)
{
  fprintf(stderr, "enigma: usage: enigma \
[ -hvicxaR ] \
[ -M model ] \
[ -u umkehrwalze ] \
[ -w wheel-order ] \
[ -r ring-settings ] \
[ -s stecker-pairs ] \
[ -m message-key ] \
[ -o output-file ] \
[ -n passes ] \
[ -z maximum-score ] \
[ -k single-key ] \
[ -f lower-bound ] \
[ -t upper-bound ] \
[ trigram-dictionary bigram-dictionary ciphertext ] \
\n\nUse enigma -h for detailed help\n\n");

  exit(EXIT_FAILURE);
}


void help(void)
{
  fprintf(stderr, "\nOptions:\n\n\
-M      Machine Type. Valid settings are H, M3 or M4. Defaults to H (Heer).\n\
        This must be the first argument on the command line!\n\n\
-u      Umkehrwalze. Select A (Heer only), B or C. Defaults to B.\n\n\
-w      Wheel Order. Valid settings are 123 thru G876.\n\
        Left letter [B,G] stands for wheels Beta or Gamma (M4 only).\n\
        Left digit stands for left wheel.\n\
        Middle digit stands for middle wheel.\n\
        Right digit stands for right wheel.\n\
        Digits must not be repeated. Defaults to 123.\n\n\
-r      Ring Settings. Valid settings are AAA thru ZZZZ. Defaults to AAA.\n\n\
-s      Stecker Pairs. Valid settings are 1 to 13 pairs of letters\n\
        Letters must not be repeated! Defaults to none\n\n\
-m      Message Key. Valid settings are AAA thru ZZZZ. Defaults to AAA.\n\n\
-o      Output file. Defaults to stdout.\n\n\
-i      Break an enigma message using ic statistics. Requires command line \n\
        arguments <trigram-dict>, <bigram-dict> and <ciphertext>.\n\n\
-c      Break an enigma message using hill climbing. Requires command line\n\
        arguments <trigram-dict>, <bigram-dict> and <ciphertext>.\n\
        This can take a really long time!\n\n\
-x      In combination with -c:  Iterate thru all middle wheel ring settings, excluding A\n\n\
-a      In combination with -c:  Iterate thru all middle wheel ring settings\n\n\
-n      In combination with -c:  Number of passes. Defaults to 1\n\n\
-z      In combination with -c:  Terminate if score n is reached. Defaults to INT_MAX.\n\n\
-k      In combination with -c:  Perform hill climb on a single key.\n\n\
-R      Standalone option:  Resume a hill climb, reading the previous state from 00hc.resume.\n\n\
-f      In combination with -i,-c:  Search from\n\n\
-t      In combination with -i,-c:  Search up to\n\n\
-h      Display this help screen.\n\n\
-v      Display version information.\n\n\n\
Examples:\n\n\
    enigma -M M3 -u C -w 781 -r GHL -s QWERTYUIOPASDFGHJKLZ -m WSK\n\
    enigma -M H -i -f \"B:456:AA:FFF\" -t \"C:123:AA:BCF\" trigram-dict bigram-dict ciphertext\n\
    enigma -M M4 -ca -n 300 -f \"B:B547:CD:JKNH\" -t \"C:G128:JH:REWQ\" trigram-dict bigram-dict ciphertext\n\n");

  exit(EXIT_SUCCESS);
}


void version(void)
{
  fprintf(stdout, "enigma-suite version 0.76\n\
Copyright (C) 2005 Stefan Krah <stefan@bytereef.org>\n\n\
    This program is free software; you can redistribute it and/or modify it\n\
    under the terms of version 2 (only) of the GNU General Public License as\n\
    published by the Free Software Foundation.\n\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\n\
    You should have received a copy of the GNU General Public License\n\
    along with this program; if not, write to the Free Software\n\
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.\n\n\
To report bugs or offer suggestions, please mail to <enigma-suite@bytereef.org>.\n\n");

  exit(EXIT_SUCCESS);
}


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
