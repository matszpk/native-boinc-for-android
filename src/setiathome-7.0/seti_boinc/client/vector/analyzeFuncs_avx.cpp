/******************
 *
 * analyzeFuncs_avx.cpp
 *
 * Description: Intel AVX optimzized functions
 * CPUs: Intel Core iX xxxx+
 *
 */

// Copyright 2011 Regents of the University of California

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

//JWS: For a release build using this module, compile with -mavx on GCC 4.4 or
// later or whatever is equivalent for the compiler used. Define USE_AVX and
// USE_INTRINSICS also.
//
// For tests on x86 hardware without AVX capability, Intel provides an SDE
// (Software Development Emulator) to do runtime replacement of AVX instructions
// or an avxintrin_emu.h header file which does compile-time replacement of the
// AVX intrinsics. For that latter method, define AVX_EMU to get the header
// file included and build for SSE3 or better.

// The following is empty if USE_AVX and USE_INTRINSICS are not both defined
#if defined(USE_AVX) && defined (USE_INTRINSICS)

#include <climits>
#include <cmath>

#ifdef AVX_EMU
#include "avxintrin_emu.h"
#elif defined(_MSC_VER)
#include <intrin.h>
#else
#include <immintrin.h>
#endif

#include "s_util.h"
#include "analyzeFuncs_vector.h"
#include "analyzeFuncs.h"
#include "x86_ops.h"
#include "pulsefind.h"

// =============================================================================
// JWS: Four variant chirp functions, first expands constants in memory
//
// Using Mendenhall Faster SinCos, coding based on
// v_vChirpData for SSE3 by: Alex Kan
//
int avx_ChirpData_a(
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

    __m256 ss1fA = _mm256_set1_ps(1.5707963235f);
    __m256 ss2fA = _mm256_set1_ps(-0.645963615f);
    __m256 ss3fA = _mm256_set1_ps(0.0796819754f);
    __m256 ss4fA = _mm256_set1_ps(-0.0046075748f);
    __m256 cc1fA = _mm256_set1_ps(-1.2336977925f);
    __m256 cc2fA = _mm256_set1_ps(0.2536086171f);
    __m256 cc3fA = _mm256_set1_ps(-0.0204391631f);
    __m256 oneA  = _mm256_set1_ps(1.0f);
    __m256 twoA  = _mm256_set1_ps(2.0f);
    __m256d eightA = _mm256_set1_pd(8.0);

    __m256d rate = _mm256_broadcast_sd(&srate);
    __m256d roundVal = _mm256_set1_pd((srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52);

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    __m256d di1 = _mm256_set_pd(5.0, 1.0, 4.0, 0.0);   // set time patterns for eventual moveldup/movehdup
    __m256d di2 = _mm256_set_pd(7.0, 3.0, 6.0, 2.0);

    for (i = 0; i < vEnd; i += 8) {
      const float *data = (const float *) (cx_DataArray + i);
      float *chirped = (float *) (cx_ChirpDataArray + i);

      __m256d a1, a2;
      __m256 d1, d2;
      __m256 cd1, cd2;
      __m256 td1, td2;
      __m256 x;
      __m256 y;
      __m256 z;
      __m256 s;
      __m256 c;
      __m256 m;

      // load the signal to be chirped
      prefetchnta((const void *)( data+64 ));
      d1 = _mm256_load_ps(data);
      d2 = _mm256_load_ps(data+8);

      // calculate the input angles
      a1 = _mm256_mul_pd(_mm256_mul_pd(di1, di1), rate);
      a2 = _mm256_mul_pd(_mm256_mul_pd(di2, di2), rate);

      // update time values for next iteration
      di1 = _mm256_add_pd(di1, eightA);
      di2 = _mm256_add_pd(di2, eightA);

      // reduce the angles to the range (-0.5, 0.5)
      a1 = _mm256_sub_pd(a1, _mm256_sub_pd(_mm256_add_pd(a1, roundVal), roundVal));
      a2 = _mm256_sub_pd(a2, _mm256_sub_pd(_mm256_add_pd(a2, roundVal), roundVal));

      // convert the 2 packed doubles to packed single
      x = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm256_cvtpd_ps(a1)), _mm256_cvtpd_ps(a2), 1);

      // square to the range [0, 0.25)
      y = _mm256_mul_ps(x, x);

      // perform the initial polynomial approximations
      z = _mm256_mul_ps(y, y);
      s = _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(y, ss4fA),
                                                                  ss3fA),
                                                    z),
                                      _mm256_add_ps(_mm256_mul_ps(y, ss2fA),
                                                    ss1fA)),
                        x);
      c = _mm256_add_ps(_mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(y, cc3fA),
                                                    cc2fA),
                                      z),
                        _mm256_add_ps(_mm256_mul_ps(y, cc1fA),
                                      oneA));

      // perform first angle doubling
      x = _mm256_sub_ps(_mm256_mul_ps(c, c), _mm256_mul_ps(s, s));
      y = _mm256_mul_ps(_mm256_mul_ps(s, c), twoA);

      // calculate scaling factor to correct the magnitude
      m = _mm256_sub_ps(_mm256_sub_ps(twoA, _mm256_mul_ps(x, x)), _mm256_mul_ps(y, y));

      // perform second angle doubling
      c = _mm256_sub_ps(_mm256_mul_ps(x, x), _mm256_mul_ps(y, y));
      s = _mm256_mul_ps(_mm256_mul_ps(y, x), twoA);

      // correct the magnitude (final sine / cosine approximations)
      s = _mm256_mul_ps(s, m);
      c = _mm256_mul_ps(c, m);              // c7     c3     c6     c2     c5     c1     c4     c0

      // chirp the data
      cd1 = _mm256_moveldup_ps(c);          // c3     c3     c2     c2     c1     c1     c0     c0
      cd2 = _mm256_movehdup_ps(c);          // c7     c7     c6     c6     c5     c5     c4     c4
      cd1 = _mm256_mul_ps(cd1, d1);         // c3.i3  c3.r3  c2.i2  c2.r2  c1.i1  c1.r1  c0.i0  c0.r0
      cd2 = _mm256_mul_ps(cd2, d2);         // c7.i7  c7.r7  c6.i6  c6.r6  c5.i5  c5.r5  c4.i4  c4.r4
      d1 = _mm256_shuffle_ps(d1, d1, 0xb1);
      d2 = _mm256_shuffle_ps(d2, d2, 0xb1);
      td1 = _mm256_moveldup_ps(s);
      td2 = _mm256_movehdup_ps(s);
      td1 = _mm256_mul_ps(td1, d1);
      td2 = _mm256_mul_ps(td2, d2);
      cd1 = _mm256_addsub_ps(cd1, td1);
      cd2 = _mm256_addsub_ps(cd2, td2);

      // store chirped values
      _mm256_stream_ps(chirped, cd1);
      _mm256_stream_ps(chirped+8, cd2);
    }
    _mm256_zeroupper ();
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

