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

// $Id: analyzeFuncs_vector.h,v 1.1.2.9 2007/06/08 03:09:47 korpela Exp $
#ifndef ANALYZEFUNCS_VECTOR_H
#define ANALYZEFUNCS_VECTOR_H

#include "chirpfft.h"
#define FUNCTION_FILE_NAME "functions.sah"

#define TWO_TO_52 4.503599627370496e15
// 2^52+2^51 recommended by AMD optimization manual, scaled to 2^64+2^63 for X87
#define ROUNDX87 6755399441055744.0 * 2048.0

// Flemming Pedersen (CERN) SinCos polynomial coefficients
#define FS1 1.5707963267948966
#define FS2 -0.64596348437163809
#define FS3 0.079679708649230657
#define FS4 -0.0046002309092153379
#define FC1 -1.2336979844380824
#define FC2 0.25360671639164339
#define FC3 -0.020427240364907607


typedef int  (*BaseLineSmooth_func)(sah_complex *, int, int, int);
typedef int (*GetPowerSpectrum_func)(sah_complex *, float*, int);
typedef int  (*ChirpData_func)(sah_complex *, sah_complex *, int, double, int, double); 
typedef int (*Transpose_func)(int, int , float *, float *);

extern void ChooseFunctions(
    BaseLineSmooth_func *, 
    GetPowerSpectrum_func *,
    ChirpData_func *,
    Transpose_func *,
    ChirpFftPair_t * ChirpFftPairs,
    int num_cfft,
    int nsamples,
    bool print_choice);

#if defined(__i386__) || defined(__x86_64__) || defined(USE_SSE)
extern double fastfrac( double val, double returnVal) 
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 3))
  __attribute__ ((__optimize__ ("-fno-fast-math")));
#define SUPPORTS_ATTRIB_OPT 1
#else
  ;
#undef SUPPORTS_ATTRIB_OPT
#endif
#endif

extern int v_vBaseLineSmooth(
    sah_complex * cx_DataIn,
    int ul_NumDataPoints,
    int ul_BoxCarLength,
    int ul_TimeLength
  );
 
extern int v_vGetPowerSpectrum(
    sah_complex * cx_FreqData,
    float * fp_PowerSpectrum,
    int ul_NumDataPoints
  );

extern int v_vChirpData(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int fpu_ChirpData (
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int ChirpRateInd,
  double ChirpRate,
  int  ul_NumDataPoints,
  double sample_rate
);
extern int fpu_opt_ChirpData (
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int ChirpRateInd,
  double ChirpRate,
  int  ul_NumDataPoints,
  double sample_rate
);
#if defined(__i386__) || defined(__x86_64__) || defined(USE_SSE)
extern int v_vChirpData_x86_64(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse1_ChirpData_ak(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse1_ChirpData_ak8e(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse1_ChirpData_ak8h(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse2_ChirpData_ak(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse2_ChirpData_ak8(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse3_ChirpData_ak(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
extern int sse3_ChirpData_ak8(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );
#endif

extern int v_vTranspose(int i, int j, float *in, float *out);
#if defined(__i386__) || defined(__x86_64__) || defined(USE_SSE)
extern int v_pfTranspose2(int i, int j, float *in, float *out);
extern int v_pfTranspose4(int i, int j, float *in, float *out);
extern int v_pfTranspose8(int i, int j, float *in, float *out);
extern int v_vTranspose4(int i, int j, float *in, float *out);
extern int v_vTranspose4np(int i, int j, float *in, float *out);
extern int v_vTranspose4ntw(int i, int j, float *in, float *out);
extern int v_vTranspose4x8ntw(int i, int j, float *in, float *out);
extern int v_vTranspose4x16ntw(int i, int j, float *in, float *out);
extern int v_vpfTranspose8x4ntw(int i, int j, float *in, float *out);
#endif


#if defined(__i386__) || defined(__x86_64__) || defined(USE_SSE)
extern int v_vGetPowerSpectrumUnrolled(  
                               sah_complex * cx_FreqData,
                               float * fp_PowerSpectrum,
                               int ul_NumDataPoints
                              );
extern int v_vGetPowerSpectrum2(  
                               sah_complex * cx_FreqData,
                               float * fp_PowerSpectrum,
                               int ul_NumDataPoints
                              );
extern int v_vGetPowerSpectrumUnrolled2(  
                               sah_complex * cx_FreqData,
                               float * fp_PowerSpectrum,
                               int ul_NumDataPoints
                              );
#endif

#ifdef USE_ALTIVEC
extern int v_vGetPowerSpectrumG4(
                                sah_complex * cx_FreqData,
                                float * fp_PowerSpectrum,
                                int ul_NumDataPoints
                                );


extern int v_vChirpDataG4(
                        sah_complex * cx_DataArray,
                        sah_complex *  cx_ChirpDataArray,
                        int ChirpRateInd,
                        double ChirpRate,
                        int  ul_NumDataPoints,
                        double sample_rate
                        );

extern int v_vChirpDataG5(
                        sah_complex * cx_DataArray,
                        sah_complex *  cx_ChirpDataArray,
                        int ChirpRateInd,
                        double ChirpRate,
                        int  ul_NumDataPoints,
                        double sample_rate
                        );
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
extern int v_vChirpData_x86_64(
                        sah_complex * cx_DataArray,
			sah_complex * cx_ChirpDataArray,
                        int ChirpRateInd, 
			double ChirpRate, 
			int ul_NumDataPoints, 
			double sample_rate
			);
#endif

#if defined(USE_AVX)
extern int avxSupported(void);

extern int avx_ChirpData_a(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
);
extern int avx_ChirpData_b(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
);
extern int avx_ChirpData_c(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
);
extern int avx_ChirpData_d(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    const double chirp_rate,
    int  ul_NumDataPoints,
    const double sample_rate
);
extern int v_avxTranspose4x8ntw(int x, int y, float *in, float *out);
extern int v_avxTranspose4x16ntw(int x, int y, float *in, float *out);
extern int v_avxTranspose8x4ntw(int x, int y, float *in, float *out);
extern int v_avxTranspose8x8ntw_a(int x, int y, float *in, float *out);
extern int v_avxTranspose8x8ntw_b(int x,int y, float *in, float *out
);
extern int v_avxGetPowerSpectrum(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
);
#endif

#endif
