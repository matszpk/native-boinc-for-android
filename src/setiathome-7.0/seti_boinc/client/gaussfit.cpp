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

// gaussfit.C
// $Id: gaussfit.cpp,v 1.21.2.12 2007/08/10 00:38:48 korpela Exp $
//

// The purpose of gaussian fitting is to find signals that rise and
// fall in power over time at a rate consistent with the beam
// pattern of the telescope.

#include "sah_config.h"

#include <stdio.h>
#include <math.h>

#include <algorithm>

// debug stuff
#define DEBUG_POT
//#define DUMP_POWER_SPECTRA
//#define DUMP_GAUSSIAN
#define POT_TO_DUMP 32
#define TOFF_TO_DUMP 15
#define FFT_TO_DUMP 131072
int gul_PoT;
int gul_Fftl;
// end debug stuff

#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

//#include "util.h"
#include "s_util.h"
#include "analyze.h"
#include "seti.h"
#include "worker.h"
#include "analyzeFuncs.h"
#include "analyzeReport.h"
#include "analyzePoT.h"
#include "lcgamm.h"
#include "gaussfit.h"
#include "chirpfft.h"

bool dump_pot = false;
//float gauss_display_power_thresh = 0;

float f_GetPeak(
  float fp_PoT[],
  int ul_TOffset,
  int ul_HalfSumLength,
  float f_MeanPower,
  float f_PeakScaleFactor,
  float f_weight[]
) {
  // Peak power is calculated as the weighted
  // sum of all powers within ul_HalfSumLength
  // of the assumed gaussian peak.
  // The weights are given by the gaussian function itself.
  // BUG WATCH : for the f_PeakScaleFactor to work,
  // ul_HalfSumLength *must* be set to sigma.

  int i;
  float f_sum;

  f_sum = 0.0;

  // Find a weighted sum
  for (i = ul_TOffset - ul_HalfSumLength; i <= ul_TOffset + ul_HalfSumLength; i++) {
    f_sum += (fp_PoT[i] - f_MeanPower) * f_weight[abs(i-ul_TOffset)];
  }
  
  analysis_state.FLOP_counter+=6.0*ul_HalfSumLength;
  return(f_sum * f_PeakScaleFactor);
}

float f_GetChiSq(
  float fp_PoT[],
  int ul_PowerLen,
  int ul_TOffset,
  float f_PeakPower,
  float f_MeanPower,
  float f_weight[],
  float *xsq_null=0
) {
  // We calculate our assumed gaussian powers
  // on the fly as we try to fit them to the
  // actual powers at each point along the PoT.

  int i;
  float f_ChiSq=0,f_null_hyp=0, f_PredictedPower, f_tot_weight=0;
  double rebin=swi.nsamples/ChirpFftPairs[analysis_state.icfft].FftLen
                  /ul_PowerLen;

  for (i = 0; i < ul_PowerLen; i++) {
    f_PredictedPower = f_MeanPower +
                       f_PeakPower * f_weight[abs(i-ul_TOffset)] ;
    f_tot_weight+=f_weight[abs(i-ul_TOffset)];
#ifdef DUMP_GAUSSIAN
    if ((gul_PoT == POT_TO_DUMP && ul_TOffset == TOFF_TO_DUMP) &&
        gul_Fftl == FFT_TO_DUMP) {
      fprintf(stdout, "%f\n", f_PredictedPower);
    }
#endif
    // ChiSq in this realm is:
    //  sum[0:i]( (observed power - expected power)^2 / expected variance )
    // The power of a signal is:
    //  power = (noise + signal)^2 = noise^2 + signal^2 + 2*noise*signal
    // With mean power normalization, noise becomes 1, leaving:
    //  power = signal^2 +or- 2*signal + 1
    f_PredictedPower/=f_MeanPower;
    double signal=f_PredictedPower;
    double noise=(2.0*sqrt(std::max(f_PredictedPower,1.0f)-1)+1);

    f_ChiSq += 
      (static_cast<float>(rebin*SQUARE(fp_PoT[i]/f_MeanPower - signal)/noise));
    f_null_hyp+=
      (static_cast<float>(rebin*SQUARE(fp_PoT[i]/f_MeanPower-1)/noise));
  }

  analysis_state.FLOP_counter+=20.0*ul_PowerLen+5;
  f_ChiSq/=ul_PowerLen;
  f_null_hyp/=ul_PowerLen;

#ifdef DUMP_GAUSSIAN
  if (gul_PoT == POT_TO_DUMP
      && ul_TOffset == TOFF_TO_DUMP
      && gul_Fftl == FFT_TO_DUMP
     ) {
    for (i = 0; i < ul_PowerLen; i++) {
      fprintf(stdout, "%f\n", fp_PoT[i]);
    }
  }
#endif
  if (xsq_null) *xsq_null=f_null_hyp;
  return f_ChiSq;
}