// JWS: Second variant, using vbroadcast to expand constants when used
int avx_ChirpData_b(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
) {
    int i, vEnd;

    if (chirp_rate_ind == 0) {
      memcpy(cx_ChirpDataArray, cx_DataArray, (int)ul_NumDataPoints * sizeof(sah_complex)  );
      return 0;
    }

    const double srate = chirp_rate * 0.5 / (sample_rate * sample_rate);

    const float ss1f = 1.5707963235f;
    const float ss2f = -0.645963615f;
    const float ss3f = 0.0796819754f;
    const float ss4f = -0.0046075748f;
    const float cc1f = -1.2336977925f;
    const float cc2f = 0.2536086171f;
    const float cc3f = -0.0204391631f;
    const float one  = 1.0f;
    const float two  = 2.0f;
    const double eight  = 8.0;

    __m256d rate = _mm256_broadcast_sd(&srate);
    __m256d roundVal = _mm256_set1_pd((srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52);

    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);

    // main vectorised loop
    __m256d di1 = _mm256_set_pd(5.0, 1.0, 4.0, 0.0);   // set time patterns for eventual moveldup/movehdup
    __m256d di2 = _mm256_set_pd(7.0, 3.0, 6.0, 2.0);

    for (i = 0; i < vEnd; i += 8) {
      const float *data = (const float *) (cx_DataArray + i);
      float *chirped = (float *) (cx_ChirpDataArray + i);

      __m256d a1, a2;
      __m256 d1, d2;
      __m256 cd1, cd2;
      __m256 td1, td2;
      __m256 x;
      __m256 y;
      __m256 z;
      __m256 s;
      __m256 c;
      __m256 m;

      // load the signal to be chirped
      prefetchnta((const void *)( data+64 ));
      d1 = _mm256_load_ps(data);
      d2 = _mm256_load_ps(data+8);

      // calculate the input angles
      a1 = _mm256_mul_pd(_mm256_mul_pd(di1, di1), rate);
      a2 = _mm256_mul_pd(_mm256_mul_pd(di2, di2), rate);

      // update time values for next iteration
      di1 = _mm256_add_pd(di1, _mm256_broadcast_sd(&eight));
      di2 = _mm256_add_pd(di2, _mm256_broadcast_sd(&eight));

      // reduce the angles to the range (-0.5, 0.5)
      a1 = _mm256_sub_pd(a1, _mm256_sub_pd(_mm256_add_pd(a1, roundVal), roundVal));
      a2 = _mm256_sub_pd(a2, _mm256_sub_pd(_mm256_add_pd(a2, roundVal), roundVal));

      // convert the 2 packed doubles to packed single
      x = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm256_cvtpd_ps(a1)), _mm256_cvtpd_ps(a2), 1);

      // square to the range [0, 0.25)
      y = _mm256_mul_ps(x, x);

      // perform the initial polynomial approximations
      z = _mm256_mul_ps(y, y);
      s = _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(y, _mm256_broadcast_ss(&ss4f)),
                                                                  _mm256_broadcast_ss(&ss3f)),
                                                    z),
                                      _mm256_add_ps(_mm256_mul_ps(y, _mm256_broadcast_ss(&ss2f)),
                                                    _mm256_broadcast_ss(&ss1f))),
                        x);
      c = _mm256_add_ps(_mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(y, _mm256_broadcast_ss(&cc3f)),
                                                    _mm256_broadcast_ss(&cc2f)),
                                      z),
                        _mm256_add_ps(_mm256_mul_ps(y, _mm256_broadcast_ss(&cc1f)),
                                      _mm256_broadcast_ss(&one)));

      // perform first angle doubling
      x = _mm256_sub_ps(_mm256_mul_ps(c, c), _mm256_mul_ps(s, s));
      y = _mm256_mul_ps(_mm256_mul_ps(s, c), _mm256_broadcast_ss(&two));

      // calculate scaling factor to correct the magnitude
      m = _mm256_sub_ps(_mm256_sub_ps(_mm256_broadcast_ss(&two), _mm256_mul_ps(x, x)), _mm256_mul_ps(y, y));

      // perform second angle doubling
      c = _mm256_sub_ps(_mm256_mul_ps(x, x), _mm256_mul_ps(y, y));
      s = _mm256_mul_ps(_mm256_mul_ps(y, x), _mm256_broadcast_ss(&two));

      // correct the magnitude (final sine / cosine approximations)
      s = _mm256_mul_ps(s, m);
      c = _mm256_mul_ps(c, m);

      // chirp the data
      cd1 = _mm256_moveldup_ps(c);
      cd2 = _mm256_movehdup_ps(c);
      cd1 = _mm256_mul_ps(cd1, d1);
      cd2 = _mm256_mul_ps(cd2, d2);
      d1 = _mm256_shuffle_ps(d1, d1, 0xb1);
      d2 = _mm256_shuffle_ps(d2, d2, 0xb1);
      td1 = _mm256_moveldup_ps(s);
      td2 = _mm256_movehdup_ps(s);
      td1 = _mm256_mul_ps(td1, d1);
      td2 = _mm256_mul_ps(td2, d2);
      cd1 = _mm256_addsub_ps(cd1, td1);
      cd2 = _mm256_addsub_ps(cd2, td2);

      // store chirped values
      _mm256_stream_ps(chirped, cd1);
      _mm256_stream_ps(chirped+8, cd2);
    }
    _mm256_zeroupper();
    _mm_sfence();

    // never happens, handle tail elements with scalar code
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

