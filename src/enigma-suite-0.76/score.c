#include "cipher.h"
#include "key.h"
#include "score.h"


extern int tridict[][26][26];
extern int bidict[][26];
extern int unidict[26];
extern int path_lookup[][26];

/* returns the trigram score of a key/ciphertext combination */
int get_triscore(const Key *key, const int *ciphertext, int len)
{
  init_path_lookup_ALL(key, len);
  return triscore(key->stbrett, ciphertext, len);
}


#ifdef SIMPLESCORE
uint64_t icscore(const int *stbrett, const int *ciphertext, int len)
{
  int f[26] = {0};
  double S = 0;
  int i;
  int c;

  if (len < 2)
    return 0;

  for (i = 0; i < len; i++) {
    c = stbrett[ciphertext[i]];
    c = path_lookup[i][c];
    c = stbrett[c];
    f[c]++;
  }

  for (i = 0; i < 26; i++)
    S += f[i]*(f[i]-1);
  S /= len*(len-1);

  return S;
}


int uniscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c;
  int s = 0;

  for (i = 0; i < len; i++) {
    c = stbrett[ciphertext[i]];
    c = path_lookup[i][c];
    c = stbrett[c];
    s += unidict[c];
  }

  return s;
}


int biscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c1, c2;
  int s = 0;

  c1 = stbrett[ciphertext[0]];
  c1 = path_lookup[0][c1];
  c1 = stbrett[c1];

  for (i = 1; i < len; i++) {
    c2 = stbrett[ciphertext[i]];
    c2 = path_lookup[i][c2];
    c2 = stbrett[c2];
    s += bidict[c1][c2];

    c1 = c2;
  }

  return s;

}

int triscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c1, c2, c3;
  int s = 0;

  c1 = stbrett[ciphertext[0]];
  c1 = path_lookup[0][c1];
  c1 = stbrett[c1];

  c2 = stbrett[ciphertext[1]];
  c2 = path_lookup[1][c2];
  c2 = stbrett[c2];

  for (i = 2; i < len; i++) {
    c3 = stbrett[ciphertext[i]];
    c3 = path_lookup[i][c3];
    c3 = stbrett[c3];
    s += tridict[c1][c2][c3];

    c1 = c2;
    c2 = c3;
  }

  return s;
}
#endif


#ifndef SIMPLESCORE
int icscore(const int *stbrett, const int *ciphertext, int len)
{
  int f[26] = {0};
  int S0, S1, S2, S3;
  int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16;
  int i;

  if (len < 2)
    return 0;

  for (i = 0; i < len-15; i += 16) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];

    c2 = stbrett[ciphertext[i+1]];
    c2 = path_lookup[i+1][c2];
    c2 = stbrett[c2];

    c3 = stbrett[ciphertext[i+2]];
    c3 = path_lookup[i+2][c3];
    c3 = stbrett[c3];

    c4 = stbrett[ciphertext[i+3]];
    c4 = path_lookup[i+3][c4];
    c4 = stbrett[c4];

    c5 = stbrett[ciphertext[i+4]];
    c5 = path_lookup[i+4][c5];
    c5 = stbrett[c5];

    c6 = stbrett[ciphertext[i+5]];
    c6 = path_lookup[i+5][c6];
    c6 = stbrett[c6];

    c7 = stbrett[ciphertext[i+6]];
    c7 = path_lookup[i+6][c7];
    c7 = stbrett[c7];

    c8 = stbrett[ciphertext[i+7]];
    c8 = path_lookup[i+7][c8];
    c8 = stbrett[c8];

    c9 = stbrett[ciphertext[i+8]];
    c9 = path_lookup[i+8][c9];
    c9 = stbrett[c9];

    c10 = stbrett[ciphertext[i+9]];
    c10 = path_lookup[i+9][c10];
    c10 = stbrett[c10];

    c11 = stbrett[ciphertext[i+10]];
    c11 = path_lookup[i+10][c11];
    c11 = stbrett[c11];

    c12 = stbrett[ciphertext[i+11]];
    c12 = path_lookup[i+11][c12];
    c12 = stbrett[c12];

    c13 = stbrett[ciphertext[i+12]];
    c13 = path_lookup[i+12][c13];
    c13 = stbrett[c13];

    c14 = stbrett[ciphertext[i+13]];
    c14 = path_lookup[i+13][c14];
    c14 = stbrett[c14];

    c15 = stbrett[ciphertext[i+14]];
    c15 = path_lookup[i+14][c15];
    c15 = stbrett[c15];

    c16 = stbrett[ciphertext[i+15]];
    c16 = path_lookup[i+15][c16];
    c16 = stbrett[c16];

    f[c1]++;
    f[c2]++;
    f[c3]++;
    f[c4]++;
    f[c5]++;
    f[c6]++;
    f[c7]++;
    f[c8]++;
    f[c9]++;
    f[c10]++;
    f[c11]++;
    f[c12]++;
    f[c13]++;
    f[c14]++;
    f[c15]++;
    f[c16]++;
  }
  for (; i < len-3; i += 4) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];

    c2 = stbrett[ciphertext[i+1]];
    c2 = path_lookup[i+1][c2];
    c2 = stbrett[c2];

    c3 = stbrett[ciphertext[i+2]];
    c3 = path_lookup[i+2][c3];
    c3 = stbrett[c3];

    c4 = stbrett[ciphertext[i+3]];
    c4 = path_lookup[i+3][c4];
    c4 = stbrett[c4];

    f[c1]++;
    f[c2]++;
    f[c3]++;
    f[c4]++;
  }
  for (; i < len; i++) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];
    f[c1]++;
  }


  S0 = S1 = S2 = S3 = 0;
  for (i = 0; i < 23; i += 4) {
    S0 += f[i]*(f[i]-1);
    S1 += f[i+1]*(f[i+1]-1);
    S2 += f[i+2]*(f[i+2]-1);
    S3 += f[i+3]*(f[i+3]-1);
  }
  S0 += f[24]*(f[24]-1);
  S1 += f[25]*(f[25]-1);

  return (S0+S1) + (S2+S3);
}


int uniscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16;
  int s0, s1, s2, s3;


  s0 = s1 = s2 = s3 = 0;
  for (i = 0; i < len-15; i += 16) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];
    s0 += unidict[c1];

    c2 = stbrett[ciphertext[i+1]];
    c2 = path_lookup[i+1][c2];
    c2 = stbrett[c2];
    s1 += unidict[c2];

    c3 = stbrett[ciphertext[i+2]];
    c3 = path_lookup[i+2][c3];
    c3 = stbrett[c3];
    s2 += unidict[c3];

    c4 = stbrett[ciphertext[i+3]];
    c4 = path_lookup[i+3][c4];
    c4 = stbrett[c4];
    s3 += unidict[c4];

    c5 = stbrett[ciphertext[i+4]];
    c5 = path_lookup[i+4][c5];
    c5 = stbrett[c5];
    s0 += unidict[c5];

    c6 = stbrett[ciphertext[i+5]];
    c6 = path_lookup[i+5][c6];
    c6 = stbrett[c6];
    s1 += unidict[c6];

    c7 = stbrett[ciphertext[i+6]];
    c7 = path_lookup[i+6][c7];
    c7 = stbrett[c7];
    s2 += unidict[c7];

    c8 = stbrett[ciphertext[i+7]];
    c8 = path_lookup[i+7][c8];
    c8 = stbrett[c8];
    s3 += unidict[c8];

    c9 = stbrett[ciphertext[i+8]];
    c9 = path_lookup[i+8][c9];
    c9 = stbrett[c9];
    s0 += unidict[c9];

    c10 = stbrett[ciphertext[i+9]];
    c10 = path_lookup[i+9][c10];
    c10 = stbrett[c10];
    s1 += unidict[c10];

    c11 = stbrett[ciphertext[i+10]];
    c11 = path_lookup[i+10][c11];
    c11 = stbrett[c11];
    s2 += unidict[c11];

    c12 = stbrett[ciphertext[i+11]];
    c12 = path_lookup[i+11][c12];
    c12 = stbrett[c12];
    s3 += unidict[c12];

    c13 = stbrett[ciphertext[i+12]];
    c13 = path_lookup[i+12][c13];
    c13 = stbrett[c13];
    s0 += unidict[c13];

    c14 = stbrett[ciphertext[i+13]];
    c14 = path_lookup[i+13][c14];
    c14 = stbrett[c14];
    s1 += unidict[c14];

    c15 = stbrett[ciphertext[i+14]];
    c15 = path_lookup[i+14][c15];
    c15 = stbrett[c15];
    s2 += unidict[c15];

    c16 = stbrett[ciphertext[i+15]];
    c16 = path_lookup[i+15][c16];
    c16 = stbrett[c16];
    s3 += unidict[c16];
  }
  for (; i < len-3; i += 4) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];
    s0 += unidict[c1];

    c2 = stbrett[ciphertext[i+1]];
    c2 = path_lookup[i+1][c2];
    c2 = stbrett[c2];
    s1 += unidict[c2];

    c3 = stbrett[ciphertext[i+2]];
    c3 = path_lookup[i+2][c3];
    c3 = stbrett[c3];
    s2 += unidict[c3];

    c4 = stbrett[ciphertext[i+3]];
    c4 = path_lookup[i+3][c4];
    c4 = stbrett[c4];
    s3 += unidict[c4];
  }
  for (; i < len; i++) {
    c1 = stbrett[ciphertext[i]];
    c1 = path_lookup[i][c1];
    c1 = stbrett[c1];
    s0 += unidict[c1];
  }

  return (s0+s1) + (s2+s3);

}


int biscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17;
  int s0, s1, s2, s3;


  c1 = stbrett[ciphertext[0]];
  c1 = path_lookup[0][c1];
  c1 = stbrett[c1];

  s0 = s1 = s2 = s3 = 0;
  for (i = 1; i < len-15; i += 16) {
    c2 = stbrett[ciphertext[i]];
    c2 = path_lookup[i][c2];
    c2 = stbrett[c2];
    s0 += bidict[c1][c2];

    c3 = stbrett[ciphertext[i+1]];
    c3 = path_lookup[i+1][c3];
    c3 = stbrett[c3];
    s1 += bidict[c2][c3];

    c4 = stbrett[ciphertext[i+2]];
    c4 = path_lookup[i+2][c4];
    c4 = stbrett[c4];
    s2 += bidict[c3][c4];

    c5 = stbrett[ciphertext[i+3]];
    c5 = path_lookup[i+3][c5];
    c5 = stbrett[c5];
    s3 += bidict[c4][c5];

    c6 = stbrett[ciphertext[i+4]];
    c6 = path_lookup[i+4][c6];
    c6 = stbrett[c6];
    s0 += bidict[c5][c6];

    c7 = stbrett[ciphertext[i+5]];
    c7 = path_lookup[i+5][c7];
    c7 = stbrett[c7];
    s1 += bidict[c6][c7];

    c8 = stbrett[ciphertext[i+6]];
    c8 = path_lookup[i+6][c8];
    c8 = stbrett[c8];
    s2 += bidict[c7][c8];

    c9 = stbrett[ciphertext[i+7]];
    c9 = path_lookup[i+7][c9];
    c9 = stbrett[c9];
    s3 += bidict[c8][c9];

    c10 = stbrett[ciphertext[i+8]];
    c10 = path_lookup[i+8][c10];
    c10 = stbrett[c10];
    s0 += bidict[c9][c10];

    c11 = stbrett[ciphertext[i+9]];
    c11 = path_lookup[i+9][c11];
    c11 = stbrett[c11];
    s1 += bidict[c10][c11];

    c12 = stbrett[ciphertext[i+10]];
    c12 = path_lookup[i+10][c12];
    c12 = stbrett[c12];
    s2 += bidict[c11][c12];

    c13 = stbrett[ciphertext[i+11]];
    c13 = path_lookup[i+11][c13];
    c13 = stbrett[c13];
    s3 += bidict[c12][c13];

    c14 = stbrett[ciphertext[i+12]];
    c14 = path_lookup[i+12][c14];
    c14 = stbrett[c14];
    s0 += bidict[c13][c14];

    c15 = stbrett[ciphertext[i+13]];
    c15 = path_lookup[i+13][c15];
    c15 = stbrett[c15];
    s1 += bidict[c14][c15];

    c16 = stbrett[ciphertext[i+14]];
    c16 = path_lookup[i+14][c16];
    c16 = stbrett[c16];
    s2 += bidict[c15][c16];

    c17 = stbrett[ciphertext[i+15]];
    c17 = path_lookup[i+15][c17];
    c17 = stbrett[c17];
    s3 += bidict[c16][c17];

    c1 = c17;
  }
  for (; i < len-3; i += 4) {
    c2 = stbrett[ciphertext[i]];
    c2 = path_lookup[i][c2];
    c2 = stbrett[c2];
    s0 += bidict[c1][c2];

    c3 = stbrett[ciphertext[i+1]];
    c3 = path_lookup[i+1][c3];
    c3 = stbrett[c3];
    s1 += bidict[c2][c3];

    c4 = stbrett[ciphertext[i+2]];
    c4 = path_lookup[i+2][c4];
    c4 = stbrett[c4];
    s2 += bidict[c3][c4];

    c5 = stbrett[ciphertext[i+3]];
    c5 = path_lookup[i+3][c5];
    c5 = stbrett[c5];
    s3 += bidict[c4][c5];

    c1 = c5;
  }
  for (; i < len; i++) {
    c2 = stbrett[ciphertext[i]];
    c2 = path_lookup[i][c2];
    c2 = stbrett[c2];
    s0 += bidict[c1][c2];

    c1 = c2;
  }
  
  return (s0+s1) + (s2+s3);

}