float f_GetTrueMean(
  float fp_PoT[],
  int ul_PowerLen,
  float f_TotalPower,
  int ul_TOffset,
  int ul_ExcludeLen
) {
  // TrueMean is the mean power of the data set minus all power
  // out to ExcludeLen from our current TOffset.
  int i, i_start, i_lim;
  float f_ExcludePower = 0;

  // take care that we do not add to exclude power beyond PoT bounds!
  i_start = std::max(ul_TOffset - ul_ExcludeLen, 0);
  i_lim = std::min<int>(ul_TOffset + ul_ExcludeLen + 1, swi.analysis_cfg.gauss_pot_length);
  for (i = i_start; i < i_lim; i++) {
    f_ExcludePower += fp_PoT[i];
  }
  
  analysis_state.FLOP_counter+=(double)(i_lim-i_start+5);
  return((f_TotalPower - f_ExcludePower) / (ul_PowerLen - (i - i_start)));
}


float f_GetPeakScaleFactor(float f_sigma) {
  // The PeakScaleFactor is calculated such that when used in f_GetPeak(),
  // the actual peak power can be extracted from a weighted sum.
  // This sum (see f_GetPeak()), is calculated as :
  // sum = SUM[x from -sigma to +sigma] of (gaussian weights * our data)
  // The gaussian weights are e^(-x^2 / sigma^2).
  // Our data is A(e^(-x^2 / sigma^2)), where 'A' is the peak power.
  // Through algebraic manipulation, we have:
  // A = sum * (1 / SUM[x from -sigma to +sigma] of (e^(-x^2 / sigma^2))^2.
  // The factor by which we multiply the sum is the PeakScaleFactor.
  // It is completely determined by sigma.

  int i, i_s = static_cast<int>(floor(f_sigma+0.5));
  float f_sigma_sq = f_sigma*f_sigma;
  float f_sum = 0.0;

  for (i = -i_s; i <= i_s; i++) {
    f_sum += static_cast<float>(EXP(i, 0, f_sigma_sq));
  }
 
  analysis_state.FLOP_counter+=(13.0*f_sigma+3);
  return(1 / f_sum);
}


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
) {

  GAUSS_INFO gi;
  float scale_factor;
  bool report, chisqOK=(ChiSq <= swi.analysis_cfg.gauss_chi_sq_thresh);

  // gaussian info
  gi.bin              = bin;
  gi.fft_ind          = ifft;
  gi.g.chirp_rate     = ChirpFftPairs[analysis_state.icfft].ChirpRate;
  gi.g.fft_len        = ChirpFftPairs[analysis_state.icfft].FftLen;
  gi.g.sigma          = sigma;
  gi.g.peak_power     = PeakPower;
  gi.g.mean_power     = TrueMean;
  gi.g.chisqr         = ChiSq;
  gi.g.null_chisqr    = null_ChiSq;
  gi.g.freq           = cnvt_bin_hz(bin,  gi.g.fft_len);
  double t_offset=(((double)gi.fft_ind+0.5)/swi.analysis_cfg.gauss_pot_length)*
                  PoTInfo.WUDuration;
  gi.g.detection_freq =calc_detection_freq(gi.g.freq,gi.g.chirp_rate,t_offset);
  gi.g.time           = swi.time_recorded+t_offset/86400.0;
  gi.g.max_power      = PoTMaxPower;
  gi.score            = -13.0;
  time_to_ra_dec(gi.g.time, &gi.g.ra, &gi.g.decl);

  // Scale PoT down to 256 16 bit ints.
  //for (i=0; i<swi.analysis_cfg.gauss_pot_length; i++) gi.pot[i] = fp_PoT[i];   // ???
  scale_factor = static_cast<float>(gi.g.max_power) / 255.0f;
  if (gi.g.pot.size() != static_cast<size_t>(swi.analysis_cfg.gauss_pot_length)) {
    gi.g.pot.set_size(swi.analysis_cfg.gauss_pot_length);
  }
  float_to_uchar(fp_PoT, &(gi.g.pot[0]), swi.analysis_cfg.gauss_pot_length, scale_factor);

  if (!swi.analysis_cfg.gauss_null_chi_sq_thresh)
      swi.analysis_cfg.gauss_null_chi_sq_thresh=1.890;
  // Gauss score used for "best of" and graphics.
  // This score is now set to be based upon the probability that a signal
  // would occur due to noise and the probability that it is shaped like 
  // a Gaussian (normalized to 0 at thresholds).  Thanks to Tetsuji for
  // making me think about this. The Gaussian has 62 degrees of freedom and
  // the null hypothesis has 63 degrees of freedom when gauss_pot_length=64;
  //JWS: Calculate invariant terms once, ala Alex Kan and Przemyslaw Zych
  static float gauss_bins = static_cast<float>(swi.analysis_cfg.gauss_pot_length);
  static float gauss_dof = gauss_bins - 2.0f;
  static float null_dof = gauss_bins - 1.0f;
  static double score_offset = (
      lcgf(0.5*null_dof, swi.analysis_cfg.gauss_null_chi_sq_thresh*0.5*gauss_bins)
     -lcgf(0.5*gauss_dof, swi.analysis_cfg.gauss_chi_sq_thresh*0.5*gauss_bins)
      );
//R: same optimization as for GPU build: if there is reportable Gaussian already - 
//R: skip score calculation for all except new reportable Gaussians
  // Final thresholding first.
  report = chisqOK
        && (gi.g.peak_power >= gi.g.mean_power * swi.analysis_cfg.gauss_peak_power_thresh)
        && (gi.g.null_chisqr >= swi.analysis_cfg.gauss_null_chi_sq_thresh);
  if (gaussian_count==0||report) {
      gi.score = score_offset
          +lcgf(0.5*gauss_dof,std::max(gi.g.chisqr*0.5*gauss_bins,0.5*gauss_dof+1))
          -lcgf(0.5*null_dof,std::max(gi.g.null_chisqr*0.5*gauss_bins,0.5*null_dof+1));
  }
  // Only include "real" Gaussians (those meeting the chisqr threshold)
  // in the best Gaussian display. 
  if (gi.score > best_gauss->score && chisqOK) {
    *best_gauss = gi;
  }
  // Update gdata gauss info regardless of whether it is the
  // best thus far or even passes the final threshold.  If
  // a gaussian has made it this far, display it.
#ifdef BOINC_APP_GRAPHICS
  if (!nographics()) sah_graphics->gi.copy(&gi);
#endif


  analysis_state.FLOP_counter+=24.0;
  // Final reporting.
  if (report) {
      return result_gaussian(gi);
  }  
  return 0;
}


