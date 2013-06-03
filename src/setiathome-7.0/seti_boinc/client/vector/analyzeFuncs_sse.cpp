
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

// $Id: analyzeFuncs_sse.cpp,v 1.1.2.10 2007/06/08 03:09:47 korpela Exp $
//

// This file is empty is __i386__ is not defined
#include "sah_config.h"
#include <vector>

#if defined(__i386__) || defined(__x86_64__) || defined(USE_SSE)

#define INVALID_CHIRP 2e+20

#include "analyzeFuncs.h"
#include "analyzeFuncs_vector.h"
#include "analyzePoT.h"
#include "analyzeReport.h"
#include "gaussfit.h"
#include "s_util.h"
#include "diagnostics.h"
#include "asmlib.h"
#include "pulsefind.h"

#include "x86_ops.h"
#include "x86_float4.h"

static bool checked=false;
static bool hasSSE=false;

bool boinc_has_sse( void ) {
#ifdef USE_ASMLIB
    static bool hasSSE = (InstructionSet() >= 3) ? true : false;
    return hasSSE;
#else
    return true;
#endif
}

template <int x>
inline void v_pfsubTranspose(float *in, float *out, int xline, int yline) {
    // Transpose an X by X subsection of a XLINE by YLINE matrix into the
    // appropriate part of a YLINE by XLINE matrix.  "IN" points to the first
    // (lowest address) element of the input submatrix.  "OUT" points to the
    // first (lowest address) element of the output submatrix.
    int i,j;
    float *p;
    register float tmp[x*x];
    for (j=0;j<x;j++) {
        p=in+j*xline;
        prefetcht0(out+j*yline);
        for (i=0;i<x;i++) {
            tmp[j+i*x]=*(p++);
        }
    }
    for (j=0;j<x;j++) {
        p=out+j*yline;
        for (i=0;i<x;i++) {
            *(p++)=tmp[i+j*x];
        }
        prefetcht0(in+j*xline+x);
    }
}

