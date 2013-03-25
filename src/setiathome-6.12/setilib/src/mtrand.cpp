/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.
 
   Before using, initialize the state by using mtrand::srandom(seed)  
   or mtrand::init_by_array(init_key, key_length).
 
   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          
 
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
 
     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
 
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
 
     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.
 
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 
   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

#include <cstdio>
#include <ctime>
#include <cstring>
#include "mtrand.h"


char* MtRand::toString() {
  char retStr[300];
  char appendStr[30];
  sprintf(retStr, "mt array:");
  for(int i = 0; i < 2; i++) {
    if(i % 5 == 0) { strcat(retStr, "\n"); }
    sprintf(appendStr, " mt[%d] = %ld", i, mt[i]);
    strcat(retStr, appendStr);
  }
  return retStr;
}

/* Period parameters */

/* initializes mt[MTRAND_N] with a seed */
void MtRand::srandom(unsigned long s) {
  mt[0]= s & 0xffffffffUL;
  for (mti=1; mti<MTRAND_N; mti++) {
    mt[mti] =
      (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
    /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
    /* In the previous versions, MSBs of the seed affect   */
    /* only MSBs of the array mt[].                        */
    /* 2002/01/09 modified by Makoto Matsumoto             */
    mt[mti] &= 0xffffffffUL;
    /* for >32 bit machines */
  }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void MtRand::init_by_array(unsigned long init_key[], unsigned long key_length) {
  int i, j, k;
  srandom(19650218UL);
  i=1; j=0;
  k = (MTRAND_N>key_length ? MTRAND_N : key_length);
  for (; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
            + init_key[j] + j; /* non linear */
    mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
    i++; j++;
    if (i>=MTRAND_N) { mt[0] = mt[MTRAND_N-1]; i=1; }
    if (j>=key_length) j=0;
  }
  for (k=MTRAND_N-1; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
            - i; /* non linear */
    mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
    i++;
    if (i>=MTRAND_N) { mt[0] = mt[MTRAND_N-1]; i=1; }
  }

  mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}

MtRand::MtRand() {
  mti=MTRAND_N+1;
  srandom(time(0));
}

MtRand::MtRand(unsigned long s) {
  mti=MTRAND_N+1;
  srandom(s);
}

MtRand::MtRand(unsigned long init_key[], unsigned long key_length) {
  mti=MTRAND_N+1;
  init_by_array(init_key,key_length);
}


/* generates a random number on [0,0xffffffff]-interval */
unsigned long MtRand::random32() {
  unsigned long y;
  static unsigned long mag01[2]={0x0UL, MTRAND_MATRIX_A};
  /* mag01[x] = x * MTRAND_MATRIX_A  for x=0,1 */

  if (mti >= MTRAND_N) { /* generate MTRAND_N words at one time */
    int kk;

    if (mti == MTRAND_N+1)   /* if init_genrand() has not been called, */
      srandom(5489UL); /* a default initial seed is used */

    for (kk=0;kk<MTRAND_N-MTRAND_M;kk++) {
      y = (mt[kk]&MTRAND_UPPER_MASK)|(mt[kk+1]&MTRAND_LOWER_MASK);
      mt[kk] = mt[kk+MTRAND_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (;kk<MTRAND_N-1;kk++) {
      y = (mt[kk]&MTRAND_UPPER_MASK)|(mt[kk+1]&MTRAND_LOWER_MASK);
      mt[kk] = mt[kk+(MTRAND_M-MTRAND_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (mt[MTRAND_N-1]&MTRAND_UPPER_MASK)|(mt[0]&MTRAND_LOWER_MASK);
    mt[MTRAND_N-1] = mt[MTRAND_M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

    mti = 0;
  }

  y = mt[mti++];

  /* Tempering */
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long MtRand::random31() {
  return (long)(random32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double MtRand::drand32_complete(void) {
  return random32()*(1.0/4294967295.0);
  /* divided by 2^32-1 */
}

/* generates a random number on [0,1)-real-interval */
double MtRand::drand32(void) {
  return random32()*(1.0/4294967296.0);
  /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double MtRand::drand32_incomplete(void) {
  return (((double)random32()) + 0.5)*(1.0/4294967296.0);
  /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double MtRand::drand53(void) {
  unsigned long a=random32()>>5, b=random32()>>6;
  return(a*67108864.0+b)*(1.0/9007199254740992.0);
}
/* These real versions are due to Isaku Wada, 2002/01/09 added */


