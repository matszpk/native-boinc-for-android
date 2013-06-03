/******************
 *
 * analyzeFuncs_SSE2.cpp
 * 
 * Description: Intel & AMD SSE2 optimzized functions
 * CPUs: Intel Pentium IV, AMD Athlon Model 6+, AMD Athlon 64
 */

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

// The following is empty if USE_INTRINSICS is not defined
#ifdef USE_INTRINSICS
#include <climits>
#include <cmath>
#include <emmintrin.h>

#include "s_util.h"
#include "x86_float4.h"
#include "analyzeFuncs_vector.h"
#include "analyzeFuncs.h"


__inline __m128 vec_recip2(__m128 v) 
 {
  // obtain estimate
 __m128 estimate = _mm_rcp_ps( v );
 // one round of Newton-Raphson
 return _mm_add_ps(_mm_mul_ps(_mm_sub_ps(ONE, _mm_mul_ps(estimate, v)), estimate), estimate);
   }  


// =============================================================================
//
// sse2_vChirpData
// version by: Alex Kan - SSE2 mods (haddsum removal) BH
//   http://tbp.berkeley.edu/~alexkan/seti/
//
int sse2_ChirpData_ak(
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int chirp_rate_ind,
  double chirp_rate,
  int  ul_NumDataPoints,
  double sample_rate
) {
  int i;

  if (chirp_rate_ind == 0) {
    memcpy(cx_ChirpDataArray, cx_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
    return 0;
  }

  int vEnd;  
  double srate = chirp_rate * 0.5 / (sample_rate * sample_rate);
  __m128d rate = _mm_set1_pd(chirp_rate * 0.5 / (sample_rate * sample_rate));
  __m128d roundVal = _mm_set1_pd(srate >= 0.0 ? TWO_TO_52 : -TWO_TO_52);

  // main vectorised loop
  vEnd = ul_NumDataPoints - (ul_NumDataPoints & 3);
  for (i = 0; i < vEnd; i += 4) {
    const float *data = (const float *) (cx_DataArray + i);
    float *chirped = (float *) (cx_ChirpDataArray + i);
    __m128d di = _mm_set1_pd(i);
    __m128d a1 = _mm_add_pd(_mm_set_pd(1.0, 0.0), di);
    __m128d a2 = _mm_add_pd(_mm_set_pd(3.0, 2.0), di);
    __m128d x1, y1;

    __m128 d1, d2;
    __m128 cd1, cd2;
    __m128 td1, td2;
    __m128 x;
    __m128 y;
    __m128 s;
    __m128 c;
    __m128 m;

    // load the signal to be chirped
    prefetchnta((const void *)( data+32 ));
    d1 = _mm_load_ps(data);
    d2 = _mm_load_ps(data+4);

    // calculate the input angle
    a1 = _mm_mul_pd(a1, a1);
    a2 = _mm_mul_pd(a2, a2);
    a1 = _mm_mul_pd(a1, rate);
    a2 = _mm_mul_pd(a2, rate);

    // reduce the angle to the range (-0.5, 0.5)
    x1 = _mm_add_pd(a1, roundVal);
    y1 = _mm_add_pd(a2, roundVal);
    x1 = _mm_sub_pd(x1, roundVal);
    y1 = _mm_sub_pd(y1, roundVal);
    a1 = _mm_sub_pd(a1, x1);
    a2 = _mm_sub_pd(a2, y1);

    // convert pair of packed double into packed single
    x = _mm_movelh_ps(_mm_cvtpd_ps(a1), _mm_cvtpd_ps(a2));

    // square to the range [0, 0.25)
    y = _mm_mul_ps(x, x);

    // perform the initial polynomial approximations
    s = _mm_mul_ps(y, SS4);
    c = _mm_mul_ps(y, CC3);            
    s = _mm_add_ps(s, SS3);
    c = _mm_add_ps(c, CC2);
    s = _mm_mul_ps(s, y);
    c = _mm_mul_ps(c, y);
    s = _mm_add_ps(s, SS2);
    c = _mm_add_ps(c, CC1);
    s = _mm_mul_ps(s, y);
    c = _mm_mul_ps(c, y);
    s = _mm_add_ps(s, SS1);
    s = _mm_mul_ps(s, x);
    c = _mm_add_ps(c, ONE);

    // perform first angle doubling
    x = _mm_sub_ps(_mm_mul_ps(c, c), _mm_mul_ps(s, s));
    y = _mm_mul_ps(_mm_mul_ps(s, c), TWO);

    // calculate scaling factor to correct the magnitude
    //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
    //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
    m = vec_recip2(_mm_add_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y)));

    // perform second angle doubling
    c = _mm_sub_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
    s = _mm_mul_ps(_mm_mul_ps(y, x), TWO);

    // correct the magnitude (final sine / cosine approximations)
    c = _mm_mul_ps(c, m);
    s = _mm_mul_ps(s, m);

/*    c1 c2 c3 c4
    s1 s2 s3 s4

    R1 i1 R2 I2    R3 i3 R4 i4

    R1 * c1  +  (i1 * s1 * -1)
    i1 * c1  +   R1 * s1  
    R2 * c2  +  (i2 * s2 * -1)
    i2 * c2  +   R2 * s2
*/

    x = d1;
    y = d2;
    x = _mm_shuffle_ps(x, x, 0xB1);
    y = _mm_shuffle_ps(y, y, 0xB1);
    x = _mm_mul_ps(x, R_NEG);
    y = _mm_mul_ps(y, R_NEG);
    cd1 = _mm_shuffle_ps(c, c, 0x50);  // 01 01 00 00  AaBb => BBbb => c3c3c4c4
    cd2 = _mm_shuffle_ps(c, c, 0xfa);  // 11 11 10 10  AaBb => AAaa => c1c1c2c2
    td1 = _mm_shuffle_ps(s, s, 0x50);
    td2 = _mm_shuffle_ps(s, s, 0xfa);

    cd1 = _mm_mul_ps(cd1, d1);
    cd2 = _mm_mul_ps(cd2, d2);
    td1 = _mm_mul_ps(td1, x);
    td2 = _mm_mul_ps(td2, y);

    cd1 = _mm_add_ps(cd1, td1);
    cd2 = _mm_add_ps(cd2, td2);

    // store chirped values
    _mm_stream_ps(chirped+0, cd1);
    _mm_stream_ps(chirped+4, cd2);
  }
  _mm_sfence();

  if( i < ul_NumDataPoints) {
    // use original routine to finish up any tailings (max stride-1 elements)
    v_ChirpData(cx_DataArray+i, cx_ChirpDataArray+i
      , chirp_rate_ind, chirp_rate, ul_NumDataPoints-i, sample_rate);
  }
  analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

  return 0;
}

