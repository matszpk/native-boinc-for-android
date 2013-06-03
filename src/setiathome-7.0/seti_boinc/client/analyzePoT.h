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

// $Id: analyzePoT.h,v 1.5.2.2 2005/07/04 22:50:09 korpela Exp $

#include "chirpfft.h"
#define POT_INACTIVE    0
#define POT_DOING_GAUSS 1
#define POT_DOING_PULSE 2

extern double beam_width;

typedef struct {
  double WUDuration;  // in seconds
  double SlewRate;   // degrees/second
  double BeamRate;   // beams/second
  int   MaxPoTLen;

  double GaussSigma;  // in gaussian PoT bins
  double GaussSigmaSq;  // in gaussian PoT bins
  int   GaussTOffsetStart; // in gaussian PoT bins
  int   GaussTOffsetStop;  // in gaussian PoT bins
  double min_slew;
  double max_slew;
  double GaussChiSqThresh;  // gaussian reporting thresh
  double GaussPowerThresh;  // PoT investigation thresh - in units
  //   of mean power across entire PoT
  double GaussPeakPowerThresh; // gaussian investigation thresh - in
  //   units of mean power across PoT,
  //   exclusive of 2 sigma area on either
  //   side of gaussian peak

  double PulseOverlapFactor; // for both pulse and triplets
  double PulseBeams;  // for both pulse and triplets

  double PulseThresh;
  int   PulseMax;
  int   PulseMin;
  int   PulseFftMax;

  double TripletThresh;
  int   TripletMax;
  int   TripletMin;
}
PoTInfo_t;

extern PoTInfo_t PoTInfo;

int analyze_pot(
  float * fp_PowerSpectrum,
  int ul_NumDataPoints,
  ChirpFftPair_t &cfft 
);

int GetFixedPoT(
  float fp_PowerSpectrum[],
  int ul_NumDataPoints,
  int ul_FftLength,
  float fp_PoT[],
  int ul_PoTLen,
  int ul_PoT
);

void ComputePoTInfo(int num_cfft, int NumDataPoints);
void GetPulsePoTLen(long FullPoTLen, int * PulsePoTLen, int * PulseOverlap);