// Third variant, like the first but instruction sequences modified so most instructions
// are not dependent on results of immediately preceding instruction.
// This may not have much effect on CPUs with excellent out of order capability.
int avx_ChirpData_c(
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

    __m256 ss1fA = _mm256_set1_ps(1.5707963235f);
    __m256 ss2fA = _mm256_set1_ps(-0.645963615f);
    __m256 ss3fA = _mm256_set1_ps(0.0796819754f);
    __m256 ss4fA = _mm256_set1_ps(-0.0046075748f);
    __m256 cc1fA = _mm256_set1_ps(-1.2336977925f);
    __m256 cc2fA = _mm256_set1_ps(0.2536086171f);
    __m256 cc3fA = _mm256_set1_ps(-0.0204391631f);
    __m256 oneA  = _mm256_set1_ps(1.0f);
    __m256 twoA  = _mm256_set1_ps(2.0f);
    __m256d eightA = _mm256_set1_pd(8.0);

    __m256d rate = _mm256_broadcast_sd(&srate);
    __m256d roundVal = _mm256_set1_pd((srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52);

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    __m256d di1 = _mm256_set_pd(5.0, 1.0, 4.0, 0.0);   // set time patterns for eventual moveldup/movehdup
    __m256d di2 = _mm256_set_pd(7.0, 3.0, 6.0, 2.0);

    for (i = 0; i < vEnd; i += 8) {
      const float *data = (const float *) (cx_DataArray + i);
      float *chirped = (float *) (cx_ChirpDataArray + i);

      __m256d a1, a2;
      __m256 d1, d2;
      __m256 cd1, cd2;
      __m256 td1, td2;
      __m256 v;
      __m256 w;
      __m256 x;
      __m256 y;
      __m256 z;
      __m256 s;
      __m256 c;
      __m256 m;

      // load the signal to be chirped
      prefetchnta((const void *)( data+64 ));
      d1 = _mm256_load_ps(data);
      d2 = _mm256_load_ps(data+8);

      // calculate the input angles
      a1 = _mm256_mul_pd(di1, di1);
      a2 = _mm256_mul_pd(di2, di2);
      a1 = _mm256_mul_pd(a1, rate);
      a2 = _mm256_mul_pd(a2, rate);

      // reduce the angles to the range (-0.5, 0.5)
      a1 = _mm256_sub_pd(a1, _mm256_sub_pd(_mm256_add_pd(a1, roundVal), roundVal));
      a2 = _mm256_sub_pd(a2, _mm256_sub_pd(_mm256_add_pd(a2, roundVal), roundVal));

      // update time values for next iteration
      di1 = _mm256_add_pd(di1, eightA);
      di2 = _mm256_add_pd(di2, eightA);

      // convert the 2 packed doubles to packed single
      y = _mm256_castps128_ps256(_mm256_cvtpd_ps(a1));
      x = _mm256_insertf128_ps(y, _mm256_cvtpd_ps(a2), 1);

      // square to the range [0, 0.25)
      y = _mm256_mul_ps(x, x);

      // perform the initial polynomial approximations with interleaved Estrin sequences
      z = _mm256_mul_ps(y, y);
      s = _mm256_mul_ps(y, ss4fA);
      c = _mm256_mul_ps(y, cc3fA);
      s = _mm256_add_ps(s, ss3fA);
      c = _mm256_add_ps(c, cc2fA);
      s = _mm256_mul_ps(s, z);
      c = _mm256_mul_ps(c, z);
      v = _mm256_mul_ps(y, ss2fA);
      w = _mm256_mul_ps(y, cc1fA);
      v = _mm256_add_ps(v, ss1fA);
      w = _mm256_add_ps(w, oneA);
      s = _mm256_add_ps(s, v);
      c = _mm256_add_ps(c, w);
      s = _mm256_mul_ps(s, x);

      // perform first angle doubling
      v = _mm256_mul_ps(c, c);
      w = _mm256_mul_ps(s, s);
      y = _mm256_mul_ps(s, c);
      x = _mm256_sub_ps(v, w);
      y = _mm256_mul_ps(y, twoA);

      // calculate scaling factor to correct the magnitude
      // and interleave the second angle doubling
      v = _mm256_mul_ps(x, x);
      w = _mm256_mul_ps(y, y);
      m = _mm256_sub_ps(twoA, v);
      c = _mm256_sub_ps(v, w);
      s = _mm256_mul_ps(y, x);
      m = _mm256_sub_ps(m, w);
      s = _mm256_mul_ps(s, twoA);

      // correct the magnitude (final sine / cosine approximations)
      c = _mm256_mul_ps(c, m);
      s = _mm256_mul_ps(s, m);

      // chirp the data
      cd1 = _mm256_moveldup_ps(c);
      td1 = _mm256_moveldup_ps(s);
      cd2 = _mm256_movehdup_ps(c);
      td2 = _mm256_movehdup_ps(s);
      cd1 = _mm256_mul_ps(cd1, d1);
      cd2 = _mm256_mul_ps(cd2, d2);
      d1 = _mm256_shuffle_ps(d1, d1, 0xb1);
      d2 = _mm256_shuffle_ps(d2, d2, 0xb1);
      td1 = _mm256_mul_ps(td1, d1);
      td2 = _mm256_mul_ps(td2, d2);
      cd1 = _mm256_addsub_ps(cd1, td1);
      cd2 = _mm256_addsub_ps(cd2, td2);

      // store chirped values
      _mm256_stream_ps(chirped, cd1);
      _mm256_stream_ps(chirped+8, cd2);
    }
    _mm256_zeroupper ();
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

// Fourth variant, like third but using _mm256_round_pd for angle reduction.
int avx_ChirpData_d(
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

    __m256 ss1fA = _mm256_set1_ps(1.5707963235f);
    __m256 ss2fA = _mm256_set1_ps(-0.645963615f);
    __m256 ss3fA = _mm256_set1_ps(0.0796819754f);
    __m256 ss4fA = _mm256_set1_ps(-0.0046075748f);
    __m256 cc1fA = _mm256_set1_ps(-1.2336977925f);
    __m256 cc2fA = _mm256_set1_ps(0.2536086171f);
    __m256 cc3fA = _mm256_set1_ps(-0.0204391631f);
    __m256 oneA  = _mm256_set1_ps(1.0f);
    __m256 twoA  = _mm256_set1_ps(2.0f);
    __m256d eightA = _mm256_set1_pd(8.0);

    __m256d rate = _mm256_broadcast_sd(&srate);

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    __m256d di1 = _mm256_set_pd(5.0, 1.0, 4.0, 0.0);
    __m256d di2 = _mm256_set_pd(7.0, 3.0, 6.0, 2.0);

    for (i = 0; i < vEnd; i += 8) {
      const float *data = (const float *) (cx_DataArray + i);
      float *chirped = (float *) (cx_ChirpDataArray + i);

      __m256d a1, a2;
      __m256 d1, d2;
      __m256 cd1, cd2;
      __m256 td1, td2;
      __m256 v;
      __m256 w;
      __m256 x;
      __m256 y;
      __m256 z;
      __m256 s;
      __m256 c;
      __m256 m;

      // load the signal to be chirped
      prefetchnta((const void *)( data+64 ));
      d1 = _mm256_load_ps(data);
      d2 = _mm256_load_ps(data+8);

      // calculate the input angles
      a1 = _mm256_mul_pd(di1, di1);
      a2 = _mm256_mul_pd(di2, di2);
      a1 = _mm256_mul_pd(a1, rate);
      a2 = _mm256_mul_pd(a2, rate);

      // reduce the angles to the range (-0.5, 0.5)
      a1 = _mm256_sub_pd(a1, _mm256_round_pd(a1, 0));  // round to nearest
      a2 = _mm256_sub_pd(a2, _mm256_round_pd(a2, 0));

      // update time values for next iteration
      di1 = _mm256_add_pd(di1, eightA);
      di2 = _mm256_add_pd(di2, eightA);

      // convert the 2 packed doubles to packed single
      y = _mm256_castps128_ps256(_mm256_cvtpd_ps(a1));
      x = _mm256_insertf128_ps(y, _mm256_cvtpd_ps(a2), 1);

      // square to the range [0, 0.25)
      y = _mm256_mul_ps(x, x);

      // perform the initial polynomial approximations with interleaved Estrin sequences
      z = _mm256_mul_ps(y, y);
      s = _mm256_mul_ps(y, ss4fA);
      c = _mm256_mul_ps(y, cc3fA);
      s = _mm256_add_ps(s, ss3fA);
      c = _mm256_add_ps(c, cc2fA);
      s = _mm256_mul_ps(s, z);
      c = _mm256_mul_ps(c, z);
      v = _mm256_mul_ps(y, ss2fA);
      w = _mm256_mul_ps(y, cc1fA);
      v = _mm256_add_ps(v, ss1fA);
      w = _mm256_add_ps(w, oneA);
      s = _mm256_add_ps(s, v);
      c = _mm256_add_ps(c, w);
      s = _mm256_mul_ps(s, x);

      // perform first angle doubling
      v = _mm256_mul_ps(c, c);
      w = _mm256_mul_ps(s, s);
      y = _mm256_mul_ps(s, c);
      x = _mm256_sub_ps(v, w);
      y = _mm256_mul_ps(y, twoA);

      // calculate scaling factor to correct the magnitude
      // and interleave the second angle doubling
      v = _mm256_mul_ps(x, x);
      w = _mm256_mul_ps(y, y);
      m = _mm256_sub_ps(twoA, v);
      c = _mm256_sub_ps(v, w);
      s = _mm256_mul_ps(y, x);
      m = _mm256_sub_ps(m, w);
      s = _mm256_mul_ps(s, twoA);

      // correct the magnitude (final sine / cosine approximations)
      c = _mm256_mul_ps(c, m);
      s = _mm256_mul_ps(s, m);

      // chirp the data
      cd1 = _mm256_moveldup_ps(c);
      td1 = _mm256_moveldup_ps(s);
      cd2 = _mm256_movehdup_ps(c);
      td2 = _mm256_movehdup_ps(s);
      cd1 = _mm256_mul_ps(cd1, d1);
      cd2 = _mm256_mul_ps(cd2, d2);
      d1 = _mm256_shuffle_ps(d1, d1, 0xb1);
      d2 = _mm256_shuffle_ps(d2, d2, 0xb1);
      td1 = _mm256_mul_ps(td1, d1);
      td2 = _mm256_mul_ps(td2, d2);
      cd1 = _mm256_addsub_ps(cd1, td1);
      cd2 = _mm256_addsub_ps(cd2, td2);

      // store chirped values
      _mm256_stream_ps(chirped, cd1);
      _mm256_stream_ps(chirped+8, cd2);
    }
    _mm256_zeroupper ();
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


// =============================================================================
// JWS: AVX transpose functions
//

inline void v_avxSubTranspose4x8ntw(float *in, float *out, int xline, int yline) {
    // Do two simultaneous 4x4 transposes in the YMM registers, non-temporal writes.
    // Input is 8 rows of 4 floats, output is 4 columns of 8 floats.
    // An sfence is needed after using this sub to ensure global visibilty of the writes.

    __m256 r04 = _mm256_insertf128_ps(_mm256_castps128_ps256(*((__m128*)(in+0*xline))), (*((__m128*)(in+4*xline))), 0x1); // a0b0c0d0a4b4c4d4
    __m256 r26 = _mm256_insertf128_ps(_mm256_castps128_ps256(*((__m128*)(in+2*xline))), (*((__m128*)(in+6*xline))), 0x1); // a2b2c2d2a6b6c6d6
    __m256 r15 = _mm256_insertf128_ps(_mm256_castps128_ps256(*((__m128*)(in+1*xline))), (*((__m128*)(in+5*xline))), 0x1); // a1b1c1d1a5b5c5d5
    __m256 r37 = _mm256_insertf128_ps(_mm256_castps128_ps256(*((__m128*)(in+3*xline))), (*((__m128*)(in+7*xline))), 0x1); // a3b3c3d3a7b7c7d7

    __m256 c01e = _mm256_unpacklo_ps(r04, r26); // a0a2b0b2a4a6b4b6
    __m256 c23e = _mm256_unpackhi_ps(r04, r26); // c0c2d0d2c4c6d4d6
    __m256 c01o = _mm256_unpacklo_ps(r15, r37); // a1a3b1b3a5a7b5b7
    __m256 c23o = _mm256_unpackhi_ps(r15, r37); // c1c3d1d3c5c7d5d7

    _mm256_stream_ps(out+0*yline, _mm256_unpacklo_ps(c01e, c01o)); // a0a1a2a3a4a5a6a7
    _mm256_stream_ps(out+1*yline, _mm256_unpackhi_ps(c01e, c01o)); // b0b1b2b3b4b5b6b7
    _mm256_stream_ps(out+2*yline, _mm256_unpacklo_ps(c23e, c23o)); // c0c1c2c3c4c5c6c7
    _mm256_stream_ps(out+3*yline, _mm256_unpackhi_ps(c23e, c23o)); // d0d1d2d3d4d5d6d7
}

int v_avxTranspose4x8ntw(int x, int y, float *in, float *out) {
    int i,j;

    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-3;i+=4) {
            v_avxSubTranspose4x8ntw(in+j*x+i,out+y*i+j,x,y);
        }
        // tail stuff which should never be used
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    // more tail stuff which should never be used
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    _mm256_zeroupper();
    _mm_sfence();
    return 0;
}
int v_avxTranspose4x16ntw(int x, int y, float *in, float *out) {
    int i,j;

    for (j=0;j<y-15;j+=16) {
        for (i=0;i<x-3;i+=4) {
            v_avxSubTranspose4x8ntw(in+j*x+i,out+y*i+j,x,y);
            v_avxSubTranspose4x8ntw(in+(j+8)*x+i,out+y*i+j+8,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    _mm256_zeroupper();
    _mm_sfence();
    return 0;
}


/*****************************************************************/
inline void v_avxSubTranspose8x4ntw(float *in, float *out, int xline, int yline) {
    // Do two simultaneous 4x4 transposes in the YMM registers, non-temporal writes.
    // Input is 4 rows of 8 floats, output is 8 columns of 4 floats.
    // An sfence is needed after using this sub to ensure global visibilty of the writes.
    __m256 t1, t2, t3, t4, t5, t6, t7, t8;
    __m128 u1, u2, u3, u4, u5, u6, u7, u8;

    t1 = _mm256_load_ps(in+0*xline);     // a7 a6 a5 a4 a3 a2 a1 a0
    t2 = _mm256_load_ps(in+1*xline);     // b7 b6 b5 b4 b3 b2 b1 b0
    t3 = _mm256_load_ps(in+2*xline);     // c7 c6 c5 c4 c3 c2 c1 c0
    t4 = _mm256_load_ps(in+3*xline);     // d7 d6 d5 d4 d3 d2 d1 d0
    t5 = _mm256_unpacklo_ps(t1,t2);      // b5 a5 b4 a4 b1 a1 b0 a0
    t6 = _mm256_unpacklo_ps(t3,t4);      // d5 c5 d4 c4 d1 c1 d0 c0
    t7 = _mm256_unpackhi_ps(t1,t2);      // b7 a7 b6 a6 b3 a3 b2 a2
    t8 = _mm256_unpackhi_ps(t3,t4);      // d7 c7 d6 c6 d3 c3 d2 c2
    t1 = _mm256_shuffle_ps(t5,t6,0x44);  // d4 c4 b4 a4 d0 c0 b0 a0
    t2 = _mm256_shuffle_ps(t5,t6,0xee);  // d5 c5 b5 a5 d1 c1 b1 a1
    t3 = _mm256_shuffle_ps(t7,t8,0x44);  // d6 c6 b6 a6 d2 c2 b2 a2
    t4 = _mm256_shuffle_ps(t7,t8,0xee);  // d7 c7 b7 a7 d3 c3 b3 a3
    // Extract the high 128 bit fields
    u5 = _mm256_extractf128_ps(t1,1);
    u6 = _mm256_extractf128_ps(t2,1);
    u7 = _mm256_extractf128_ps(t3,1);
    u8 = _mm256_extractf128_ps(t4,1);
    // then convert YMM to Xmm for the low fields
    u1 = _mm256_castps256_ps128(t1);
    u2 = _mm256_castps256_ps128(t2);
    u3 = _mm256_castps256_ps128(t3);
    u4 = _mm256_castps256_ps128(t4);

    _mm_stream_ps(out+0*yline, u1);      // d0 c0 b0 a0
    _mm_stream_ps(out+1*yline, u2);      // d1 c1 b1 a1
    _mm_stream_ps(out+2*yline, u3);      // d2 c2 b2 a2
    _mm_stream_ps(out+3*yline, u4);      // d3 c3 b3 a3
    _mm_stream_ps(out+4*yline, u5);      // d4 c4 b4 a4
    _mm_stream_ps(out+5*yline, u6);      // d5 c5 b5 a5
    _mm_stream_ps(out+6*yline, u7);      // d6 c6 b6 a6
    _mm_stream_ps(out+7*yline, u8);      // d7 c7 b7 a7
}

int v_avxTranspose8x4ntw(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-7;i+=8) {
            v_avxSubTranspose8x4ntw(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    _mm256_zeroupper();
    _mm_sfence();
    return 0;
}


/*****************************************************************/
inline void v_avxSubTranspose8x8ntw_a(float *in, float *out, int xline, int yline) {
    // Do a direct 8x8 transpose in YMM registers.
    // Might be slowed by register spills for 32 bit builds.
    // An sfence is needed after using this sub to ensure global visibilty of the writes.
    __m256 t0, t1, t2, t3, t4, t5, t6, t7, ta, tb, tc, td, te, tf;

    ta = _mm256_load_ps(in+0*xline);
    tb = _mm256_load_ps(in+4*xline);
    t0 = _mm256_permute2f128_ps(ta, tb, 0x20);
    t4 = _mm256_permute2f128_ps(ta, tb, 0x31);

    ta = _mm256_load_ps(in+1*xline);
    tb = _mm256_load_ps(in+5*xline);
    t1 = _mm256_permute2f128_ps(ta, tb, 0x20);
    t5 = _mm256_permute2f128_ps(ta, tb, 0x31);

    t2 = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    tc = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    t3 = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t4), _mm256_castps_pd(t5)));
    td = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t4), _mm256_castps_pd(t5)));

    ta = _mm256_load_ps(in+2*xline);
    tb = _mm256_load_ps(in+6*xline);
    t0 = _mm256_permute2f128_ps(ta, tb, 0x20);
    t4 = _mm256_permute2f128_ps(ta, tb, 0x31);

    ta = _mm256_load_ps(in+3*xline);
    tb = _mm256_load_ps(in+7*xline);
    t1 = _mm256_permute2f128_ps(ta, tb, 0x20);
    t5 = _mm256_permute2f128_ps(ta, tb, 0x31);

    t6 = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    te = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    t7 = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t4), _mm256_castps_pd(t5)));
    tf = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t4), _mm256_castps_pd(t5)));

    _mm256_stream_ps(out+0*yline, _mm256_shuffle_ps(t2, t6, 0x88));
    _mm256_stream_ps(out+1*yline, _mm256_shuffle_ps(t2, t6, 0xDD));
    _mm256_stream_ps(out+2*yline, _mm256_shuffle_ps(tc, te, 0x88));
    _mm256_stream_ps(out+3*yline, _mm256_shuffle_ps(tc, te, 0xDD));
    _mm256_stream_ps(out+4*yline, _mm256_shuffle_ps(t3, t7, 0x88));
    _mm256_stream_ps(out+5*yline, _mm256_shuffle_ps(t3, t7, 0xDD));
    _mm256_stream_ps(out+6*yline, _mm256_shuffle_ps(td, tf, 0x88));
    _mm256_stream_ps(out+7*yline, _mm256_shuffle_ps(td, tf, 0xDD));
}

