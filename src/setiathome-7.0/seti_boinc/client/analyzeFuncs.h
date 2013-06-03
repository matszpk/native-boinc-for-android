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

// Title      : analyzeFuncs.h
// $Id: analyzeFuncs.h,v 1.5.2.9 2007/05/31 22:03:09 korpela Exp $
#ifndef  ANALYZE_FUNCS_H
#include "seti.h"

// The PROGRESS_FACTORs are based on the relative time that
// it takes to apply the given algorithm to the data block,
// at some given chirp/fft pair.  They are more flakey than
// I would like.  On rewrite, I would run calibrating
// benchmarks on program startup.
//
// The GAUSS_PROGRESS_FACTOR of 10 seems about right for gaussian
// finding at an FFT length of 16K and maybe 8K.  At shorter FFTs,
// gaussian finding goes alot faster, but this situation occurs a
// lot fewer times and  so it's contribution gets washed out.
//
// Pulse finding with array size 14 is given 1 factor unit, as that
// very roughly takes the same amount of time as  FFT/spike finding.
// As the array size doubles, the number of factor units will also
// double. N here is the array size.

/* These constants are no longer used, see functions in progress.cpp

#define SPIKE_PROGRESS_FACTOR        1.0f
#define CHIRP_PROGRESS_FACTOR           2.0f
#define GAUSS_PROGRESS_FACTOR          10.0f
#define TRIPLET_PROGRESS_FACTOR         0.1f
#define PULSE_PROGRESS_FACTOR(N)        (N/14.0f)
*/

#define PROGRESS_DISPLAY_RES 1.0
// for text-only versions: display progress with every 1%

extern int seti_analyze(ANALYSIS_STATE&);

#include "vector/analyzeFuncs_vector.h"

extern int v_BaseLineSmooth(
    sah_complex * cx_DataIn,
    int ul_NumDataPoints,
    int ul_BoxCarLength,
    int ul_TimeLength
  );

extern int v_GetPowerSpectrum(
    sah_complex * cx_FreqData,
    float * fp_PowerSpectrum,
    int ul_NumDataPoints
  );

#define TESTCHIRPIND 27040

extern int v_ChirpData(
    sah_complex * cx_DataArray,
    sah_complex *  cx_ChirpDataArray,
    int ChirpRateInd,
    double ChirpRate,
    int  ul_NumDataPoints,
    double sample_rate
  );

extern int v_Transpose(int xsize, int ysize, float *in, float *out);
extern int v_Transpose2(int xsize, int ysize, float *in, float *out);
extern int v_Transpose4(int xsize, int ysize, float *in, float *out);
extern int v_Transpose8(int xsize, int ysize, float *in, float *out);

extern BaseLineSmooth_func BaseLineSmooth;
extern GetPowerSpectrum_func GetPowerSpectrum;
extern ChirpData_func ChirpData;
extern Transpose_func Transpose;

// These are used to calculate chirped signals
// TrigStep contains trigonometric functions for MinChirpStep over time
// CurrentTrig contains current trigonometric fuctions
//    Tetsuji "Maverick" Rai

// Trigonometric arrays
typedef struct {double Sin, Cos; }
SinCosArray;

extern SinCosArray* TrigStep;     // trigonometric array of MinChirpStep
extern SinCosArray* CurrentTrig;  // current chirprate trigonometric array
extern int CurrentChirpRateInd;          // current chirprate index (absolute value)
extern double MinChirpStep;
extern bool use_transposed_pot;

//extern float GetProgressUnitSize(int NumDataPoints, int num_cfft, SETI_WU_INFO& swi);
extern float GetProgressUnitSize(int NumDataPoints, int num_cfft);

extern double ProgressUnitSize;
extern double progress;
extern double remaining;
extern float min_slew;
extern float max_slew;
#endif
