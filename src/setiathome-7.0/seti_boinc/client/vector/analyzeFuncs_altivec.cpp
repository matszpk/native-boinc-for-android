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

// $Id: analyzeFuncs_altivec.cpp,v 1.1.2.4 2006/12/14 22:21:59 korpela Exp $
//

// This file is empty is USE_ALTIVEC is not defined
#include "sah_config.h"

#if defined(__ppc__) && defined(USE_ALTIVEC)

#define INVALID_CHIRP 2e+20

#include "analyzeFuncs.h"
#include "analyzeFuncs_vector.h"
#include "analyzePoT.h"
#include "analyzeReport.h"
#include "gaussfit.h"
#include "s_util.h"
#include "pulsefind.h"

#include <CoreServices/CoreServices.h>
#include <Accelerate/Accelerate.h>
#include <ppc_intrinsics.h>

// polynomial coefficients for sine / cosine approximation in the chirp routine
#define SS1 ((vector float) (1.5707963268))
#define SS2 ((vector float) (-0.6466386396))
#define SS3 ((vector float) (0.0679105987))
#define SS4 ((vector float) (-0.0011573807))
#define CC1 ((vector float) (-1.2341299769))
#define CC2 ((vector float) (0.2465220241))
#define CC3 ((vector float) (-0.0123926179))

#define ZERO		((vector float) (0))
#define ONE         ((vector float) (1.0))
#define TWO         ((vector float) (2.0))
#define RECIP_TWO       ((vector float) (0.5))
#define RECIP_THREE     ((vector float) (1.0 / 3.0))
#define RECIP_FOUR      ((vector float) (0.25))
#define RECIP_FIVE      ((vector float) (0.2))

// 2^52 (used by quickRound)
#define TWO_TO_52	4.503599627370496e15



static bool checked=false;
static bool hasAltiVec=false;

bool AltiVec_Available( void )
{
    if (!checked) {
        checked=true;
        long cpuAttributes;
        OSErr err = Gestalt( gestaltPowerPCProcessorFeatures, &cpuAttributes );

        if( noErr == err )
             hasAltiVec = ( 1 << gestaltPowerPCHasVectorInstructions) & cpuAttributes;
    }
    return hasAltiVec;
}

inline vector float vec_fpsplat(float f_Constant) {
  vector unsigned char vuc_Splat;
  vector float vf_Constant;
  vuc_Splat = vec_lvsl(0, &f_Constant);
  vf_Constant = vec_lde(0, &f_Constant);
  vuc_Splat = (vector unsigned char) vec_splat((vector float) vuc_Splat, 0);
  vf_Constant = vec_perm(vf_Constant, vf_Constant, vuc_Splat);
  return vf_Constant;
}

inline vector float vec_rsqrt(vector float v) {
  const vector float _0 = (vector float)(-.0); 
  // obtain estimate
  vector float y = vec_rsqrte(v);
  // two rounds of Newton-Raphson
  y = vec_madd(vec_nmsub(v,vec_madd(y,y,_0),(vector float)(1.0)),vec_madd(y,(vector float)(0.5),_0),y);
  y = vec_madd(vec_nmsub(v,vec_madd(y,y,_0),(vector float)(1.0)),vec_madd(y,(vector float)(0.5),_0),y);
  return y;
}

inline vector float vec_sqrt(vector float v) {
  const vector float _0 = (vector float)(-.0);
  return vec_sel(vec_madd(v,vec_rsqrt(v),_0),_0,vec_cmpeq(v,_0));
}

inline vector float vec_recip(vector float v) {
  // obtain estimate
  vector float estimate = vec_re( v );
  // one round of Newton-Raphson
  return vec_madd( vec_nmsub( estimate, v, (vector float) (1.0) ), estimate, estimate );
}  

int v_vTranspose(int i, int j, float *in, float *out) {
  if (AltiVec_Available()) {
      vSgetmo(j, i, (vector float *) in, (vector float *) out);
      return 0;
  } else {
      return UNSUPPORTED_FUNCTION;
  }
}

int v_vGetPowerSpectrum(
  sah_complex* FreqData,
  float* PowerSpectrum,
  int NumDataPoints
) {
  int i, vEnd;
  const vector unsigned char real = (vector unsigned char) (0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27);
  const vector unsigned char imag = (vector unsigned char) (4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31);

  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;

  analysis_state.FLOP_counter+=3.0*NumDataPoints;

  vEnd = NumDataPoints - (NumDataPoints & 3);
  for (i = 0; i < vEnd; i += 4) {
    const float *f = (const float *) (FreqData + i);
    float *p = PowerSpectrum + i;
    vector float f1, f2;
    vector float re1, im1;
    vector float p1;
    
    f1 = vec_ld(0, f);
    f2 = vec_ld(16, f);
    re1 = vec_perm(f1, f2, real);
    im1 = vec_perm(f1, f2, imag);
    p1 = vec_madd(re1, re1, vec_madd(im1, im1, ZERO));
    vec_st(p1, 0, p);    
  }
  
  // handle tail elements, although in practice this never happens
  for (; i < NumDataPoints; i++) {
    PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
    + FreqData[i][1] * FreqData[i][1];
  }  
  return 0;
}


int v_vGetPowerSpectrumG4(
                         sah_complex* FreqData,
                         float* PowerSpectrum,
                         int NumDataPoints
                         ) {
  int i = 0, vEnd;
  const vector unsigned char real = (vector unsigned char) (0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27);
  const vector unsigned char imag = (vector unsigned char) (4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31);

  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
  analysis_state.FLOP_counter+=3.0*NumDataPoints;

  vEnd = NumDataPoints - (NumDataPoints & 15);
  for (i = 0; i < vEnd; i += 16) {
    const float *f = (const float *) (FreqData + i);
    float *p = PowerSpectrum + i;
    vector float f1, f2, f3, f4, f5, f6, f7, f8;
    vector float re1, im1, re2, im2, re3, im3, re4, im4;
    vector float p1, p2, p3, p4;
    
    // prefetch next 512 bytes into L1 cache
    vec_dstt(f, 0x10020100, 0);

    f1 = vec_ld(0, f);
    f2 = vec_ld(16, f);
    f3 = vec_ld(32, f);
    f4 = vec_ld(48, f);
    f5 = vec_ld(64, f);
    f6 = vec_ld(80, f);
    f7 = vec_ld(96, f);
    f8 = vec_ld(112, f);
    re1 = vec_perm(f1, f2, real);
    im1 = vec_perm(f1, f2, imag);
    re2 = vec_perm(f3, f4, real);
    im2 = vec_perm(f3, f4, imag);
    re3 = vec_perm(f5, f6, real);
    im3 = vec_perm(f5, f6, imag);
    re4 = vec_perm(f7, f8, real);
    im4 = vec_perm(f7, f8, imag);

    // zero out a spot for our power data in the cache
    __dcbz(0, p);
    __dcbz(32, p);

    p1 = vec_madd(re1, re1, vec_madd(im1, im1, ZERO));
    p2 = vec_madd(re2, re2, vec_madd(im2, im2, ZERO));
    p3 = vec_madd(re3, re3, vec_madd(im3, im3, ZERO));
    p4 = vec_madd(re4, re4, vec_madd(im4, im4, ZERO));
    vec_st(p1, 0, p);
    vec_st(p2, 16, p);
    vec_st(p3, 32, p);
    vec_st(p4, 48, p);
  }  
  
  vEnd = NumDataPoints - (NumDataPoints & 3);
  for (; i < vEnd; i += 4) {
    const float *f = (const float *) (FreqData + i);
    float *p = PowerSpectrum + i;
    vector float f1, f2;
    vector float re1, im1;
    vector float p1;
    
    f1 = vec_ld(0, f);
    f2 = vec_ld(16, f);
    re1 = vec_perm(f1, f2, real);
    im1 = vec_perm(f1, f2, imag);
    p1 = vec_madd(re1, re1, vec_madd(im1, im1, ZERO));
    vec_st(p1, 0, p);    
  }  

  // handle tail elements, although in practice this never happens
  for (; i < NumDataPoints; i++) {
    PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
    + FreqData[i][1] * FreqData[i][1];
  }  
  return 0;
}



// quickRound
//
// Quickly round a value to the nearest integer. This is done by adding a large
// value (which depends on the format of an IEEE double precision variable) and
// then subtracting it again to extract the fractional part of the variable.
//
// If x is positive, roundVal must be TWO_TO_52.
// If x is negative, roundVal must be -TWO_TO_52.
//
// NOTE: This function does not do any fancy IEEE compliant arithmetic (like
// handling NaN etc. or preserving the sign of +/-0).
//
// NOTE: Make sure that you don't use any compiler flags that will cancel
// out the operations.
inline double
quickRound(double x, double roundVal)
{
  return (x + roundVal) - roundVal;
}


