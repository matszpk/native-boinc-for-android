// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public 
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend 
// this exception to your version of the file, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.


/*
 * $Id: pulsefind.cpp,v 1.16.2.16 2007/08/10 00:38:48 korpela Exp $
 *
 */

#include "sah_config.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cstring>
#include <ctime>

#include "diagnostics.h"
#include "util.h"
#include "s_util.h"
#include "boinc_api.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

#include "seti.h"
#include "seti_header.h"
#include "analyzePoT.h"
#include "analyzeReport.h"
#include "worker.h"
#include "malloc_a.h"
#include "lcgamm.h"
#include "pulsefind.h"

//#define DEBUG_TRIPLET_VERBOSE
//#define DEBUG_PULSE_VERBOSE
//#define DEBUG_PULSE_BOUNDS


/**********************
 *
 *   find_triplets - Analyzes the input signal for a triplet signal.
 *   
 *   Written by Eric Heien.
 */
int find_triplets( const float *power, int len_power, float triplet_thresh, int time_bin, int freq_bin ) {
  static int    *binsAboveThreshold;
  int            i,n,numBinsAboveThreshold=0,p,q,blksize;
  float        midpoint,mean_power=0,peak_power,period,total=0.0f,partial;

  if (!binsAboveThreshold) {
    binsAboveThreshold=(int *)malloc_a(PoTInfo.TripletMax*sizeof(int),MEM_ALIGN);
    if (!binsAboveThreshold) SETIERROR(MALLOC_FAILED, "!binsAboveThreshold");
  }

  /* Get all the bins that are above the threshold, and find the power array mean value */
#ifdef DEBUG_TRIPLET_VERBOSE
  fprintf(stderr, "In find_triplets()...   PulsePotLen = %d   triplet_thresh = %f   TOffset = %d   PoT = %d\n",  len_power,  triplet_thresh, time_bin, freq_bin);
#endif

  blksize = UNSTDMAX(1, UNSTDMIN(pow2((unsigned int) sqrt((float) (len_power / 32)) * 32), 512));
  int b;
  for(b=0;b<len_power/blksize;b++) {
      partial = 0.0f;
	  for(i = 0; i < blksize; i++) {
		partial += power[b*blksize+i];
	  }
	  total += partial;
  }
  //catch the tail if needed
  i=0;
  while (b*blksize+i < len_power)
  {
	total += power[b*blksize+i]; 
	i++;
  }
  mean_power = total / (float) len_power;

  triplet_thresh*=mean_power;
  for( i=0;i<len_power;i++ ) {
    if( power[i] >= triplet_thresh ) {
      binsAboveThreshold[numBinsAboveThreshold] = i;
      numBinsAboveThreshold++;
    }
  }
  analysis_state.FLOP_counter+=10.0+len_power;

  /* Check each bin combination for a triplet */
  if (numBinsAboveThreshold>2) {
    for( i=0;i<numBinsAboveThreshold-1;i++ ) {
      for( n=i+1;n<numBinsAboveThreshold;n++ ) {
        midpoint = (binsAboveThreshold[i]+binsAboveThreshold[n])/2.0f;
        period = (float)fabs((binsAboveThreshold[i]-binsAboveThreshold[n])/2.0f);

        /* Get the peak power of this triplet */
        peak_power = power[binsAboveThreshold[i]];

        if( power[binsAboveThreshold[n]] > peak_power )
          peak_power = power[binsAboveThreshold[n]];

        p = binsAboveThreshold[i];
        while( (power[p] >= triplet_thresh) && (p <= (static_cast<int>(floor(midpoint)))) ) {    /* Check if there is a pulse "off" in between init and midpoint */
          p++;
        }

        q = static_cast<int>(floor(midpoint))+1;
        while( (power[q] >= triplet_thresh) && (q <= binsAboveThreshold[n])) {    /* Check if there is a pulse "off" in between midpoint and end */
          q++;
        }

        if( p >= static_cast<int>(floor(midpoint)) || q >= binsAboveThreshold[n]) {
          /* if this pulse doesn't have an "off" between all the three spikes, it's dropped */
        } else if( (midpoint - floor(midpoint)) > 0.1f ) {    /* if it's spread among two bins */
          if( power[(int)midpoint] >= triplet_thresh ) {
            if( power[(int)midpoint] > peak_power )
              peak_power = power[(int)midpoint];

            ReportTripletEvent( peak_power/mean_power, mean_power, period, midpoint,time_bin, freq_bin, len_power, power, 1 );
          }

          if( power[(int)(midpoint+1.0f)] >= triplet_thresh ) {
            if( power[(int)(midpoint+1.0f)] > peak_power )
              peak_power = power[(int)(midpoint+1.0f)];

            ReportTripletEvent( peak_power/mean_power, mean_power, period, midpoint,time_bin, freq_bin, len_power, power, 1 );
          }
        } else {            /* otherwise just check the single midpoint bin */
          if( power[(int)midpoint] >= triplet_thresh ) {
            if( power[(int)midpoint] > peak_power )
              peak_power = power[(int)midpoint];

            ReportTripletEvent( peak_power/mean_power, mean_power, period, midpoint,time_bin, freq_bin, len_power, power, 1 );
          }
        }
      }
    }
  }
  analysis_state.FLOP_counter+=(10.0*numBinsAboveThreshold*numBinsAboveThreshold);
  return (0);
}



/**********************
 *
 * Pulse finding
 *
 *
 * Default folding subroutines, FPU optimized
 *
 */