int triscore(const int *stbrett, const int *ciphertext, int len)
{
  int i;
  int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17, c18;
  int s0, s1, s2, s3;


  c1 = stbrett[ciphertext[0]];
  c1 = path_lookup[0][c1];
  c1 = stbrett[c1];

  c2 = stbrett[ciphertext[1]];
  c2 = path_lookup[1][c2];
  c2 = stbrett[c2];

  s0 = s1 = s2 = s3 = 0;
  for (i = 2; i < len-15; i += 16) {
    c3 = stbrett[ciphertext[i]];
    c3 = path_lookup[i][c3];
    c3 = stbrett[c3];
    s0 += tridict[c1][c2][c3];

    c4 = stbrett[ciphertext[i+1]];
    c4 = path_lookup[i+1][c4];
    c4 = stbrett[c4];
    s1 += tridict[c2][c3][c4];

    c5 = stbrett[ciphertext[i+2]];
    c5 = path_lookup[i+2][c5];
    c5 = stbrett[c5];
    s2 += tridict[c3][c4][c5];

    c6 = stbrett[ciphertext[i+3]];
    c6 = path_lookup[i+3][c6];
    c6 = stbrett[c6];
    s3 += tridict[c4][c5][c6];

    c7 = stbrett[ciphertext[i+4]];
    c7 = path_lookup[i+4][c7];
    c7 = stbrett[c7];
    s0 += tridict[c5][c6][c7];

    c8 = stbrett[ciphertext[i+5]];
    c8 = path_lookup[i+5][c8];
    c8 = stbrett[c8];
    s1 += tridict[c6][c7][c8];

    c9 = stbrett[ciphertext[i+6]];
    c9 = path_lookup[i+6][c9];
    c9 = stbrett[c9];
    s2 += tridict[c7][c8][c9];

    c10 = stbrett[ciphertext[i+7]];
    c10 = path_lookup[i+7][c10];
    c10 = stbrett[c10];
    s3 += tridict[c8][c9][c10];

    c11 = stbrett[ciphertext[i+8]];
    c11 = path_lookup[i+8][c11];
    c11 = stbrett[c11];
    s0 += tridict[c9][c10][c11];

    c12 = stbrett[ciphertext[i+9]];
    c12 = path_lookup[i+9][c12];
    c12 = stbrett[c12];
    s1 += tridict[c10][c11][c12];

    c13 = stbrett[ciphertext[i+10]];
    c13 = path_lookup[i+10][c13];
    c13 = stbrett[c13];
    s2 += tridict[c11][c12][c13];

    c14 = stbrett[ciphertext[i+11]];
    c14 = path_lookup[i+11][c14];
    c14 = stbrett[c14];
    s3 += tridict[c12][c13][c14];

    c15 = stbrett[ciphertext[i+12]];
    c15 = path_lookup[i+12][c15];
    c15 = stbrett[c15];
    s0 += tridict[c13][c14][c15];

    c16 = stbrett[ciphertext[i+13]];
    c16 = path_lookup[i+13][c16];
    c16 = stbrett[c16];
    s1 += tridict[c14][c15][c16];

    c17 = stbrett[ciphertext[i+14]];
    c17 = path_lookup[i+14][c17];
    c17 = stbrett[c17];
    s2 += tridict[c15][c16][c17];

    c18 = stbrett[ciphertext[i+15]];
    c18 = path_lookup[i+15][c18];
    c18 = stbrett[c18];
    s3 += tridict[c16][c17][c18];

    c1 = c17;
    c2 = c18;
  }
  for (; i < len-3; i += 4) {
    c3 = stbrett[ciphertext[i]];
    c3 = path_lookup[i][c3];
    c3 = stbrett[c3];
    s0 += tridict[c1][c2][c3];

    c4 = stbrett[ciphertext[i+1]];
    c4 = path_lookup[i+1][c4];
    c4 = stbrett[c4];
    s1 += tridict[c2][c3][c4];

    c5 = stbrett[ciphertext[i+2]];
    c5 = path_lookup[i+2][c5];
    c5 = stbrett[c5];
    s2 += tridict[c3][c4][c5];

    c6 = stbrett[ciphertext[i+3]];
    c6 = path_lookup[i+3][c6];
    c6 = stbrett[c6];
    s3 += tridict[c4][c5][c6];

    c1 = c5;
    c2 = c6;
  }
  for (; i < len; i++) {
    c3 = stbrett[ciphertext[i]];
    c3 = path_lookup[i][c3];
    c3 = stbrett[c3];
    s0 += tridict[c1][c2][c3];

    c1 = c2;
    c2 = c3;
  }
  
  return (s0+s1) + (s2+s3);

}
#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