int GaussFit(
  float * fp_PoT,
  int ul_FftLength,
  int ul_PoT
) {
  int i, retval;
  BOOLEAN b_IsAPeak;
  float f_NormMaxPower;
  float f_null_hyp;

  int ul_TOffset;
  int iSigma = static_cast<int>(floor(PoTInfo.GaussSigma+0.5));
  float f_GroupSum, f_GroupMax;
  int i_f, iPeakLoc;

  float f_TotalPower,
        f_MeanPower,
        f_TrueMean,
        f_ChiSq,
        f_PeakPower;

  // For setiathome the Sigma and Gaussian PoT length don't change during
  // a run of the application, so these frequently used values can be
  // precalculated and kept.
  static float f_PeakScaleFactor;
  static float *f_weight;

  if (!f_weight) {
    f_PeakScaleFactor = f_GetPeakScaleFactor(static_cast<float>(PoTInfo.GaussSigma));
    f_weight = reinterpret_cast<float *>(malloc(PoTInfo.GaussTOffsetStop*sizeof(float)));
    if (!f_weight) SETIERROR(MALLOC_FAILED, "!f_weight");
    for (i = 0; i < PoTInfo.GaussTOffsetStop; i++) {
      f_weight[i] = static_cast<float>(EXP(i, 0, PoTInfo.GaussSigmaSq));
    }
  }
  // Find mean over power-of-time array
  f_TotalPower = 0;
  for (i = 0; i < swi.analysis_cfg.gauss_pot_length; i++) {
    f_TotalPower += fp_PoT[i];
  }
  f_MeanPower = f_TotalPower / swi.analysis_cfg.gauss_pot_length;

  // Normalize power-of-time and check for the existence
  // of at least 1 peak.
  b_IsAPeak = false;
  double r_MeanPower=1.0/f_MeanPower;
  for (i = 0; i < swi.analysis_cfg.gauss_pot_length; i++) {
    fp_PoT[i] *= r_MeanPower;
    if (fp_PoT[i] > PoTInfo.GaussPowerThresh) b_IsAPeak = true;
  }
  analysis_state.FLOP_counter+=3.0*swi.analysis_cfg.gauss_pot_length+2;

  if (!b_IsAPeak) {
    //printf("no peak\n");
    return 0;  // no peak - bail on this PoT
  }

  // Recalculate Total Power across normalized array.
  // Given that powers are positive, f_TotalPower will
  // end up being == swi.analysis_cfg.gauss_pot_length.
  f_TotalPower   = 0;
  f_NormMaxPower = 0;
  // Also locate group with the highest sum for use in second bailout check.
  f_GroupSum     = 0;
  f_GroupMax     = 0;
  for (i = 0, i_f = -iSigma; i < swi.analysis_cfg.gauss_pot_length; i++, i_f++) {
    f_TotalPower += fp_PoT[i];
    if(fp_PoT[i] > f_NormMaxPower) f_NormMaxPower = fp_PoT[i];
    f_GroupSum += fp_PoT[i] - ((i_f < 0) ? 0.0f : fp_PoT[i_f]);
    if (f_GroupSum > f_GroupMax) {
      f_GroupMax = f_GroupSum;
      iPeakLoc = i - iSigma/2;
    }
  }

  // Check at the group peak location whether data may contain Gaussians
  // (but only after the first hurry-up Gaussian has been set for graphics)

  if (best_gauss->display_power_thresh != 0) {
    iPeakLoc = std::max(PoTInfo.GaussTOffsetStart,
                       (std::min(PoTInfo.GaussTOffsetStop - 1, iPeakLoc)));
  
    f_TrueMean = f_GetTrueMean(
                   fp_PoT,
                   swi.analysis_cfg.gauss_pot_length,
                   f_TotalPower,
                   iPeakLoc,
                   2 * iSigma
                 );
  
    f_PeakPower = f_GetPeak(
                    fp_PoT,
                    iPeakLoc,
                    iSigma,
                    f_TrueMean,
                    f_PeakScaleFactor,
                    f_weight
                  );

    analysis_state.FLOP_counter+=5.0*swi.analysis_cfg.gauss_pot_length+5;

    if (f_PeakPower < f_TrueMean*best_gauss->display_power_thresh*0.5f) {
      return 0;  // not even a weak peak at max group - bail on this PoT
    }
  }


  // slide dynamic gaussian across the Power Of Time array
  for (ul_TOffset = PoTInfo.GaussTOffsetStart;
       ul_TOffset < PoTInfo.GaussTOffsetStop;
       ul_TOffset++
      ) {

    // TrueMean is the mean power of the data set minus all power
    // out to 2 sigma from our current TOffset.
    f_TrueMean = f_GetTrueMean(
                   fp_PoT,
                   swi.analysis_cfg.gauss_pot_length,
                   f_TotalPower,
                   ul_TOffset,
                   2 * iSigma
                 );

    f_PeakPower = f_GetPeak(
                    fp_PoT,
                    ul_TOffset,
                    iSigma,
                    f_TrueMean,
                    f_PeakScaleFactor,
                    f_weight
                  );

    // worth looking at ?
    if (f_PeakPower < f_TrueMean*best_gauss->display_power_thresh) {
      continue;
    }

    // bump up the display threshold to its final value.
    // We could bump it up only to the gaussian just found,
    // but that would cause a lot of time waste
    // computing chisq etc.
    if (best_gauss->display_power_thresh == 0) {
        best_gauss->display_power_thresh = PoTInfo.GaussPeakPowerThresh/3;
    }

#ifdef TEXT_UI
    if(dump_pot) {
      printf("Found good peak at this PoT element.... truemean is %f, power is %f\n", f_TrueMean, f_PeakPower);
    }
#endif

#ifdef DUMP_GAUSSIAN
    gul_PoT = ul_PoT;
    gul_Fftl = ul_FftLength;
#endif

    // look at it - try to fit
    f_ChiSq = f_GetChiSq(
                fp_PoT,
                swi.analysis_cfg.gauss_pot_length,
                ul_TOffset,
                f_PeakPower,
                f_TrueMean,
                f_weight,
                &f_null_hyp
              );

#ifdef TEXT_UI
    if(dump_pot) {
      printf("Checking ChiSqr for PoT dump....\n");
      if(f_ChiSq <= swi.analysis_cfg.gauss_chi_sq_thresh) {
        int dump_i;
        printf(
          "POT %d %f %f %f %f %d ",
          ul_TOffset,
          f_PeakPower,
          f_TrueMean,
          PoTInfo.GaussSigmaSq,
          f_ChiSq,
          ul_PoT
        );
        for (dump_i = 0; dump_i < swi.analysis_cfg.gauss_pot_length; dump_i++) {
          printf("%f ", fp_PoT[dump_i]);
        }
        printf("\n");
      } else {
        printf("ChiSqr is %f, not good enough.\n", f_ChiSq);
      }
    }
#endif

    retval = ChooseGaussEvent(
               ul_TOffset,
               f_PeakPower,
               f_TrueMean,
               f_ChiSq,
               f_null_hyp,
               ul_PoT,
               static_cast<float>(PoTInfo.GaussSigma),
               f_NormMaxPower,
               fp_PoT
             );
    if (retval) SETIERROR(retval,"from ChooseGaussEvent");

  } // End of sliding gaussian

  return 0;

} // End of gaussfit()