float sw_sum3_t31(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmax2, tmax1;
  int i = P->di;
  float *one   = ss[0];
  float *two   = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  tmax2 = tmax1 = float(0.0);

  if ( i & 1 ) {
    i -= 1;
    tmax1 = one[i] + two[i];
    tmax1 += three[i];
    P->dest[i] = tmax1;
  }
  switch (i) {
    case 30:
      sum1 = one[29] + two[29];           sum2 = one[28] + two[28];
      sum1 += three[29];                  sum2 += three[28];
      P->dest[29] = sum1;                 P->dest[28] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 28:
      sum1 = one[27] + two[27];           sum2 = one[26] + two[26];
      sum1 += three[27];                  sum2 += three[26];
      P->dest[27] = sum1;                 P->dest[26] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 26:
      sum1 = one[25] + two[25];           sum2 = one[24] + two[24];
      sum1 += three[25];                  sum2 += three[24];
      P->dest[25] = sum1;                 P->dest[24] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 24:
      sum1 = one[23] + two[23];           sum2 = one[22] + two[22];
      sum1 += three[23];                  sum2 += three[22];
      P->dest[23] = sum1;                 P->dest[22] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 22:
      sum1 = one[21] + two[21];           sum2 = one[20] + two[20];
      sum1 += three[21];                  sum2 += three[20];
      P->dest[21] = sum1;                 P->dest[20] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 20:
      sum1 = one[19] + two[19];           sum2 = one[18] + two[18];
      sum1 += three[19];                  sum2 += three[18];
      P->dest[19] = sum1;                 P->dest[18] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 18:
      sum1 = one[17] + two[17];           sum2 = one[16] + two[16];
      sum1 += three[17];                  sum2 += three[16];
      P->dest[17] = sum1;                 P->dest[16] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 16:
      sum1 = one[15] + two[15];           sum2 = one[14] + two[14];
      sum1 += three[15];                  sum2 += three[14];
      P->dest[15] = sum1;                 P->dest[14] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 14:
      sum1 = one[13] + two[13];           sum2 = one[12] + two[12];
      sum1 += three[13];                  sum2 += three[12];
      P->dest[13] = sum1;                 P->dest[12] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 12:
      sum1 = one[11] + two[11];           sum2 = one[10] + two[10];
      sum1 += three[11];                  sum2 += three[10];
      P->dest[11] = sum1;                 P->dest[10] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 10:
      sum1 = one[9] + two[9];             sum2 = one[8] + two[8];
      sum1 += three[9];                   sum2 += three[8];
      P->dest[9] = sum1;                  P->dest[8] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 8:
      sum1 = one[7] + two[7];             sum2 = one[6] + two[6];
      sum1 += three[7];                   sum2 += three[6];
      P->dest[7] = sum1;                  P->dest[6] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 6:
      sum1 = one[5] + two[5];             sum2 = one[4] + two[4];
      sum1 += three[5];                   sum2 += three[4];
      P->dest[5] = sum1;                  P->dest[4] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 4:
      sum1 = one[3] + two[3];             sum2 = one[2] + two[2];
      sum1 += three[3];                   sum2 += three[2];
      P->dest[3] = sum1;                  P->dest[2] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 2:
      sum1 = one[1] + two[1];             sum2 = one[0] + two[0];
      sum1 += three[1];                   sum2 += three[0];
      P->dest[1] = sum1;                  P->dest[0] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
  }
  if (tmax1 > tmax2) return tmax1;
  return tmax2;
}
//
// use for higher sum3 folds
//
float top_sum3(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmaxB, tmax;
  int   i;
  float *one = ss[0];
  float *two = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  tmaxB = tmax = float(0.0);

  for (i = 0 ; i < P->di-21; i += 22) {
    sum1 = one[i+0] + two[i+0];         sum2 = one[i+1] + two[i+1];
    sum1 += three[i+0];                 sum2 += three[i+1];
    P->dest[i+0] = sum1;                P->dest[i+1] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+2] + two[i+2];         sum2 = one[i+3] + two[i+3];
    sum1 += three[i+2];                 sum2 += three[i+3];
    P->dest[i+2] = sum1;                P->dest[i+3] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+4] + two[i+4];         sum2 = one[i+5] + two[i+5];
    sum1 += three[i+4];                 sum2 += three[i+5];
    P->dest[i+4] = sum1;                P->dest[i+5] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+6] + two[i+6];         sum2 = one[i+7] + two[i+7];
    sum1 += three[i+6];                 sum2 += three[i+7];
    P->dest[i+6] = sum1;                P->dest[i+7] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+8] + two[i+8];         sum2 = one[i+9] + two[i+9];
    sum1 += three[i+8];                 sum2 += three[i+9];
    P->dest[i+8] = sum1;                P->dest[i+9] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+10] + two[i+10];       sum2 = one[i+11] + two[i+11];
    sum1 += three[i+10];                sum2 += three[i+11];
    P->dest[i+10] = sum1;               P->dest[i+11] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+12] + two[i+12];       sum2 = one[i+13] + two[i+13];
    sum1 += three[i+12];                sum2 += three[i+13];
    P->dest[i+12] = sum1;               P->dest[i+13] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+14] + two[i+14];       sum2 = one[i+15] + two[i+15];
    sum1 += three[i+14];                sum2 += three[i+15];
    P->dest[i+14] = sum1;               P->dest[i+15] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+16] + two[i+16];       sum2 = one[i+17] + two[i+17];
    sum1 += three[i+16];                sum2 += three[i+17];
    P->dest[i+16] = sum1;               P->dest[i+17] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+18] + two[i+18];       sum2 = one[i+19] + two[i+19];
    sum1 += three[i+18];                sum2 += three[i+19];
    P->dest[i+18] = sum1;               P->dest[i+19] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+20] + two[i+20];       sum2 = one[i+21] + two[i+21];
    sum1 += three[i+20];                sum2 += three[i+21];
    P->dest[i+20] = sum1;               P->dest[i+21] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;
  }
  for (   ; i < P->di; i++) {
    sum1 = one[i] + two[i] + three[i];
    P->dest[i] = sum1;
    if (sum1 > tmax) tmax = sum1;
  }
  if (tmax > tmaxB) return tmax;
  return tmaxB;
}