int v_avxTranspose8x8ntw_a(int x, int y, float *in, float *out) {
    int i,j;
    // Processors with AVX have efficient "hardware" prefetching,
    // so just do one set of prefetches to help that get started.
    prefetcht0(in+0*x);
    prefetcht0(in+1*x);
    prefetcht0(in+2*x);
    prefetcht0(in+3*x);
    prefetcht0(in+4*x);
    prefetcht0(in+5*x);
    prefetcht0(in+6*x);
    prefetcht0(in+7*x);

    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-7;i+=8) {
            v_avxSubTranspose8x8ntw_a(in+j*x+i,out+y*i+j,x,y);
        }
        // tail stuff which should never be used
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    // more tail stuff which should never be used
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    _mm256_zeroupper();
    _mm_sfence();
    return 0;
}

/*****************************************************************/
inline void v_avxSubTranspose8x8ntw_b(float *in, float *out, int xline, int yline) {
    // Alternative 8x8 transpose with port 5 pressure reduction.
    // An sfence is needed after using this sub to ensure global visibilty of the writes.
    __m256 t0, t1, ta, tb, tc, td;
    float *in2 = in+4;

    t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in+0*xline)), _mm_load_ps(in+4*xline), 1);
    t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in+1*xline)), _mm_load_ps(in+5*xline), 1);
    ta = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    tb = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

    t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in+2*xline)), _mm_load_ps(in+6*xline), 1);
    t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in+3*xline)), _mm_load_ps(in+7*xline), 1);
    tc = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    td = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

    _mm256_stream_ps(out+0*yline, _mm256_shuffle_ps(ta, tc, 0x88));
    _mm256_stream_ps(out+1*yline, _mm256_shuffle_ps(ta, tc, 0xDD));
    _mm256_stream_ps(out+2*yline, _mm256_shuffle_ps(tb, td, 0x88));
    _mm256_stream_ps(out+3*yline, _mm256_shuffle_ps(tb, td, 0xDD));

    t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in2+0*xline)), _mm_load_ps(in2+4*xline), 1);
    t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in2+1*xline)), _mm_load_ps(in2+5*xline), 1);
    ta = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    tb = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

    t0 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in2+2*xline)), _mm_load_ps(in2+6*xline), 1);
    t1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_load_ps(in2+3*xline)), _mm_load_ps(in2+7*xline), 1);
    tc = _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));
    td = _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(t0), _mm256_castps_pd(t1)));

    _mm256_stream_ps(out+4*yline, _mm256_shuffle_ps(ta, tc, 0x88));
    _mm256_stream_ps(out+5*yline, _mm256_shuffle_ps(ta, tc, 0xDD));
    _mm256_stream_ps(out+6*yline, _mm256_shuffle_ps(tb, td, 0x88));
    _mm256_stream_ps(out+7*yline, _mm256_shuffle_ps(tb, td, 0xDD));
}

