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

#ifndef SAH_CHIRPFFT_H
#define SAH_CHIRPFFT_H

#define TEST_MIN_CHIRP_STEP 0.002776    // chirps .5 bins with 128K point FFT

// Variable (usually an array of such
// variables) to hold chirp/fft pairs.
typedef struct {
  double  ChirpRate;
  int   ChirpRateInd; // chirprate index (= ChirpRate / MinChirpStep)
  int   FftLen;
  int	GaussFit;
  int 	PulseFind;
}
ChirpFftPair_t;

size_t GenChirpFftPairs(
  ChirpFftPair_t ** ChirpFftPairs,
  double * MinChirpStep
);

extern ChirpFftPair_t * ChirpFftPairs;
extern char * cfft_file;

void CalcChirpSteps(double* ChirpSteps, double* MinChirpStep);

bool VeryClose(double a, double b, double CloseEnough);

int ReadCFftFile(ChirpFftPair_t ** ChirpFftPairs, double * MinChirpStep);

#endif
