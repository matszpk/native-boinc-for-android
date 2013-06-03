// Copyright 2007 Regents of the University of California

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
#include "sah_config.h"
#include <cmath>
#include <vector>
#include "analyzeFuncs.h"
#include "analyzeFuncs_vector.h"
#include "sincos.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// *********************************************************
//
// JWS: FPU chirp using Flemming Pedersen (CERN) fast double sincos,
// http://pedersen.web.cern.ch/pedersen/project-leir-dsp-bc/matlab/fast_sine_cosine/fast_sine_cosine.doc
// Similar to Marcus Mendenhall Faster SinCos except no final adjustment
// because doubles produce good accuracy without.
//
// Includes quick FPU rounding developed for sse1 version.
//
inline void set_up_fastfrac(double roundVal) {
// this routine only exists for compilers that can't tell that a value is
// being popped from the FP stack and then immediately reloaded.
#if defined(_MSC_VER) && !defined(_WIN64)
    __asm fld roundVal;   // get roundVal
#endif
}

inline void clean_up_fastfrac() {
// this routine only exists for compilers than needed set_up_fastfrac()
#if defined(_MSC_VER) && !defined(_WIN64)
    __asm fstp st(0);     // pop roundVal off FPU stack
#endif
}

// This routine should work as long as x86_64 supports x87 instructions.
// After that the illegal instruction trap should take care of it.
inline double fastfrac(double val, double roundVal) {
      // reduce val to the range (-0.5, 0.5) using "val - round(val)" 
#if defined(_MSC_VER) && !defined(_WIN64)
    __asm {
      fld val             // get angle
      fadd st(0), st(1)   // + roundVal
      fsub st(0), st(1)   // - roundVal, integer in st(0)
      fsubr val           // angle - integer
      fstp val            // store reduced angle
    } 

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    __asm__ __volatile__( 
        "fadd  %2,%0\n"  // + roundVal
"        fsub  %2,%0\n"  // - roundVal, integer in st(0)
"        fsubr %3,%0\n"  // angle - integer
        : "=&t" (val)
        : "0" (val), "f" (roundVal), "f" (val)
    );
#elif defined(_WIN64)
	val -= ((val + roundVal) - roundVal);  // TODO: ADD CHECK THAT THIS WORKS
#else
    val -= floor(val + 0.5);
#endif
    return val;
}     

static unsigned short fpucw1;

inline void set_extended_precision() {
    // Windows and *BSD operate the X87 FPU so it rounds at mantissa bit 53, the
    // quick rounding algorithm needs rounding at the last bit.
    unsigned short fpucw2;
#if defined(_MSC_VER) && !defined(_WIN64) // MSVC no inline assembly for 64 bit
    __asm fnstcw fpucw1;
    fpucw2 = fpucw1 | 0x300;
    __asm fldcw fpucw2;
#elif defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64) || defined(_BSD))
    __asm__ __volatile__ ("fnstcw %0" : "=m" (fpucw1));
    fpucw2 = fpucw1 | 0x300;
    __asm__ __volatile__ ("fldcw %0" : : "m" (fpucw2));
#else  
    // Nothing necessary for linux, osx, _WIN64 VC++ and most everything else.
#endif
}

inline void restore_fpucw() {
#if defined(_MSC_VER) && !defined(_WIN64)
    __asm fldcw fpucw1;
#elif defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64) || defined(_BSD))
    __asm__ __volatile__ ("fldcw %0" : : "m" (fpucw1));
#else
    // NADA
#endif
}

double z;