sum_func swi3tb[FOLDTBLEN] = {
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, top_sum3
};
//
//
float sw_sum4_t31(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmax2, tmax1;
  int i = P->di;
  float *one   = ss[0];
  float *two   = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  float *four  = ss[0]+P->tmp2;
  tmax2 = tmax1 = float(0.0);

  if ( i & 1 ) {
    i -= 1;
    tmax1 = one[i] + two[i];            sum2 = three[i] + four[i];
    tmax1 += sum2;
    P->dest[i] = tmax1;
  }
  switch (i) {
    case 30:
      sum1 = one[29] + two[29];           sum2 = one[28] + two[28];
      sum1 += three[29];                  sum2 += three[28];
      sum1 += four[29];                   sum2 += four[28];
      P->dest[29] = sum1;                 P->dest[28] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 28:
      sum1 = one[27] + two[27];           sum2 = one[26] + two[26];
      sum1 += three[27];                  sum2 += three[26];
      sum1 += four[27];                   sum2 += four[26];
      P->dest[27] = sum1;                 P->dest[26] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 26:
      sum1 = one[25] + two[25];           sum2 = one[24] + two[24];
      sum1 += three[25];                  sum2 += three[24];
      sum1 += four[25];                   sum2 += four[24];
      P->dest[25] = sum1;                 P->dest[24] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 24:
      sum1 = one[23] + two[23];           sum2 = one[22] + two[22];
      sum1 += three[23];                  sum2 += three[22];
      sum1 += four[23];                   sum2 += four[22];
      P->dest[23] = sum1;                 P->dest[22] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 22:
      sum1 = one[21] + two[21];           sum2 = one[20] + two[20];
      sum1 += three[21];                  sum2 += three[20];
      sum1 += four[21];                   sum2 += four[20];
      P->dest[21] = sum1;                 P->dest[20] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 20:
      sum1 = one[19] + two[19];           sum2 = one[18] + two[18];
      sum1 += three[19];                  sum2 += three[18];
      sum1 += four[19];                   sum2 += four[18];
      P->dest[19] = sum1;                 P->dest[18] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 18:
      sum1 = one[17] + two[17];           sum2 = one[16] + two[16];
      sum1 += three[17];                  sum2 += three[16];
      sum1 += four[17];                   sum2 += four[16];
      P->dest[17] = sum1;                 P->dest[16] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 16:
      sum1 = one[15] + two[15];           sum2 = one[14] + two[14];
      sum1 += three[15];                  sum2 += three[14];
      sum1 += four[15];                   sum2 += four[14];
      P->dest[15] = sum1;                 P->dest[14] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 14:
      sum1 = one[13] + two[13];           sum2 = one[12] + two[12];
      sum1 += three[13];                  sum2 += three[12];
      sum1 += four[13];                   sum2 += four[12];
      P->dest[13] = sum1;                 P->dest[12] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 12:
      sum1 = one[11] + two[11];           sum2 = one[10] + two[10];
      sum1 += three[11];                  sum2 += three[10];
      sum1 += four[11];                   sum2 += four[10];
      P->dest[11] = sum1;                 P->dest[10] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 10:
      sum1 = one[9] + two[9];             sum2 = one[8] + two[8];
      sum1 += three[9];                   sum2 += three[8];
      sum1 += four[9];                    sum2 += four[8];
      P->dest[9] = sum1;                  P->dest[8] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 8:
      sum1 = one[7] + two[7];             sum2 = one[6] + two[6];
      sum1 += three[7];                   sum2 += three[6];
      sum1 += four[7];                    sum2 += four[6];
      P->dest[7] = sum1;                  P->dest[6] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 6:
      sum1 = one[5] + two[5];             sum2 = one[4] + two[4];
      sum1 += three[5];                   sum2 += three[4];
      sum1 += four[5];                    sum2 += four[4];
      P->dest[5] = sum1;                  P->dest[4] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 4:
      sum1 = one[3] + two[3];             sum2 = one[2] + two[2];
      sum1 += three[3];                   sum2 += three[2];
      sum1 += four[3];                    sum2 += four[2];
      P->dest[3] = sum1;                  P->dest[2] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 2:
      sum1 = one[1] + two[1];             sum2 = one[0] + two[0];
      sum1 += three[1];                   sum2 += three[0];
      sum1 += four[1];                    sum2 += four[0];
      P->dest[1] = sum1;                  P->dest[0] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
  }
  if (tmax1 > tmax2) return tmax1;
  return tmax2;
}
//
// use for higher sum4 folds
//
float top_sum4(float *ss[], struct PoTPlan *P) {
  float        sum1, sum2, tmaxB, tmax;
  int i;
  float *one = ss[0];
  float *two = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  float *four = ss[0]+P->tmp2;
  tmaxB = tmax = float(0.0);

  for (i = 0 ; i < P->di-15; i += 16) {
    sum1 = one[i+0] + two[i+0];         sum2 = one[i+1] + two[i+1];
    sum1 += three[i+0];                 sum2 += three[i+1];
    sum1 += four[i+0];                  sum2 += four[i+1];
    P->dest[i+0] = sum1;                P->dest[i+1] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+2] + two[i+2];         sum2 = one[i+3] + two[i+3];
    sum1 += three[i+2];                 sum2 += three[i+3];
    sum1 += four[i+2];                  sum2 += four[i+3];
    P->dest[i+2] = sum1;                P->dest[i+3] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+4] + two[i+4];         sum2 = one[i+5] + two[i+5];
    sum1 += three[i+4];                 sum2 += three[i+5];
    sum1 += four[i+4];                  sum2 += four[i+5];
    P->dest[i+4] = sum1;                P->dest[i+5] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+6] + two[i+6];         sum2 = one[i+7] + two[i+7];
    sum1 += three[i+6];                 sum2 += three[i+7];
    sum1 += four[i+6];                  sum2 += four[i+7];
    P->dest[i+6] = sum1;                P->dest[i+7] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+8] + two[i+8];         sum2 = one[i+9] + two[i+9];
    sum1 += three[i+8];                 sum2 += three[i+9];
    sum1 += four[i+8];                  sum2 += four[i+9];
    P->dest[i+8] = sum1;                P->dest[i+9] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+10] + two[i+10];       sum2 = one[i+11] + two[i+11];
    sum1 += three[i+10];                sum2 += three[i+11];
    sum1 += four[i+10];                 sum2 += four[i+11];
    P->dest[i+10] = sum1;               P->dest[i+11] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+12] + two[i+12];       sum2 = one[i+13] + two[i+13];
    sum1 += three[i+12];                sum2 += three[i+13];
    sum1 += four[i+12];                 sum2 += four[i+13];
    P->dest[i+12] = sum1;               P->dest[i+13] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+14] + two[i+14];       sum2 = one[i+15] + two[i+15];
    sum1 += three[i+14];                sum2 += three[i+15];
    sum1 += four[i+14];                 sum2 += four[i+15];
    P->dest[i+14] = sum1;               P->dest[i+15] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;
  }
  for (   ; i < P->di; i++) {
    sum1 = (one[i] + two[i] );          sum2 = (three[i] + four[i] );
    sum1 += sum2;
    P->dest[i] = sum1;
    if (sum1 > tmax) tmax = sum1;
  }
  if (tmax > tmaxB) return tmax;
  return tmaxB;
}
//
sum_func swi4tb[FOLDTBLEN] = {
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, top_sum4
};
//
//
float sw_sum5_t31(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmax2, tmax1;
  int i = P->di;
  float *one   = ss[0];
  float *two   = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  float *four  = ss[0]+P->tmp2;
  float *five  = ss[0]+P->tmp3;
  tmax2 = tmax1 = float(0.0);

  if ( i & 1 ) {
    i -= 1;
    tmax1 = one[i] + two[i];            sum2 = three[i] + four[i];
    tmax1 += five[i];
    tmax1 += sum2;
    P->dest[i] = tmax1;
  }
  switch (i) {
    case 30:
      sum1 = one[29] + two[29];           sum2 = one[28] + two[28];
      sum1 += three[29];                  sum2 += three[28];
      sum1 += four[29];                   sum2 += four[28];
      sum1 += five[29];                   sum2 += five[28];
      P->dest[29] = sum1;                 P->dest[28] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 28:
      sum1 = one[27] + two[27];           sum2 = one[26] + two[26];
      sum1 += three[27];                  sum2 += three[26];
      sum1 += four[27];                   sum2 += four[26];
      sum1 += five[27];                   sum2 += five[26];
      P->dest[27] = sum1;                 P->dest[26] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 26:
      sum1 = one[25] + two[25];           sum2 = one[24] + two[24];
      sum1 += three[25];                  sum2 += three[24];
      sum1 += four[25];                   sum2 += four[24];
      sum1 += five[25];                   sum2 += five[24];
      P->dest[25] = sum1;                 P->dest[24] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 24:
      sum1 = one[23] + two[23];           sum2 = one[22] + two[22];
      sum1 += three[23];                  sum2 += three[22];
      sum1 += four[23];                   sum2 += four[22];
      sum1 += five[23];                   sum2 += five[22];
      P->dest[23] = sum1;                 P->dest[22] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 22:
      sum1 = one[21] + two[21];           sum2 = one[20] + two[20];
      sum1 += three[21];                  sum2 += three[20];
      sum1 += four[21];                   sum2 += four[20];
      sum1 += five[21];                   sum2 += five[20];
      P->dest[21] = sum1;                 P->dest[20] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 20:
      sum1 = one[19] + two[19];           sum2 = one[18] + two[18];
      sum1 += three[19];                  sum2 += three[18];
      sum1 += four[19];                   sum2 += four[18];
      sum1 += five[19];                   sum2 += five[18];
      P->dest[19] = sum1;                 P->dest[18] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 18:
      sum1 = one[17] + two[17];           sum2 = one[16] + two[16];
      sum1 += three[17];                  sum2 += three[16];
      sum1 += four[17];                   sum2 += four[16];
      sum1 += five[17];                   sum2 += five[16];
      P->dest[17] = sum1;                 P->dest[16] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 16:
      sum1 = one[15] + two[15];           sum2 = one[14] + two[14];
      sum1 += three[15];                  sum2 += three[14];
      sum1 += four[15];                   sum2 += four[14];
      sum1 += five[15];                   sum2 += five[14];
      P->dest[15] = sum1;                 P->dest[14] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 14:
      sum1 = one[13] + two[13];           sum2 = one[12] + two[12];
      sum1 += three[13];                  sum2 += three[12];
      sum1 += four[13];                   sum2 += four[12];
      sum1 += five[13];                   sum2 += five[12];
      P->dest[13] = sum1;                 P->dest[12] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 12:
      sum1 = one[11] + two[11];           sum2 = one[10] + two[10];
      sum1 += three[11];                  sum2 += three[10];
      sum1 += four[11];                   sum2 += four[10];
      sum1 += five[11];                   sum2 += five[10];
      P->dest[11] = sum1;                 P->dest[10] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 10:
      sum1 = one[9] + two[9];             sum2 = one[8] + two[8];
      sum1 += three[9];                   sum2 += three[8];
      sum1 += four[9];                    sum2 += four[8];
      sum1 += five[9];                    sum2 += five[8];
      P->dest[9] = sum1;                  P->dest[8] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 8:
      sum1 = one[7] + two[7];             sum2 = one[6] + two[6];
      sum1 += three[7];                   sum2 += three[6];
      sum1 += four[7];                    sum2 += four[6];
      sum1 += five[7];                    sum2 += five[6];
      P->dest[7] = sum1;                  P->dest[6] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 6:
      sum1 = one[5] + two[5];             sum2 = one[4] + two[4];
      sum1 += three[5];                   sum2 += three[4];
      sum1 += four[5];                    sum2 += four[4];
      sum1 += five[5];                    sum2 += five[4];
      P->dest[5] = sum1;                  P->dest[4] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 4:
      sum1 = one[3] + two[3];             sum2 = one[2] + two[2];
      sum1 += three[3];                   sum2 += three[2];
      sum1 += four[3];                    sum2 += four[2];
      sum1 += five[3];                    sum2 += five[2];
      P->dest[3] = sum1;                  P->dest[2] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 2:
      sum1 = one[1] + two[1];             sum2 = one[0] + two[0];
      sum1 += three[1];                   sum2 += three[0];
      sum1 += four[1];                    sum2 += four[0];
      sum1 += five[1];                    sum2 += five[0];
      P->dest[1] = sum1;                  P->dest[0] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
  }
  if (tmax1 > tmax2) return tmax1;
  return tmax2;
}
//
// use for higher sum5 folds
//
float top_sum5(float *ss[], struct PoTPlan *P) {
  float        sum1, sum2, tmaxB, tmax;
  int i;
  float *one = ss[0];
  float *two = ss[0]+P->tmp0;
  float *three = ss[0]+P->tmp1;
  float *four = ss[0]+P->tmp2;
  float *five = ss[0]+P->tmp3;
  tmaxB = tmax = float(0.0);

  for (i = 0 ; i < P->di-15; i += 16) {
    sum1 = one[i+0] + two[i+0];         sum2 = one[i+1] + two[i+1];
    sum1 += three[i+0];                 sum2 += three[i+1];
    sum1 += four[i+0];                  sum2 += four[i+1];
    sum1 += five[i+0];                  sum2 += five[i+1];
    P->dest[i+0] = sum1;                P->dest[i+1] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+2] + two[i+2];         sum2 = one[i+3] + two[i+3];
    sum1 += three[i+2];                 sum2 += three[i+3];
    sum1 += four[i+2];                  sum2 += four[i+3];
    sum1 += five[i+2];                  sum2 += five[i+3];
    P->dest[i+2] = sum1;                P->dest[i+3] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+4] + two[i+4];         sum2 = one[i+5] + two[i+5];
    sum1 += three[i+4];                 sum2 += three[i+5];
    sum1 += four[i+4];                  sum2 += four[i+5];
    sum1 += five[i+4];                  sum2 += five[i+5];
    P->dest[i+4] = sum1;                P->dest[i+5] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+6] + two[i+6];         sum2 = one[i+7] + two[i+7];
    sum1 += three[i+6];                 sum2 += three[i+7];
    sum1 += four[i+6];                  sum2 += four[i+7];
    sum1 += five[i+6];                  sum2 += five[i+7];
    P->dest[i+6] = sum1;                P->dest[i+7] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+8] + two[i+8];         sum2 = one[i+9] + two[i+9];
    sum1 += three[i+8];                 sum2 += three[i+9];
    sum1 += four[i+8];                  sum2 += four[i+9];
    sum1 += five[i+8];                  sum2 += five[i+9];
    P->dest[i+8] = sum1;                P->dest[i+9] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+10] + two[i+10];       sum2 = one[i+11] + two[i+11];
    sum1 += three[i+10];                sum2 += three[i+11];
    sum1 += four[i+10];                 sum2 += four[i+11];
    sum1 += five[i+10];                 sum2 += five[i+11];
    P->dest[i+10] = sum1;               P->dest[i+11] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+12] + two[i+12];       sum2 = one[i+13] + two[i+13];
    sum1 += three[i+12];                sum2 += three[i+13];
    sum1 += four[i+12];                 sum2 += four[i+13];
    sum1 += five[i+12];                 sum2 += five[i+13];
    P->dest[i+12] = sum1;               P->dest[i+13] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+14] + two[i+14];       sum2 = one[i+15] + two[i+15];
    sum1 += three[i+14];                sum2 += three[i+15];
    sum1 += four[i+14];                 sum2 += four[i+15];
    sum1 += five[i+14];                 sum2 += five[i+15];
    P->dest[i+14] = sum1;               P->dest[i+15] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;
  }
  for (   ; i < P->di; i++) {
    sum1 = (one[i] + two[i] );          sum2 = (three[i] + four[i] );
    sum1 += five[i];
    sum1 += sum2;
    P->dest[i] = sum1;
    if (sum1 > tmax) tmax = sum1;
  }
  if (tmax > tmaxB) return tmax;
  return tmaxB;
}
//
//
sum_func swi5tb[FOLDTBLEN] = {
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,
  sw_sum5_t31,  sw_sum5_t31,  sw_sum5_t31,  top_sum5
};
//
//
float sw_sum2_t31(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmax2, tmax1;
  int i = P->di;
  float *one   = ss[1]+P->offset;
  float *two   = ss[1]+P->tmp0;
  tmax2 = tmax1 = float(0.0);

  if ( i & 1 ) {
    i -= 1;
    tmax1 = one[i] + two[i];
    P->dest[i] = tmax1;
  }
  switch (i) {
    case 30:
      sum1 = one[29] + two[29];           sum2 = one[28] + two[28];
      P->dest[29] = sum1;                 P->dest[28] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 28:
      sum1 = one[27] + two[27];           sum2 = one[26] + two[26];
      P->dest[27] = sum1;                 P->dest[26] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 26:
      sum1 = one[25] + two[25];           sum2 = one[24] + two[24];
      P->dest[25] = sum1;                 P->dest[24] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 24:
      sum1 = one[23] + two[23];           sum2 = one[22] + two[22];
      P->dest[23] = sum1;                 P->dest[22] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 22:
      sum1 = one[21] + two[21];           sum2 = one[20] + two[20];
      P->dest[21] = sum1;                 P->dest[20] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 20:
      sum1 = one[19] + two[19];           sum2 = one[18] + two[18];
      P->dest[19] = sum1;                 P->dest[18] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 18:
      sum1 = one[17] + two[17];           sum2 = one[16] + two[16];
      P->dest[17] = sum1;                 P->dest[16] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 16:
      sum1 = one[15] + two[15];           sum2 = one[14] + two[14];
      P->dest[15] = sum1;                 P->dest[14] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 14:
      sum1 = one[13] + two[13];           sum2 = one[12] + two[12];
      P->dest[13] = sum1;                 P->dest[12] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 12:
      sum1 = one[11] + two[11];           sum2 = one[10] + two[10];
      P->dest[11] = sum1;                 P->dest[10] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 10:
      sum1 = one[9] + two[9];             sum2 = one[8] + two[8];
      P->dest[9] = sum1;                  P->dest[8] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 8:
      sum1 = one[7] + two[7];             sum2 = one[6] + two[6];
      P->dest[7] = sum1;                  P->dest[6] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 6:
      sum1 = one[5] + two[5];             sum2 = one[4] + two[4];
      P->dest[5] = sum1;                  P->dest[4] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 4:
      sum1 = one[3] + two[3];             sum2 = one[2] + two[2];
      P->dest[3] = sum1;                  P->dest[2] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
    case 2:
      sum1 = one[1] + two[1];             sum2 = one[0] + two[0];
      P->dest[1] = sum1;                  P->dest[0] = sum2;
      if (sum1 > tmax1) tmax1 = sum1;     if (sum2 > tmax2) tmax2 = sum2;
  }
  if (tmax1 > tmax2) return tmax1;
  return tmax2;
}
//
// use for higher sum2 folds
//
float top_sum2(float *ss[], struct PoTPlan *P) {
  float sum1, sum2, tmaxB, tmax;
  int   i;
  float *one = ss[1]+P->offset;
  float *two = ss[1]+P->tmp0;
  tmaxB = tmax = float(0.0);

  for (i = 0 ; i < P->di-21; i += 22) {
    sum1 = one[i+0] + two[i+0];         sum2 = one[i+1] + two[i+1];
    P->dest[i+0] = sum1;                P->dest[i+1] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+2] + two[i+2];         sum2 = one[i+3] + two[i+3];
    P->dest[i+2] = sum1;                P->dest[i+3] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+4] + two[i+4];         sum2 = one[i+5] + two[i+5];
    P->dest[i+4] = sum1;                P->dest[i+5] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+6] + two[i+6];         sum2 = one[i+7] + two[i+7];
    P->dest[i+6] = sum1;                P->dest[i+7] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+8] + two[i+8];         sum2 = one[i+9] + two[i+9];
    P->dest[i+8] = sum1;                P->dest[i+9] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+10] + two[i+10];       sum2 = one[i+11] + two[i+11];
    P->dest[i+10] = sum1;               P->dest[i+11] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+12] + two[i+12];       sum2 = one[i+13] + two[i+13];
    P->dest[i+12] = sum1;               P->dest[i+13] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+14] + two[i+14];       sum2 = one[i+15] + two[i+15];
    P->dest[i+14] = sum1;               P->dest[i+15] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+16] + two[i+16];       sum2 = one[i+17] + two[i+17];
    P->dest[i+16] = sum1;               P->dest[i+17] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+18] + two[i+18];       sum2 = one[i+19] + two[i+19];
    P->dest[i+18] = sum1;               P->dest[i+19] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;

    sum1 = one[i+20] + two[i+20];       sum2 = one[i+21] + two[i+21];
    P->dest[i+20] = sum1;               P->dest[i+21] = sum2;
    if (sum1 > tmax) tmax = sum1;       if (sum2 > tmaxB) tmaxB = sum2;
  }
  for (   ; i < P->di; i++) {
    sum1 = one[i] + two[i];
    P->dest[i] = sum1;
    if (sum1 > tmax) tmax = sum1;
  }
  if (tmax > tmaxB) return tmax;
  return tmaxB;
}

