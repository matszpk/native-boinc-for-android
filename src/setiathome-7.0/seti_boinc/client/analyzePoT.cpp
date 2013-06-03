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

// $Id: analyzePoT.cpp,v 1.24.2.13 2007/08/10 00:38:47 korpela Exp $
//

#include "sah_config.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

#include "util.h"
#include "malloc_a.h"
#include "analyze.h"
#include "seti.h"
#include "worker.h"
#include "analyzeFuncs.h"
#include "analyzePoT.h"
#include "analyzeReport.h"
#include "gaussfit.h"
#include "pulsefind.h"
#include "chirpfft.h"
#include "filesys.h"
#include "util.h"
#include "s_util.h"
#include "progress.h"

//#define DEBUG_POT

// Compile time inits.  Command line parms of setiathome_test can
// change a number of these.
PoTInfo_t PoTInfo =  {0};

int analyze_pot(
    float *PowerSpectrum,
    int NumDataPoints,
    ChirpFftPair_t &cfft
) {


    // This function analyses Power over Time for the current data block.
    // The PoT array  is created by taking an array of power spectra (a
    // standard row-major 2D array)  and extracting the PoT as column-major
    // data.  We essentialy turn the array on its side.

    int  retval = 0,
                  i,
                  FftLength=cfft.FftLen,  // Current FFT length
                            ThisPoT,          // index of current PoT along the freq axis
                            PoTLen,           // compliment of FFT length - determines time res
                            PulsePoTLen,      // length of PoT segment passed to pulse finders
                            Overlap,          // PoT segment overlap in bins
                            TOffset,          // index into ThisPoT of current pulse segment
                            PulsePoTNum  = 0, // the oridinal position of a pulse PoT w/in a full PoT
                                           NumPulsePoTs = 0, // the number of pulse PoTs w/in a full PoT.  This is
                                                          //   constant regardless of FFT or PoT length and is
                                                          //   determined by slew rate.
                                                          AdvanceBy;        // the number of bins to advance for the next pulse PoT

    float ProgressAddFactor = 0.0,    // sum of progress adds for ThisPoT
                              ProgressPerPulsePoT = 0.0;  // for local progress display

    bool SkipGauss   = false,
                       SkipPulse   = false,
                                     SkipTriplet = false,
                                                   TOffsetOK   = true;

    static float  *GaussPoT = NULL,
                              *PulsePoT = NULL;

#ifdef DUMP_POWER_SPECTRA

    if (ul_FftLength == FFT_TO_DUMP) {
        for (i=0;i<ul_NumDataPoints;i++) {
            fprintf(stdout,"%f\n",fp_PowerSpectrum[i]);
        }
    }
#endif
#ifdef DEBUG_POT
    fprintf(stderr, "========= FftLength = %d =========\n", FftLength);
#endif

    PoTLen              = NumDataPoints / FftLength;               // in bins
    GetPulsePoTLen(PoTLen, &PulsePoTLen, &Overlap);     // in bins
    AdvanceBy  = PulsePoTLen - Overlap;     // in bins

    // Max limits how *slow* the slewrate can be, while Min limits how
    // *fast* the slewrate can be.  Max is limited only by the client
    // memory budget.
    if(PulsePoTLen > PoTInfo.TripletMax || PulsePoTLen < PoTInfo.TripletMin)
        SkipTriplet = true;
    SkipGauss = !(cfft.GaussFit);
    SkipPulse = !(cfft.PulseFind);

    if(!SkipPulse || !SkipTriplet) {
        // NumPulsePoTs is the number of PoT segments that we pass to the pulse
        // detectors per frequency bin.  ProgressPerPulsePoT is the inverse of
        // number of pulse detection segments in the entire data block, taking
        // into account that we skip the first (DC) frequency bin.  An assumption
        // is made here that minimum pulse/triplet PoT length will always be
        // greater than 1. Otherwise, AdvanceBy can become zero and a divide by
        // zero can occur.  The assumption is also made that FftLength is always
        // greater than 1!
        NumPulsePoTs = 1 + (PoTLen-PulsePoTLen)/AdvanceBy + ((PoTLen-PulsePoTLen)%AdvanceBy ? 1 : 0);
        ProgressPerPulsePoT = (float)1 / ((FftLength - 1) * NumPulsePoTs);
    }

#ifdef DEBUG_POT
    fprintf(stderr, "SlewRate = %f\n", PoTInfo.SlewRate);
    fprintf(stderr, "PoTLen = %d\n", PoTLen);
    fprintf(stderr, "MaxPoTLen = %d\n", PoTInfo.MaxPoTLen);
    fprintf(stderr, "PoTDuration = %f\n", PoTInfo.WUDuration);
    fprintf(stderr, "BeamRate = %f\n", PoTInfo.BeamRate);
    fprintf(stderr, "PulsePoTLen = %d\n", PulsePoTLen);
    fprintf(stderr, "Overlap = %d\n", Overlap);
    fprintf(stderr, "AdvanceBy = %d\n", AdvanceBy);
    fprintf(stderr, "min_slew = %f\n", PoTInfo.min_slew);
    fprintf(stderr, "max_slew = %f\n", PoTInfo.max_slew);
    fprintf(stderr, "PulseOverlapFactor = %f\n", PoTInfo.PulseOverlapFactor);
    fprintf(stderr, "PulseBeams = %f\n", PoTInfo.PulseBeams);
    fprintf(stderr, "PulseThresh = %f\n", PoTInfo.PulseThresh);
    fprintf(stderr, "PulseMax = %d\n", PoTInfo.PulseMax);
    fprintf(stderr, "PulseMin = %d\n", PoTInfo.PulseMin);
    fprintf(stderr, "PulseFftMax = %d\n", PoTInfo.PulseFftMax);
    fprintf(stderr, "TripletThresh = %f\n", PoTInfo.TripletThresh);
    fprintf(stderr, "TripletMax = %d\n", PoTInfo.TripletMax);
    fprintf(stderr, "TripletMin = %d\n", PoTInfo.TripletMin);
#endif

#ifndef USE_PULSE

    SkipPulse = TRUE;
#endif
#ifndef USE_TRIPLET

    SkipTriplet = TRUE;
#endif

    // Get memory fot the PoT arrays. The PoT array for gausian analysis is
    // of set size.  The PoT array for pulse analysis is sized to cover
    // PulseBeams beams, regardless of whether this violates either the
    // triplet or pulse limits on array size.
    if(!GaussPoT) {
        GaussPoT = (float *)malloc_a(swi.analysis_cfg.gauss_pot_length * sizeof(float), MEM_ALIGN);
        if(GaussPoT == NULL) {
            SETIERROR(MALLOC_FAILED, "GaussPoT == NULL");
        }
    }
    if(!PulsePoT) {
        PulsePoT = (float *)calloc_a(PoTInfo.MaxPoTLen+3, sizeof(float), MEM_ALIGN);
        if(PulsePoT == NULL) {
            SETIERROR(MALLOC_FAILED, "PulsePoT == NULL");
        }
    }

    // Look for gaussians ---------------------------------------------------
    if(!SkipGauss && (analysis_state.PoT_activity == POT_DOING_GAUSS ||
                      analysis_state.PoT_activity == POT_INACTIVE)) {

#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics)
            strcpy(sah_graphics->status, "Searching for Gaussians");
#endif

        // If we are back from being interrupted in the middle of gaussian PoT
        // analysis, load state and continue.  Otherwise start anew, skipping
        // the DC (0) bin.
        if(analysis_state.PoT_activity == POT_DOING_GAUSS) {
            ThisPoT = analysis_state.PoT_freq_bin;
        } else {
            ThisPoT = 1; // skip the DC bin on start of new cfft pair
        }

        // Initial display of local Progress / CPU time. Assumes that
        // we start ThisPoT at 1 each time in!
#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics)
            sah_graphics->local_progress = ((float)ThisPoT-1)/(FftLength-1);
#endif

        // loop through frequencies
        for(; ThisPoT < FftLength; ThisPoT++) {

            // Create PowerOfTime array for gaussian fit
            retval = GetFixedPoT(
                         PowerSpectrum,
                         NumDataPoints,
                         FftLength,
                         GaussPoT,
                         swi.analysis_cfg.gauss_pot_length,
                         ThisPoT
                     );
            if (retval)
                continue;

            retval = GaussFit(GaussPoT, FftLength, ThisPoT);
            if (retval)
                SETIERROR(retval,"from GaussFit");



            progress += ProgressUnitSize * GaussianProgressUnits() / (float)(FftLength - 1);
            progress=std::min(progress,1.0); // prevent display of > 100%
            fraction_done(progress,remaining);
            
            // At the end of each frequency bin we update progress and save state.
#ifdef BOINC_APP_GRAPHICS
            if (!nographics()) {
                sah_graphics->local_progress = ((float)ThisPoT)/(FftLength-1);
            }
#endif  
            analysis_state.PoT_freq_bin = ThisPoT;
            analysis_state.PoT_activity = POT_DOING_GAUSS;
            retval = checkpoint();
            if (retval)
                SETIERROR(retval,"from checkpoint()");

        }   // end loop through frequencies

        analysis_state.PoT_freq_bin = -1;
        analysis_state.PoT_activity = POT_INACTIVE;

    }   // end looking for gaussians


    // Look for pulses -------------------------------------------------------
    if(!SkipPulse || !SkipTriplet) {

#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics) {
            strcpy(sah_graphics->status, "Searching for Pulses / Triplets");
            // init local progress for pulse search
            sah_graphics->local_progress = 0;
        }
#endif


        // If we are back from being interrupted in the middle of pulse PoT
        // analysis, load state and continue.  Otherwise start anew, skipping
        // the DC (0) bin.
        if(analysis_state.PoT_activity == POT_DOING_PULSE) {
            ThisPoT = analysis_state.PoT_freq_bin;
        } else {
            ThisPoT = 1; // skip the DC bin on start of new cfft pair
        }

        PulsePoTNum = 0;

#ifdef BOINC_APP_GRAPHICS
        // Inital display of Local Progress
        if (sah_graphics) {
            sah_graphics->local_progress = (((ThisPoT-1) * NumPulsePoTs) +
                                            PulsePoTNum)                   *
                                           ProgressPerPulsePoT;
        }
#endif

        // loop through frequencies
        for(; ThisPoT < FftLength; ThisPoT++) {

            // loop through time for each frequency.  PulsePoTNum is
            // used only for progress calculation.
            for(TOffset = 0, PulsePoTNum = 1, TOffsetOK = true;
                    TOffsetOK;
                    PulsePoTNum++, TOffset += AdvanceBy) {

                // Create PowerOfTime array for pulse detection.  If there
                // are not enough points left in this PoT, adjust TOffset
                // to get the latest possible pulse PoT.
                if(TOffset + PulsePoTLen >= PoTLen) {
                    TOffsetOK = false;
                    TOffset = PoTLen - PulsePoTLen;
                }
                if (use_transposed_pot) {
                  memcpy(PulsePoT, &PowerSpectrum[ThisPoT * PoTLen + TOffset], PulsePoTLen*sizeof(float));
		} else {
                   for(i = 0; i < PulsePoTLen; i++) {
                       PulsePoT[i] = PowerSpectrum[ThisPoT + (TOffset+i) * FftLength];
                   }
		}

                if(!SkipTriplet) {
                    retval = find_triplets(PulsePoT,
                                           PulsePoTLen,
                                           (float)PoTInfo.TripletThresh,
                                           TOffset,
                                           ThisPoT);
                    if (retval)
                        SETIERROR(retval,"from find_triplets()");
                }

                if(!SkipPulse) {
                    retval = find_pulse(PulsePoT,
                                        PulsePoTLen,
                                        (float)PoTInfo.PulseThresh,
                                        TOffset,
                                        ThisPoT
                                       );
                    if (retval)
                        SETIERROR(retval,"from find_pulse()");
                }

                // At the end of each pulse PoT we update progress.  Progress
                // will thus get updted several times per frequency bin.
#ifdef BOINC_APP_GRAPHICS
                if (sah_graphics)
                {
                    sah_graphics->local_progress = (((ThisPoT-1) * NumPulsePoTs) +
                                                    PulsePoTNum)                   *
                                                   ProgressPerPulsePoT;
                }
#endif
                if(!SkipTriplet) {
                    progress += (ProgressUnitSize * TripletProgressUnits()) /
                                (float)(FftLength - 1) / NumPulsePoTs;
                }
                if(!SkipPulse) {
                    progress += (ProgressUnitSize * PulseProgressUnits(PulsePoTLen, FftLength - 1)) /
                                (float)(FftLength - 1) / NumPulsePoTs;

                }
                progress=std::min(progress,1.0); // prevent display of > 100%
                fraction_done(progress,remaining);

            }  // end loop through time for each frequency

            // At the end of each frequency bin we save state.
            analysis_state.PoT_activity = POT_DOING_PULSE;
            analysis_state.PoT_freq_bin = ThisPoT;
            retval = checkpoint();
            if (retval)
                SETIERROR(retval,"from checkpoint()");

        }   // end loop through frequencies

        analysis_state.PoT_freq_bin = -1;
        analysis_state.PoT_activity = POT_INACTIVE;

    }   // end looking for pulses


    return (retval);   // no error return point
}