int fpu_ChirpData (
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int ChirpRateInd,
  double ChirpRate,
  int  ul_NumDataPoints,
  double sample_rate
) {
  if (ChirpRateInd == 0) {
    memcpy(cx_ChirpDataArray, cx_DataArray, (int)ul_NumDataPoints * sizeof(sah_complex) );
    return 0;
  }

  double srate    = ChirpRate * 0.5 / (sample_rate * sample_rate);


#if (!defined(__GNUC__) && defined(_WIN64)) || \
    (defined(__GNUC__) && !(defined(__i386__) || defined(__x86_64__)))
  double roundVal = (srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52;
#else
  double roundVal = (srate >= 0.0) ? ROUNDX87 : -ROUNDX87;
#endif

  int i, j, vEnd;
  unsigned short fpucw1, fpucw2;

  vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);

  // main loop
  for (i = 0; i < vEnd; i += 8) {
    double angles[8], dtemp;
    double cd1, cd2, cd3;
    double x, y;
    double s, c;
    float real, imag;

    dtemp = double( i );
    angles[0] = dtemp+0.0;
    angles[1] = dtemp+1.0;
    angles[2] = dtemp+2.0;
    angles[3] = dtemp+3.0;
    angles[4] = dtemp+4.0;
    angles[5] = dtemp+5.0;
    angles[6] = dtemp+6.0;
    angles[7] = dtemp+7.0;

    // calculate the input angle
    angles[0] *= angles[0] * srate;  // angle^2 * rate
    angles[1] *= angles[1] * srate;
    angles[2] *= angles[2] * srate;
    angles[3] *= angles[3] * srate;
    angles[4] *= angles[4] * srate;
    angles[5] *= angles[5] * srate;
    angles[6] *= angles[6] * srate;
    angles[7] *= angles[7] * srate;

    // Do 8 angles to amortize the time cost of precision switching.
    set_extended_precision();
    set_up_fastfrac(roundVal);
    angles[0]=fastfrac(angles[0],roundVal);
    angles[1]=fastfrac(angles[1],roundVal);
    angles[2]=fastfrac(angles[2],roundVal);
    angles[3]=fastfrac(angles[3],roundVal);
    angles[4]=fastfrac(angles[4],roundVal);
    angles[5]=fastfrac(angles[5],roundVal);
    angles[6]=fastfrac(angles[6],roundVal);
    angles[7]=fastfrac(angles[7],roundVal);
    clean_up_fastfrac();
    restore_fpucw();

    for ( j = 0; j < 8; j++ ) {
    
      // square angle to the range [0, 0.25)
      y = angles[j] * angles[j];
    
      // perform the initial polynomial approximations
      s = y * FS4;
      c = y * FC3;
      s += FS3;
      c += FC2;
      s *= y;
      c *= y;
      s += FS2;
      c += FC1;
      s *= y;
      c *= y;
      s += FS1;
      s *= angles[j];
      c += 1;
    
      // perform first angle doubling
      x = c * c - s * s;
      y = s * c * 2;
      cd1 = x * y;
      cd2 = x * x;
      cd3 = y * y;
    
      // perform second angle doubling
      s = cd1 * 2;
      c = cd2 - cd3;
    
      // chirp and store
      real = (float)(cx_DataArray[i + j][0] *  c -  cx_DataArray[i + j][1] * s);
      imag = (float)(cx_DataArray[i + j][0] *  s +  cx_DataArray[i + j][1] * c);
      cx_ChirpDataArray[i + j][0]  = real;
      cx_ChirpDataArray[i + j][1]  = imag;
    
    }
  }
  
  if( i < ul_NumDataPoints) {
    // use original routine to finish up any tailings (max stride-1 elements)
    v_ChirpData(cx_DataArray+i, cx_ChirpDataArray+i
      , ChirpRateInd, ChirpRate, ul_NumDataPoints-i, sample_rate);
  }
  analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

  return 0;
}