sum_func swi2tb[FOLDTBLEN] = {
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, top_sum2
};
//
//
FoldSet swifold = {swi3tb, swi4tb, swi5tb, swi2tb, swi2tb, "FPU opt"};

// Arrays from which folding subroutines will be chosen during execution.
// Other sets can be inserted to replace the defaults.
//

sum_func sumsel3[FOLDTBLEN] = {
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, sw_sum3_t31,
  sw_sum3_t31, sw_sum3_t31, sw_sum3_t31, top_sum3
};
sum_func sumsel4[FOLDTBLEN] = {
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, sw_sum4_t31,
  sw_sum4_t31, sw_sum4_t31, sw_sum4_t31, top_sum4
};
sum_func sumsel5[FOLDTBLEN] = {
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, sw_sum5_t31,
  sw_sum5_t31, sw_sum5_t31, sw_sum5_t31, top_sum5
};
sum_func sumsel2[FOLDTBLEN] = {
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, top_sum2
};
sum_func sumsel2AL[FOLDTBLEN] = {
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, sw_sum2_t31,
  sw_sum2_t31, sw_sum2_t31, sw_sum2_t31, top_sum2
};

FoldSet Foldmain = {sumsel3, sumsel4, sumsel5, sumsel2, sumsel2AL, "opt FPU"};