// chirp
//
// Increase the frequency of the input signal over time.
//
// Note that the input angle is calculated in double-precision. The angle
// becomes so large that single-precision arithmetic is insufficient for the
// accuracy that we need. We can only convert to single-precision after
// reducing the range to (-0.5, 0.5).
//
// The algorithm used to generate the sine / cosine values is based on that
// described in "A Fast, Vectorizable Algorithm for Producing Single-Precision
// Sine-Cosine Pairs" by Marcus H. Mendenhall. The paper is available at:
//     http://arxiv.org/pdf/cs.MS/0406049
//
// Differences from the implementation in the paper:
// 1. The arguments are not scaled by 1/(2 pi) because the original chirp
//    algorithm multiplies the chirp rate by (2 pi).
// 2. The slower version of the algorithm has been used (as if FASTER_SINCOS
//    had not been defined for the code in the paper).
// 3. The loop has been unrolled once so the variables have been renamed to make
//    things more readable.
// 4. The "struct phase" data type is not used.
// 5. The rounding of the input value to the range (-0.5, 0.5) is done in
//    double-precision as mentioned above, and not via Altivec.
int v_vChirpData(
  sah_complex* cx_DataArray,
  sah_complex* cx_ChirpDataArray,
  int chirp_rate_ind,
  double chirp_rate,
  int  ul_NumDataPoints,
  double sample_rate
) {
// int offset, int numPoints, double chirpRate
  int i;

  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
  
  if (chirp_rate_ind == 0) {
    memcpy(cx_ChirpDataArray,
           cx_DataArray,
           (int)ul_NumDataPoints * sizeof(sah_complex)
          );
  } else {
    int vEnd;  
    const vector unsigned char real = (vector unsigned char) (0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27);
	const vector unsigned char imag = (vector unsigned char) (4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31);
    double rate = chirp_rate * 0.5 / (sample_rate * sample_rate);
    double roundVal = rate >= 0.0 ? TWO_TO_52 : -TWO_TO_52;
    
    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    for (i = 0; i < vEnd; i += 8) {
      const float *d = (const float *) (cx_DataArray + i);
      float *c = (float *) (cx_ChirpDataArray + i);
      double a1 = i;
      double a2 = i + 1.0;
      double a3 = i + 2.0;
      double a4 = i + 3.0;
      double a5 = i + 4.0;
      double a6 = i + 5.0;
      double a7 = i + 6.0;
      double a8 = i + 7.0;
      float v[8] __attribute__ ((aligned (16)));
      vector float d1, d2, d3, d4;
      vector float cd1, cd2, cd3, cd4;
      vector float dre1, dre2;
      vector float dim1, dim2;
      vector float cre1, cre2;
      vector float cim1, cim2;
      vector float x1, x2;
      vector float y1, y2;
      vector float s1, s2;
      vector float c1, c2;
      vector float m1, m2;
      
      // load the signal to be chirped
      d1 = vec_ld(0, d);
      d2 = vec_ld(16, d);
      d3 = vec_ld(32, d);
      d4 = vec_ld(48, d);
      
      // calculate the input angle
      a1 *= a1 * rate; a2 *= a2 * rate;
      a3 *= a3 * rate; a4 *= a4 * rate;
      a5 *= a5 * rate; a6 *= a6 * rate;
      a7 *= a7 * rate; a8 *= a8 * rate;
      
      // reduce the angle to the range (-0.5, 0.5) and store to
      // memory so we can load it into the vector unit
      v[0] = a1 - quickRound(a1, roundVal);
      v[1] = a2 - quickRound(a2, roundVal);
      v[2] = a3 - quickRound(a3, roundVal);
      v[3] = a4 - quickRound(a4, roundVal);
      v[4] = a5 - quickRound(a5, roundVal);
      v[5] = a6 - quickRound(a6, roundVal);
      v[6] = a7 - quickRound(a7, roundVal);
      v[7] = a8 - quickRound(a8, roundVal);
      
      // load angle into the vector unit
      x1 = vec_ld(0, v);
      x2 = vec_ld(16, v);
      
      // square to the range [0, 0.25)
      y1 = vec_madd(x1, x1, ZERO);
      y2 = vec_madd(x2, x2, ZERO);
      
      // perform the initial polynomial approximations
      s1 = vec_madd(x1,
                    vec_madd(y1,
                             vec_madd(y1,
                                      vec_madd(y1, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      s2 = vec_madd(x2,
                    vec_madd(y2,
                             vec_madd(y2,
                                      vec_madd(y2, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      c1 = vec_madd(y1,
                    vec_madd(y1, vec_madd(y1, CC3, CC2), CC1),
                    ONE);
      c2 = vec_madd(y2,
                    vec_madd(y2, vec_madd(y2, CC3, CC2), CC1),
                    ONE);
      
      // permute the loaded data into separate real and complex vectors
      dre1 = vec_perm(d1, d2, real);
      dim1 = vec_perm(d1, d2, imag);
      dre2 = vec_perm(d3, d4, real);
      dim2 = vec_perm(d3, d4, imag);
      
      // perform first angle doubling
      x1 = vec_nmsub(s1, s1, vec_madd(c1, c1, ZERO));
      x2 = vec_nmsub(s2, s2, vec_madd(c2, c2, ZERO));
      y1 = vec_madd(TWO, vec_madd(s1, c1, ZERO), ZERO);
      y2 = vec_madd(TWO, vec_madd(s2, c2, ZERO), ZERO);
      
      // calculate scaling factor to correct the magnitude
//      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
//      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
      m1 = vec_recip(vec_madd(y1, y1, vec_madd(x1, x1, ZERO)));
      m2 = vec_recip(vec_madd(y2, y2, vec_madd(x2, x2, ZERO)));
      
      // perform second angle doubling
      c1 = vec_nmsub(y1, y1, vec_madd(x1, x1, ZERO));
      c2 = vec_nmsub(y2, y2, vec_madd(x2, x2, ZERO));
      s1 = vec_madd(TWO, vec_madd(y1, x1, ZERO), ZERO);
      s2 = vec_madd(TWO, vec_madd(y2, x2, ZERO), ZERO);
      
      // correct the magnitude (final sine / cosine approximations)
      s1 = vec_madd(s1, m1, ZERO);
      s2 = vec_madd(s2, m2, ZERO);
      c1 = vec_madd(c1, m1, ZERO);
      c2 = vec_madd(c2, m2, ZERO);
      
      // chirp the data
      cre1 = vec_nmsub(dim1, s1, vec_madd(dre1, c1, ZERO));
      cre2 = vec_nmsub(dim2, s2, vec_madd(dre2, c2, ZERO));
      cim1 = vec_madd(dim1, c1, vec_madd(dre1, s1, ZERO));
      cim2 = vec_madd(dim2, c2, vec_madd(dre2, s2, ZERO));
      
      // permute into interleaved form
      cd1 = vec_mergeh(cre1,cim1);
      cd2 = vec_mergel(cre1,cim1);
      cd3 = vec_mergeh(cre2,cim2);
      cd4 = vec_mergel(cre2,cim2);
      
      // store chirped values
      vec_st(cd1, 0, c);
      vec_st(cd2, 16, c);
      vec_st(cd3, 32, c);
      vec_st(cd4, 48, c);
	}
    
    // handle tail elements with scalar code
    for (; i < ul_NumDataPoints; ++i) {
      double angle = rate * i * i * 0.5;
      double s = sin(angle);
      double c = cos(angle);
      float re = cx_DataArray[i][0];
      float im = cx_DataArray[i][1];
      
      cx_ChirpDataArray[i][0] = re * c - im * s;
      cx_ChirpDataArray[i][1] = re * s + im * c;
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
  }
  return 0;
}


int v_vChirpDataG4(
                 sah_complex* cx_DataArray,
                 sah_complex* cx_ChirpDataArray,
                 int chirp_rate_ind,
                 double chirp_rate,
                 int  ul_NumDataPoints,
                 double sample_rate
                 ) {
  // int offset, int numPoints, double chirpRate
  int i;
  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
  
  if (chirp_rate_ind == 0) {
    memcpy(cx_ChirpDataArray,
           cx_DataArray,
           (int)ul_NumDataPoints * sizeof(sah_complex)
           );
  } else {
    int vEnd;  
    const vector unsigned char real = (vector unsigned char) (0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27);
	const vector unsigned char imag = (vector unsigned char) (4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31);
    double rate = chirp_rate * 0.5 / (sample_rate * sample_rate);
    double roundVal = rate >= 0.0 ? TWO_TO_52 : -TWO_TO_52;
    
    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    for (i = 0; i < vEnd; i += 8) {
      const float *d = (const float *) (cx_DataArray + i);
      float *c = (float *) (cx_ChirpDataArray + i);
      double a1 = i;
      double a2 = i + 1.0;
      double a3 = i + 2.0;
      double a4 = i + 3.0;
      double a5 = i + 4.0;
      double a6 = i + 5.0;
      double a7 = i + 6.0;
      double a8 = i + 7.0;
      float v[8] __attribute__ ((aligned (16)));
      vector float d1, d2, d3, d4;
      vector float cd1, cd2, cd3, cd4;
      vector float dre1, dre2;
      vector float dim1, dim2;
      vector float cre1, cre2;
      vector float cim1, cim2;
      vector float x1, x2;
      vector float y1, y2;
      vector float s1, s2;
      vector float c1, c2;
      vector float m1, m2;      
      
      // prefetch next 256 bytes into L1 cache
      vec_dstt(d, 0x10010100, 0);
      
      // load the signal to be chirped
      d1 = vec_ld(0, d);
      d2 = vec_ld(16, d);
      d3 = vec_ld(32, d);
      d4 = vec_ld(48, d);
      
      // calculate the input angle
      a1 *= a1 * rate; a2 *= a2 * rate;
      a3 *= a3 * rate; a4 *= a4 * rate;
      a5 *= a5 * rate; a6 *= a6 * rate;
      a7 *= a7 * rate; a8 *= a8 * rate;
      
      // reduce the angle to the range (-0.5, 0.5) and store to
      // memory so we can load it into the vector unit
      v[0] = a1 - quickRound(a1, roundVal);
      v[1] = a2 - quickRound(a2, roundVal);
      v[2] = a3 - quickRound(a3, roundVal);
      v[3] = a4 - quickRound(a4, roundVal);
      v[4] = a5 - quickRound(a5, roundVal);
      v[5] = a6 - quickRound(a6, roundVal);
      v[6] = a7 - quickRound(a7, roundVal);
      v[7] = a8 - quickRound(a8, roundVal);
      
      // load angle into the vector unit
      x1 = vec_ld(0, v);
      x2 = vec_ld(16, v);
      
      // square to the range [0, 0.25)
      y1 = vec_madd(x1, x1, ZERO);
      y2 = vec_madd(x2, x2, ZERO);
      
      // perform the initial polynomial approximations
      s1 = vec_madd(x1,
                    vec_madd(y1,
                             vec_madd(y1,
                                      vec_madd(y1, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      s2 = vec_madd(x2,
                    vec_madd(y2,
                             vec_madd(y2,
                                      vec_madd(y2, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      c1 = vec_madd(y1,
                    vec_madd(y1, vec_madd(y1, CC3, CC2), CC1),
                    ONE);
      c2 = vec_madd(y2,
                    vec_madd(y2, vec_madd(y2, CC3, CC2), CC1),
                    ONE);
      
      // permute the loaded data into separate real and complex vectors
      dre1 = vec_perm(d1, d2, real);
      dim1 = vec_perm(d1, d2, imag);
      dre2 = vec_perm(d3, d4, real);
      dim2 = vec_perm(d3, d4, imag);
      
      // perform first angle doubling
      x1 = vec_nmsub(s1, s1, vec_madd(c1, c1, ZERO));
      x2 = vec_nmsub(s2, s2, vec_madd(c2, c2, ZERO));
      y1 = vec_madd(TWO, vec_madd(s1, c1, ZERO), ZERO);
      y2 = vec_madd(TWO, vec_madd(s2, c2, ZERO), ZERO);
      
      // calculate scaling factor to correct the magnitude
      //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
      //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
      m1 = vec_recip(vec_madd(y1, y1, vec_madd(x1, x1, ZERO)));
      m2 = vec_recip(vec_madd(y2, y2, vec_madd(x2, x2, ZERO)));
      
      // zero out a spot for our chirped data in the cache
      __dcbz(0, c);
      __dcbz(32, c);

      // perform second angle doubling
      c1 = vec_nmsub(y1, y1, vec_madd(x1, x1, ZERO));
      c2 = vec_nmsub(y2, y2, vec_madd(x2, x2, ZERO));
      s1 = vec_madd(TWO, vec_madd(y1, x1, ZERO), ZERO);
      s2 = vec_madd(TWO, vec_madd(y2, x2, ZERO), ZERO);
      
      // correct the magnitude (final sine / cosine approximations)
      s1 = vec_madd(s1, m1, ZERO);
      s2 = vec_madd(s2, m2, ZERO);
      c1 = vec_madd(c1, m1, ZERO);
      c2 = vec_madd(c2, m2, ZERO);
      
      // chirp the data
      cre1 = vec_nmsub(dim1, s1, vec_madd(dre1, c1, ZERO));
      cre2 = vec_nmsub(dim2, s2, vec_madd(dre2, c2, ZERO));
      cim1 = vec_madd(dim1, c1, vec_madd(dre1, s1, ZERO));
      cim2 = vec_madd(dim2, c2, vec_madd(dre2, s2, ZERO));
      
      // permute into interleaved form
      cd1 = vec_mergeh(cre1,cim1);
      cd2 = vec_mergel(cre1,cim1);
      cd3 = vec_mergeh(cre2,cim2);
      cd4 = vec_mergel(cre2,cim2);
      
      // store chirped values
      vec_st(cd1, 0, c);
      vec_st(cd2, 16, c);
      vec_st(cd3, 32, c);
      vec_st(cd4, 48, c);
	}
    
    // handle tail elements with scalar code
    for (; i < ul_NumDataPoints; ++i) {
      double angle = rate * i * i * 0.5;
      double s = sin(angle);
      double c = cos(angle);
      float re = cx_DataArray[i][0];
      float im = cx_DataArray[i][1];
      
      cx_ChirpDataArray[i][0] = re * c - im * s;
      cx_ChirpDataArray[i][1] = re * s + im * c;
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
  }
  return 0;
}


int v_vChirpDataG5(
                 sah_complex* cx_DataArray,
                 sah_complex* cx_ChirpDataArray,
                 int chirp_rate_ind,
                 double chirp_rate,
                 int  ul_NumDataPoints,
                 double sample_rate
                 ) {
  // int offset, int numPoints, double chirpRate
  int i;
  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
  
  if (chirp_rate_ind == 0) {
    memcpy(cx_ChirpDataArray,
           cx_DataArray,
           (int)ul_NumDataPoints * sizeof(sah_complex)
           );
  } else {
    int vEnd;  
    const vector unsigned char real = (vector unsigned char) (0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27);
	const vector unsigned char imag = (vector unsigned char) (4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31);
    double rate = chirp_rate * 0.5 / (sample_rate * sample_rate);
    double roundVal = rate >= 0.0 ? TWO_TO_52 : -TWO_TO_52;
    
    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 15);
    for (i = 0; i < vEnd; i += 16) {
      const float *d = (const float *) (cx_DataArray + i);
      float *c = (float *) (cx_ChirpDataArray + i);
      double a1 = i;
      double a2 = i + 1.0;
      double a3 = i + 2.0;
      double a4 = i + 3.0;
      double a5 = i + 4.0;
      double a6 = i + 5.0;
      double a7 = i + 6.0;
      double a8 = i + 7.0;
      double a9 = i + 8.0;
      double a10 = i + 9.0;
      double a11 = i + 10.0;
      double a12 = i + 11.0;
      double a13 = i + 12.0;
      double a14 = i + 13.0;
      double a15 = i + 14.0;
      double a16 = i + 15.0;
      float v[16] __attribute__ ((aligned (16)));
      vector float d1, d2, d3, d4, d5, d6, d7, d8;
      vector float cd1, cd2, cd3, cd4, cd5, cd6, cd7, cd8;
      vector float dre1, dre2, dre3, dre4;
      vector float dim1, dim2, dim3, dim4;
      vector float cre1, cre2, cre3, cre4;
      vector float cim1, cim2, cim3, cim4;
      vector float x1, x2, x3, x4;
      vector float y1, y2, y3, y4;
      vector float s1, s2, s3, s4;
      vector float c1, c2, c3, c4;
      vector float m1, m2, m3, m4;
      
      // load the signal to be chirped
      d1 = vec_ld(0, d);
      d2 = vec_ld(16, d);
      d3 = vec_ld(32, d);
      d4 = vec_ld(48, d);
      d5 = vec_ld(64, d);
      d6 = vec_ld(80, d);
      d7 = vec_ld(96, d);
      d8 = vec_ld(112, d);
      
      // calculate the input angle
      a1 *= a1 * rate; a2 *= a2 * rate;
      a3 *= a3 * rate; a4 *= a4 * rate;
      a5 *= a5 * rate; a6 *= a6 * rate;
      a7 *= a7 * rate; a8 *= a8 * rate;
      a9 *= a9 * rate; a10 *= a10 * rate;
      a11 *= a11 * rate; a12 *= a12 * rate;
      a13 *= a13 * rate; a14 *= a14 * rate;
      a15 *= a15 * rate; a16 *= a16 * rate;
      
      // reduce the angle to the range (-0.5, 0.5) and store to
      // memory so we can load it into the vector unit
      v[0] = a1 - quickRound(a1, roundVal);
      v[1] = a2 - quickRound(a2, roundVal);
      v[2] = a3 - quickRound(a3, roundVal);
      v[3] = a4 - quickRound(a4, roundVal);
      v[4] = a5 - quickRound(a5, roundVal);
      v[5] = a6 - quickRound(a6, roundVal);
      v[6] = a7 - quickRound(a7, roundVal);
      v[7] = a8 - quickRound(a8, roundVal);
      v[8] = a9 - quickRound(a9, roundVal);
      v[9] = a10 - quickRound(a10, roundVal);
      v[10] = a11 - quickRound(a11, roundVal);
      v[11] = a12 - quickRound(a12, roundVal);
      v[12] = a13 - quickRound(a13, roundVal);
      v[13] = a14 - quickRound(a14, roundVal);
      v[14] = a15 - quickRound(a15, roundVal);
      v[15] = a16 - quickRound(a16, roundVal);
      
      // load angle into the vector unit
      x1 = vec_ld(0, v);
      x2 = vec_ld(16, v);
      x3 = vec_ld(32, v);
      x4 = vec_ld(48, v);
      
      // square to the range [0, 0.25)
      y1 = vec_madd(x1, x1, ZERO);
      y2 = vec_madd(x2, x2, ZERO);
      y3 = vec_madd(x3, x3, ZERO);
      y4 = vec_madd(x4, x4, ZERO);
      
      // perform the initial polynomial approximations
      s1 = vec_madd(x1,
                    vec_madd(y1,
                             vec_madd(y1,
                                      vec_madd(y1, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      s2 = vec_madd(x2,
                    vec_madd(y2,
                             vec_madd(y2,
                                      vec_madd(y2, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      c1 = vec_madd(y1,
                    vec_madd(y1, vec_madd(y1, CC3, CC2), CC1),
                    ONE);
      c2 = vec_madd(y2,
                    vec_madd(y2, vec_madd(y2, CC3, CC2), CC1),
                    ONE);
      s3 = vec_madd(x3,
                    vec_madd(y3,
                             vec_madd(y3,
                                      vec_madd(y3, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      s4 = vec_madd(x4,
                    vec_madd(y4,
                             vec_madd(y4,
                                      vec_madd(y4, SS4, SS3),
                                      SS2),
                             SS1),
                    ZERO);
      c3 = vec_madd(y3,
                    vec_madd(y3, vec_madd(y3, CC3, CC2), CC1),
                    ONE);
      c4 = vec_madd(y4,
                    vec_madd(y4, vec_madd(y4, CC3, CC2), CC1),
                    ONE);
      
      // permute the loaded data into separate real and complex vectors
      dre1 = vec_perm(d1, d2, real);
      dim1 = vec_perm(d1, d2, imag);
      dre2 = vec_perm(d3, d4, real);
      dim2 = vec_perm(d3, d4, imag);
      dre3 = vec_perm(d5, d6, real);
      dim3 = vec_perm(d5, d6, imag);
      dre4 = vec_perm(d7, d8, real);
      dim4 = vec_perm(d7, d8, imag);
      
      // perform first angle doubling
      x1 = vec_nmsub(s1, s1, vec_madd(c1, c1, ZERO));
      x2 = vec_nmsub(s2, s2, vec_madd(c2, c2, ZERO));
      y1 = vec_madd(TWO, vec_madd(s1, c1, ZERO), ZERO);
      y2 = vec_madd(TWO, vec_madd(s2, c2, ZERO), ZERO);
      x3 = vec_nmsub(s3, s3, vec_madd(c3, c3, ZERO));
      x4 = vec_nmsub(s4, s4, vec_madd(c4, c4, ZERO));
      y3 = vec_madd(TWO, vec_madd(s3, c3, ZERO), ZERO);
      y4 = vec_madd(TWO, vec_madd(s4, c4, ZERO), ZERO);
      
      // calculate scaling factor to correct the magnitude
      //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
      //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
      m1 = vec_recip(vec_madd(y1, y1, vec_madd(x1, x1, ZERO)));
      m2 = vec_recip(vec_madd(y2, y2, vec_madd(x2, x2, ZERO)));
      m3 = vec_recip(vec_madd(y3, y3, vec_madd(x3, x3, ZERO)));
      m4 = vec_recip(vec_madd(y4, y4, vec_madd(x4, x4, ZERO)));
      
      // perform second angle doubling
      c1 = vec_nmsub(y1, y1, vec_madd(x1, x1, ZERO));
      c2 = vec_nmsub(y2, y2, vec_madd(x2, x2, ZERO));
      s1 = vec_madd(TWO, vec_madd(y1, x1, ZERO), ZERO);
      s2 = vec_madd(TWO, vec_madd(y2, x2, ZERO), ZERO);
      c3 = vec_nmsub(y3, y3, vec_madd(x3, x3, ZERO));
      c4 = vec_nmsub(y4, y4, vec_madd(x4, x4, ZERO));
      s3 = vec_madd(TWO, vec_madd(y3, x3, ZERO), ZERO);
      s4 = vec_madd(TWO, vec_madd(y4, x4, ZERO), ZERO);
      
      // correct the magnitude (final sine / cosine approximations)
      s1 = vec_madd(s1, m1, ZERO);
      s2 = vec_madd(s2, m2, ZERO);
      c1 = vec_madd(c1, m1, ZERO);
      c2 = vec_madd(c2, m2, ZERO);
      s3 = vec_madd(s3, m3, ZERO);
      s4 = vec_madd(s4, m4, ZERO);
      c3 = vec_madd(c3, m3, ZERO);
      c4 = vec_madd(c4, m4, ZERO);
      
      // chirp the data
      cre1 = vec_nmsub(dim1, s1, vec_madd(dre1, c1, ZERO));
      cre2 = vec_nmsub(dim2, s2, vec_madd(dre2, c2, ZERO));
      cim1 = vec_madd(dim1, c1, vec_madd(dre1, s1, ZERO));
      cim2 = vec_madd(dim2, c2, vec_madd(dre2, s2, ZERO));
      cre3 = vec_nmsub(dim3, s3, vec_madd(dre3, c3, ZERO));
      cre4 = vec_nmsub(dim4, s4, vec_madd(dre4, c4, ZERO));
      cim3 = vec_madd(dim3, c3, vec_madd(dre3, s3, ZERO));
      cim4 = vec_madd(dim4, c4, vec_madd(dre4, s4, ZERO));
      
      // permute into interleaved form
      cd1 = vec_mergeh(cre1,cim1);
      cd2 = vec_mergel(cre1,cim1);
      cd3 = vec_mergeh(cre2,cim2);
      cd4 = vec_mergel(cre2,cim2);
      cd5 = vec_mergeh(cre3,cim3);
      cd6 = vec_mergel(cre3,cim3);
      cd7 = vec_mergeh(cre4,cim4);
      cd8 = vec_mergel(cre4,cim4);
      
      // store chirped values
      vec_st(cd1, 0, c);
      vec_st(cd2, 16, c);
      vec_st(cd3, 32, c);
      vec_st(cd4, 48, c);
      vec_st(cd5, 64, c);
      vec_st(cd6, 80, c);
      vec_st(cd7, 96, c);
      vec_st(cd8, 112, c);
	}
    
    // handle tail elements with scalar code
    for (; i < ul_NumDataPoints; ++i) {
      double angle = rate * i * i * 0.5;
      double s = sin(angle);
      double c = cos(angle);
      float re = cx_DataArray[i][0];
      float im = cx_DataArray[i][1];
      
      cx_ChirpDataArray[i][0] = re * c - im * s;
      cx_ChirpDataArray[i][1] = re * s + im * c;
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
  }
  return 0;
}

float f_vGetChiSq(
  float fp_PoT[],
  int ul_PowerLen,
  int ul_TOffset,
  float f_PeakPower,
  float f_MeanPower,
  float f_weight[],
  float *xsq_null=0
) {
  // We calculate our assumed gaussian powers
  // on the fly as we try to fit them to the
  // actual powers at each point along the PoT.
 
  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
 
  int i;
  int vEnd, vul_PowerLen;
  float f_ChiSq=0,f_null_hyp=0, f_PredictedPower;
  vector float v_PeakPower, v_MeanPower, v_rMeanPower, v_rebin;
  vector float v_TempChiSq = ZERO, v_ChiSq, v_temp_null_hyp = ZERO, v_null_hyp, v_PredictedPower, v_weight;
  double rebin=swi.nsamples/ChirpFftPairs[analysis_state.icfft].FftLen
    /ul_PowerLen;
  
  // splat Altivec versions of function constants
  v_PeakPower = vec_fpsplat(f_PeakPower);
  v_MeanPower = vec_fpsplat(f_MeanPower);
  v_rMeanPower = vec_fpsplat(1.0f / f_MeanPower);
  v_rebin = vec_fpsplat((float) rebin);
  
  // set up unaligned loads for weight array
  int offwt = PoTInfo.GaussTOffsetStop - 1 - ul_TOffset;
  vector float msqwt = vec_ld(0, f_weight + offwt);
  vector unsigned char maskwt = vec_add(vec_lvsl(-1L, f_weight + offwt), vec_splat_u8(1));
  
  vul_PowerLen = ul_PowerLen - (ul_PowerLen & 0x3);
  for (i = 0; i < vul_PowerLen; i += 4) {
    const float *p = fp_PoT + i;
    const float *wt = f_weight + offwt + i;
    vector float lsqwt = vec_ld(15, wt);
    vector float v_PoT = vec_ld(0, p);
    vector float v_weight = vec_perm(msqwt, lsqwt, maskwt);
    vector float v_noise, v_rnoise;

    v_PredictedPower = vec_madd(v_PeakPower, v_weight, v_MeanPower);
    v_PredictedPower = vec_madd(v_PredictedPower, v_rMeanPower, ZERO);
    v_noise = vec_madd(vec_sqrt(vec_sub(vec_max(v_PredictedPower, ONE), ONE)), TWO, ONE);
    v_rnoise = vec_recip(v_noise);
    
    v_TempChiSq = vec_madd(vec_madd(vec_madd(vec_nmsub(v_PoT, v_rMeanPower, v_PredictedPower),
                                             vec_nmsub(v_PoT, v_rMeanPower, v_PredictedPower),
                                             ZERO),
                                    v_rnoise, ZERO),
                           v_rebin,
                           v_TempChiSq);
    v_temp_null_hyp = vec_madd(vec_madd(vec_madd(vec_nmsub(v_PoT, v_rMeanPower, ONE),
                                            vec_nmsub(v_PoT, v_rMeanPower, ONE),
                                            ZERO),
                                   v_rnoise, ZERO),
                          v_rebin,
                          v_temp_null_hyp);
    
    msqwt = lsqwt;
  }
  v_ChiSq = v_TempChiSq;
  v_null_hyp = v_temp_null_hyp;
  
  // handle tail elements
  for (; i < ul_PowerLen; i++) {
    f_PredictedPower = f_MeanPower + f_PeakPower * f_weight[offwt+i] ;
    // ChiSq in this realm is:
    //  sum[0:i]( (observed power - expected power)^2 / expected variance )
    // The power of a signal is:
    //  power = (noise + signal)^2 = noise^2 + signal^2 + 2*noise*signal
    // With mean power normalization, noise becomes 1, leaving:
    //  power = signal^2 +or- 2*signal + 1
    f_PredictedPower/=f_MeanPower;
    double signal=f_PredictedPower;
    double noise=(2.0*sqrt(std::max(f_PredictedPower,1.0f)-1)+1);
    
    f_ChiSq += 
      (static_cast<float>(rebin)*SQUARE(fp_PoT[i]/f_MeanPower - signal)/noise);
    f_null_hyp+=
      (static_cast<float>(rebin)*SQUARE(fp_PoT[i]/f_MeanPower-1)/noise);
  }
  
  // sum vector and scalar elements
  float *vp_ChiSq = (float *) &v_ChiSq;
  float *vp_null_hyp = (float *) &v_null_hyp;
  f_ChiSq += vp_ChiSq[0] + vp_ChiSq[1] + vp_ChiSq[2] + vp_ChiSq[3];
  f_null_hyp += vp_null_hyp[0] + vp_null_hyp[1] + vp_null_hyp[2] + vp_null_hyp[3];
  
  analysis_state.FLOP_counter+=20.0*ul_PowerLen+5;
  f_ChiSq/=ul_PowerLen;
  f_null_hyp/=ul_PowerLen;
  
  if (xsq_null) *xsq_null=f_null_hyp;
  return f_ChiSq;
}


float f_vGetTrueMean(
                    float fp_PoT[],
                    int ul_PowerLen,
                    float f_TotalPower,
                    int ul_TOffset,
                    int ul_ExcludeLen
                    ) {
  // TrueMean is the mean power of the data set minus all power
  // out to ExcludeLen from our current TOffset.
  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;
  int i, i_start, i_lim, vStart, vEnd;
  float f_ExcludePower = 0;
  vector float v_TempExcludePower = ZERO, v_ExcludePower;
  
  // take care that we do not add to exclude power beyond PoT bounds!
  i_start = std::max(ul_TOffset - ul_ExcludeLen, 0);
  i_lim = std::min<int>(ul_TOffset + ul_ExcludeLen + 1, swi.analysis_cfg.gauss_pot_length);
  
  vStart = i_start + 3;
  vStart = vStart - (vStart & 3);
  vEnd = i_lim - (i_lim & 3);
  
  // handle leading elements
  for (i = i_start; i < vStart; i++) {
    f_ExcludePower += fp_PoT[i];
  }
  for (; i < vEnd; i += 4) {
    v_TempExcludePower = vec_add(vec_ld(0, fp_PoT + i), v_TempExcludePower);
  }
  v_ExcludePower = v_TempExcludePower;
  // handle tail elements
  for (; i < i_lim; i++) {
    f_ExcludePower += fp_PoT[i];
  }
  
  // sum vector and scalar elements
  float *vp_ExcludePower = (float *) &v_ExcludePower;
  f_ExcludePower += vp_ExcludePower[0] + vp_ExcludePower[1] + vp_ExcludePower[2] + vp_ExcludePower[3];  
  
  analysis_state.FLOP_counter+=(double)(i_lim-i_start+5);
  return((f_TotalPower - f_ExcludePower) / (ul_PowerLen - (i - i_start)));
}





int vGaussFit(
  float * fp_PoT,
  int ul_FftLength,
  int ul_PoT
) {
  int i, retval;
  BOOLEAN b_IsAPeak;
  float f_NormMaxPower;
  float f_null_hyp;

  if (!AltiVec_Available()) return UNSUPPORTED_FUNCTION;

  int ul_TOffset;
  int iSigma = static_cast<int>(floor(PoTInfo.GaussSigma+0.5));
  double diSigma = iSigma;
  float f_GroupSum, f_GroupMax;
  int i_f, iPeakLoc;

  float f_TotalPower,
        f_TrueMean,
        f_ChiSq,
        f_PeakPower;
  float f_rMeanPower;
  float f_TotalPowerOverTrueMeanElems;
  vector float v_TotalPower;
  vector float v_rMeanPower;
  vector float v_PowerThresh;
  
  // For setiathome the Sigma and Gaussian PoT length don't change during
  // a run of the application, so these frequently used values can be
  // precalculated and kept.
  static float f_PeakScaleFactor;
  static float *f_weight;
  static float *f_vweight;
  static float *f_PeakFilter;
  static float *f_BoxFilter;
  static float *f_corr;
  static float *f_swsum;
  static float f_rTrueMeanElems = 1.0f / (swi.analysis_cfg.gauss_pot_length - (4*iSigma+1));

  if (!f_weight) {
    f_PeakScaleFactor = f_GetPeakScaleFactor(static_cast<float>(PoTInfo.GaussSigma));
    // Altivec code uses double-sided f_weight to make memory accesses simpler
    f_vweight = reinterpret_cast<float *>(malloc((2*PoTInfo.GaussTOffsetStop-1)*sizeof(float)));
    if (!f_vweight) SETIERROR(MALLOC_FAILED, "!f_vweight");
    f_vweight[PoTInfo.GaussTOffsetStop-1] = 1;
    
    f_PeakFilter = reinterpret_cast<float *>(malloc((2*iSigma+1)*sizeof(float)));
    if (!f_PeakFilter) SETIERROR(MALLOC_FAILED, "!f_PeakFilter");
    f_BoxFilter = reinterpret_cast<float *>(malloc((4*iSigma+1)*sizeof(float)));
    if (!f_BoxFilter) SETIERROR(MALLOC_FAILED, "!f_BoxFilter");
    f_PeakFilter[iSigma] = f_PeakScaleFactor;
    f_BoxFilter[2*iSigma] = f_rTrueMeanElems;
    
    f_weight = reinterpret_cast<float *>(malloc(PoTInfo.GaussTOffsetStop*sizeof(float)));
    if (!f_weight) SETIERROR(MALLOC_FAILED, "!f_weight");
    f_weight[0] = 1;

    for (i = 1; i <= iSigma; i++) {
      float weight = static_cast<float>(EXP(i, 0, PoTInfo.GaussSigmaSq));
      f_vweight[PoTInfo.GaussTOffsetStop-1+i] = weight;
      f_vweight[PoTInfo.GaussTOffsetStop-1-i] = weight;
      f_PeakFilter[iSigma+i] = weight * f_PeakScaleFactor;
      f_PeakFilter[iSigma-i] = weight * f_PeakScaleFactor;
      f_BoxFilter[2*iSigma+i] = f_rTrueMeanElems;
      f_BoxFilter[2*iSigma-i] = f_rTrueMeanElems;
      
      f_weight[i] = weight;
    }
    
    for (; i <= 2*iSigma; i++) {
      float weight = static_cast<float>(EXP(i, 0, PoTInfo.GaussSigmaSq));
      f_vweight[PoTInfo.GaussTOffsetStop-1+i] = weight;
      f_vweight[PoTInfo.GaussTOffsetStop-1-i] = weight;
      f_BoxFilter[2*iSigma+i] = f_rTrueMeanElems;
      f_BoxFilter[2*iSigma-i] = f_rTrueMeanElems;
      
      f_weight[i] = weight;      
    }

    for (; i < PoTInfo.GaussTOffsetStop; i++) {
      float weight = static_cast<float>(EXP(i, 0, PoTInfo.GaussSigmaSq));
      f_vweight[PoTInfo.GaussTOffsetStop-1+i] = weight;
      f_vweight[PoTInfo.GaussTOffsetStop-1-i] = weight;

      f_weight[i] = weight;
    }    
  }
  
  // Find reciprocal of mean over power-of-time array
  // Assumes that swi.analysis_cfg.gauss_pot_length is a multiple of 4
  v_TotalPower = ZERO;
  for (i = 0; i < swi.analysis_cfg.gauss_pot_length; i += 4) {
    const float *p = fp_PoT + i;
    vector float v_PoT = vec_ld(0, p);
    v_TotalPower = vec_add(v_TotalPower, v_PoT);
  }
  v_TotalPower = vec_add(v_TotalPower, vec_sld(v_TotalPower, v_TotalPower, 8));
  v_TotalPower = vec_add(v_TotalPower, vec_sld(v_TotalPower, v_TotalPower, 4));
  v_rMeanPower = vec_madd(vec_recip(v_TotalPower),
                         vec_fpsplat((float) swi.analysis_cfg.gauss_pot_length), ZERO);
  
  // Normalize power-of-time and check for the existence
  // of at least 1 peak.
  b_IsAPeak = false;
  v_PowerThresh = vec_fpsplat(PoTInfo.GaussPowerThresh);
  for (i = 0; i < swi.analysis_cfg.gauss_pot_length; i += 4) {
    float *p = fp_PoT + i;
    vector float v_PoT = vec_ld(0, p);
    v_PoT = vec_madd(v_PoT, v_rMeanPower, ZERO);
    b_IsAPeak |= vec_any_gt(v_PoT, v_PowerThresh);
    vec_st(v_PoT, 0, p);
  }
  analysis_state.FLOP_counter+=3.0*swi.analysis_cfg.gauss_pot_length+2;

  if (!b_IsAPeak) {
    //printf("no peak\n");
    return 0;  // no peak - bail on this PoT
  }

  // Recalculate Total Power across normalized array.
  // Given that powers are positive, f_TotalPower will
  // end up being == swi.analysis_cfg.gauss_pot_length.
  f_TotalPower   = 0;
  f_NormMaxPower = 0;
  // Also locate group with the highest sum for use in second bailout check.
  f_GroupSum     = 0;
  f_GroupMax     = 0;
  for (i = 0, i_f = -iSigma; i < swi.analysis_cfg.gauss_pot_length; i++, i_f++) {
    f_TotalPower += fp_PoT[i];
    if(fp_PoT[i] > f_NormMaxPower) f_NormMaxPower = fp_PoT[i];
    f_GroupSum += fp_PoT[i] - ((i_f < 0) ? 0.0f : fp_PoT[i_f]);
    if (f_GroupSum > f_GroupMax) {
      f_GroupMax = f_GroupSum;
      iPeakLoc = i - iSigma/2;
    }
  }
  f_TotalPowerOverTrueMeanElems = f_TotalPower * f_rTrueMeanElems;
  
  // Check at the group peak location whether data may contain Gaussians
  // (but only after the first hurry-up Gaussian has been set for graphics)

  if (best_gauss->display_power_thresh != 0) {
    iPeakLoc = std::max(PoTInfo.GaussTOffsetStart,
                       (std::min(PoTInfo.GaussTOffsetStop - 1, iPeakLoc)));
  
    f_TrueMean = f_vGetTrueMean(
                   fp_PoT,
                   swi.analysis_cfg.gauss_pot_length,
                   f_TotalPower,
                   iPeakLoc,
                   2 * iSigma
                 );
  
    f_PeakPower = f_GetPeak(
                    fp_PoT,
                    iPeakLoc,
                    iSigma,
                    f_TrueMean,
                    f_PeakScaleFactor,
                    f_weight
                  );

    analysis_state.FLOP_counter+=5.0*swi.analysis_cfg.gauss_pot_length+5;

    if (f_PeakPower < f_TrueMean * best_gauss->display_power_thresh*0.5f) {
      return 0;  // not even a weak peak at max group - bail on this PoT
    }
  }

  f_corr = reinterpret_cast<float *>(malloc((PoTInfo.GaussTOffsetStop - PoTInfo.GaussTOffsetStart)*sizeof(float)));
  f_swsum = reinterpret_cast<float *>(malloc((swi.analysis_cfg.gauss_pot_length-4*iSigma)*sizeof(float)));
  conv(fp_PoT, 1, f_PeakFilter, 1, f_corr, 1,
       PoTInfo.GaussTOffsetStop - PoTInfo.GaussTOffsetStart, 2*iSigma+1);
  conv(fp_PoT, 1, f_BoxFilter, 1, f_swsum, 1,
       swi.analysis_cfg.gauss_pot_length-4*iSigma, 4*iSigma+1);
  
  // slide dynamic gaussian across the Power Of Time array
  double loop_flops = 0;
  float display_power_thresh = best_gauss->display_power_thresh;
  float f_tempTotalPowerOverTrueMeanElems = f_TotalPowerOverTrueMeanElems;
  for (ul_TOffset = PoTInfo.GaussTOffsetStart;
       ul_TOffset < PoTInfo.GaussTOffsetStop;
       ul_TOffset++
      ) {

    // TrueMean is the mean power of the data set minus all power
    // out to 2 sigma from our current TOffset.
    if ((ul_TOffset - 2 * iSigma) < 0 || (ul_TOffset + 2 * iSigma) >= swi.analysis_cfg.gauss_pot_length) {
      f_TrueMean = f_vGetTrueMean(
                     fp_PoT,
                     swi.analysis_cfg.gauss_pot_length,
                     f_TotalPower,
                     ul_TOffset,
                     2 * iSigma
                   );
    } else {
      f_TrueMean = f_tempTotalPowerOverTrueMeanElems - f_swsum[ul_TOffset - 2 * iSigma];
      loop_flops+=4.0*diSigma+6.0;
    }
    
    f_PeakPower = f_corr[ul_TOffset-iSigma] - f_TrueMean;
    loop_flops+=6.0*diSigma;
    
    // worth looking at ?
    if (f_PeakPower < f_TrueMean * display_power_thresh) {
      continue;
    }

    // bump up the display threshold to its final value.
    // We could bump it up only to the gaussian just found,
    // but that would cause a lot of time waste
    // computing chisq etc.
    if (display_power_thresh == 0) {
    	display_power_thresh = best_gauss->display_power_thresh = PoTInfo.GaussPeakPowerThresh/3;
    }

    f_ChiSq = f_vGetChiSq(
                fp_PoT,
                swi.analysis_cfg.gauss_pot_length,
                ul_TOffset,
                f_PeakPower,
                f_TrueMean,
                f_vweight,
                &f_null_hyp
              );
    
    retval = ChooseGaussEvent(
               ul_TOffset,
               f_PeakPower,
               f_TrueMean,
               f_ChiSq,
	       f_null_hyp,
               ul_PoT,
               static_cast<float>(PoTInfo.GaussSigma),
               f_NormMaxPower,
               fp_PoT
             );
    if (retval) SETIERROR(retval,"from ChooseGaussEvent");

  } // End of sliding gaussian
  
  analysis_state.FLOP_counter+=loop_flops;
  free(f_corr);
  free(f_swsum);

  return 0;

} // End of vGaussFit()


/**********************
*
* Subroutines for pulse folding, refactored by Joe Segur from code by Alex Kan
*
*/
inline vector unsigned int vec_uisplat(unsigned int ui_Constant) {
  vector unsigned char vuc_Splat;
  vector unsigned int vui_Constant;
  
  vuc_Splat = vec_lvsl(0, &ui_Constant);
  vui_Constant = vec_lde(0, &ui_Constant);
  vuc_Splat = (vector unsigned char) vec_splat((vector unsigned int) vuc_Splat, 0);
  vui_Constant = vec_perm(vui_Constant, vui_Constant, vuc_Splat);
  
  return vui_Constant;
}

/**********************
*
* foldArrayBy2 - Perform "folds" on 2 adjacent parts of the power over time
*   array. Each element in the "folded" array is the sum of the 2 elements
*   in the power over time array. Array is folded in-place.
*
* Assumes 16-byte alignment of input array, but offsets within the array may be
*   arbitrary.
*
*/
float foldArrayBy2SPA(float *ss[], struct PoTPlan *P) {
  float max;
  int vEnd;
  vector float tempMaxV = ZERO, maxV;
  vector float msq2 = vec_ld(0, ss[1]+P->tmp0);
  vector float msqz;
  vector unsigned char mask2 = vec_add(vec_lvsl(-1L, ss[1]+P->tmp0), vec_splat_u8(1));
  vector unsigned int tailelem = {0, 1, 2, 3};
  vector unsigned int taillim = vec_uisplat(((P->di-1)&0x3) + 1);
  vector unsigned int tailmask;
  int i = 0, index = 0;
  
  //  int d_elemsWritten = 0;
  const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
  float *pst = P->dest;
  vector float lsq2, x1, x2, xs, zst;
      
#define F2_STAGE1 x1 = vec_ld(0, p1); lsq2 = vec_ld(15, p2); i += 4; p1 += 4; p2 += 4;
#define F2_STAGE2 x2 = vec_perm(msq2, lsq2, mask2); msq2 = lsq2; xs = vec_add(x1, x2);
                 // was vec_madd(xs, RECIP_TWO, ZERO);
#define F2_STAGE3 zst = xs;
#define F2_STAGE4 tempMaxV = vec_max(tempMaxV, zst); vec_st(zst, index, pst); pst += 4;
    
  // software pipelined main loop - omit last stage of tail element
  if (P->di-i >= 12) {
    F2_STAGE1;
    F2_STAGE2; F2_STAGE1;
    F2_STAGE3; F2_STAGE2; F2_STAGE1;
    while (i < P->di) {
      F2_STAGE4; F2_STAGE3; F2_STAGE2; F2_STAGE1;
    }
    F2_STAGE4; F2_STAGE3; F2_STAGE2;
    F2_STAGE4; F2_STAGE3;
//    F2_STAGE4;
  } else {    
    // main loop
    while (i < P->di-4) {
      F2_STAGE1; F2_STAGE2; F2_STAGE3; F2_STAGE4;
    }
    F2_STAGE1; F2_STAGE2; F2_STAGE3;
  }
  
#undef F2_STAGE1
#undef F2_STAGE2
#undef F2_STAGE3
#undef F2_STAGE4
  
  tailmask = (vector unsigned int) vec_cmplt(tailelem, taillim);
  zst = vec_and(zst, (vector float) tailmask);
  tempMaxV = vec_max(tempMaxV, zst);
  vec_st(zst, index, pst);
  
  // take the maximum across max and maxV and return it
  tempMaxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 8));
  maxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 4));
  float *maxVp = (float *) &maxV;
  max = *maxVp;
  
  return max;  
}

/* Assumes 16-byte alignment of both halves of input array
*/
float foldArrayBy2SPAL(float *ss[], struct PoTPlan *P) {
  float max;
  int vEnd;
  vector float tempMaxV = ZERO, maxV;
  vector unsigned int tailelem = {0, 1, 2, 3};
  vector unsigned int taillim = vec_uisplat(((P->di-1)&0x3) + 1);
  vector unsigned int tailmask;
  int i = 0, index = 0;
  
  //  int d_elemsWritten = 0;
  const float *p1 = ss[1]+P->offset + i, *p2 = ss[1]+P->tmp0 + i;
  float *pst = P->dest + i;
  vector float lsq1, lsq2, x1, x2, xs, lsqz, zst;
  
#define F2_STAGE1 x1 = vec_ld(0, p1); x2 = vec_ld(0, p2); i += 4; p1 += 4; p2 += 4;
#define F2_STAGE2 xs = vec_add(x1, x2);
                 // was vec_madd(xs, RECIP_TWO, ZERO);
#define F2_STAGE3 zst = xs;
#define F2_STAGE4 tempMaxV = vec_max(tempMaxV, zst); vec_st(zst, index, pst); pst += 4;
  
  // software pipelined main loop - omit last stage of tail element
  if (P->di-i >= 12) {
    F2_STAGE1;
    F2_STAGE2; F2_STAGE1;
    F2_STAGE3; F2_STAGE2; F2_STAGE1;
    while (i < P->di) {
      F2_STAGE4; F2_STAGE3; F2_STAGE2; F2_STAGE1;
    }
    F2_STAGE4; F2_STAGE3; F2_STAGE2;
    F2_STAGE4; F2_STAGE3;
    //    F2_STAGE4;
  } else {    
    // main loop
    while (i < P->di-4) {
      F2_STAGE1; F2_STAGE2; F2_STAGE3; F2_STAGE4;
    }
    F2_STAGE1; F2_STAGE2; F2_STAGE3;
  }
  
#undef F2_STAGE1
#undef F2_STAGE2
#undef F2_STAGE3
#undef F2_STAGE4
  
  tailmask = (vector unsigned int) vec_cmplt(tailelem, taillim);
  zst = vec_and(zst, (vector float) tailmask);
  tempMaxV = vec_max(tempMaxV, zst);
  vec_st(zst, index, pst);
  
  // take the maximum across max and maxV and return it
  tempMaxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 8));
  maxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 4));
  float *maxVp = (float *) &maxV;
  max = *maxVp;
  
  return max;  
}


/**********************
*
* foldArrayBy3 - Perform "folds" on 3 adjacent parts of the power over time
*   array. Each element in the "folded" array is the sum of the 3 elements
*   in the power over time array.
*
* Assumes 16-byte alignment of input and output arrays.
*
*/
float foldArrayBy3SP(float *ss[], struct PoTPlan *P) {
  float max;
  int vEnd;
  vector float tempMaxV = ZERO, maxV;
  vector float msq2 = vec_ld(0, ss[0]+P->tmp0);
  vector float msq3 = vec_ld(0, ss[0]+P->tmp1);
  vector unsigned char mask2 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp0), vec_splat_u8(1));
  vector unsigned char mask3 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp1), vec_splat_u8(1));
  vector unsigned int tailelem = {0, 1, 2, 3};
  vector unsigned int taillim = vec_uisplat(((P->di-1)&0x3) + 1);
  vector unsigned int tailmask;
  int i = 0;

  const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1;
  float *pst = P->dest;
  vector float lsq2, lsq3, x1, x2, x3, xs12, xs, z;
  
#define F3_STAGE1 lsq2 = vec_ld(15, p2); i += 4; p2 += 4; x1 = vec_ld(0, p1); x2 = vec_perm(msq2, lsq2, mask2); lsq3 = vec_ld(15, p3); p1 += 4; msq2 = lsq2; p3 += 4; xs12 = vec_add(x1, x2); x3 = vec_perm(msq3, lsq3, mask3); msq3 = lsq3;
#define F3_STAGE2 xs = vec_add(xs12, x3);
               // was vec_madd(xs, RECIP_THREE, ZERO);
#define F3_STAGE3 z = xs;
#define F3_STAGE4 tempMaxV = vec_max(tempMaxV, z); vec_st(z, 0, pst); pst += 4;
  
  // software pipelined main loop
  if (P->di-i >= 12) {
    F3_STAGE1;
    F3_STAGE2; F3_STAGE1;
    F3_STAGE3; F3_STAGE2; F3_STAGE1;
    while (i < P->di) {
      F3_STAGE4; F3_STAGE3; F3_STAGE2; F3_STAGE1;
    }
                 F3_STAGE4; F3_STAGE3; F3_STAGE2;
                            F3_STAGE4; F3_STAGE3;
//                                       F3_STAGE4;
  } else {
    // main loop
    while (i < P->di-4) {
      F3_STAGE1; F3_STAGE2; F3_STAGE3; F3_STAGE4;
    }
    F3_STAGE1; F3_STAGE2; F3_STAGE3;
  }
  
#undef F3_STAGE1
#undef F3_STAGE2
#undef F3_STAGE3
#undef F3_STAGE4
  
  tailmask = (vector unsigned int) vec_cmplt(tailelem, taillim);
  z = vec_and(z, (vector float) tailmask);
  tempMaxV = vec_max(tempMaxV, z);
  vec_st(z, 0, pst);
  
  // take the maximum across max and maxV and return it
  tempMaxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 8));
  maxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 4));
  float *maxVp = (float *) &maxV;
  max = *maxVp;
  
  return max;
}


/**********************
*
* foldArrayBy4 - Perform "folds" on 4 adjacent parts of the power over time
*   array. Each element in the "folded" array is the sum of the 4 elements
*   in the power over time array.
*
* Assumes 16-byte alignment of input and output arrays.
*
*/
float foldArrayBy4SP(float *ss[], struct PoTPlan *P) {
  float max;
  int vEnd;
  vector float tempMaxV = ZERO, maxV;
  vector float msq2 = vec_ld(0, ss[0]+P->tmp0);
  vector float msq3 = vec_ld(0, ss[0]+P->tmp1);
  vector float msq4 = vec_ld(0, ss[0]+P->tmp2);
  vector unsigned char mask2 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp0), vec_splat_u8(1));
  vector unsigned char mask3 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp1), vec_splat_u8(1));
  vector unsigned char mask4 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp2), vec_splat_u8(1));
  vector unsigned int tailelem = {0, 1, 2, 3};
  vector unsigned int taillim = vec_uisplat(((P->di-1)&0x3) + 1);
  vector unsigned int tailmask;
  int i = 0;
  
  const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2;
  float *pst = P->dest;
  vector float lsq2, lsq3, lsq4, x1, x2, x3, x4, xs12, xs34, xs, z;
  
#define F4_STAGE1 lsq2 = vec_ld(15, p2); lsq3 = vec_ld(15, p3); lsq4 = vec_ld(15, p4); i += 4; p2 += 4; p3 += 4; p4 += 4; x1 = vec_ld(0, p1); x2 = vec_perm(msq2, lsq2, mask2); x3 = vec_perm(msq3, lsq3, mask3); x4 = vec_perm(msq4, lsq4, mask4); p1 += 4; msq2 = lsq2; msq3 = lsq3; msq4 = lsq4; xs12 = vec_add(x1, x2); xs34 = vec_add(x3, x4);
#define F4_STAGE2 xs = vec_add(xs12, xs34);
               // was vec_madd(xs, RECIP_FOUR, ZERO);
#define F4_STAGE3 z = xs;
#define F4_STAGE4 tempMaxV = vec_max(tempMaxV, z); vec_st(z, 0, pst); pst += 4;
  
  // software pipelined main loop
  if (P->di-i >= 12) {
    F4_STAGE1;
    F4_STAGE2; F4_STAGE1;
    F4_STAGE3; F4_STAGE2; F4_STAGE1;
    while (i < P->di) {
      F4_STAGE4; F4_STAGE3; F4_STAGE2; F4_STAGE1;
    }
                 F4_STAGE4; F4_STAGE3; F4_STAGE2;
                            F4_STAGE4; F4_STAGE3;
//                                       F4_STAGE4;
  } else {  
    // main loop
    while (i < P->di-4) {
      F4_STAGE1; F4_STAGE2; F4_STAGE3; F4_STAGE4;
    }
    F4_STAGE1; F4_STAGE2; F4_STAGE3;
  }
    
#undef F4_STAGE1
#undef F4_STAGE2
#undef F4_STAGE3
#undef F4_STAGE4
  
  tailmask = (vector unsigned int) vec_cmplt(tailelem, taillim);
  z = vec_and(z, (vector float) tailmask);
  tempMaxV = vec_max(tempMaxV, z);
  vec_st(z, 0, pst);
  
  // take the maximum across max and maxV and return it
  tempMaxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 8));
  maxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 4));
  float *maxVp = (float *) &maxV;
  max = *maxVp;
  
  return max;
}


/**********************
*
* foldArrayBy5 - Perform "folds" on 5 adjacent parts of the power over time
*   array. Each element in the "folded" array is the sum of the 5 elements
*   in the power over time array.
*
* Assumes 16-byte alignment of input and output arrays.
*
*/
float foldArrayBy5SP(float *ss[], struct PoTPlan *P) {
  float max;
  int vEnd;
  vector float tempMaxV = ZERO, maxV;
  vector float msq2 = vec_ld(0, ss[0]+P->tmp0);
  vector float msq3 = vec_ld(0, ss[0]+P->tmp1);
  vector float msq4 = vec_ld(0, ss[0]+P->tmp2);
  vector float msq5 = vec_ld(0, ss[0]+P->tmp3);
  vector unsigned char mask2 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp0), vec_splat_u8(1));
  vector unsigned char mask3 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp1), vec_splat_u8(1));
  vector unsigned char mask4 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp2), vec_splat_u8(1));
  vector unsigned char mask5 = vec_add(vec_lvsl(-1L, ss[0]+P->tmp3), vec_splat_u8(1));
  vector unsigned int tailelem = {0, 1, 2, 3};
  vector unsigned int taillim = vec_uisplat(((P->di-1)&0x3) + 1);
  vector unsigned int tailmask;
  int i = 0;
  
  const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2, *p5 = ss[0]+P->tmp3;
  float *pst = P->dest;
  vector float lsq2, lsq3, lsq4, lsq5, x1, x2, x3, x4, x5, xs12, xs34, xs345, xs, z;
  