int GetFixedPoT(
    float fp_PowerSpectrum[],
    int ul_NumDataPoints,
    int ul_FftLength,
    float fp_PoT[],
    int ul_PoTLen,
    int ul_PoT
) {

    // This routine returns a PoT array of fixed length, regardless
    // of the actual number of time bins in the current data block.

    int    ul_PoT_i,
    ul_PoTChunkSize,
    ul_PoTChunk_i,
    ul_PoTChunkLimit,
    ul_NumSpectra,
    ul_PoTsPerSpec,
    ul_MidPoint,
    ul_BaseSpec,
    ul_SubPoT_i;

    float  f_Factor,
    f_BaseSpecPowerPerPoT    = 0.0,
                               f_AftSpecPowerPerPoT     = 0.0,
                                                          f_ForwardSpecPowerPerPoT = 0.0,
                                                                                     f_BaseWeight;

    ul_NumSpectra    = ul_NumDataPoints / ul_FftLength;
    ul_PoTChunkSize  = ul_NumSpectra / ul_PoTLen;

    // Populate PoT

    // For setiathome_enhanced, the number of spectra is the same as the
    // number of elements in a PoT array more times than not (16K FFT length).
    // For an 0.426 angle range, there are 2.2104e10 cases where they're equal
    // and 2.2082e10 cases where they're not. Copying directly rather than
    // adding also means no FLOP_counter increment is needed.
    if (ul_NumSpectra == ul_PoTLen) {
        if (use_transposed_pot) {
          memcpy(fp_PoT, &fp_PowerSpectrum[ul_PoT * ul_NumSpectra],
               swi.analysis_cfg.gauss_pot_length * sizeof(float));
        } else { 
          for (ul_PoT_i = 0, ul_PoTChunk_i = 0;
                ul_PoT_i < swi.analysis_cfg.gauss_pot_length;
                ul_PoT_i++, ul_PoTChunk_i += ul_FftLength) {
            fp_PoT[ul_PoT_i] = fp_PowerSpectrum[ul_PoT + ul_PoTChunk_i];
          }
	}
        // If the number of spectra is greater than the number
        // of elements in a PoT array, we add sum adjacent spectra
        // into PoT elements.
        // ul_PoTChunkSize indicates how many time-wise
        // power spectra bins are added together to make one PoT bin.
    } else if (ul_NumSpectra > ul_PoTLen) {
        ul_PoTChunk_i = 0;
        for (ul_PoT_i = 0; ul_PoT_i < swi.analysis_cfg.gauss_pot_length; ul_PoT_i++) {
//            fp_PoT[ul_PoT_i] = 0.0;
            ul_PoTChunkLimit = ul_PoTChunk_i + ul_PoTChunkSize;

            // Stuff chunk of power spectra bins into 1 PoT bin
			int blksize = UNSTDMAX(1, UNSTDMIN(pow2((unsigned int) sqrt((float) ((ul_PoTChunkLimit-ul_PoTChunk_i) / 32)) * 32), 512));
			int b;
			float total = 0.0f,partial;
	        if (use_transposed_pot) {
				for(b=0;b<(ul_PoTChunkLimit-ul_PoTChunk_i)/blksize;b++) {
				  partial = 0.0f;
				  for(ul_PoTChunk_i = 0; ul_PoTChunk_i < blksize; ul_PoTChunk_i++) {
					partial += fp_PowerSpectrum[ul_PoT*ul_NumSpectra+b*blksize+ul_PoTChunk_i];
				  }
				  total += partial;
				}
				//catch the tail if needed
				ul_PoTChunk_i = 0;
				while (b*blksize+ul_PoTChunk_i < (ul_PoTChunkLimit-ul_PoTChunk_i))
				{
				  total += fp_PowerSpectrum[ul_PoT*ul_NumSpectra+b*blksize+ul_PoTChunk_i]; 
				  ul_PoTChunk_i++;
				}
	            fp_PoT[ul_PoT_i] = total;
            } else { 
				for(b=0;b<(ul_PoTChunkLimit-ul_PoTChunk_i)/blksize;b++) {
				  partial = 0.0f;
				  for(ul_PoTChunk_i = 0; ul_PoTChunk_i < blksize; ul_PoTChunk_i++) {
					partial += fp_PowerSpectrum[ul_PoT+b*blksize+ul_FftLength*ul_PoTChunk_i];
				  }
				  total += partial;
				}
				//catch the tail if needed
				ul_PoTChunk_i = 0;
				while (b*blksize+ul_PoTChunk_i < (ul_PoTChunkLimit-ul_PoTChunk_i))
				{
				  total += fp_PowerSpectrum[ul_PoT+b*blksize+ul_FftLength*ul_PoTChunk_i]; 
				  ul_PoTChunk_i++;
				}
	            fp_PoT[ul_PoT_i] = total;
            }
        }
        analysis_state.FLOP_counter+=((double)ul_NumSpectra+swi.analysis_cfg.gauss_pot_length);
    } else {
        // If the number of spectra is less than the the number
        // of elements in a PoT array, we smear the power in each
        // spectrum over more than one PoT element.  Further, we
        // perform a linear interpolation to smooth the transition
        // from one spectrum to the next. That is, we allow the
        // spectra before or after our "base" spectrum to effect
        // the powers that go into a given PoT bin.  The closer
        // in time that the PoT bin is to the "before" or "after"
        // spectra, the more that PoT bin is effected by the
        // neighboring spectrum.
        // NOTE : This is all effectively turned off by not letting
        //        gaussfit() see data analyzed with an FFT length
        //        greater than GAUSS_POT_LEN.

        ul_PoTsPerSpec = ul_PoTLen / ul_NumSpectra;
        // Number of PoT bins over which to smear 1 spectrum.
        f_Factor = 1.0f / ul_PoTsPerSpec;
        // For dividing up 1 spectrum's power pre-interpolation.
        ul_MidPoint = ul_PoTsPerSpec / 2;

        for (ul_PoT_i = 0; ul_PoT_i < swi.analysis_cfg.gauss_pot_length; ul_PoT_i++) {
            ul_BaseSpec = int(ul_PoT_i / ul_PoTsPerSpec);
            // The spectrum bounding the current PoT bin.
            ul_SubPoT_i = ul_PoT_i - ul_PoTsPerSpec * ul_BaseSpec;
            // Relative to the start of the base spectrum.
            // Powers pre-interpolation.
            if (use_transposed_pot) {
              f_BaseSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                    * ul_NumSpectra + ul_BaseSpec] * f_Factor;
	    } else {
              f_BaseSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                    + ul_FftLength * ul_BaseSpec] * f_Factor;
            }
            if (ul_BaseSpec > 0) {
                if (use_transposed_pot) {
                  f_AftSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                       * ul_NumSpectra + (ul_BaseSpec-1)] * f_Factor;
                } else {
                  f_AftSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                                        + ul_FftLength * (ul_BaseSpec-1)] * f_Factor;
                }
            }
            if (ul_BaseSpec < ul_NumSpectra - 1) {
                if (use_transposed_pot) {
                  f_ForwardSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                           * ul_NumSpectra + (ul_BaseSpec+1)] * f_Factor;
                } else {
                  f_ForwardSpecPowerPerPoT = fp_PowerSpectrum[ul_PoT
                                           + ul_FftLength * (ul_BaseSpec+1)] * f_Factor;
                }
            }

            // Begin interpolation.
            if (ul_SubPoT_i < ul_MidPoint) {
                // Use the Aft spectrum if there is one.
                // If not, use non-interpoated powers.
                if (ul_BaseSpec == 0) {
                    fp_PoT[ul_PoT_i] = f_BaseSpecPowerPerPoT;
                } else {
                    f_BaseWeight = (float)(ul_PoTsPerSpec - (ul_MidPoint - ul_SubPoT_i)) / ul_PoTsPerSpec;
                    fp_PoT[ul_PoT_i] = (f_BaseSpecPowerPerPoT * f_BaseWeight)
                                       + (f_AftSpecPowerPerPoT * (1.0f - f_BaseWeight));
                }
            } else {
                // Use the Forward spectrum if there is one.
                // If not, use non-interpoated powers.
                if (ul_BaseSpec == ul_NumSpectra-1) {
                    fp_PoT[ul_PoT_i] = f_BaseSpecPowerPerPoT;
                } else {
                    f_BaseWeight = (float)(ul_PoTsPerSpec - (ul_SubPoT_i - ul_MidPoint)) / ul_PoTsPerSpec;
                    fp_PoT[ul_PoT_i] = (f_BaseSpecPowerPerPoT * f_BaseWeight)
                                       + (f_ForwardSpecPowerPerPoT * (1.0f - f_BaseWeight));
                }
            }
        } // end for ul_PoT_i < swi.analysis_cfg.gauss_pot_length
        analysis_state.FLOP_counter+=(double)(9*swi.analysis_cfg.gauss_pot_length);
    }
    return 0;
}