int v_pfTranspose2(int x, int y, float *in, float *out) {
// Attempts to improve cache hit ratio by transposing 4 elements at a time.
    int i,j;
    for (j=0;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
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
    return 0;
}

int v_pfTranspose4(int x, int y, float *in, float *out) {
// Attempts to improve cache hit ratio by transposing 16 elements at a time.
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_pfsubTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
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
    return 0;
}

int v_pfTranspose8(int x, int y, float *in, float *out) {
// Attempts to improve cache hit ratio by transposing 64 elements at a time.
    int i,j;
    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-7;i+=8) {
            v_pfsubTranspose<8>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-3;i+=4) {
            v_pfsubTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_pfsubTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
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
    return 0;
}


inline void v_vsubTranspose4(float *in, float *out, int xline, int yline) {
    // do a 4x4 transpose in the SSE registers.
    // This could probably be optimized a bit further.
    prefetcht0(out);
    prefetcht0(out+yline);
    prefetcht0(out+2*yline);
    prefetcht0(out+3*yline);
    // TODO: figure out why the intrinsic version crashes for MinGW build
    // not critical, but shuffle-only _MM_TRANSPOSE4_PS is optimal on some
#if defined(USE_INTRINSICS) && defined(_MM_TRANSPOSE4_PS) && !defined(__GNUC__)
    register float4 row0=*(__m128 *)in;
    register float4 row1=*(__m128 *)(in+xline);
    register float4 row2=*(__m128 *)(in+2*xline);
    register float4 row3=*(__m128 *)(in+3*xline);
    _MM_TRANSPOSE4_PS(row0,row1,row2,row3);
    *(__m128 *)out=row0;
    *(__m128 *)(out+yline)=row1;
    *(__m128 *)(out+2*yline)=row2;
    *(__m128 *)(out+3*yline)=row3;
#elif defined(__GNUC__)
    asm (
        // %4=a1 b1 c1 d1
        // %5=a2 b2 c2 d2
        // %6=a3 b3 c3 d3
        // %7=a4 b4 c4 d4
        "movaps %4,%0\n\t"        // %0=a1 b1 c1 d1
        "movaps %6,%1\n\t"        // %1=a3 b3 c3 d3
        "unpcklps %5,%0\n\t"      // %0=a1 a2 b1 b2
        "movaps %4,%2\n\t"        // %2=a1 b1 c1 d1
        "unpcklps %7,%1\n\t"      // %1=a3 a4 b3 b4
        "movaps %6,%3\n\t"        // %3=a2 b2 c2 d2
        "unpckhps %5,%2\n\t"      // %2=c1 c2 d1 d2
        "unpckhps %7,%3\n\t"      // %3=c3 c4 d3 d4
        "movaps %1,%5\n\t"        // %5=a3 a4 b3 b4
        "movaps %0,%1\n\t"        // %1=a1 a2 b1 b2
        "shufps $68,%5,%0\n\t"    // %0=a1 a2 a3 a4
        "shufps $238,%5,%1\n\t"   // %4=b1 b2 b3 b4
        "movaps %3,%5\n\t"        // %5=a3 a4 b3 b4
        "movaps %2,%3\n\t"        // %3=a3 a4 b3 b4
        "shufps $68,%5,%2\n\t"    // %2=c1 c2 c3 c4
        "shufps $238,%5,%3\n\t"   // %3=d1 d2 d3 d4
    : "=&x" (*(__m128 *)out), "=&x" (*(__m128 *)(out+yline)),
        "=&x" (*(__m128 *)(out+2*yline)), "=&x" (*(__m128 *)(out+3*yline))
                : "x" (*(__m128 *)in), "x" (*(__m128 *)(in+xline)),
                "x" (*(__m128 *)(in+2*xline)), "x" (*(__m128 *)(in+3*xline))
            );
#else
    // no intrinsics, no GCC, just do something which should work
    v_pfsubTranspose<4>(in, out, xline, yline);
#endif
    prefetcht0(in+0*xline+4);
    prefetcht0(in+1*xline+4);
    prefetcht0(in+2*xline+4);
    prefetcht0(in+3*xline+4);
}

int v_vTranspose4(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_vsubTranspose4(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_pfsubTranspose<2>(in+j*x+i,out+y*i+j,x,y);
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
    return 0;
}
inline void v_vsubTranspose4np(float *in, float *out, int xline, int yline) {
    // do a 4x4 transpose in the SSE registers.
    // This could probably be optimized a bit further.
    // JWS: No prefetches in this version, faster on some systems.

    // TODO: figure out why the intrinsic version crashes for MinGW build
    // not critical, but the shuffle-only _MM_TRANSPOSE4_PS is optimal on some
#if defined(USE_INTRINSICS) && defined(_MM_TRANSPOSE4_PS) && !defined(__GNUC__)
    register float4 row0=*(__m128 *)in;
    register float4 row1=*(__m128 *)(in+xline);
    register float4 row2=*(__m128 *)(in+2*xline);
    register float4 row3=*(__m128 *)(in+3*xline);
    _MM_TRANSPOSE4_PS(row0,row1,row2,row3);
    *(__m128 *)out=row0;
    *(__m128 *)(out+yline)=row1;
    *(__m128 *)(out+2*yline)=row2;
    *(__m128 *)(out+3*yline)=row3;
#elif defined(__GNUC__)
    asm (
        // %4=a1 b1 c1 d1
        // %5=a2 b2 c2 d2
        // %6=a3 b3 c3 d3
        // %7=a4 b4 c4 d4
        "movaps %4,%0\n\t"        // %0=a1 b1 c1 d1
        "movaps %6,%1\n\t"        // %1=a3 b3 c3 d3
        "unpcklps %5,%0\n\t"      // %0=a1 a2 b1 b2
        "movaps %4,%2\n\t"        // %2=a1 b1 c1 d1
        "unpcklps %7,%1\n\t"      // %1=a3 a4 b3 b4
        "movaps %6,%3\n\t"        // %3=a2 b2 c2 d2
        "unpckhps %5,%2\n\t"      // %2=c1 c2 d1 d2
        "unpckhps %7,%3\n\t"      // %3=c3 c4 d3 d4
        "movaps %1,%5\n\t"        // %5=a3 a4 b3 b4
        "movaps %0,%1\n\t"        // %1=a1 a2 b1 b2
        "shufps $68,%5,%0\n\t"    // %0=a1 a2 a3 a4
        "shufps $238,%5,%1\n\t"   // %1=b1 b2 b3 b4
        "movaps %3,%5\n\t"        // %5=a3 a4 b3 b4
        "movaps %2,%3\n\t"        // %3=a3 a4 b3 b4
        "shufps $68,%5,%2\n\t"    // %2=c1 c2 c3 c4
        "shufps $238,%5,%3\n\t"   // %3=d1 d2 d3 d4
    : "=&x" (*(__m128 *)out), "=&x" (*(__m128 *)(out+yline)),
        "=&x" (*(__m128 *)(out+2*yline)), "=&x" (*(__m128 *)(out+3*yline))
                : "x" (*(__m128 *)in), "x" (*(__m128 *)(in+xline)),
                "x" (*(__m128 *)(in+2*xline)), "x" (*(__m128 *)(in+3*xline))
            );
#else
    // no intrinsics, no GCC, just do something which should work
    v_pfsubTranspose<4>(in, out, xline, yline);
#endif
}

int v_vTranspose4np(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_vsubTranspose4np(in+j*x+i,out+y*i+j,x,y);
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
    return 0;
}

// Following section disappears without intrinsics
#ifdef USE_INTRINSICS
inline void v_vsubTranspose4ntw(float *in, float *out, int xline, int yline) {
    // Do a 4x4 transpose in the SSE registers, non-temporal writes.
    // An sfence is needed after using this sub to ensure global visibilty of the writes.
    __m128 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

    tmp1 = _mm_load_ps(in);                 // a3 a2 a1 a0
    tmp2 = _mm_load_ps(in+xline);           // b3 b2 b1 b0
    tmp3 = _mm_load_ps(in+2*xline);         // c3 c2 c1 c0
    tmp4 = _mm_load_ps(in+3*xline);         // d3 d2 d1 d0
    tmp5 = tmp1;                            // a3 a2 a1 a0
    tmp6 = tmp3;                            // c3 c2 c1 c0
    tmp1 = _mm_unpacklo_ps(tmp1,tmp2);      // b1 a1 b0 a0
    tmp3 = _mm_unpacklo_ps(tmp3,tmp4);      // d1 c1 d0 c0
    tmp5 = _mm_unpackhi_ps(tmp5,tmp2);      // b3 a3 b2 a2
    tmp6 = _mm_unpackhi_ps(tmp6,tmp4);      // d3 c3 d2 c2
    tmp2 = tmp1;                            // b1 a1 b0 a0
    tmp1 = _mm_movelh_ps(tmp1,tmp3);        // d0 c0 b0 a0
    tmp3 = _mm_movehl_ps(tmp3,tmp2);        // d1 c1 b1 a1
    _mm_stream_ps(out+0*yline, tmp1);
    tmp2 = tmp5;                            // b3 a3 b2 a2
    _mm_stream_ps(out+1*yline, tmp3);
    tmp5 = _mm_movelh_ps(tmp5,tmp6);        // d2 c2 b2 a2
    tmp6 = _mm_movehl_ps(tmp6,tmp2);        // d3 c3 b3 a3
    _mm_stream_ps(out+2*yline, tmp5);
    _mm_stream_ps(out+3*yline, tmp6);
}

int v_vTranspose4ntw(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_vsubTranspose4ntw(in+j*x+i,out+y*i+j,x,y);
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
    _mm_sfence();
    return 0;
}

int v_vTranspose4x8ntw(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-3;i+=4) {
            v_vsubTranspose4ntw(in+j*x+i,out+y*i+j,x,y);
            v_vsubTranspose4ntw(in+(j+4)*x+i,out+y*i+j+4,x,y);
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
    _mm_sfence();
    return 0;
}

int v_vTranspose4x16ntw(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-15;j+=16) {
        for (i=0;i<x-3;i+=4) {
            v_vsubTranspose4ntw(in+j*x+i,out+y*i+j,x,y);
            v_vsubTranspose4ntw(in+(j+4)*x+i,out+y*i+j+4,x,y);
            v_vsubTranspose4ntw(in+(j+8)*x+i,out+y*i+j+8,x,y);
            v_vsubTranspose4ntw(in+(j+12)*x+i,out+y*i+j+12,x,y);
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
    _mm_sfence();
    return 0;
}
int v_vpfTranspose8x4ntw(int x, int y, float *in, float *out) {
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-7;i+=8) {
            float *tin = in+j*x+i;
            v_vsubTranspose4ntw(tin,out+y*i+j,x,y);
            v_vsubTranspose4ntw(tin+4,out+y*(i+4)+j,x,y);
            prefetcht0(tin+0*x+10);
            prefetcht0(tin+1*x+10);
            prefetcht0(tin+2*x+10);
            prefetcht0(tin+3*x+10);
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
    _mm_sfence();
    return 0;
}
#endif  // USE_INTRINSICS

int v_vGetPowerSpectrum(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, vEnd;
#if defined(__GNUC__) && (__GNUC__ < 4)
    register __m128 xmm6 asm("xmm6");
    register __m128 xmm7 asm("xmm7");
#endif

    if (!boinc_has_sse()) return UNSUPPORTED_FUNCTION;

    analysis_state.FLOP_counter+=3.0*NumDataPoints;

    vEnd = NumDataPoints - (NumDataPoints & 3);
    for (i = 0; i < vEnd; i += 4) {
        prefetcht0(FreqData+i+64);
        prefetcht0(PowerSpectrum+i+64);
#if defined(__GNUC__) && (__GNUC__ < 4)
        __asm__ (
            "mulps %0,%0\n\t"
            "mulps %4,%4\n\t"
            "movaps %0,%1\n\t"
            "movaps %4,%2\n\t"
            "shufps $177,%0,%0\n\t"
            "shufps $177,%4,%4\n\t"
            "addps %1,%0\n\t"
            "addps %2,%4\n\t"
            "shufps $136,%4,%0\n\t"
    : "=x" (*(__m128 *)(&(PowerSpectrum[i]))), "=x" (xmm6), "=x" (xmm7)
                    : "0" (*(__m128 *)(&(FreqData[i]))), "x" (*(__m128 *)(&(FreqData[i+2])))
                ) ;
#elif defined(USE_INTRINSICS)
//Jason: Trial Intrinsics version 
        __m128 fdata = _mm_load_ps( (float *) &(FreqData[i]) );
        __m128 fdata2 = _mm_load_ps( (float *) &(FreqData[i+2]) );
        fdata = _mm_mul_ps(fdata, fdata);
        fdata2 = _mm_mul_ps(fdata2, fdata2);
        __m128 tmp1 = fdata;
        __m128 tmp2 = fdata2;
        fdata = _mm_shuffle_ps( fdata,fdata,0xb1);
        fdata2 = _mm_shuffle_ps( fdata2,fdata2,0xb1);
        fdata = _mm_add_ps( fdata, tmp1 );
        fdata2 = _mm_add_ps( fdata2, tmp2 );
        fdata = _mm_shuffle_ps( fdata, fdata2, 0x88 );
        _mm_store_ps( (float *) (&(PowerSpectrum[i])), fdata); 
#else
        break;
#endif
    }

    // handle tail elements, although in practice this never happens
    for (; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    return 0;
}

int v_vGetPowerSpectrumUnrolled(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, vEnd;
#if defined(__GNUC__) && (__GNUC__ < 4)
    register __m128 xmm6 asm("xmm6");
    register __m128 xmm7 asm("xmm7");
#endif
    if (!boinc_has_sse()) return UNSUPPORTED_FUNCTION;

    analysis_state.FLOP_counter+=3.0*NumDataPoints;

    vEnd = NumDataPoints - (NumDataPoints & 7);
    for (i = 0; i < vEnd; i += 8) {
        prefetcht0(FreqData+i+64);
        prefetcht0(PowerSpectrum+i+64);
#if defined(__GNUC__) && ((__GNUC__ < 4) || !defined(USE_INTRINSICS))
        __asm__ (
            "mulps %4,%4\n\t"
            "mulps %5,%5\n\t"
            "movaps %4,%2\n\t"
            "mulps %6,%6\n\t"
            "movaps %5,%3\n\t"
            "mulps %7,%7\n\t"
            "shufps $177,%4,%4\n\t"
            "shufps $177,%5,%5\n\t"
            "addps %2,%4\n\t"
            "addps %3,%5\n\t"
            "movaps %6,%2\n\t"
            "shufps $136,%5,%0\n\t"
            "movaps %7,%3\n\t"
            "shufps $177,%6,%6\n\t"
            "shufps $177,%7,%7\n\t"
            "addps %2,%6\n\t"
            "addps %3,%7\n\t"
            "shufps $136,%7,%6\n\t"
    : "=x" (*(__m128 *)(&(PowerSpectrum[i]))),
            "=x" (*(__m128 *)(&(PowerSpectrum[i+4]))),
            "=x" (xmm6), "=x" (xmm7)
                    : "0" (*(__m128 *)(&(FreqData[i]))),
                    "x" (*(__m128 *)(&(FreqData[i+2]))),
                    "1" (*(__m128 *)(&(FreqData[i+4]))),
                    "x" (*(__m128 *)(&(FreqData[i+6])))
                ) ;
#elif defined(USE_INTRINSICS)
//Jason: Trial Intrinsics version 
        __m128 fdata = _mm_load_ps( (float *) &(FreqData[i]) );
        __m128 fdata2 = _mm_load_ps( (float *) &(FreqData[i+2]) );
        __m128 fdata3 = _mm_load_ps( (float *) &(FreqData[i+4]) );
        __m128 fdata4 = _mm_load_ps( (float *) &(FreqData[i+6]) );
        fdata = _mm_mul_ps(fdata, fdata);
        fdata2 = _mm_mul_ps(fdata2, fdata2);
        __m128 tmp1 = fdata;
        fdata3 = _mm_mul_ps(fdata3, fdata3);
        __m128 tmp2 = fdata2;
        fdata4 = _mm_mul_ps(fdata4, fdata4);
        fdata = _mm_shuffle_ps( fdata,fdata,0xb1);
        fdata2 = _mm_shuffle_ps( fdata2,fdata2,0xb1);
        fdata = _mm_add_ps( fdata, tmp1 );
        fdata2 = _mm_add_ps( fdata2, tmp2 );
        tmp1 = fdata3;
        fdata = _mm_shuffle_ps( fdata, fdata2, 0x88 );
        tmp2 = fdata4;
        fdata3 = _mm_shuffle_ps( fdata3,fdata3,0xb1);
        fdata4 = _mm_shuffle_ps( fdata4,fdata4,0xb1);
        fdata3 = _mm_add_ps( fdata3, tmp1 );
        fdata4 = _mm_add_ps( fdata4, tmp2 );
        fdata3 = _mm_shuffle_ps( fdata3, fdata4, 0x88 );
        _mm_store_ps( (float *) (&(PowerSpectrum[i])), fdata); 
        _mm_store_ps( (float *) (&(PowerSpectrum[i+4])), fdata3); 
#else
        break;
#endif
    }

    // handle tail elements, although in practice this never happens
    for (; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    return 0;
}

// No prefetches, faster on some systems.
int v_vGetPowerSpectrum2(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, vEnd;
#if defined(__GNUC__) && (__GNUC__ < 4)
    register __m128 xmm6 asm("xmm6");
    register __m128 xmm7 asm("xmm7");
#endif
    if (!boinc_has_sse()) return UNSUPPORTED_FUNCTION;

    analysis_state.FLOP_counter+=3.0*NumDataPoints;

    vEnd = NumDataPoints - (NumDataPoints & 3);
    for (i = 0; i < vEnd; i += 4) {
#if defined(__GNUC__) && (__GNUC__ < 4)
        __asm__ (
            "mulps %0,%0\n\t"
            "mulps %4,%4\n\t"
            "movaps %0,%1\n\t"
            "movaps %4,%2\n\t"
            "shufps $177,%0,%0\n\t"
            "shufps $177,%4,%4\n\t"
            "addps %1,%0\n\t"
            "addps %2,%4\n\t"
            "shufps $136,%4,%0\n\t"
        : "=x" (*(__m128 *)(&(PowerSpectrum[i]))), "=x" (xmm6), "=x" (xmm7)
        : "0" (*(__m128 *)(&(FreqData[i]))), "x" (*(__m128 *)(&(FreqData[i+2])))
        ) ;
#elif defined(USE_INTRINSICS)
//Jason: Trial Intrinsics version 
        __m128 fdata = _mm_load_ps( (float *) &(FreqData[i]) );
        __m128 fdata2 = _mm_load_ps( (float *) &(FreqData[i+2]) );
        fdata = _mm_mul_ps(fdata, fdata);
        fdata2 = _mm_mul_ps(fdata2, fdata2);
        __m128 tmp1 = fdata;
        __m128 tmp2 = fdata2;
        fdata = _mm_shuffle_ps( fdata,fdata,0xb1);
        fdata2 = _mm_shuffle_ps( fdata2,fdata2,0xb1);
        fdata = _mm_add_ps( fdata, tmp1 );
        fdata2 = _mm_add_ps( fdata2, tmp2 );
        fdata = _mm_shuffle_ps( fdata, fdata2, 0x88 );
        _mm_store_ps( (float *) (&(PowerSpectrum[i])), fdata); 
#else
        break;
#endif
    }

    // handle tail elements, although in practice this never happens
    for (; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    return 0;
}

// No prefetches, faster on some systems.
int v_vGetPowerSpectrumUnrolled2(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, vEnd;
#if  defined(__GNUC__) && (__GNUC__ < 4)
    register __m128 xmm6 asm("xmm6");
    register __m128 xmm7 asm("xmm7");
#endif

    if (!boinc_has_sse()) return UNSUPPORTED_FUNCTION;

    analysis_state.FLOP_counter+=3.0*NumDataPoints;

    vEnd = NumDataPoints - (NumDataPoints & 7);
    for (i = 0; i < vEnd; i += 8) {
#if defined(__GNUC__) && (__GNUC__ < 4)
        __asm__ (
            "mulps %4,%4\n\t"
            "mulps %5,%5\n\t"
            "movaps %4,%2\n\t"
            "mulps %6,%6\n\t"
            "movaps %5,%3\n\t"
            "mulps %7,%7\n\t"
            "shufps $177,%4,%4\n\t"
            "shufps $177,%5,%5\n\t"
            "addps %2,%4\n\t"
            "addps %3,%5\n\t"
            "movaps %6,%2\n\t"
            "shufps $136,%5,%0\n\t"
            "movaps %7,%3\n\t"
            "shufps $177,%6,%6\n\t"
            "shufps $177,%7,%7\n\t"
            "addps %2,%6\n\t"
            "addps %3,%7\n\t"
            "shufps $136,%7,%6\n\t"
        : "=x" (*(__m128 *)(&(PowerSpectrum[i]))),
          "=x" (*(__m128 *)(&(PowerSpectrum[i+4]))),
          "=x" (xmm6), "=x" (xmm7)
        : "0" (*(__m128 *)(&(FreqData[i]))),
          "x" (*(__m128 *)(&(FreqData[i+2]))),
          "1" (*(__m128 *)(&(FreqData[i+4]))),
          "x" (*(__m128 *)(&(FreqData[i+6])))
        );
#elif defined(USE_INTRINSICS)
//Jason: Trial Intrinsics version 
        __m128 fdata = _mm_load_ps( (float *) &(FreqData[i]) );
        __m128 fdata2 = _mm_load_ps( (float *) &(FreqData[i+2]) );
        __m128 fdata3 = _mm_load_ps( (float *) &(FreqData[i+4]) );
        __m128 fdata4 = _mm_load_ps( (float *) &(FreqData[i+6]) );
        fdata = _mm_mul_ps(fdata, fdata);
        fdata2 = _mm_mul_ps(fdata2, fdata2);
        __m128 tmp1 = fdata;
        fdata3 = _mm_mul_ps(fdata3, fdata3);
        __m128 tmp2 = fdata2;
        fdata4 = _mm_mul_ps(fdata4, fdata4);
        fdata = _mm_shuffle_ps( fdata,fdata,0xb1);
        fdata2 = _mm_shuffle_ps( fdata2,fdata2,0xb1);
        fdata = _mm_add_ps( fdata, tmp1 );
        fdata2 = _mm_add_ps( fdata2, tmp2 );
        tmp1 = fdata3;
        fdata = _mm_shuffle_ps( fdata, fdata2, 0x88 );
        tmp2 = fdata4;
        fdata3 = _mm_shuffle_ps( fdata3,fdata3,0xb1);
        fdata4 = _mm_shuffle_ps( fdata4,fdata4,0xb1);
        fdata3 = _mm_add_ps( fdata3, tmp1 );
        fdata4 = _mm_add_ps( fdata4, tmp2 );
        fdata3 = _mm_shuffle_ps( fdata3, fdata4, 0x88 );
        _mm_store_ps( (float *) (&(PowerSpectrum[i])), fdata); 
        _mm_store_ps( (float *) (&(PowerSpectrum[i+4])), fdata3); 
#else
        break;
#endif
    }

    // handle tail elements, although in practice this never happens
    for (; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    return 0;
}


// Following disappears without intrinsics
#ifdef USE_INTRINSICS

// =============================================================================
//
// sse1_ChirpData_ak
// version by: Alex Kan
//   http://tbp.berkeley.edu/~alexkan/seti/
//
// refactored for SSE by Ben Herndon
// modified by Joe Segur:
//   Added switch to 64 bit precision on Win32 to allow fast rounding
//   Unrolled loop once (to stride 8) to better amortize precision switching

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

#elif defined(__GNUC__) && defined(__i386__) && !defined(__x86_64__)
    __asm__ __volatile__( 
        "fadd  %2,%0\n"  // + roundVal
"        fsub  %2,%0\n"  // - roundVal, integer in st(0)
"        fsubr %3,%0\n"  // angle - integer
        : "=&t" (val)
        : "0" (val), "f" (roundVal), "f" (val)
    );
#elif defined(_WIN64) || defined(SUPPORTS_ATTRIB_OPT)
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


__m128 vec_recip1(__m128 v) {
    // obtain estimate
    __m128 estimate = _mm_rcp_ps( v );
    // one round of Newton-Raphson
    return _mm_add_ps(_mm_mul_ps(_mm_sub_ps(ONE, _mm_mul_ps(estimate, v)), estimate), estimate);
}

int sse1_ChirpData_ak(
    sah_complex * fp_DataArray,
    sah_complex * fp_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
) {

    if (ChirpRateInd == 0) {
        memcpy(fp_ChirpDataArray, fp_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
        return 0;
    }

    double srate    = ChirpRate * 0.5 / (sample_rate * sample_rate);
#if defined(_LP64) || defined(_WIN64) || (defined(__GNUC__) && defined(__SSE_MATH__))
    // For scalar XMM code or any FPU with 64 bit registers
    double roundVal = (srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52; 
#else
    // For X87 FPU
    double roundVal = (srate >= 0.0) ? ROUNDX87 : -ROUNDX87;
#endif
    int i, vEnd;
    unsigned short fpucw1, fpucw2;

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);
    for (i = 0; i < vEnd; i += 8) {
        const float *data = (const float *) (fp_DataArray + i);
        float *chirped = (float *) (fp_ChirpDataArray + i);
        double angles[8], dtemp;
        ALIGNED(float, 16) v_angle[4], v_angle2[4];
//      float __attribute__ ((aligned (16))) v_angle[4], v_angle2[4];

        __m128 d1, d2;
        __m128 cd1, cd2, cd3;
        __m128 td1, td2;
        __m128 x;
        __m128 y;
        __m128 s;
        __m128 c;
        __m128 m;

        prefetchnta((const void *)( data+32 ));

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

        // reduce the angle to the range (-0.5, 0.5)
        // convert doubles into singles
        // This does "angles - round(angles)"
        set_extended_precision();
        set_up_fastfrac(roundVal);
        v_angle[0] = fastfrac(angles[0],roundVal);
        v_angle[1] = fastfrac(angles[1],roundVal);
        v_angle[2] = fastfrac(angles[2],roundVal);
        v_angle[3] = fastfrac(angles[3],roundVal);
        v_angle2[0] = fastfrac(angles[4],roundVal);
        v_angle2[1] = fastfrac(angles[5],roundVal);
        v_angle2[2] = fastfrac(angles[6],roundVal);
        v_angle2[3] = fastfrac(angles[7],roundVal);
        clean_up_fastfrac();
        restore_fpucw();

        x = _mm_load_ps( v_angle );

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
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data);
        d2 = _mm_load_ps(data+4);

        // calculate scaling factor to correct the magnitude
        //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
        //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
        cd1 = _mm_mul_ps(x, y);
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        m = vec_recip1(_mm_add_ps(cd2, cd3 )); //scaling factor

        // perform second angle doubling
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        c = _mm_mul_ps(c, m);
        s = _mm_mul_ps(s, m);

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

        // Retrieve second set of reduced angles
        x = _mm_load_ps( v_angle2 );

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
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data+8);
        d2 = _mm_load_ps(data+12);

        // calculate scaling factor to correct the magnitude
        //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
        //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
        cd1 = _mm_mul_ps(x, y);
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        m = vec_recip1(_mm_add_ps(cd2, cd3 )); //scaling factor

        // perform second angle doubling
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        c = _mm_mul_ps(c, m);
        s = _mm_mul_ps(s, m);

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
        _mm_stream_ps(chirped+8, cd1);
        _mm_stream_ps(chirped+12, cd2);
    }
    _mm_sfence();

    if ( i < ul_NumDataPoints) {
        // use original routine to finish up any tailings (max stride-1 elements)
        v_ChirpData(fp_DataArray+i, fp_ChirpDataArray+i
                    , ChirpRateInd, ChirpRate, ul_NumDataPoints-i, sample_rate);
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

    return 0;
}

/***************************************************
 *
 * Alternate chirp function using Faster SinCos and Estrin method
 * for polynomial calculations. Derived from version 8 Alex Kan code.
 */
int sse1_ChirpData_ak8e(
    sah_complex * fp_DataArray,
    sah_complex * fp_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
) {

    if (ChirpRateInd == 0) {
        memcpy(fp_ChirpDataArray, fp_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
        return 0;
    }

    double srate    = ChirpRate * 0.5 / (sample_rate * sample_rate);
#if defined(_LP64) || defined(_WIN64) || (defined(__GNUC__) && defined(__SSE_MATH__))
    // For scalar XMM code or any FPU with 64 bit registers
    double roundVal = (srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52; 
#else
    // For X87 FPU
    double roundVal = (srate >= 0.0) ? ROUNDX87 : -ROUNDX87;
#endif
    int i, vEnd;
    unsigned short fpucw1, fpucw2;

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);

    for (i = 0; i < vEnd; i += 8) {
        const float *data = (const float *) (fp_DataArray + i);
        float *chirped = (float *) (fp_ChirpDataArray + i);
        double angles[8], dtemp;

        ALIGNED(float, 16) v_angle[4], v_angle2[4];

        __m128 d1, d2;
        __m128 cd1, cd2, cd3;
        __m128 td1, td2;
        __m128 v;
        __m128 w;
        __m128 x;
        __m128 y;
        __m128 z;
        __m128 s;
        __m128 c;
        __m128 m;

        prefetchnta((const void *)( data+32 ));

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

        // reduce the angle to the range (-0.5, 0.5)
        // convert doubles into singles
        // This does "angles - round(angles)"
        set_extended_precision();
        set_up_fastfrac(roundVal);
        v_angle[0] = fastfrac(angles[0],roundVal);
        v_angle[1] = fastfrac(angles[1],roundVal);
        v_angle[2] = fastfrac(angles[2],roundVal);
        v_angle[3] = fastfrac(angles[3],roundVal);
        v_angle2[0] = fastfrac(angles[4],roundVal);
        v_angle2[1] = fastfrac(angles[5],roundVal);
        v_angle2[2] = fastfrac(angles[6],roundVal);
        v_angle2[3] = fastfrac(angles[7],roundVal);
        clean_up_fastfrac();
        restore_fpucw();

        x = _mm_load_ps( v_angle );

        // square to the range [0, 0.25)
        y = _mm_mul_ps(x, x);

        // perform the initial polynomial approximations with interleaved Estrin sequences
        z = _mm_mul_ps(y, y);
        s = _mm_mul_ps(y, SS4F);
        c = _mm_mul_ps(y, CC3F);
        s = _mm_add_ps(s, SS3F);
        c = _mm_add_ps(c, CC2F);
        s = _mm_mul_ps(s, z);
        c = _mm_mul_ps(c, z);
        v = _mm_mul_ps(y, SS2F);
        w = _mm_mul_ps(y, CC1F);
        v = _mm_add_ps(v, SS1F);
        w = _mm_add_ps(w, ONE);
        s = _mm_add_ps(s, v);
        c = _mm_add_ps(c, w);
        s = _mm_mul_ps(s, x);

        // perform first angle doubling
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data);
        d2 = _mm_load_ps(data+4);

        // calculate scaling factor to correct the magnitude
        // and perform second angle doubling
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        c = _mm_sub_ps(TWO, cd2);
        cd1 = _mm_mul_ps(x, y);
        m = _mm_sub_ps(c, cd3);
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        s = _mm_mul_ps(s, m);
        c = _mm_mul_ps(c, m);

        x = d1;
        y = d2;
        x = _mm_shuffle_ps(x, x, 0xB1);
        y = _mm_shuffle_ps(y, y, 0xB1);
        x = _mm_mul_ps(x, R_NEG);
        y = _mm_mul_ps(y, R_NEG);
        cd1 = _mm_shuffle_ps(c, c, 0x50);
        cd2 = _mm_shuffle_ps(c, c, 0xfa);
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

        // Retrieve second set of reduced angles
        x = _mm_load_ps( v_angle2 );

        // square to the range [0, 0.25)
        y = _mm_mul_ps(x, x);

        // perform the initial polynomial approximations with interleaved Estrin sequences
        z = _mm_mul_ps(y, y);
        s = _mm_mul_ps(y, SS4F);
        c = _mm_mul_ps(y, CC3F);
        s = _mm_add_ps(s, SS3F);
        c = _mm_add_ps(c, CC2F);
        s = _mm_mul_ps(s, z);
        c = _mm_mul_ps(c, z);
        v = _mm_mul_ps(y, SS2F);
        w = _mm_mul_ps(y, CC1F);
        v = _mm_add_ps(v, SS1F);
        w = _mm_add_ps(w, ONE);
        s = _mm_add_ps(s, v);
        c = _mm_add_ps(c, w);
        s = _mm_mul_ps(s, x);

        // perform first angle doubling
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data+8);
        d2 = _mm_load_ps(data+12);

        // calculate scaling factor to correct the magnitude
        // and perform second angle doubling
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        c = _mm_sub_ps(TWO, cd2);
        cd1 = _mm_mul_ps(x, y);
        m = _mm_sub_ps(c, cd3);
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        c = _mm_mul_ps(c, m);
        s = _mm_mul_ps(s, m);

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
        _mm_stream_ps(chirped+8, cd1);
        _mm_stream_ps(chirped+12, cd2);
    }
    _mm_sfence();

    if ( i < ul_NumDataPoints) {
        // use original routine to finish up any tailings (max stride-1 elements)
        v_ChirpData(fp_DataArray+i, fp_ChirpDataArray+i
                    , ChirpRateInd, ChirpRate, ul_NumDataPoints-i, sample_rate);
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

    return 0;
}

/***************************************************
 *
 * Alternate chirp function using Faster SinCos and Horner method
 * for polynomial calculations. Derived from version 8 Alex Kan code.
 */
int sse1_ChirpData_ak8h(
    sah_complex * fp_DataArray,
    sah_complex * fp_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
) {

    if (ChirpRateInd == 0) {
        memcpy(fp_ChirpDataArray, fp_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
        return 0;
    }

    double srate    = ChirpRate * 0.5 / (sample_rate * sample_rate);
#if defined(_LP64) || defined(_WIN64) || (defined(__GNUC__) && defined(__SSE_MATH__))
    // For scalar XMM code or any FPU with 64 bit registers
    double roundVal = (srate >= 0.0) ? TWO_TO_52 : -TWO_TO_52; 
#else
    // For X87 FPU
    double roundVal = (srate >= 0.0) ? ROUNDX87 : -ROUNDX87;
#endif
    int i, vEnd;
    unsigned short fpucw1, fpucw2;

    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 7);

    for (i = 0; i < vEnd; i += 8) {
        const float *data = (const float *) (fp_DataArray + i);
        float *chirped = (float *) (fp_ChirpDataArray + i);
        double angles[8], dtemp;

        ALIGNED(float, 16) v_angle[4], v_angle2[4];

        __m128 d1, d2;
        __m128 cd1, cd2, cd3;
        __m128 td1, td2;
        __m128 x;
        __m128 y;
        __m128 s;
        __m128 c;
        __m128 m;

        prefetchnta((const void *)( data+32 ));

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

        // reduce the angle to the range (-0.5, 0.5)
        // convert doubles into singles
        // This does "angles - round(angles)"
        set_extended_precision();
        set_up_fastfrac(roundVal);
        v_angle[0] = fastfrac(angles[0],roundVal);
        v_angle[1] = fastfrac(angles[1],roundVal);
        v_angle[2] = fastfrac(angles[2],roundVal);
        v_angle[3] = fastfrac(angles[3],roundVal);
        v_angle2[0] = fastfrac(angles[4],roundVal);
        v_angle2[1] = fastfrac(angles[5],roundVal);
        v_angle2[2] = fastfrac(angles[6],roundVal);
        v_angle2[3] = fastfrac(angles[7],roundVal);
        clean_up_fastfrac();
        restore_fpucw();

        x = _mm_load_ps( v_angle );

        // square to the range [0, 0.25)
        y = _mm_mul_ps(x, x);

        // perform the initial polynomial approximations using interleaved Horner sequences
        s = _mm_mul_ps(y, SS4F);
        c = _mm_mul_ps(y, CC3F);
        s = _mm_add_ps(s, SS3F);
        c = _mm_add_ps(c, CC2F);
        s = _mm_mul_ps(s, y);
        c = _mm_mul_ps(c, y);
        s = _mm_add_ps(s, SS2F);
        c = _mm_add_ps(c, CC1F);
        s = _mm_mul_ps(s, y);
        c = _mm_mul_ps(c, y);
        s = _mm_add_ps(s, SS1F);
        c = _mm_add_ps(c, ONE);
        s = _mm_mul_ps(s, x);

        // perform first angle doubling
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data);
        d2 = _mm_load_ps(data+4);

        // calculate scaling factor to correct the magnitude
        // and perform second angle doubling
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        c = _mm_sub_ps(TWO, cd2);
        cd1 = _mm_mul_ps(x, y);
        m = _mm_sub_ps(c, cd3);
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        s = _mm_mul_ps(s, m);
        c = _mm_mul_ps(c, m);

        x = d1;
        y = d2;
        x = _mm_shuffle_ps(x, x, 0xB1);
        y = _mm_shuffle_ps(y, y, 0xB1);
        x = _mm_mul_ps(x, R_NEG);
        y = _mm_mul_ps(y, R_NEG);
        cd1 = _mm_shuffle_ps(c, c, 0x50);
        cd2 = _mm_shuffle_ps(c, c, 0xfa);
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

        // Retrieve second set of reduced angles
        x = _mm_load_ps( v_angle2 );

        // square to the range [0, 0.25)
        y = _mm_mul_ps(x, x);

        // perform the initial polynomial approximations using interleaved Horner sequences
        s = _mm_mul_ps(y, SS4F);
        c = _mm_mul_ps(y, CC3F);
        s = _mm_add_ps(s, SS3F);
        c = _mm_add_ps(c, CC2F);
        s = _mm_mul_ps(s, y);
        c = _mm_mul_ps(c, y);
        s = _mm_add_ps(s, SS2F);
        c = _mm_add_ps(c, CC1F);
        s = _mm_mul_ps(s, y);
        c = _mm_mul_ps(c, y);
        s = _mm_add_ps(s, SS1F);
        c = _mm_add_ps(c, ONE);
        s = _mm_mul_ps(s, x);

        // perform first angle doubling
        cd1 = _mm_mul_ps(s, c);
        cd2 = _mm_mul_ps(c, c);
        cd3 = _mm_mul_ps(s, s);
        y = _mm_mul_ps(cd1, TWO);
        x = _mm_sub_ps(cd2, cd3);

        // load the signal to be chirped
        d1 = _mm_load_ps(data+8);
        d2 = _mm_load_ps(data+12);

        // calculate scaling factor to correct the magnitude
        // and perform second angle doubling
        cd2 = _mm_mul_ps(x, x);
        cd3 = _mm_mul_ps(y, y);
        c = _mm_sub_ps(TWO, cd2);
        cd1 = _mm_mul_ps(x, y);
        m = _mm_sub_ps(c, cd3);
        s = _mm_mul_ps(cd1, TWO);
        c = _mm_sub_ps(cd2, cd3);

        // correct the magnitude (final sine / cosine approximations)
        c = _mm_mul_ps(c, m);
        s = _mm_mul_ps(s, m);

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
        _mm_stream_ps(chirped+8, cd1);
        _mm_stream_ps(chirped+12, cd2);
    }
    _mm_sfence();

    if ( i < ul_NumDataPoints) {
        // use original routine to finish up any tailings (max stride-1 elements)
        v_ChirpData(fp_DataArray+i, fp_ChirpDataArray+i
                    , ChirpRateInd, ChirpRate, ul_NumDataPoints-i, sample_rate);
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

    return 0;
}



/*********************************************************************
 *
 * SSE Folding subroutine sets
 *
 */
// These functions reduce an array length by a factor of 3, 4, 5,
// or 2 by summing and return the maximum of the summed values.
// Since each adjacent float is added to the next, simd works well.
// To keep multiple FPU or SSE units in any processor busy we try to
// avoid dependancy chains  ( x += a, x += b, x += c) where each
// computation must wait til previous sum is computed.
// having two maximums running from two colums of numbers
// avoids a single max register from being the bottleneck

int sse_mask1[] = {4, 1, 2, 3, 0, 0, 0, 0 };
int sse_mask2[] = {4, 4, 4, 4, 4, 1, 2, 3 };

// ALIGN_SIMD unsigned int sse_partMask[][4] = {
ALIGNED(unsigned int, 16) sse_partMask[][4] = {
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF }, // 0
            { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 }, // 1
            { 0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000 }, // 2
            { 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000 }, // 3
            { 0x00000000,0x00000000,0x00000000,0x00000000 }, // 4
        };

#define SSE_MASK(idx) (*(__m128 *)&sse_partMask[idx])


// maxp2f -
//  function: Given a 4 packed float input, return the max among them
//  concept:  Ben Herndon
//
inline float s_maxp2f( __m128 max1 ) {
    float   tMax;
    __m128  maxReg = max1;

    maxReg = _mm_movehl_ps( maxReg, max1 );
    maxReg = _mm_max_ps( maxReg, max1 );          // [0][1] vs [3][4]
    max1   = _mm_shuffle_ps( maxReg, maxReg, 1 );
    maxReg = _mm_max_ss( maxReg, max1 );          // [1] vs [0]

    _mm_store_ss( &tMax, maxReg );
    return ( tMax );
}

#define s_getU( aaaa, ptr )  \
        aaaa = _mm_loadh_pi( _mm_loadl_pi(aaaa, (__m64 *)ptr), ((__m64 *)(ptr))+1 )

#define s_putU( ptr, aaaa ) \
        _mm_storel_pi((__m64 *)ptr, aaaa), _mm_storeh_pi( ((__m64 *)ptr)+1 , aaaa)


/******************
 *
 * ben SSE folding
 *
 * Description: Intel & AMD SSE optimized folding
 *  CPUs: Intel Pentium III and beyond,  AMD Athlon model 6 -  and beyond
 *
 * Original code by Ben Herndon, modified by Joe Segur
 *
 */

//
// fold by 2
//
float sse_sum2(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i, length =  P->di;
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;
    float *sums = P->dest;

    max1 = max2 = ZERO;

    const int   stride = 8;

    for (i = 0; i < length-(stride - 1); i += stride ) {
        //  SSE Pipeline #1                               SSE Pipeline #2
        //
        s_getU(sum1, &ptr1[i + 0] );
        s_getU(sum2, &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        s_putU( &sums[i + 0], sum1 );
        s_putU( &sums[i +4], sum2 );
    }

    int mask1 = sse_mask1[ length-i];
    int mask2 = sse_mask2[ length-i];
    s_getU(sum1, &ptr1[i + 0] );
    s_getU(sum2, &ptr1[i + 4] );
    s_getU(tmp1, &ptr2[i + 0] );
    s_getU(tmp2, &ptr2[i + 4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    sum1 = _mm_and_ps(sum1, SSE_MASK( mask1 ) );
    sum2 = _mm_and_ps(sum2, SSE_MASK( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );
    s_putU( &sums[i + 0], sum1 );
    s_putU( &sums[i +4], sum2 );

    return s_maxp2f( _mm_max_ps( max1, max2 ) );
}

//
// fold by 3s
//
float sse_sum3(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i, length =  P->di;
    float *ptr1 = ss[0];
    float *ptr2 = ss[0]+P->tmp0;
    float *ptr3 = ss[0]+P->tmp1;
    float *sums = P->dest;

    max1 = max2 = ZERO;

    const int   stride = 8;
    for (i = 0; i < length-(stride - 1); i += stride ) {
        //  SSE Pipeline #1                      SSE Pipeline #2
        //
        s_getU(sum1, &ptr1[i + 0] );
        s_getU(sum2, &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        max1 = _mm_max_ps( max1,    sum1 );
        max2 = _mm_max_ps( max2,    sum2 );
        s_putU( &sums[i + 0], sum1 );
        s_putU( &sums[i +4], sum2 );
    }

    int mask1 = sse_mask1[ length-i];
    int mask2 = sse_mask2[ length-i];
    s_getU(sum1, &ptr1[i + 0] );
    s_getU(sum2, &ptr1[i + 4] );
    s_getU(tmp1, &ptr2[i + 0] );
    s_getU(tmp2, &ptr2[i + 4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[i + 0] );
    s_getU(tmp2, &ptr3[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    sum1 = _mm_and_ps(sum1, SSE_MASK( mask1 ) );
    sum2 = _mm_and_ps(sum2, SSE_MASK( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );
    s_putU( &sums[i + 0], sum1 );
    s_putU( &sums[i +4], sum2 );

    return s_maxp2f( _mm_max_ps( max1, max2 ) );
}

//
// fold by 4
//
float sse_sum4(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i, length =  P->di;
    float *ptr1 = ss[0];
    float *ptr2 = ss[0]+P->tmp0;
    float *ptr3 = ss[0]+P->tmp1;
    float *ptr4 = ss[0]+P->tmp2;
    float *sums = P->dest;

    max1 = max2 = ZERO;

    const int   stride = 8;
    for (i = 0; i < length-(stride - 1); i += stride ) {
        //  SSE Pipeline #1                      SSE Pipeline #2
        //
        s_getU(sum1, &ptr1[i + 0] );
        s_getU(sum2, &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        max1 = _mm_max_ps( max1,    sum1 );
        max2 = _mm_max_ps( max2,    sum2 );
        s_putU( &sums[i + 0], sum1 );
        s_putU( &sums[i +4], sum2 );
    }

    int mask1 = sse_mask1[ length-i];
    int mask2 = sse_mask2[ length-i];
    s_getU(sum1, &ptr1[i + 0] );
    s_getU(sum2, &ptr1[i + 4] );
    s_getU(tmp1, &ptr2[i + 0] );
    s_getU(tmp2, &ptr2[i + 4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[i + 0] );
    s_getU(tmp2, &ptr3[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    s_getU(tmp1, &ptr4[i + 0] );
    s_getU(tmp2, &ptr4[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    sum1 = _mm_and_ps(sum1, SSE_MASK( mask1 ) );
    sum2 = _mm_and_ps(sum2, SSE_MASK( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );
    s_putU( &sums[i + 0], sum1 );
    s_putU( &sums[i +4], sum2 );

    return s_maxp2f( _mm_max_ps( max1, max2 ) );
}

//
// fold by 5
//
float sse_sum5(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i, length =  P->di;
    float *ptr1 = ss[0];
    float *ptr2 = ss[0]+P->tmp0;
    float *ptr3 = ss[0]+P->tmp1;
    float *ptr4 = ss[0]+P->tmp2;
    float *ptr5 = ss[0]+P->tmp3;
    float *sums = P->dest;

    max1 = max2 = ZERO;

    const int   stride = 8;
    for (i = 0; i < length-(stride - 1); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        s_getU(sum1, &ptr1[i + 0] );
        s_getU(sum2, &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        s_getU(tmp1, &ptr5[i + 0] );
        s_getU(tmp2, &ptr5[i + 4] );
        sum1 = _mm_add_ps( sum1,    tmp1 );
        sum2 = _mm_add_ps( sum2,    tmp2 );
        max1 = _mm_max_ps( max1,    sum1 );
        max2 = _mm_max_ps( max2,    sum2 );
        s_putU( &sums[i + 0], sum1 );
        s_putU( &sums[i +4], sum2 );
    }

    int mask1 = sse_mask1[ length-i];
    int mask2 = sse_mask2[ length-i];
    s_getU(sum1, &ptr1[i + 0] );
    s_getU(sum2, &ptr1[i + 4] );
    s_getU(tmp1, &ptr2[i + 0] );
    s_getU(tmp2, &ptr2[i + 4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[i + 0] );
    s_getU(tmp2, &ptr3[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    s_getU(tmp1, &ptr4[i + 0] );
    s_getU(tmp2, &ptr4[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    s_getU(tmp1, &ptr5[i + 0] );
    s_getU(tmp2, &ptr5[i + 4] );
    sum1 = _mm_add_ps( sum1,    tmp1 );
    sum2 = _mm_add_ps( sum2,    tmp2 );
    sum1 = _mm_and_ps(sum1, SSE_MASK( mask1 ) );
    sum2 = _mm_and_ps(sum2, SSE_MASK( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );
    s_putU( &sums[i + 0], sum1 );
    s_putU( &sums[i +4], sum2 );

    return s_maxp2f( _mm_max_ps( max1, max2 ) );
}

sum_func sse_list3[FOLDTBLEN] = {
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3,
                                    sse_sum3, sse_sum3, sse_sum3, sse_sum3
                                };
sum_func sse_list4[FOLDTBLEN] = {
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4,
                                    sse_sum4, sse_sum4, sse_sum4, sse_sum4
                                };
sum_func sse_list5[FOLDTBLEN] = {
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5,
                                    sse_sum5, sse_sum5, sse_sum5, sse_sum5
                                };
sum_func sse_list2[FOLDTBLEN] = {
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                    sse_sum2, sse_sum2, sse_sum2, sse_sum2
                                };
sum_func sse_list2_L[FOLDTBLEN] = {
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2,
                                      sse_sum2, sse_sum2, sse_sum2, sse_sum2
                                  };

FoldSet sse_ben_fold = {sse_list3, sse_list4, sse_list5, sse_list2, sse_list2_L, "ben SSE"};



/******************
 *
 * BH SSE folding
 *
 * Description: Intel & AMD SSE optimized folding
 *  CPUs: Intel Pentium III and beyond,  AMD Athlon model 6 -  and beyond
 *
 * Original code by Ben Herndon, modified by Joe Segur
 *
 */

int sse_msks1[] = {4, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int sse_msks2[] = {4, 4, 4, 4, 4, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//ALIGN_SIMD float sse_foldLim[][4] = {
ALIGNED(float, 16) sse_foldLim[][4] = {
                                          {  0.0f,  0.0f,  0.0f,  0.0f }, // 0
                                          {  0.0f, 1e37f, 1e37f, 1e37f }, // 1
                                          {  0.0f,  0.0f, 1e37f, 1e37f }, // 2
                                          {  0.0f,  0.0f,  0.0f, 1e37f }, // 3
                                          { 1e37f, 1e37f, 1e37f, 1e37f }, // 4
                                      };

#define SSE_LIM(idx) (*(__m128 *)&sse_foldLim[idx])


//
// Fold by 3 versions
//
float sse_pulPoTf3u(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;

    const int   stride = 24;
    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 8] );
        sum2 = _mm_load_ps( &ptr1[i + 12] );
        s_getU(tmp1, &ptr2[i + 8] );
        s_getU(tmp2, &ptr2[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 8] );
        s_getU(tmp2, &ptr3[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 8], sum1 );
        _mm_store_ps( &P->dest[i + 12], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 16] );
        sum2 = _mm_load_ps( &ptr1[i + 20] );
        s_getU(tmp1, &ptr2[i + 16] );
        s_getU(tmp2, &ptr2[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 16] );
        s_getU(tmp2, &ptr3[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 16], sum1 );
        _mm_store_ps( &P->dest[i + 20], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf3(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;

    const int   stride = 8;
    for (i = 0; i < P->di - (stride - 1); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf3L8(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;

    // SSE  Pipeline #1                           SSE Pipeline #2
    //
    int mask1 = sse_msks1[ P->di ];
    int mask2 = sse_msks2[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    sum2 = _mm_load_ps( &ptr1[4] );
    s_getU(tmp1, &ptr2[0] );
    s_getU(tmp2, &ptr2[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[0] );
    s_getU(tmp2, &ptr3[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    _mm_store_ps( &P->dest[0], sum1 );
    _mm_store_ps( &P->dest[4], sum2 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    sum2 = _mm_sub_ps(sum2, SSE_LIM( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );

    return ( s_maxp2f( _mm_max_ps( max1, max2 ) ) );
}

sum_func BHSSETB3[FOLDTBLEN] =
    {
        sse_pulPoTf3L8, sse_pulPoTf3L8, sse_pulPoTf3L8, sse_pulPoTf3L8,
        sse_pulPoTf3L8, sse_pulPoTf3L8, sse_pulPoTf3L8, sse_pulPoTf3L8,
        sse_pulPoTf3L8, sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,
        sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,
        sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,
        sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,   sse_pulPoTf3,
        sse_pulPoTf3u
    };

//
// Fold by 4 versions
//
float sse_pulPoTf4u(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;

    const int   stride = 24;
    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 8] );
        sum2 = _mm_load_ps( &ptr1[i + 12] );
        s_getU(tmp1, &ptr2[i + 8] );
        s_getU(tmp2, &ptr2[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 8] );
        s_getU(tmp2, &ptr3[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 8] );
        s_getU(tmp2, &ptr4[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 8], sum1 );
        _mm_store_ps( &P->dest[i + 12], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 16] );
        sum2 = _mm_load_ps( &ptr1[i + 20] );
        s_getU(tmp1, &ptr2[i + 16] );
        s_getU(tmp2, &ptr2[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 16] );
        s_getU(tmp2, &ptr3[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 16] );
        s_getU(tmp2, &ptr4[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 16], sum1 );
        _mm_store_ps( &P->dest[i + 20], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr4[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf4(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;

    const int   stride = 8;
    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr4[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}


float sse_pulPoTf4L8(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;

    // SSE  Pipeline #1                           SSE Pipeline #2
    //
    int mask1 = sse_msks1[ P->di ];
    int mask2 = sse_msks2[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    sum2 = _mm_load_ps( &ptr1[4] );
    s_getU(tmp1, &ptr2[0] );
    s_getU(tmp2, &ptr2[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[0] );
    s_getU(tmp2, &ptr3[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr4[0] );
    s_getU(tmp2, &ptr4[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    _mm_store_ps( &P->dest[0], sum1 );
    _mm_store_ps( &P->dest[4], sum2 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    sum2 = _mm_sub_ps(sum2, SSE_LIM( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );

    return ( s_maxp2f( _mm_max_ps( max1, max2 ) ) );
}

sum_func BHSSETB4[FOLDTBLEN] = {
                                   sse_pulPoTf4L8, sse_pulPoTf4L8, sse_pulPoTf4L8, sse_pulPoTf4L8,
                                   sse_pulPoTf4L8, sse_pulPoTf4L8, sse_pulPoTf4L8, sse_pulPoTf4L8,
                                   sse_pulPoTf4L8, sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,
                                   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,
                                   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,
                                   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,   sse_pulPoTf4,
                                   sse_pulPoTf4u
                               };

//
// Fold by 5 versions
//
float sse_pulPoTf5u(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;
    float *ptr5 = ss[0] + P->tmp3;

    const int   stride = 24;
    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr5[i + 0] );
        s_getU(tmp2, &ptr5[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 8] );
        sum2 = _mm_load_ps( &ptr1[i + 12] );
        s_getU(tmp1, &ptr2[i + 8] );
        s_getU(tmp2, &ptr2[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 8] );
        s_getU(tmp2, &ptr3[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 8] );
        s_getU(tmp2, &ptr4[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr5[i + 8] );
        s_getU(tmp2, &ptr5[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 8], sum1 );
        _mm_store_ps( &P->dest[i + 12], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 16] );
        sum2 = _mm_load_ps( &ptr1[i + 20] );
        s_getU(tmp1, &ptr2[i + 16] );
        s_getU(tmp2, &ptr2[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 16] );
        s_getU(tmp2, &ptr3[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 16] );
        s_getU(tmp2, &ptr4[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr5[i + 16] );
        s_getU(tmp2, &ptr5[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 16], sum1 );
        _mm_store_ps( &P->dest[i + 20], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr4[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr5[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf5(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;
    float *ptr5 = ss[0] + P->tmp3;

    const int   stride = 8;
    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                           SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr3[i + 0] );
        s_getU(tmp2, &ptr3[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr4[i + 0] );
        s_getU(tmp2, &ptr4[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        s_getU(tmp1, &ptr5[i + 0] );
        s_getU(tmp2, &ptr5[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i + 4], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr3[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr4[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        s_getU(tmp1, &ptr5[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf5L8(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;
    float *ptr5 = ss[0] + P->tmp3;

    //  SSE Pipeline #1                           SSE Pipeline #2
    //
    int mask1 = sse_msks1[ P->di ];
    int mask2 = sse_msks2[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    sum2 = _mm_load_ps( &ptr1[4] );
    s_getU(tmp1, &ptr2[0] );
    s_getU(tmp2, &ptr2[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr3[0] );
    s_getU(tmp2, &ptr3[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr4[0] );
    s_getU(tmp2, &ptr4[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    s_getU(tmp1, &ptr5[0] );
    s_getU(tmp2, &ptr5[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    _mm_store_ps( &P->dest[0], sum1 );
    _mm_store_ps( &P->dest[4], sum2 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    sum2 = _mm_sub_ps(sum2, SSE_LIM( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );

    return ( s_maxp2f( _mm_max_ps( max1, max2 ) ) );
}

float sse_pulPoTf5L4(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, max1, max2;
    __m128     tmp1, tmp2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[0];
    float *ptr2 = ss[0] + P->tmp0;
    float *ptr3 = ss[0] + P->tmp1;
    float *ptr4 = ss[0] + P->tmp2;
    float *ptr5 = ss[0] + P->tmp3;

    int mask1 = sse_msks1[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    s_getU(tmp1, &ptr2[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    s_getU(tmp1, &ptr3[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    s_getU(tmp1, &ptr4[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    s_getU(tmp1, &ptr5[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    _mm_store_ps( &P->dest[0], sum1 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    max1 = _mm_max_ps( max1, sum1 );

    return ( s_maxp2f( max1 ) );
}

sum_func BHSSETB5[FOLDTBLEN] = {
                                   sse_pulPoTf5L4, sse_pulPoTf5L4, sse_pulPoTf5L4, sse_pulPoTf5L4,
                                   sse_pulPoTf5L4, sse_pulPoTf5L8, sse_pulPoTf5L8, sse_pulPoTf5L8,
                                   sse_pulPoTf5L8, sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,
                                   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,
                                   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,
                                   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,   sse_pulPoTf5,
                                   sse_pulPoTf5u
                               };

//
// Fold by 2 versions
//
float sse_pulPoTf2u(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    const int   stride = 24;

    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                               SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i +4], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 8] );
        sum2 = _mm_load_ps( &ptr1[i + 12] );
        s_getU(tmp1, &ptr2[i + 8] );
        s_getU(tmp2, &ptr2[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 8], sum1 );
        _mm_store_ps( &P->dest[i +12], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 16] );
        sum2 = _mm_load_ps( &ptr1[i + 20] );
        s_getU(tmp1, &ptr2[i + 16] );
        s_getU(tmp2, &ptr2[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 16], sum1 );
        _mm_store_ps( &P->dest[i +20], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf2(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    const int   stride = 8;

    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                               SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        s_getU(tmp1, &ptr2[i + 0] );
        s_getU(tmp2, &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i +4], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        s_getU(tmp1, &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf2L8(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    //  SSE Pipeline #1                               SSE Pipeline #2
    //
    int mask1 = sse_msks1[ P->di ];
    int mask2 = sse_msks2[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    sum2 = _mm_load_ps( &ptr1[4] );
    s_getU(tmp1, &ptr2[0] );
    s_getU(tmp2, &ptr2[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    _mm_store_ps( &P->dest[0], sum1 );
    _mm_store_ps( &P->dest[4], sum2 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    sum2 = _mm_sub_ps(sum2, SSE_LIM( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );

    return ( s_maxp2f( _mm_max_ps( max1, max2 ) ) );
}

float sse_pulPoTf2L4(float *ss[], struct PoTPlan *P) {
    __m128     sum1, tmp1, max1;
    int i;

    max1 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    //
    int mask1 = sse_msks1[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    s_getU(tmp1, &ptr2[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    _mm_store_ps( &P->dest[0], sum1 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    max1 = _mm_max_ps( max1, sum1 );

    return ( s_maxp2f( max1 ) );
}

sum_func BHSSETB2[FOLDTBLEN] = {
                                   sse_pulPoTf2L4, sse_pulPoTf2L4, sse_pulPoTf2L4, sse_pulPoTf2L4,
                                   sse_pulPoTf2L4, sse_pulPoTf2L8, sse_pulPoTf2L8, sse_pulPoTf2L8,
                                   sse_pulPoTf2L8, sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,
                                   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,
                                   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,
                                   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,   sse_pulPoTf2,
                                   sse_pulPoTf2u,  sse_pulPoTf2u,  sse_pulPoTf2u,  sse_pulPoTf2u,
                                   sse_pulPoTf2u,  sse_pulPoTf2u,  sse_pulPoTf2u,  sse_pulPoTf2u
                               };


// versions for tmp0 aligned
//
float sse_pulPoTf2ALu(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    const int   stride = 24;

    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                               SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        tmp1 = _mm_load_ps( &ptr2[i + 0] );
        tmp2 = _mm_load_ps( &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i +4], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 8] );
        sum2 = _mm_load_ps( &ptr1[i + 12] );
        tmp1 = _mm_load_ps( &ptr2[i + 8] );
        tmp2 = _mm_load_ps( &ptr2[i + 12] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 8], sum1 );
        _mm_store_ps( &P->dest[i +12], sum2 );

        sum1 = _mm_load_ps( &ptr1[i + 16] );
        sum2 = _mm_load_ps( &ptr1[i + 20] );
        tmp1 = _mm_load_ps( &ptr2[i + 16] );
        tmp2 = _mm_load_ps( &ptr2[i + 20] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 16], sum1 );
        _mm_store_ps( &P->dest[i +20], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        tmp1 = _mm_load_ps( &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}

float sse_pulPoTf2AL(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    const int   stride = 8;

    for (i = 0; i < P->di - ( stride - 1 ); i += stride ) {
        //  SSE Pipeline #1                               SSE Pipeline #2
        //
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        sum2 = _mm_load_ps( &ptr1[i + 4] );
        tmp1 = _mm_load_ps( &ptr2[i + 0] );
        tmp2 = _mm_load_ps( &ptr2[i + 4] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        sum2 = _mm_add_ps( sum2, tmp2 );
        max1 = _mm_max_ps( max1, sum1 );
        max2 = _mm_max_ps( max2, sum2 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        _mm_store_ps( &P->dest[i +4], sum2 );
    }
    max1 = _mm_max_ps( max1, max2 );
    for (   ; i < P->di ; i += 4) {
        int mask1 = sse_msks1[ P->di-i];
        sum1 = _mm_load_ps( &ptr1[i + 0] );
        tmp1 = _mm_load_ps( &ptr2[i + 0] );
        sum1 = _mm_add_ps( sum1, tmp1 );
        _mm_store_ps( &P->dest[i + 0], sum1 );
        sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
        max1 = _mm_max_ps( max1, sum1 );
    }
    return ( s_maxp2f(  max1 ) );
}


float sse_pulPoTf2AL8(float *ss[], struct PoTPlan *P) {
    __m128     sum1, sum2, tmp1, tmp2, max1, max2;
    int i;

    max1 = max2 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    //  SSE Pipeline #1                               SSE Pipeline #2
    //
    int mask1 = sse_msks1[ P->di ];
    int mask2 = sse_msks2[ P->di ];
    sum1 = _mm_load_ps( &ptr1[0] );
    sum2 = _mm_load_ps( &ptr1[4] );
    tmp1 = _mm_load_ps( &ptr2[0] );
    tmp2 = _mm_load_ps( &ptr2[4] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    sum2 = _mm_add_ps( sum2, tmp2 );
    _mm_store_ps( &P->dest[0], sum1 );
    _mm_store_ps( &P->dest[4], sum2 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    sum2 = _mm_sub_ps(sum2, SSE_LIM( mask2 ) );
    max1 = _mm_max_ps( max1, sum1 );
    max2 = _mm_max_ps( max2, sum2 );

    return ( s_maxp2f( _mm_max_ps( max1, max2 ) ) );
}

float sse_pulPoTf2AL4(float *ss[], struct PoTPlan *P) {
    __m128     sum1, tmp1, max1;
    int i;

    max1 = _mm_set_ps1( 0.0 );
    float *ptr1 = ss[1]+P->offset;
    float *ptr2 = ss[1]+P->tmp0;

    //
    int mask1 = sse_msks1[ P->di];
    sum1 = _mm_load_ps( &ptr1[0] );
    tmp1 = _mm_load_ps( &ptr2[0] );
    sum1 = _mm_add_ps( sum1, tmp1 );
    _mm_store_ps( &P->dest[0], sum1 );
    sum1 = _mm_sub_ps(sum1, SSE_LIM( mask1 ) );
    max1 = _mm_max_ps( max1, sum1 );

    return ( s_maxp2f( max1 ) );
}

sum_func BHSSETB2AL[FOLDTBLEN] = {
                                     sse_pulPoTf2AL4, sse_pulPoTf2AL4, sse_pulPoTf2AL4, sse_pulPoTf2AL4,
                                     sse_pulPoTf2AL4, sse_pulPoTf2AL8, sse_pulPoTf2AL8, sse_pulPoTf2AL8,
                                     sse_pulPoTf2AL8, sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,
                                     sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,
                                     sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,
                                     sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,  sse_pulPoTf2AL,
                                     sse_pulPoTf2ALu
                                 };


FoldSet BHSSEfold = {BHSSETB3, BHSSETB4, BHSSETB5, BHSSETB2, BHSSETB2AL, "BH SSE"};

/*************************************
 *
 * SSE folding subroutines from SSE2 by Alex Kan
 *
 * Modified by Joe Segur
 *
 */

typedef union submask {
    float    f[4];
    __m128   v;
};

submask tailmask[4] = {
                          { 0.0f,  0.0f,   0.0f,   0.0f},
                          { 0.0f, 1e37f,  1e37f,  1e37f},
                          { 0.0f,  0.0f,  1e37f,  1e37f},
                          { 0.0f,  0.0f,   0.0f,  1e37f}
                      };


float foldArrayBy3(float *ss[], struct PoTPlan *P) {
    float max;
    __m128 maxV = ZERO;
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1;
    float *pst = P->dest;
    __m128 x1, x2, x3, xs12, xs;

    while (i < P->di-4) {
        x1 = _mm_load_ps(p1+i);
        x2 = _mm_loadu_ps(p2+i);
        x3 = _mm_loadu_ps(p3+i);
        xs12 = _mm_add_ps(x1, x2);
        xs = _mm_add_ps(xs12, x3);
        _mm_store_ps(pst+i, xs);
        maxV = _mm_max_ps(maxV, xs);
        i += 4;
    }
    x1 = _mm_load_ps(p1+i);
    x2 = _mm_loadu_ps(p2+i);
    x3 = _mm_loadu_ps(p3+i);
    xs12 = _mm_add_ps(x1, x2);
    xs = _mm_add_ps(xs12, x3);

    _mm_store_ps(pst+i, xs);
    xs = _mm_sub_ps(xs, tailmask[P->di & 0x3].v);
    maxV = _mm_max_ps(maxV, xs);

    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x4e));
    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x39));
    _mm_store_ss(&max, maxV);

    return max;
}


float foldArrayBy3LO(float *ss[], struct PoTPlan *P) {

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1;
    float *pst = P->dest;
    float max = 0.0f;
    int i = 0;

    while (i < P->di) {
        pst[i] = p1[i] + p2[i] + p3[i];
        if (pst[i] > max) max = pst[i];
        i += 1;
    }
    return max;
}
sum_func AKSSETB3[FOLDTBLEN] = {
                                   foldArrayBy3LO, foldArrayBy3LO, foldArrayBy3LO, foldArrayBy3LO,
                                   foldArrayBy3LO, foldArrayBy3LO, foldArrayBy3LO, foldArrayBy3LO,
                                   foldArrayBy3,   foldArrayBy3LO, foldArrayBy3LO, foldArrayBy3LO,
                                   foldArrayBy3
                               };

float foldArrayBy4(float *ss[], struct PoTPlan *P) {
    float max;
    __m128 maxV = ZERO;
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2;
    float *pst = P->dest;
    __m128 x1, x2, x3, x4, xs12, xs34, xs;

    while (i < P->di-4) {
        x1 = _mm_load_ps(p1+i);
        x2 = _mm_loadu_ps(p2+i);
        x3 = _mm_loadu_ps(p3+i);
        x4 = _mm_loadu_ps(p4+i);
        xs12 = _mm_add_ps(x1, x2);
        xs34 = _mm_add_ps(x3, x4);
        xs = _mm_add_ps(xs12, xs34);
        _mm_store_ps(pst+i, xs);
        maxV = _mm_max_ps(maxV, xs);
        i += 4;
    }
    x1 = _mm_load_ps(p1+i);
    x2 = _mm_loadu_ps(p2+i);
    x3 = _mm_loadu_ps(p3+i);
    x4 = _mm_loadu_ps(p4+i);
    xs12 = _mm_add_ps(x1, x2);
    xs34 = _mm_add_ps(x3, x4);
    xs = _mm_add_ps(xs12, xs34);

    _mm_store_ps(pst+i, xs);
    xs = _mm_sub_ps(xs, tailmask[P->di & 0x3].v);
    maxV = _mm_max_ps(maxV, xs);

    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x4e));
    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x39));
    _mm_store_ss(&max, maxV);

    return max;
}

float foldArrayBy4LO(float *ss[], struct PoTPlan *P) {

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2;
    float *pst = P->dest;
    float max = 0.0f;
    int i = 0;

    while (i < P->di) {
        pst[i] = p1[i] + p2[i] + p3[i] + p4[i];
        if (pst[i] > max) max = pst[i];
        i += 1;
    }
    return max;
}
sum_func AKSSETB4[FOLDTBLEN] = {
                                   foldArrayBy4LO, foldArrayBy4LO, foldArrayBy4LO, foldArrayBy4LO,
                                   foldArrayBy4LO, foldArrayBy4LO, foldArrayBy4LO, foldArrayBy4LO,
                                   foldArrayBy4,   foldArrayBy4LO, foldArrayBy4LO, foldArrayBy4LO,
                                   foldArrayBy4
                               };

float foldArrayBy5(float *ss[], struct PoTPlan *P) {
    float max;
    __m128 maxV = ZERO;
    int i = 0;

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2, *p5 = ss[0]+P->tmp3;
    float *pst = P->dest;
    __m128 x1, x2, x3, x4, x5, xs12, xs34, xs125, xs;

    while (i < P->di-4) {
        x1 = _mm_load_ps(p1+i);
        x2 = _mm_loadu_ps(p2+i);
        x3 = _mm_loadu_ps(p3+i);
        x4 = _mm_loadu_ps(p4+i);
        x5 = _mm_loadu_ps(p5+i);
        xs12 = _mm_add_ps(x1, x2);
        xs34 = _mm_add_ps(x3, x4);
        xs125 = _mm_add_ps(xs12, x5);
        xs = _mm_add_ps(xs34, xs125);
        _mm_store_ps(pst+i, xs);
        maxV = _mm_max_ps(maxV, xs);
        i += 4;
    }
    x1 = _mm_load_ps(p1+i);
    x2 = _mm_loadu_ps(p2+i);
    x3 = _mm_loadu_ps(p3+i);
    x4 = _mm_loadu_ps(p4+i);
    x5 = _mm_loadu_ps(p5+i);
    xs12 = _mm_add_ps(x1, x2);
    xs34 = _mm_add_ps(x3, x4);
    xs125 = _mm_add_ps(xs12, x5);
    xs = _mm_add_ps(xs34, xs125);

    _mm_store_ps(pst+i, xs);
    xs = _mm_sub_ps(xs, tailmask[P->di & 0x3].v);
    maxV = _mm_max_ps(maxV, xs);

    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x4e));
    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x39));
    _mm_store_ss(&max, maxV);

    return max;
}

float foldArrayBy5LO(float *ss[], struct PoTPlan *P) {

    const float *p1 = ss[0], *p2 = ss[0]+P->tmp0, *p3 = ss[0]+P->tmp1, *p4 = ss[0]+P->tmp2, *p5 = ss[0]+P->tmp3;
    float *pst = P->dest;
    float max = 0.0f;
    int i = 0;

    while (i < P->di) {
        pst[i] = p1[i] + p2[i] + p3[i] + p4[i] + p5[i];
        if (pst[i] > max) max = pst[i];
        i += 1;
    }
    return max;
}

sum_func AKSSETB5[FOLDTBLEN] = {
                                   foldArrayBy5LO, foldArrayBy5LO, foldArrayBy5LO, foldArrayBy5LO,
                                   foldArrayBy5LO, foldArrayBy5LO, foldArrayBy5LO, foldArrayBy5LO,
                                   foldArrayBy5,   foldArrayBy5LO, foldArrayBy5LO, foldArrayBy5LO,
                                   foldArrayBy5
                               };

// 2A version allows non-aligned tmp0

float foldArrayBy2A(float *ss[], struct PoTPlan *P) {
    float max;
    __m128 maxV = ZERO;
    int i = 0;

    const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
    float *pst = P->dest;
    __m128 x1, x2, xs;

    while (i < P->di-4) {
        x1 = _mm_load_ps(p1+i);
        x2 = _mm_loadu_ps(p2+i);
        xs = _mm_add_ps(x1, x2);
        _mm_store_ps(pst+i, xs);
        maxV = _mm_max_ps(maxV, xs);
        i += 4;
    }
    x1 = _mm_load_ps(p1+i);
    x2 = _mm_loadu_ps(p2+i);
    xs = _mm_add_ps(x1, x2);

    _mm_store_ps(pst+i, xs);
    xs = _mm_sub_ps(xs, tailmask[P->di & 0x3].v);
    maxV = _mm_max_ps(maxV, xs);

    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x4e));
    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x39));
    _mm_store_ss(&max, maxV);

    return max;
}

// 2AL version requires aligned tmp0


float foldArrayBy2AL(float *ss[], struct PoTPlan *P) {
    float max;
    __m128 maxV = ZERO;
    int i = 0;

    const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
    float *pst = P->dest;
    __m128 x1, x2, xs;

    while (i < P->di-4) {
        x1 = _mm_load_ps(p1+i);
        x2 = _mm_load_ps(p2+i);
        xs = _mm_add_ps(x1, x2);
        _mm_store_ps(pst+i, xs);
        maxV = _mm_max_ps(maxV, xs);
        i += 4;
    }
    x1 = _mm_load_ps(p1+i);
    x2 = _mm_load_ps(p2+i);
    xs = _mm_add_ps(x1, x2);

    _mm_store_ps(pst+i, xs);
    xs = _mm_sub_ps(xs, tailmask[P->di & 0x3].v);
    maxV = _mm_max_ps(maxV, xs);

    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x4e));
    maxV = _mm_max_ps(maxV, _mm_shuffle_ps(maxV, maxV, 0x39));
    _mm_store_ss(&max, maxV);

    return max;
}

float foldArrayBy2LO(float *ss[], struct PoTPlan *P) {

    const float *p1 = ss[1]+P->offset, *p2 = ss[1]+P->tmp0;
    float *pst = P->dest;
    float max = 0.0f;
    int i = 0;

    while (i < P->di) {
        pst[i] = p1[i] + p2[i];
        if (pst[i] > max) max = pst[i];
        i += 1;
    }
    return max;
}
sum_func AKSSETB2[FOLDTBLEN] = {
                                   foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                   foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                   foldArrayBy2A,  foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                   foldArrayBy2A
                               };

sum_func AKSSETB2AL[FOLDTBLEN] = {
                                     foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                     foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                     foldArrayBy2AL, foldArrayBy2LO, foldArrayBy2LO, foldArrayBy2LO,
                                     foldArrayBy2AL
                                 };

FoldSet AKSSEfold = {AKSSETB3, AKSSETB4, AKSSETB5, AKSSETB2, AKSSETB2AL, "AK SSE"};


#endif // USE_INTRINSICS

#endif // (__i386__) || (__x86_64__)