int v_avxTranspose8x8ntw_b(int x, int y, float *in, float *out) {
    int i,j;
    // Processors with AVX have efficient "hardware" prefetching,
    // so just do one set of prefetches to help that get started.
    prefetcht0(in+0*x);
    prefetcht0(in+1*x);
    prefetcht0(in+2*x);
    prefetcht0(in+3*x);
    prefetcht0(in+4*x);
    prefetcht0(in+5*x);
    prefetcht0(in+6*x);
    prefetcht0(in+7*x);

    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-7;i+=8) {
            v_avxSubTranspose8x8ntw_b(in+j*x+i,out+y*i+j,x,y);
        }
        // tail stuff which should never be used
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    // more tail stuff which should never be used
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    _mm256_zeroupper();
    _mm_sfence();
    return 0;
}


// =============================================================================
// JWS: AVX GetPowerSpectrum
//
int v_avxGetPowerSpectrum(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, vEnd;

    analysis_state.FLOP_counter+=3.0*NumDataPoints;

    if (NumDataPoints == 8) {
        i = 8;
        __m256 fd1 = _mm256_load_ps( (float*) &(FreqData[0]) );    //  r0  i0  r1  i1  r2  i2  r3  i3
        __m256 fd2 = _mm256_load_ps( (float*) &(FreqData[4]) );    //  r4  i4  r5  i5  r6  i6  r7  i7
        fd1 = _mm256_mul_ps(fd1, fd1);
        fd2 = _mm256_mul_ps(fd2, fd2);
        __m256 fd3 = _mm256_permute2f128_ps(fd1, fd2, 0x20);         // sr0 si0 sr1 si1 sr4 si4 sr5 si5
        __m256 fd4 = _mm256_permute2f128_ps(fd1, fd2, 0x31);         // sr2 si2 sr3 si3 sr6 si6 sr7 si7
        _mm256_store_ps((float*)(&(PowerSpectrum[0])), _mm256_hadd_ps(fd3, fd4)); // 0 1 2 3 4 5 6 7

    } else {
        vEnd = NumDataPoints - (NumDataPoints & 15);
        for (i = 0; i < vEnd; i += 16) {

            prefetcht0(FreqData+i+64);
            prefetcht0(PowerSpectrum+i+64);

            __m256 fd1 = _mm256_load_ps( (float*) &(FreqData[i]) );
            __m256 fd2 = _mm256_load_ps( (float*) &(FreqData[i+4]) );
            __m256 fd3 = _mm256_load_ps( (float*) &(FreqData[i+8]) );
            __m256 fd4 = _mm256_load_ps( (float*) &(FreqData[i+12]) );
            fd1 = _mm256_mul_ps(fd1, fd1);
            fd2 = _mm256_mul_ps(fd2, fd2);
            __m256 fd5 = _mm256_permute2f128_ps(fd1, fd2, 0x20);
            __m256 fd6 = _mm256_permute2f128_ps(fd1, fd2, 0x31);
            fd1 = _mm256_hadd_ps(fd5, fd6);
            fd3 = _mm256_mul_ps(fd3, fd3);
            fd4 = _mm256_mul_ps(fd4, fd4);
            fd5 = _mm256_permute2f128_ps(fd3, fd4, 0x20);
            fd6 = _mm256_permute2f128_ps(fd3, fd4, 0x31);

            _mm256_store_ps( (float*)(&(PowerSpectrum[i])), fd1);
            _mm256_store_ps( (float*)(&(PowerSpectrum[i+8])), _mm256_hadd_ps(fd5, fd6));
        }
    }

    // handle tail elements, although in practice this never happens
    for (; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    _mm256_zeroupper();
    return 0;
}

/*********************************************************************
 *
 * AVX Folding subroutine sets
 *
 * Coded by Joe Segur based on prior art from Alex Kan and Ben Herndon
 * plus Sandy Bridge implementation details from Intel.
 * 
 * AVX does 8 floats at once. Unaligned memory arguments are allowed
 * for most instructions, so near-duplicate routines aren't needed.
 */

// First version ignores misalignments to achieve smallest code size
float foldBy3(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    // No unroll, Sandy Bridge has a limited uop cache which should be conserved.
    while (i < P->di-8) {
        x1 = _mm256_load_ps(p1+i);
        x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
        x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));
        _mm256_store_ps(pst+i, x1);
        maxV = _mm256_max_ps(maxV, x1);
        i += 8;
    }
    x1 = _mm256_load_ps(p1+i);
    x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
    x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);  // taillim floats >= tailelem floats
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x1);
    x2 = _mm256_and_ps(x1, tailmask);
    maxV = _mm256_max_ps(maxV, x2);
    x1 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x1);                                  // max in low 4
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee)); //     in low 2
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01)); //            1
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBa3[FOLDTBLEN] = {foldBy3};