#define F5_STAGE1 lsq3 = vec_ld(15, p3); lsq4 = vec_ld(15, p4); i += 4; p3 += 4; p4 += 4; lsq2 = vec_ld(15, p2); x3 = vec_perm(msq3, lsq3, mask3); x4 = vec_perm(msq4, lsq4, mask4); lsq5 = vec_ld(15, p5); p2 += 4; msq3 = lsq3; msq4 = lsq4; p5 += 4; x1 = vec_ld(0, p1); x2 = vec_perm(msq2, lsq2, mask2); xs34 = vec_add(x3, x4); x5 = vec_perm(msq5, lsq5, mask5); p1 += 4; msq2 = lsq2; msq5 = lsq5;
#define F5_STAGE2 xs12 = vec_add(x1, x2); xs345 = vec_add(xs34, x5);
#define F5_STAGE3 xs = vec_add(xs12, xs345);
               // was vec_madd(xs, RECIP_FIVE, ZERO);
#define F5_STAGE4 z = xs;
#define F5_STAGE5 tempMaxV = vec_max(tempMaxV, z); vec_st(z, 0, pst); pst += 4;
  
  // software pipelined main loop
  if (P->di-i >= 16) {
    F5_STAGE1;
    F5_STAGE2; F5_STAGE1;
    F5_STAGE3; F5_STAGE2; F5_STAGE1;
    F5_STAGE4; F5_STAGE3; F5_STAGE2; F5_STAGE1;
    while (i < P->di) {
      F5_STAGE5; F5_STAGE4; F5_STAGE3; F5_STAGE2; F5_STAGE1;
    }
                 F5_STAGE5; F5_STAGE4; F5_STAGE3; F5_STAGE2;
                            F5_STAGE5; F5_STAGE4; F5_STAGE3;
                                       F5_STAGE5; F5_STAGE4;
//                                                  F5_STAGE5; 
  } else {
    // main loop
    while (i < P->di-4) {
      F5_STAGE1; F5_STAGE2; F5_STAGE3; F5_STAGE4; F5_STAGE5;
    }
    F5_STAGE1; F5_STAGE2; F5_STAGE3; F5_STAGE4;
  }
    
#undef F5_STAGE1
#undef F5_STAGE2
#undef F5_STAGE3
#undef F5_STAGE4
#undef F5_STAGE5
  
  tailmask = (vector unsigned int) vec_cmplt(tailelem, taillim);
  z = vec_and(z, (vector float) tailmask);
  tempMaxV = vec_max(tempMaxV, z);
  vec_st(z, 0, pst);
  
  // take the maximum across max and maxV and return it
  tempMaxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 8));
  maxV = vec_max(tempMaxV, vec_sld(tempMaxV, tempMaxV, 4));
  float *maxVp = (float *) &maxV;
  max = *maxVp;
  
  return max;
}

sum_func AKavTB3[FOLDTBLEN] = { foldArrayBy3SP };
sum_func AKavTB4[FOLDTBLEN] = { foldArrayBy4SP };
sum_func AKavTB5[FOLDTBLEN] = { foldArrayBy5SP };
sum_func AKavTB2[FOLDTBLEN] = { foldArrayBy2SPA };
sum_func AKavTB2AL[FOLDTBLEN] = { foldArrayBy2SPAL };

FoldSet AKavfold = {AKavTB3, AKavTB4, AKavTB5, AKavTB2, AKavTB2AL, "AK AltiVec"};


#endif