// =============================================================================
//
// JWS: alternate SSE2 chirp
//
// Using Mendenhall Faster SinCos, coding based on
// version 8 v_vChirpData for SSE3 by Alex Kan
//
int sse2_ChirpData_ak8(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
) {
    int i, vEnd;

    if (chirp_rate_ind == 0) {
      memcpy(cx_ChirpDataArray, cx_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
      return 0;
    }

    const double srate = chirp_rate * 0.5 / (sample_rate * sample_rate);

    __m128d dfourv = _mm_set1_pd(4.0);

    __m128d rate = _mm_set1_pd(srate);
    __m128d roundVal = _mm_set1_pd((srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52);

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 3);
    __m128d di1 = _mm_set_pd(1.0, 0.0);
    __m128d di2 = _mm_set_pd(3.0, 2.0);

    for (i = 0; i < vEnd; i += 4) {
      const float *data = (const float *) (cx_DataArray + i);
      float *chirped = (float *) (cx_ChirpDataArray + i);

      __m128d a1, a2;
      __m128 d1, d2;
      __m128 x;
      __m128 y;
      __m128 z;
      __m128 s;
      __m128 c;
      __m128 m;

      // load the signal to be chirped
      prefetchnta((const void *)( data+32 ));
      d1 = _mm_load_ps(data);
      d2 = _mm_load_ps(data+4);

      // calculate the input angles
      a1 = _mm_mul_pd(_mm_mul_pd(di1, di1), rate);
      a2 = _mm_mul_pd(_mm_mul_pd(di2, di2), rate);

      // update time values for next iteration
      di1 = _mm_add_pd(di1, dfourv);
      di2 = _mm_add_pd(di2, dfourv);

      // reduce the angles to the range (-0.5, 0.5)
      a1 = _mm_sub_pd(a1, _mm_sub_pd(_mm_add_pd(a1, roundVal), roundVal));
      a2 = _mm_sub_pd(a2, _mm_sub_pd(_mm_add_pd(a2, roundVal), roundVal));

      // convert the 2 packed doubles to packed single
      x = _mm_movelh_ps(_mm_cvtpd_ps(a1), _mm_cvtpd_ps(a2));

      // square to the range [0, 0.25)
      y = _mm_mul_ps(x, x);

      // perform the initial polynomial approximations
      z = _mm_mul_ps(y, y);
      s = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, SS4F),
                                                      SS3F),
                                           z),
                                _mm_add_ps(_mm_mul_ps(y, SS2F),
                                           SS1F)),
                     x);
      c = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, CC3F),
                                           CC2F),
                                z),
                     _mm_add_ps(_mm_mul_ps(y, CC1F),
                                ONE));

      // perform first angle doubling
      x = _mm_sub_ps(_mm_mul_ps(c, c), _mm_mul_ps(s, s));
      y = _mm_mul_ps(_mm_mul_ps(s, c), TWO);

      // calculate scaling factor to correct the magnitude
      m = _mm_sub_ps(_mm_sub_ps(TWO, _mm_mul_ps(x, x)), _mm_mul_ps(y, y));

      // perform second angle doubling
      c = _mm_sub_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
      s = _mm_mul_ps(_mm_mul_ps(y, x), TWO);

      // correct the magnitude (final sine / cosine approximations)
      s = _mm_mul_ps(s, m);
      c = _mm_mul_ps(c, m);

      // chirp the data
      x = _mm_mul_ps(_mm_shuffle_ps(d1, d1, 0xb1), R_NEG);
      y = _mm_mul_ps(_mm_shuffle_ps(d2, d2, 0xb1), R_NEG);

      d1 = _mm_mul_ps(d1, _mm_shuffle_ps(c, c, 0x50));
      d2 = _mm_mul_ps(d2, _mm_shuffle_ps(c, c, 0xfa));
      x = _mm_mul_ps(x, _mm_shuffle_ps(s, s, 0x50));
      y = _mm_mul_ps(y, _mm_shuffle_ps(s, s, 0xfa));

      // store chirped values
      _mm_stream_ps(chirped+0, _mm_add_ps(d1, x));
      _mm_stream_ps(chirped+4, _mm_add_ps(d2, y));
    }
    _mm_sfence();

    // handle tail elements with scalar code
    for (   ; i < ul_NumDataPoints; ++i) {
      double angle = srate * i * i * 0.5;
      angle -= floor(angle);
      angle *= M_PI * 2;
      double s = sin(angle);
      double c = cos(angle);
      float re = cx_DataArray[i][0];
      float im = cx_DataArray[i][1];

      cx_ChirpDataArray[i][0] = re * c - im * s;
      cx_ChirpDataArray[i][1] = re * s + im * c;
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

    return 0;
}


#endif // USE_INTRINSICS
