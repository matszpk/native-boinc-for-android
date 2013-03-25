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

// Title      : gaussfit.h
// $Id: gaussfit.h,v 1.3.2.2 2006/06/22 23:53:57 korpela Exp $

extern int GaussFit(
    float * fp_PoT,
    int ul_FftLength,
    int ul_PoT
  );

extern float f_GetPeak(
    float fp_PoT[],
    int ul_TOffset,
    int ul_HalfSumLength,
    float f_MeanPower,
    float f_PeakScaleFactor,
    float f_weight[]
);

int ChooseGaussEvent(
  int ifft,
  float PeakPower,
  float TrueMean,
  float ChiSq,
  float null_ChiSq,
  int bin,
  float sigma,
  float PoTMaxPower,
  float fp_PoT[]
);

float f_GetPeakScaleFactor(float f_sigma);

extern void gauss_display_init();

extern bool dump_pot;
