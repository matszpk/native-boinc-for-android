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

// Title      : analyzeReport.h
// $Id: analyzeReport.h,v 1.8.2.3 2007/05/31 22:03:09 korpela Exp $

#include <vector>

#include "seti.h"
#include "boinc_api.h"
#include "mfile.h"

int ReportEvents(
    float * fp_PowerSpectrum,
    int ul_NumDataPoints,
    int us_FFT,
    double spike_sigma_thresh
  );

int ReportGaussEvent(
    int ul_TOffset,
    float f_PeakPower,
    float f_TrueMean,
    float f_SumSq,
    int ul_PoT,
    float sigma,
    float f_PoTMaxPower,
    float fp_PoT[]
  );

int ReportPulseEvent(
    float PulsePower,
    float MeanPower,
    float PulsePeriod,
    int   time_bin,
    int   freq_bin,
    float snr,
    float thresh,
    float *foldedPOT,
    int scale,
    int write
  );

int ReportTripletEvent(
    float PulsePower,
    float MeanPower,
    float PulsePeriod,
    float mid_time_bin,
    int   start_time_bin,
    int   freq_bin,
    int   pot_len,
    const float *PoT,
    int write
  );

int result_spike(SPIKE_INFO &si);

int result_autocorr(AUTOCORR_INFO &ai);

int result_gaussian(GAUSS_INFO &gi);

void time_to_ra_dec(double time_jd, double *ra, double *dec);

extern void reset_high_scores();

//extern void report_init();
extern void reload_graphics_state();

extern SPIKE_INFO * best_spike;
extern AUTOCORR_INFO * best_autocorr;
extern GAUSS_INFO * best_gauss;
extern PULSE_INFO * best_pulse;
extern TRIPLET_INFO * best_triplet;
extern MFILE outfile;

extern int signal_count;
extern int autocorr_count;
extern int spike_count;
extern int pulse_count;
extern int gaussian_count;
extern int triplet_count;
