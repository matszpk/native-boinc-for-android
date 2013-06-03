// Copyright 2003-2005 Regents of the University of California

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

// AMD optimizations by Evandro Menezes

// $Id: analyzeFuncs_x86_64.cpp,v 1.1.2.4 2007/05/31 22:03:13 korpela Exp $


#if defined(__i386__) || defined(__x86_64__) || defined (_M_AMD64) || defined(_M_IX86)
#include "sah_config.h"

#include <climits>
#include <cmath>
#include <memory.h>
#include <cstdio>

#include "x86_ops.h"
#include "x86_float4.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "seti.h"
#include "s_util.h"
#include "worker.h"

// chirp_rate is in Hz per second
int v_vChirpData_x86_64(
    sah_complex * fp_DataArray,
    sah_complex * fp_ChirpDataArray,
    int ChirpRateInd,
    double chirp_rate,
    int  ul_NumDataPoints,
    double sample_rate
) {
    static const int as [4]  __attribute__((aligned(16)))= {INT_MIN, 0, INT_MIN, 0} ; // {-, +, -, +}
    char *cblock = (char *)alloca(11*16);
    cblock+=(16-((ssize_t)cblock % 16));
    x86_m128 *fblock=reinterpret_cast<x86_m128 *>(cblock);
    x86_m128d *dblock=reinterpret_cast<x86_m128d *>(cblock);
    #define CC dblock[0]
    #define DD dblock[1]
    #define cc fblock[2]
    #define dd fblock[3]
    #define ri fblock[4]
    #define ri1 fblock[5]
    #define ri2 fblock[6]
    #define ss fblock[7]
    #define zz fblock[8]

    memcpy(&(ss),as,sizeof(as));
    zz = _mm_setzero_ps ();
    int i, j;
    // float c, d, real, imag;
    float time;
    float ang;

    double aC []  __attribute__((aligned(16))) = {0, 0};
    double aD []  __attribute__((aligned(16))) = {0, 0} ;

    float chirpInvariant = (float)(0.5*chirp_rate/(sample_rate*sample_rate));
    
    if (chirp_rate == 0.0) {
        memcpy(fp_ChirpDataArray,
       fp_DataArray,
       (int)ul_NumDataPoints * 2 * sizeof(float)
   );    // NOTE INT CAST
    } else {
            for (i = 0, j = 0; i < (ul_NumDataPoints - 2); i += 2, j += 2) {

                // _mm_prefetch (fp_DataArray      + i + 32, _MM_HINT_T0);
                _mm_prefetch ((char *) (fp_ChirpDataArray + i) + 384, _MM_HINT_T0);

                time = (float)j*j;
		        ang = time*chirpInvariant;
                ang -= floor(ang);
		        ang *= (float)(M_PI*2);
#ifndef HAVE_SINCOS
                aC [0] = cos (ang);
                aD [0] = sin (ang);
#else
                sincos (ang, aD + 0, aC + 0);
#endif

                time = (float)(j + 1)*(j + 1);
		        ang = chirpInvariant*time;
                ang -= floor(ang);
		        ang *= (float)(M_PI*2);
#ifndef HAVE_SINCOS
                aC [1] = cos (ang);
                aD [1] = sin (ang);
#else
                sincos (ang, aD + 1, aC + 1);
#endif

                CC = _mm_loadu_pd (aC);
                DD = _mm_loadu_pd (aD);

                cc = _mm_cvtpd_ps (CC);
                dd = _mm_cvtpd_ps (DD);

                cc = _mm_unpacklo_ps (cc, cc);
                dd = _mm_unpacklo_ps (dd, dd);

                // Sometimes chirping is done in place.
                // We don't want to overwrite data prematurely.
                // real = fp_DataArray[i] * c - fp_DataArray[i+1] * d;
                // imag = fp_DataArray[i] * d + fp_DataArray[i+1] * c;
                ri1 = _mm_loadu_ps ((float *)(fp_DataArray + i));
                ri2 = _mm_shuffle_ps (ri1, ri1, _MM_SHUFFLE (2, 3, 0, 1));
                ri2 = _mm_xor_ps (ri2, ss);
                ri1 = _mm_mul_ps (ri1, cc);
                ri2 = _mm_mul_ps (ri2, dd);
                ri  = _mm_add_ps (ri1, ri2);

                // fp_ChirpDataArray[i] = real;
                // fp_ChirpDataArray[i+1] = imag;
                _mm_storeu_ps ((float *)(fp_ChirpDataArray + i), ri);
            }
            for (; i < ul_NumDataPoints; i ++, j++) {

                // _mm_prefetch (fp_DataArray      + i + 16, _MM_HINT_T0);
                _mm_prefetch ((char *) (fp_ChirpDataArray + i) + 384, _MM_HINT_T0);

                time = (float)j*j;
		        ang = chirpInvariant*time;
                ang -= floor(ang);
		        ang *= (float)(M_PI*2);
#ifndef HAVE_SINCOS
                aC [0] = cos (ang);
                aC [1] = sin (ang);
#else
                sincos (ang, aC + 0, aC + 1);
#endif

                CC = _mm_loadu_pd (aC);

                cc = _mm_cvtpd_ps (CC);

                cc = _mm_unpacklo_ps (cc, cc);
                dd = _mm_movehl_ps (zz, cc);

                // Sometimes chirping is done in place.
                // We don't want to overwrite data prematurely.
                // real = fp_DataArray[i] * c - fp_DataArray[i+1] * d;
                // imag = fp_DataArray[i] * d + fp_DataArray[i+1] * c;
                ri1 = _mm_loadl_pi (zz, (__m64 *) (fp_DataArray + i));
                ri2 = _mm_shuffle_ps (ri1, ri1, _MM_SHUFFLE (2, 3, 0, 1));
                ri2 = _mm_xor_ps (ri2, ss);
                ri1 = _mm_mul_ps (ri1, cc);
                ri2 = _mm_mul_ps (ri2, dd);
                ri  = _mm_add_ps (ri1, ri2);

                // fp_ChirpDataArray[i] = real;
                // fp_ChirpDataArray[i+1] = imag;
                _mm_storel_pi ((__m64 *) (fp_ChirpDataArray + i), ri);
            }
    }
    return 0;
}
#endif  

