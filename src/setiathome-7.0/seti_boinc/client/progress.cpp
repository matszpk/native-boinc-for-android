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

// progress.C

#include "sah_config.h"

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cmath>

#include "diagnostics.h"
#include "util.h"
#include "s_util.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif


#include "progress.h"
#include "analyzePoT.h"
#include "seti.h"
#include "chirpfft.h"

double NormalizedToSpike = 0.0;

double triplet_units;
double pulse_units;
double spike_units;
double gauss_units;

static double prev_progress = 0.0;
// fraction_done sets the boinc_fraction_done() with a smooth transition 
// from using "progress" to "remaining" to determine the fraction done.
// The speed of the transition is determine by PROG_POWER.  6 means that
// "remaining" is the dominant term for about the final 1/6th of the run.
#define PROG_POWER 6
void fraction_done(double progress,double remaining) {
    double prog2=1.0-remaining;
    progress=std::min(progress,1.0);
    //double prog=progress*(1.0-pow(prog2,PROG_POWER))+prog2*pow(prog2,PROG_POWER);
    double prog2_6 = prog2*prog2*prog2;
    prog2_6*=prog2_6;
    double prog=progress*(1.0-prog2_6)+prog2*prog2_6;
    
   if (prog-prev_progress >= 2.0e-5)
   {
      boinc_fraction_done(prog);
#ifdef PRINT_PROGRESS
      printf("Progress: %3.3f\r",prog*100.0);
      fflush(stdout);
#endif
      prev_progress = prog;
   }
}

void reset_units() {
  triplet_units=0;
  pulse_units=0;
  spike_units=0;
  gauss_units=0;
}

float GetProgressUnitSize(int NumDataPoints, int num_cfft, long ac_fft_len) {

  // A ProgressUnit is defined as the computation time of a
  // detection algorithm on the entire data block at a
  // given chirpfft pair. 

  // Spike detection takes place for every FFT length and at 
  // any slew rate.  PoT detctors, on the other hand, may opt 
  // to not execute if slew rate and/or FFT length limits are 
  // not met.


  int i, ThisPoTLen, ThisPulsePoTLen, DummyOverlap;

  double NumProgressUnits;
  double LastChirpRate = 0.0f;

  double TotalSpikeProgressUnits = 0.0;
  double TotalGaussianProgressUnits = 0.0;
  double TotalPulseProgressUnits = 0.0;
  double TotalTripletProgressUnits = 0.0;
  double TotalChirpProgressUnits = 0.0;
  NumProgressUnits = 0.0;

  for (i = 0; i < num_cfft; i++) {

    // FFTs and spike finding
    TotalSpikeProgressUnits += SpikeProgressUnits(ChirpFftPairs[i].FftLen);

    // Autocorr FFTs and finding
    if ((long)ChirpFftPairs[i].FftLen == ac_fft_len) {
      TotalSpikeProgressUnits += SpikeProgressUnits(ChirpFftPairs[i].FftLen);
    }

    // Chirping
    if(ChirpFftPairs[i].ChirpRate != LastChirpRate) {
      TotalChirpProgressUnits += ChirpProgressUnits();
      LastChirpRate = ChirpFftPairs[i].ChirpRate;
    }

    ThisPoTLen = NumDataPoints / ChirpFftPairs[i].FftLen;

    // Gaussians....
    if  (ChirpFftPairs[i].GaussFit) {
      TotalGaussianProgressUnits += GaussianProgressUnits();
    }

    // Pulses and Triplets....
    GetPulsePoTLen(ThisPoTLen, &ThisPulsePoTLen, &DummyOverlap);
#ifdef USE_PULSE
    if(ChirpFftPairs[i].PulseFind)
      TotalPulseProgressUnits += PulseProgressUnits(ThisPulsePoTLen, ChirpFftPairs[i].FftLen - 1);
#endif
#ifdef USE_TRIPLET
    if(ThisPulsePoTLen >= PoTInfo.TripletMin && ThisPulsePoTLen <= PoTInfo.TripletMax)
      TotalTripletProgressUnits += TripletProgressUnits();
#endif

  }

  NumProgressUnits = 	TotalChirpProgressUnits 	+
   			TotalSpikeProgressUnits 	+
			TotalGaussianProgressUnits	+
			TotalPulseProgressUnits		+
			TotalTripletProgressUnits;

/*
   fprintf(stderr, "%f ChirpProgressUnits (%f\%)\n", TotalChirpProgressUnits, TotalChirpProgressUnits/NumProgressUnits);
   fprintf(stderr, "%f SpikeProgressUnits (%f\%)\n", TotalSpikeProgressUnits, TotalSpikeProgressUnits/NumProgressUnits);
   fprintf(stderr, "%f GaussianProgressUnits (%f\%)\n", TotalGaussianProgressUnits, TotalGaussianProgressUnits/NumProgressUnits);
   fprintf(stderr, "%f TripletProgressUnits (%f\%)\n", TotalTripletProgressUnits, TotalTripletProgressUnits/NumProgressUnits);
   fprintf(stderr, "%f PulseProgressUnits (%f\%)\n", TotalPulseProgressUnits, TotalPulseProgressUnits/NumProgressUnits);
   fprintf(stderr, "%f NumProgressUnits\n", NumProgressUnits);
*/

  // Add a fudge factor of 0.01% to make sure we do not hit 100% done too soon
  return(1.0f/(float)(NumProgressUnits + NumProgressUnits * 0.0001));
}


  // Algorithm specific functions that return the number of progress 
  // units for the entire data block for some chirp/fft pair.  All other
  // algorithms are normalized to fft/spike finding, which is said
  // to take 1.0 progress units.

  // If progress us updated at finer intervals than the computation 
  // on the entire data block, care must be taken to divide the 
  // return value by the number of such intervals in a data block.
  // Eg, if you update for each FFT bin, divide the number of units
  // by FFT length. 

double SpikeProgressUnits(int FftLen) {
        spike_units+=(1.0+log((double)FftLen)/11.783);
	return (1.0+log((double)FftLen)/11.783);
}

double GaussianProgressUnits() {
        gauss_units+=(14.5*0.6);
	return(14.5*0.6);
}

// Pulse finding is an (n^2)log(n) computation.  The final factor
// is a scaling factor and can be tuned.
double PulseProgressUnits(double PulsePoTLen, int FftLen) { 
   pulse_units+=(PulsePoTLen * PulsePoTLen * log(PulsePoTLen) * FftLen / 2.65e6/4);
   return(PulsePoTLen * PulsePoTLen * log(PulsePoTLen) * FftLen / 2.65e6/4);
}

double TripletProgressUnits() { 
        triplet_units+=0.1;
	return(0.1);
}

double ChirpProgressUnits() {

	return (2.0);
}