void ComputePoTInfo(int num_cfft, int NumDataPoints) {

    // Tis routine calculates various PoT values based on program constants
    // and data contained in the current WU.

    // A note on "sigma", calculated here and used by the gaussian finder:
    // The sigma calculated here is in units of PoT, ie, in bins of the
    // Power Of Time arrays.  Gaussian PoT arrays are of fixed length, given by
    // GAUSS_POT_LEN.  The basic definition of sigma here is that it is the
    // amount of time it takes to cover 1/2 the half-power-beamwidth of the
    // serendip feed.  This is also a constant - BEAM_WIDTH - in units of
    // degrees.  We scan BEAM_WIDTH degrees at a given, but variable, slew rate.


    volatile double DegPerPotBin;    // slew rate in degrees per PoT bin
    volatile float ftmp;             // temporary to ensure float rounding
    int i, MinFftLen;

    // jeffc - is there really any need for these PoTInfo fields
    // at this point?
    PoTInfo.min_slew  = swi.analysis_cfg.pot_min_slew;
    PoTInfo.max_slew  = swi.analysis_cfg.pot_max_slew;
    PoTInfo.PulseOverlapFactor = swi.analysis_cfg.pot_overlap_factor; // ?
    PoTInfo.PulseBeams  = swi.analysis_cfg.pulse_beams;
    PoTInfo.PulseThresh  = swi.analysis_cfg.pulse_thresh;
    PoTInfo.PulseMax  = swi.analysis_cfg.pulse_max;
    PoTInfo.PulseMin  = swi.analysis_cfg.pulse_min;
    PoTInfo.PulseFftMax  = swi.analysis_cfg.pulse_fft_max;
    PoTInfo.TripletThresh  = swi.analysis_cfg.triplet_thresh;
    PoTInfo.TripletMax  = swi.analysis_cfg.triplet_max;
    PoTInfo.TripletMin  = swi.analysis_cfg.triplet_min;
    PoTInfo.GaussChiSqThresh      = swi.analysis_cfg.gauss_chi_sq_thresh;
    PoTInfo.GaussPowerThresh      = swi.analysis_cfg.gauss_power_thresh;
    PoTInfo.GaussPeakPowerThresh  = swi.analysis_cfg.gauss_peak_power_thresh;

    // Find the max fft length in the chirp/fft table
    for(i = 1, MinFftLen = ChirpFftPairs[0].FftLen; i < num_cfft; i++) {
        if(ChirpFftPairs[i].FftLen < MinFftLen) {
            MinFftLen = ChirpFftPairs[i].FftLen;
        }
    }

    PoTInfo.MaxPoTLen  = NumDataPoints / MinFftLen;
    ftmp               = (float)(swi.nsamples / swi.subband_sample_rate);
    PoTInfo.WUDuration = ftmp;

    // Here we calculate slew rate and slew related values.
    // If there is no telescope movement, slew related values remain
    // in their initialized state (zero).
    if(swi.angle_range != 0.0) {
        ftmp                        = (float)(swi.angle_range / PoTInfo.WUDuration);
        PoTInfo.SlewRate            = ftmp;
        ftmp                        = (float)(PoTInfo.SlewRate / swi.beam_width);
        PoTInfo.BeamRate            = ftmp;
        DegPerPotBin                = swi.angle_range / swi.analysis_cfg.gauss_pot_length;
        ftmp                        = (float)((swi.beam_width/2.0f) / DegPerPotBin);
        PoTInfo.GaussSigma          = ftmp;
        PoTInfo.GaussSigmaSq        = PoTInfo.GaussSigma * PoTInfo.GaussSigma;
        PoTInfo.GaussTOffsetStart   = static_cast<int>(floor(swi.analysis_cfg.pot_t_offset * PoTInfo.GaussSigma+0.5));
        PoTInfo.GaussTOffsetStop    = swi.analysis_cfg.gauss_pot_length - PoTInfo.GaussTOffsetStart;
    }
}