/**********************
 *
 * Routine to copy fold subroutine tables.
 *
 */
int CopyFoldSet(FoldSet *dst, FoldSet *src) {
  int  i, j3, j4, j5, j2, j2AL;

  j3 = j4 = j5 = j2 = j2AL = 0;

  for (i = 0; i < FOLDTBLEN; i++) {

    if (src->f3[i] != 0)  {
      dst->f3[i] = src->f3[i];
      j3 = i;
    } else dst->f3[i] = src->f3[j3];

    if (src->f4[i] != 0)  {
      dst->f4[i] = src->f4[i];
      j4 = i;
    } else dst->f4[i] = src->f4[j4];

    if (src->f5[i] != 0)  {
      dst->f5[i] = src->f5[i];
      j5 = i;
    } else dst->f5[i] = src->f5[j5];

    if (src->f2[i] != 0)  {
      dst->f2[i] = src->f2[i];
      j2 = i;
    } else dst->f2[i] = src->f2[j2];

    if (src->f2AL[i] != 0)  {
      dst->f2AL[i] = src->f2AL[i];
      j2AL = i;
    } else dst->f2AL[i] = src->f2AL[j2AL];
  }
  dst->name = src->name;
  return 0;
}

/**********************
 *
 * t_funct - Caching routine for calls to invert_lcgf, return cache value if present
 *
 * This version caches and returns the threshold factor for dis_thresh rather than cur_thresh.
 *
 * The threshold factor assumes folding subroutines which do NOT divide the sums by num_adds.
 */