float foldBy4(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_load_ps(p1+i);
        x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
        x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));
        x2 = _mm256_add_ps(x1, *(__m256*)(p4+i));
        _mm256_store_ps(pst+i, x2);
        maxV = _mm256_max_ps(maxV, x2);
        i += 8;
    }
    x1 = _mm256_load_ps(p1+i);
    x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
    x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));
    x2 = _mm256_add_ps(x1, *(__m256*)(p4+i));

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x2);
    x1 = _mm256_and_ps(x2, tailmask);
    maxV = _mm256_max_ps(maxV, x1);
    x2 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x2);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBa4[FOLDTBLEN] = {foldBy4};


float foldBy5(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2, *p5 = ss[0]+P->tmp3;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_load_ps(p1+i);
        x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
        x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));
        x2 = _mm256_add_ps(x1, *(__m256*)(p4+i));
        x1 = _mm256_add_ps(x2, *(__m256*)(p5+i));
        _mm256_store_ps(pst+i, x1);
        maxV = _mm256_max_ps(maxV, x1);
        i += 8;
    }
    x1 = _mm256_load_ps(p1+i);
    x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
    x1 = _mm256_add_ps(x2, *(__m256*)(p3+i));
    x2 = _mm256_add_ps(x1, *(__m256*)(p4+i));
    x1 = _mm256_add_ps(x2, *(__m256*)(p5+i));

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x1);
    x2 = _mm256_and_ps(x1, tailmask);
    maxV = _mm256_max_ps(maxV, x2);
    x1 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x1);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBa5[FOLDTBLEN] = {foldBy5};