void GetPulsePoTLen(long FullPoTLen, int * PulsePoTLen, int * PulseOverlap) {

    // This routine, given the time axis length of a WU in bins and data available
    // in the PoTInfo struct, will calculate how many time bins to pass the pulse
    // detectors for the given time axis length.  Time axis length in seconds is
    // is constant, while length in bins is inversely proportional to FFT length.

    // If slew rate is so low that, over the duration of the entire WU,
    // we do not cover the number of beams passed to each call of the
    // pulse finders then assume that we will be passing the entire PoT
    // to the pulse finders.  This also takes care of the case where slew
    // rate is zero.  Otherwise, calculate that portion of the PoT that
    // we will be passing.

    // NOTE: the code calling this routine may be concerned with either pulse finding
    // or triplet finding.  There may be different max and/or min PoT length constraints
    // between these 2 algorithms.  Thus, this routine just returns the length that
    // will contain PoTInfo.PulseBeams worth of data.  It is up to the calling code
    // to then apply any length constaints.

    double       BinRate,                // function of time resolution and slew rate
    BinsPerBeam;            // function of time res, slew rate, and beam width

    // jeffc - the following line in temporary.  Pulse beams will go into the
    // analysis config.
    //PoTInfo.PulseBeams = 1.0;

    BinRate             = FullPoTLen / PoTInfo.WUDuration;             // in bins/second

    if(PoTInfo.BeamRate * PoTInfo.WUDuration < PoTInfo.PulseBeams) {
        BinsPerBeam     = (float)FullPoTLen;    // slow slew - pass FullPoT
    } else {
        BinsPerBeam     = BinRate / PoTInfo.BeamRate;           // we will pass a portion of FullPoT
    }

    *PulsePoTLen         = IROUND(BinsPerBeam * PoTInfo.PulseBeams);              // in bins
    if(*PulsePoTLen > FullPoTLen) {
        *PulsePoTLen = FullPoTLen;    // pulse PoT longer than full PoT
    }        // ... so pass as much as we can
    // ... ie the full PoT

    *PulseOverlap        = IROUND(BinsPerBeam * PoTInfo.PulseOverlapFactor);      // in bins

#ifdef DEBUG_POT

    fprintf(stderr, "BinRate = %f\n", BinRate);
    fprintf(stderr, "BinsPerBeam = %f\n", BinsPerBeam);
#endif

}