// opt_v_ChirpData 
// 8-10-06 BENH - Unrolled loop 3 times to allow for FPU latency & reduce loop overhead 
//..............- Removed conditionals from loop 
// xx-xx-03 ERICK -  Created function
//
extern void CalcTrigArray (int len, int ChirpRateInd);
int fpu_opt_ChirpData (
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int ChirpRateInd,
  double ChirpRate,
  int  ul_NumDataPoints,
  double sample_rate

){
    float   chirp_sign; // BENH - Pulled conditional out of loop
    if ( ChirpRateInd == 0 )
        {
        memcpy( cx_ChirpDataArray, cx_DataArray,  int( ul_NumDataPoints * sizeof(sah_complex)) );  
        return ( 0 );
        }

    // calculate trigonometric array this function returns w/o doing
    // anything when sign of chirp_rate_ind reverses. so we have to take care
    // of it.
    //
    double recip_sample_rate=1.0/sample_rate;
    chirp_sign = ( ChirpRateInd >= 0 ) ? 1 : -1;

    const int stride = 2;
    int i = 0;
    int last = ul_NumDataPoints - ( stride - 1 );
    // what we do depends on how much memory we have...
    // If we have more than 64MB, we'll cache the chirp table.  If not
    // we'll calculate it each time.
    bool CacheChirpCalc=((app_init_data.host_info.m_nbytes == 0)  ||
                             (app_init_data.host_info.m_nbytes >= (double)(64*1024*1024)));
    // calculate trigonometric array
    // this function returns w/o doing nothing when sign of chirp_rate_ind
    // reverses.  so we have to take care of it.
    if ( CacheChirpCalc ){
        CalcTrigArray(ul_NumDataPoints, ChirpRateInd );
        for (       ; i < last ; i += stride ){
            register double c1, c2, d1, d2;
            register float  R1, R2, I1, I2, t1, t2;

            c1 = CurrentTrig[i + 0].Cos;
            d1 = CurrentTrig[i + 0].Sin * chirp_sign;   // BENH - opt - avoid branch by multiply
            c2 = CurrentTrig[i + 1].Cos;
            d2 = CurrentTrig[i + 1].Sin * chirp_sign;

            // d = (chirp_rate_ind >0)? CurrentTrig[i].Sin :
            // -CurrentTrig[i].Sin; Sometimes chirping is done in place. We
            // don't want to overwrite data prematurely.
            //
            R1 = cx_DataArray[i + 0][0] * c1;
            t1 = cx_DataArray[i + 0][1] * d1;
            I1 = cx_DataArray[i + 0][0] * d1;
            t2 = cx_DataArray[i + 0][1] * c1;
            cx_ChirpDataArray[i + 0][0] = R1 - t1;
            cx_ChirpDataArray[i + 0][1] = I1 + t2;
            R2 = cx_DataArray[i + 1][0] * c2;
            t1 = cx_DataArray[i + 1][1] * d2;
            I2 = cx_DataArray[i + 1][0] * d2;
            t2 = cx_DataArray[i + 1][1] * c2;
            cx_ChirpDataArray[i + 1][0] = R2 - t1;
            cx_ChirpDataArray[i + 1][1] = I2 + t2;
            }
        }
//  cx_ChirpDataArray[64][1] = 3.012345;

    // Too little memory to cache sin/cos or just falling out of
    // 'CacheChirpCalc' loop
    // use original routine to finish up any tailings (max stride-1 elements)
    for( ;i < ul_NumDataPoints; i++) {
        double dd,cc;
        double time=static_cast<double>(i)*recip_sample_rate;
        // since ang is getting moded by 2pi, we calculate "ang mod 2pi"
        // before the call to sincos() inorder to reduce roundoff error.
        // (Bug submitted by Tetsuji "Maverick" Rai)
        double ang  = 0.5*ChirpRate*time*time;
        float c, d, real, imag;
        ang -= floor(ang);
        ang *= M_PI*2;
        sincos(ang,&dd,&cc);
        c=cc;
        d=dd;
        // Sometimes chirping is done in place.
        // We don't want to overwrite data prematurely.
        real = cx_DataArray[i][0] * c - cx_DataArray[i][1] * d;
        imag = cx_DataArray[i][0] * d + cx_DataArray[i][1] * c;
        cx_ChirpDataArray[i][0] = real;
        cx_ChirpDataArray[i][1] = imag;
    }

//R    count_flops( 12 * ul_NumDataPoints );
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
    return ( 0 );
}