float foldBy2(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_load_ps(p1+i);
        x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));
        _mm256_store_ps(pst+i, x2);
        maxV = _mm256_max_ps(maxV, x2);
        i += 8;
    }
    x1 = _mm256_load_ps(p1+i);
    x2 = _mm256_add_ps(x1, *(__m256*)(p2+i));

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x2);
    x1 = _mm256_and_ps(x2, tailmask);
    maxV = _mm256_max_ps(maxV, x1);
    x2 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x2);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBa2[FOLDTBLEN] = {foldBy2};

FoldSet AVXfold_a = {AVXTBa3, AVXTBa4, AVXTBa5, AVXTBa2, AVXTBa2, "JS AVX_a"};



// Alternate set using 16 byte unaligned loads and inserts for the probably unaligned
// elements, based on Intel optimization recommendations.
float foldBy3c(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);
        x1 = _mm256_add_ps(x2, *(__m256*)(p1+i));
        x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);
        x1 = _mm256_add_ps(x1, x2);
        _mm256_store_ps(pst+i, x1);
        maxV = _mm256_max_ps(maxV, x1);
        i += 8;
    }
    x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);      
    x1 = _mm256_add_ps(x2, *(__m256*)(p1+i));                                                                          
    x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);      
    x1 = _mm256_add_ps(x1, x2);                                                                        

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x1);
    x2 = _mm256_and_ps(x1, tailmask);
    maxV = _mm256_max_ps(maxV, x2);
    x1 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x1);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBc3[FOLDTBLEN] = {foldBy3c};