float t_funct(int m, int n, int x) {
 static struct tftab {
   int n;
   float y;
 }
 *t_funct_tab;

 float c_dis_thresh = (float)swi.analysis_cfg.pulse_display_thresh;

 if (!t_funct_tab) {
   int tablen = (PoTInfo.PulseMax+2)/3 +
                3*(int)(log((float)PoTInfo.PulseMax)/log(2.0)-3) - 1;
   t_funct_tab=(struct tftab *)malloc_a(tablen*sizeof(struct tftab),MEM_ALIGN);
   if (!t_funct_tab) SETIERROR(MALLOC_FAILED, "!t_funct_tab");
   memset(t_funct_tab, 0, tablen*sizeof(struct tftab));
 }
 if (t_funct_tab[x].n!=n) {
   t_funct_tab[x].n=n;
    t_funct_tab[x].y = (invert_lcgf((float)(-PoTInfo.PulseThresh - log((float)m)),
                                     (float)n, (float)1e-4) - n) * c_dis_thresh + n; 
 }
 return t_funct_tab[x].y;
}

/**********************
 *
 * Preplanning routine called from find_pulse()
 *
 */
int plan_PulsePoT(PoTPlan * PSeq, int PulsePotLen, float *div, int *dbinoffs) {
  float period;
  int ndivs;
  int i, j, di, dbins, offset;
  int num_adds_2;
  long p;
  unsigned long cperiod;
  int num_adds;
  register float tmp_max,t1;
  int res=1;
  int k = 0; // plan index

  for (i = 32, ndivs = 1; i <= PulsePotLen; ndivs++, i *= 2);

  int32_t thePotLen = PulsePotLen;

  //  Periods from PulsePotLen/3 to PulsePotLen/4, and power of 2 fractions of.
  //   then (len/4 to len/5) and finally (len/5 to len/6)
  //
  int32_t firstP, lastP;
  for(num_adds = 3; num_adds <= 5; num_adds++) {
    int num_adds_minus1;
    switch(num_adds) {
      case 3: lastP = (thePotLen*2)/3;  firstP = (thePotLen*1)/2; num_adds_minus1 = 2; break;
      case 4: lastP = (thePotLen*3)/4;  firstP = (thePotLen*3)/5; num_adds_minus1 = 3; break;
      case 5: lastP = (thePotLen*4)/5;  firstP = (thePotLen*4)/6; num_adds_minus1 = 4; break;
    }

    for (p = lastP ; p > firstP ; p--) {
      int tabofst = ndivs*3+2-num_adds;
      PSeq[k].dest = div+dbinoffs[0]; // Output storage
      PSeq[k].cperiod = cperiod = p*(C3X2TO14 / num_adds_minus1);
      PSeq[k].tmp0 = (int)((cperiod+C3X2TO13)/C3X2TO14);
      PSeq[k].tmp1 = (int)((cperiod*2+C3X2TO13)/C3X2TO14);
      PSeq[k].di = di  = (int)cperiod/C3X2TO14;
      PSeq[k].na = num_adds;
      PSeq[k].thresh = t_funct(di, num_adds, di+tabofst);

      switch(num_adds) {
        case 3:
          PSeq[k].fun_ptr = (di < FOLDTBLEN) ? sumsel3[di] : sumsel3[FOLDTBLEN-1];
          break;
        case 4:
          PSeq[k].tmp2 = (int)((cperiod*3+C3X2TO13)/C3X2TO14);
          PSeq[k].fun_ptr = (di < FOLDTBLEN) ? sumsel4[di] : sumsel4[FOLDTBLEN-1];
          break;
        case 5:
          PSeq[k].tmp2 = (int)((cperiod*3+C3X2TO13)/C3X2TO14);
          PSeq[k].tmp3 = (int)((cperiod*4+C3X2TO13)/C3X2TO14);
          PSeq[k].fun_ptr = (di < FOLDTBLEN) ? sumsel5[di] : sumsel5[FOLDTBLEN-1];
          break;
      }

      k++;                    // next plan
      num_adds_2 = 2* num_adds;

      for (j = 1; j < ndivs ; j++) {
        tabofst -= 3;
        PSeq[k].offset = dbinoffs[j-1];
        PSeq[k].dest = div+dbinoffs[j];
        PSeq[k].cperiod = PSeq[k-1].cperiod/2;
        PSeq[k].tmp0 = di & 1;
        PSeq[k].di = di = PSeq[k].cperiod/C3X2TO14;
        PSeq[k].tmp0 += di + PSeq[k].offset; //PSeq[k].offset + (PSeq[k].cperiod+C3X2TO13)/C3X2TO14
        if (PSeq[k].tmp0 & 3) PSeq[k].fun_ptr  = (di < FOLDTBLEN) ? sumsel2[di] : sumsel2[FOLDTBLEN-1];
        else PSeq[k].fun_ptr  = (di < FOLDTBLEN) ? sumsel2AL[di] : sumsel2AL[FOLDTBLEN-1];
        PSeq[k].na = num_adds_2;
        PSeq[k].thresh = t_funct(di, num_adds_2, di+tabofst);

        k++;                    // next plan
        num_adds_2 *=2;
      }  // for (j = 1; j < ndivs
    } // for (p = lastP
  } // for(num_adds =
  PSeq[k].di = 0; // stop
  return (k);
}

