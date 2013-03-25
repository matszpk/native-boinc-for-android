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
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
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
  double roundVal = (srate >= 0.0) ? ROUNDX87 : -ROUNDX87;
  double signedHalf = (srate >= 0.0) ? 0.5 : -0.5;
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

    // reduce the angle to the range (-0.5, 0.5) using "angles - round(angles)"
    //
    // Windows and *BSD operate the X87 FPU so it rounds at mantissa bit 53, the
    // quick rounding algorithm needs rounding at the last bit.
    // Do 8 angles to amortize the time cost of precision switching.
#if (defined(_WIN32) || defined(_Win64) || defined(_BSD)) && (defined(__i386__) || defined(__x86_64__))
  #if defined(_MSC_VER)
    __asm fnstcw fpucw1;
  #elif defined(__GNUC__)
    __asm__ __volatile__ ("fnstcw %0" : "=m" (fpucw1));
  #endif
    fpucw2 = fpucw1 | 0x300;
  #if defined(_MSC_VER)
    __asm
      {
      fldcw fpucw2        // set extended precision
      fld roundVal
      fld angles[0 * 8]   // get angle
      fadd st(0), st(1)   // + roundVal
      fsub st(0), st(1)   // - roundVal, integer in st(0)
      fsubr angles[0 * 8] // angle - integer
      fstp angles[0 * 8]  // store reduced angle
      fld angles[1 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[1 * 8]
      fstp angles[1 * 8]
      fld angles[2 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[2 * 8]
      fstp angles[2 * 8]
      fld angles[3 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[3 * 8]
      fstp angles[3 * 8]
      fld angles[4 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[4 * 8]
      fstp angles[4 * 8]
      fld angles[5 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[5 * 8]
      fstp angles[5 * 8]
      fld angles[6 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[6 * 8]
      fstp angles[6 * 8]
      fld angles[7 * 8]
      fadd st(0), st(1)
      fsub st(0), st(1)
      fsubr angles[7 * 8]
      fstp angles[7 * 8]
      fstp st(0)      // pop roundVal
      fldcw fpucw1    // restore normal precision
      }
  #elif defined(__GNUC__)
    __asm__ __volatile__ ("fldcw %0" : : "m" (fpucw2));
    angles[0] -= ((angles[0] + roundVal) - roundVal);
    angles[1] -= ((angles[1] + roundVal) - roundVal);
    angles[2] -= ((angles[2] + roundVal) - roundVal);
    angles[3] -= ((angles[3] + roundVal) - roundVal);
    angles[4] -= ((angles[4] + roundVal) - roundVal);
    angles[5] -= ((angles[5] + roundVal) - roundVal);
    angles[6] -= ((angles[6] + roundVal) - roundVal);
    angles[7] -= ((angles[7] + roundVal) - roundVal);
    __asm__ __volatile__ ("fldcw %0" : : "m" (fpucw1));
  #endif
#elif defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
    // Linux operates the X87 FPU at full extended precision
    angles[0] -= ((angles[0] + roundVal) - roundVal);
    angles[1] -= ((angles[1] + roundVal) - roundVal);
    angles[2] -= ((angles[2] + roundVal) - roundVal);
    angles[3] -= ((angles[3] + roundVal) - roundVal);
    angles[4] -= ((angles[4] + roundVal) - roundVal);
    angles[5] -= ((angles[5] + roundVal) - roundVal);
    angles[6] -= ((angles[6] + roundVal) - roundVal);
    angles[7] -= ((angles[7] + roundVal) - roundVal);
#else
    // Unknown, use the slower floor() method
    angles[0] -= floor(angles[0] + signedHalf);
    angles[1] -= floor(angles[1] + signedHalf);
    angles[2] -= floor(angles[2] + signedHalf);
    angles[3] -= floor(angles[3] + signedHalf);
    angles[4] -= floor(angles[4] + signedHalf);
    angles[5] -= floor(angles[5] + signedHalf);
    angles[6] -= floor(angles[6] + signedHalf);
    angles[7] -= floor(angles[7] + signedHalf);
#endif

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