float foldBy4c(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);      
        x2 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                          
        x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);      
        x2 = _mm256_add_ps(x2, x1);                                                                        
        x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p4+i)), _mm_load_ps(p4+i+4), 1);      
        x2 = _mm256_add_ps(x2, x1);                                                                        
        _mm256_store_ps(pst+i, x2);
        maxV = _mm256_max_ps(maxV, x2);
        i += 8;
    }
    x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);
    x2 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                
    x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);
    x2 = _mm256_add_ps(x2, x1);                                                                  
    x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p4+i)), _mm_load_ps(p4+i+4), 1);
    x2 = _mm256_add_ps(x2, x1);                                                                  

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x2);
    x1 = _mm256_and_ps(x2, tailmask);
    maxV = _mm256_max_ps(maxV, x1);
    x2 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x2);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBc4[FOLDTBLEN] = {foldBy4c};


float foldBy5c(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2, *p5 = ss[0]+P->tmp3;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);
        x1 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                
        x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);
        x1 = _mm256_add_ps(x1, x2);                                                                  
        x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p4+i)), _mm_load_ps(p4+i+4), 1);
        x1 = _mm256_add_ps(x1, x2);                                                                  
        x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p5+i)), _mm_load_ps(p5+i+4), 1);
        x1 = _mm256_add_ps(x1, x2);                                                                  
        _mm256_store_ps(pst+i, x1);
        maxV = _mm256_max_ps(maxV, x1);
        i += 8;
    }
    x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);
    x1 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                
    x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p3+i)), _mm_load_ps(p3+i+4), 1);
    x1 = _mm256_add_ps(x1, x2);                                                                  
    x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p4+i)), _mm_load_ps(p4+i+4), 1);
    x1 = _mm256_add_ps(x1, x2);                                                                  
    x2 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p5+i)), _mm_load_ps(p5+i+4), 1);
    x1 = _mm256_add_ps(x1, x2);                                                                  

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x1);
    x2 = _mm256_and_ps(x1, tailmask);
    maxV = _mm256_max_ps(maxV, x2);
    x1 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x1);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBc5[FOLDTBLEN] = {foldBy5c};


float foldBy2c(float *ss[], struct PoTPlan *P) {
    const float lim = (float)(((P->di) - 1) & 7);
    __m256 tailelem = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
    __m256 taillim = _mm256_broadcast_ss(&lim);
    __m256 maxV = _mm256_setzero_ps();
    int i = 0;

    const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
    float *pst = P->dest;
    __m256 x1, x2, tailmask;

    while (i < P->di-8) {
        x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);      
        x2 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                          
        _mm256_store_ps(pst+i, x2);
        maxV = _mm256_max_ps(maxV, x2);
        i += 8;
    }
    x1 = _mm256_insertf128_ps(_mm256_castps128_ps256(_mm_loadu_ps(p2+i)), _mm_load_ps(p2+i+4), 1);      
    x2 = _mm256_add_ps(x1, *(__m256*)(p1+i));                                                                          

    tailmask = _mm256_cmp_ps(taillim, tailelem, 0x0d);
    _mm256_maskstore_ps(pst+i, AVX_MASKSTORE_TYPECAST(tailmask), x2);
    x1 = _mm256_and_ps(x2, tailmask);
    maxV = _mm256_max_ps(maxV, x1);
    x2 = _mm256_permute2f128_ps(maxV, maxV, 0x81);
    maxV = _mm256_max_ps(maxV, x2);
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0xee));
    maxV = _mm256_max_ps(maxV, _mm256_shuffle_ps(maxV, maxV, 0x01));
    float max = _mm_cvtss_f32(_mm256_castps256_ps128(maxV));
    _mm256_zeroupper();
    return max;
}

sum_func AVXTBc2[FOLDTBLEN] = {foldBy2c};

FoldSet AVXfold_c = {AVXTBc3, AVXTBc4, AVXTBc5, AVXTBc2, AVXTBc2, "JS AVX_c"};



#endif // (USE_AVX) && (USE_INTRINSICS)