/**********************
 *
 * find_pulse - uses a folding algorithm to find repeating pulses
 *   Initially folds by prime number then by powers of two.
 *
 * Initial code by Eric Korpela
 * Major revisions and optimization by Ben Herndon
 * Further revised by Joe Segur
 *
 */
int find_pulse(const float * fp_PulsePot, int PulsePotLen,
               float pulse_thresh, int TOffset, int FOffset) {
  static float *div,*FoldedPOT;
  static int *dbinoffs, PrevPPL, PrevPoTln;
  static PoTPlan *PSeq;
  static float rcfg_dis_thresh = 1.0f / (float)swi.analysis_cfg.pulse_display_thresh;
  PoTPlan PTPln = {0} ;
  float *SrcSel[2];
  float period;
  int blksize;
  int ndivs;
  int i, j, di, maxs = 0;
  int num_adds, num_adds_2;
  long p;
  float max=0,maxd=0,avg=0,maxp=0, snr=0, fthresh=0,total=0.0f,partial;
  register float tmp_max,t1;
  int res=1;

  // debug possible heap corruption -- jeffc
#ifdef _WIN32
  BOINCASSERT(_CrtCheckMemory());
#endif

#ifdef DEBUG_PULSE_VERBOSE
  fprintf(stderr, "In find_pulse()...   PulsePotLen = %d   pulse_thresh = %f   TOffset = %d   PoT = %d\n",  PulsePotLen,  pulse_thresh, TOffset, FOffset);
#endif

  // boinc_worker_timer();
  
  //  Create internal arrays....
  if (!div) {
    int n, maxdivs = 1;
    for (i = 32; i <= PoTInfo.PulseMax; maxdivs++, i *= 2);
    div=(float *)calloc_a(((PoTInfo.PulseMax*7/4)+maxdivs*MEM_ALIGN/sizeof(float)), sizeof(float), MEM_ALIGN);
    FoldedPOT=(float *)malloc_a((PoTInfo.PulseMax+1)*sizeof(float)/3, MEM_ALIGN);
    dbinoffs=(int *)malloc_a(maxdivs*sizeof(int), MEM_ALIGN);
    for (i = 32, n = 1; i <= PPLANMAX; n++, i *= 2);
    i =  ((PPLANMAX*2)/3)-((PPLANMAX*2)/4);
    i += ((PPLANMAX*3)/4)-((PPLANMAX*3)/5);
    i += ((PPLANMAX*4)/5)-((PPLANMAX*4)/6);
    i *= n;
    PSeq = (PoTPlan *)malloc_a((i+1)*sizeof(PoTPlan), MEM_ALIGN);
    if ((!div) || (!FoldedPOT) || (!dbinoffs) || (!PSeq))
      SETIERROR(MALLOC_FAILED, "(!div) || (!FoldedPOT) || (!dbinoffs) || (!PSeq)");
    if (t_funct(6,1000,0)<0) SETIERROR(MALLOC_FAILED, "t_funct(6,1000,0)<0");;
  }

  SrcSel[0] = (float *)fp_PulsePot;  // source of data for 3, 4, 5 folds
  SrcSel[1] = div;                   // source of data for 2 folds

/* Uncomment this block if res > 1 is ever actually used

  //  Rebin to lower resolution if required
  if (res <= 1) {
    memcpy(div,fp_PulsePot,PulsePotLen*sizeof(float));
  } else {
    for (j=0;j<PulsePotLen/res;j++) {
      div[j]=0;
      for (i=0;i<res;i++) {
        div[j]+=fp_PulsePot[j*res+i];
      }
      div[j]/=res;
    }
    PulsePotLen/=res;
    SrcSel[0] = div;
  }
*/

  //  Calculate average power
  blksize = UNSTDMAX(4, UNSTDMIN(pow2((unsigned int) sqrt((float) (PulsePotLen / 32)) * 32), 512));
  int b;
  for(b = 0; b < PulsePotLen/blksize; b++) {
	  partial = 0.0f;
	  for(i = 0; i < blksize; i++) {
		 partial += *(SrcSel[0] + b*blksize + i);
	  }
	  total += partial;
  }
  //catch the tail if needed
  i=0;
  while (b*blksize+i < PulsePotLen)
  {
	total += *(SrcSel[0] + b*blksize + i); 
	i++;
  }
  avg = total / PulsePotLen;

  for (i = 32, ndivs = 1; i <= PulsePotLen; ndivs++, i *= 2);
  // ndivs=(int)(log((float)PulsePotLen)/log(2.0)-3);

  int32_t thePotLen = PulsePotLen;

  if (PrevPPL != PulsePotLen ) {
    PrevPPL = PulsePotLen;
    dbinoffs[0] = (PulsePotLen + (MEM_ALIGN/sizeof(float))-1) & -(MEM_ALIGN/sizeof(float));
    period = (float)((int)((PulsePotLen*2)/3))/2;
    for (i = 1; i < ndivs; i++, period/=2) {
      dbinoffs[i] = (((int)(period+0.5f) + (MEM_ALIGN/sizeof(float))-1) & -(MEM_ALIGN/sizeof(float))) + dbinoffs[i-1];
    }
  }

  if (PulsePotLen <= PPLANMAX) {    // If short, work from plan.
    int k;
    float cur_thresh, dis_thresh, t1;

    if (PulsePotLen != PrevPoTln) {  // if new length, generate plan.
      plan_PulsePoT(PSeq, PulsePotLen, div, dbinoffs);
      PrevPoTln = PulsePotLen;
    }

    for ( k = 0; PSeq[k].di; k++) {
      dis_thresh = PSeq[k].thresh*avg;

      tmp_max = PSeq[k].fun_ptr(SrcSel, &PSeq[k]);

      if (tmp_max>dis_thresh) {
        // unscale for reporting
        tmp_max /= PSeq[k].na;
        cur_thresh = (dis_thresh / PSeq[k].na - avg) * rcfg_dis_thresh + avg;

        ReportPulseEvent(tmp_max/avg,avg,res*(float)PSeq[k].cperiod/(float)C3X2TO14,
                         TOffset+PulsePotLen/2,FOffset,
                         (tmp_max-avg)*(float)sqrt((float)PSeq[k].na)/avg,
                         (cur_thresh-avg)*(float)sqrt((float)PSeq[k].na)/avg,
                         PSeq[k].dest, PSeq[k].na, 0);

        if ((tmp_max>cur_thresh) && ((t1=tmp_max-cur_thresh)>maxd)) {
          maxp  = (float)PSeq[k].cperiod/(float)C3X2TO14;
          maxd  = t1;
          maxs  = PSeq[k].na;
          max = tmp_max;
          snr = (float)((tmp_max-avg)*sqrt((float)PSeq[k].na)/avg);
          fthresh=(float)((cur_thresh-avg)*sqrt((float)PSeq[k].na)/avg);
          memcpy(FoldedPOT, PSeq[k].dest, PSeq[k].di*sizeof(float));
        }
      }
    }  // for ( k =
  }
  else {                         // If not plan,

    //  Periods from PulsePotLen/3 to PulsePotLen/4, and power of 2 fractions of.
    //   then (len/4 to len/5) and finally (len/5 to len/6)
    //
    int32_t firstP, lastP;
    for(num_adds = 3; num_adds <= 5; num_adds++) {
      int num_adds_minus1;
      switch(num_adds) {
        case 3: lastP = (thePotLen*2)/3;  firstP = (thePotLen*1)/2; num_adds_minus1 = 2; break;
        case 4: lastP = (thePotLen*3)/4;  firstP = (thePotLen*3)/5; num_adds_minus1 = 3; break;
        case 5: lastP = (thePotLen*4)/5;  firstP = (thePotLen*4)/6; num_adds_minus1 = 4; break;
      }
  
      for (p = lastP ; p > firstP ; p--) {
        float cur_thresh, dis_thresh;
        int tabofst, mper, perdiv;

        tabofst = ndivs*3+2-num_adds;
        mper = p * (12/num_adds_minus1);
        perdiv = num_adds_minus1;
        PTPln.tmp0 = (int)((mper + 6)/12);             // round(period)
        PTPln.tmp1 = (int)((mper * 2 + 6)/12);         // round(period*2)
        PTPln.di = (int)p/perdiv;                      // (int)period
        PTPln.dest = div+dbinoffs[0]; // Output storage
        dis_thresh = t_funct(PTPln.di, num_adds, PTPln.di+tabofst)*avg;

        switch(num_adds) {
          case 3:
            tmp_max = (PTPln.di < FOLDTBLEN) ? sumsel3[PTPln.di](SrcSel, &PTPln) : sumsel3[FOLDTBLEN-1](SrcSel, &PTPln);
            break;
          case 4:
            PTPln.tmp2 = (int)((mper * 3 + 6)/12);     // round(period*3)
            tmp_max = (PTPln.di < FOLDTBLEN) ? sumsel4[PTPln.di](SrcSel, &PTPln) : sumsel4[FOLDTBLEN-1](SrcSel, &PTPln);
            break;
          case 5:
            PTPln.tmp2 = (int)((mper * 3 + 6)/12);     // round(period*3)
            PTPln.tmp3 = (int)((mper * 4 + 6)/12);     // round(period*4)
            tmp_max = (PTPln.di < FOLDTBLEN) ? sumsel5[PTPln.di](SrcSel, &PTPln) : sumsel5[FOLDTBLEN-1](SrcSel, &PTPln);
            break;
        }
  
        if (tmp_max>dis_thresh) {
          // unscale for reporting
          tmp_max /= num_adds;
          cur_thresh = (dis_thresh / num_adds - avg) * rcfg_dis_thresh + avg;

          ReportPulseEvent(tmp_max/avg,avg,((float)p)/(float)perdiv*res,
                           TOffset+PulsePotLen/2,FOffset,
                           (tmp_max-avg)*(float)sqrt((float)num_adds)/avg,
                           (cur_thresh-avg)*(float)sqrt((float)num_adds)/avg,
                           PTPln.dest, num_adds, 0);

          if ((tmp_max>cur_thresh) && ((t1=tmp_max-cur_thresh)>maxd)) {
            maxp  = (float)p/(float)perdiv;
            maxd  = t1;
            maxs  = num_adds;
            max = tmp_max;
            snr = (float)((tmp_max-avg)*sqrt((float)num_adds)/avg);
            fthresh=(float)((cur_thresh-avg)*sqrt((float)num_adds)/avg);
            memcpy(FoldedPOT, PTPln.dest, PTPln.di*sizeof(float));
          }
        }

        num_adds_2 = 2* num_adds;

        for (j = 1; j < ndivs ; j++) {
          PTPln.offset = dbinoffs[j-1];
          PTPln.dest = div+dbinoffs[j];
          perdiv *=2;
          PTPln.tmp0 = PTPln.di & 1;
          PTPln.di /= 2;
          PTPln.tmp0 += PTPln.di + PTPln.offset;
          tabofst -=3;
          dis_thresh = t_funct(PTPln.di, num_adds_2, PTPln.di+tabofst) * avg;

          if (PTPln.tmp0 & 3)
            tmp_max = (PTPln.di < FOLDTBLEN) ? sumsel2[PTPln.di](SrcSel, &PTPln) : sumsel2[FOLDTBLEN-1](SrcSel, &PTPln);
          else
            tmp_max = (PTPln.di < FOLDTBLEN) ? sumsel2AL[PTPln.di](SrcSel, &PTPln) : sumsel2AL[FOLDTBLEN-1](SrcSel, &PTPln);

          if (tmp_max>dis_thresh) {
            // unscale for reporting
            tmp_max /= num_adds_2;
            cur_thresh = (dis_thresh / num_adds_2 - avg) * rcfg_dis_thresh + avg;

            ReportPulseEvent(tmp_max/avg,avg,((float)p)/(float)perdiv*res,
                             TOffset+PulsePotLen/2,FOffset,
                             (tmp_max-avg)*(float)sqrt((float)num_adds_2)/avg,
                             (cur_thresh-avg)*(float)sqrt((float)num_adds_2)/avg,
                             PTPln.dest, num_adds_2, 0);

            if ((tmp_max>cur_thresh) && ((t1=tmp_max-cur_thresh)>maxd)) {
              maxp = (float)p/(float)perdiv;
              maxd = t1;
              maxs = num_adds_2;
              max  = tmp_max;
              snr  = (float)((tmp_max-avg)*sqrt((float)num_adds_2)/avg);
              fthresh = (float)((cur_thresh-avg)*sqrt((float)num_adds_2)/avg);
              memcpy(FoldedPOT, PTPln.dest, PTPln.di*sizeof(float));
            }
          }

          num_adds_2 *=2;
        }  // for (j = 1; j < ndivs
      } // for (p = lastP
    } // for(num_adds =
  }

  analysis_state.FLOP_counter+=(PulsePotLen*0.1818181818182+400.0)*PulsePotLen;
  if (maxp!=0)
    ReportPulseEvent(max/avg,avg,maxp*res,TOffset+PulsePotLen/2,FOffset,
                     snr, fthresh, FoldedPOT, maxs, 1);

  // debug possible heap corruption -- jeffc
#ifdef _WIN32
  BOINCASSERT(_CrtCheckMemory());
#endif

  return 0;
}

