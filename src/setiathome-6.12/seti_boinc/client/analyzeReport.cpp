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

// analyzeReport.C
// $Id: analyzeReport.cpp,v 1.40.2.9 2007/05/31 22:03:09 korpela Exp $
//

#include "sah_config.h"

#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "diagnostics.h"
#include "util.h"
#include "s_util.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

#include "analyze.h"
#include "gaussfit.h"
#include "seti.h"
#include "chirpfft.h"
#include "analyzeReport.h"
#include "analyzePoT.h"

#include "../db/schema_master.h"

// outfile will be flushed at checkpoint time and just before
// normal program exit.
MFILE outfile;

SPIKE_INFO * best_spike;
GAUSS_INFO * best_gauss;
PULSE_INFO * best_pulse;
TRIPLET_INFO * best_triplet;

int signal_count = 0;
int spike_count = 0;
int pulse_count = 0;
int triplet_count = 0;
int gaussian_count = 0;

void reload_graphics_state() {
#ifdef BOINC_APP_GRAPHICS
    if (!nographics()) {
      sah_graphics->si.copy(best_spike, true);
      sah_graphics->gi.copy(best_gauss, true);
      sah_graphics->pi.copy(best_pulse, true);
      sah_graphics->ti.copy(best_triplet, true);
    }
#endif
}

void reset_high_scores() {
  if(best_spike) delete(best_spike);
  if(best_gauss) delete(best_gauss);
  if(best_pulse) delete(best_pulse);
  if(best_triplet) delete(best_triplet);

  best_spike = new(SPIKE_INFO);
  best_gauss = new(GAUSS_INFO);
  best_gauss->score=-12;
  best_pulse = new(PULSE_INFO);
  best_triplet = new(TRIPLET_INFO);
}

static double interpol(double x1,double y1,double x2,double y2, double x) {
  double slope=(y2-y1)/(x2-x1);
  return slope*(x-x1)+y1;
}

void time_to_ra_dec(double time_jd, double *ra, double *dec) {
  int i=0;
  double raoffs=0;

  while ((i<swi.num_positions-2) && (swi.position_history[i+1].time<time_jd)) {
    i++;
  }

  if ((swi.position_history[i+1].ra-swi.position_history[i].ra)> 12.0) {
    raoffs=-24.0;
  }
  if ((swi.position_history[i].ra-swi.position_history[i+1].ra)> 12.0) {
    raoffs=24.0;
  }

  *ra=interpol(
        swi.position_history[i].time,
        swi.position_history[i].ra,
        swi.position_history[i+1].time,
        swi.position_history[i+1].ra+raoffs,
        time_jd
      );
  if (*ra<0) {
    *ra+=24;
  } else if (*ra>24) {
    *ra-=24;
  }

  *dec=interpol(
         swi.position_history[i].time,
         swi.position_history[i].dec,
         swi.position_history[i+1].time,
         swi.position_history[i+1].dec,
         time_jd
       );
}


int result_spike(SPIKE_INFO &si) {

  int retval=0;

  if (signal_count >= swi.analysis_cfg.max_signals) {
    SETIERROR(RESULT_OVERFLOW,"in result_spike");
  }

  retval = outfile.printf("%s", si.s.print_xml(0,0,1).c_str());

  if (retval < 0) {
    SETIERROR(WRITE_FAILED,"in result_spike");
  } else {
    signal_count++;
    spike_count++;
  }

  return 0;
}

int result_gaussian(GAUSS_INFO &gi) {

  int retval=0;

  if (signal_count > swi.analysis_cfg.max_signals) {
    SETIERROR(RESULT_OVERFLOW,"in result_gaussian");
  }

  retval = outfile.printf("%s", gi.g.print_xml(0,0,1).c_str());

  if (retval >= 0) {
    retval= outfile.printf("\n");
  }

  if (retval < 0) {
    SETIERROR(WRITE_FAILED,"in result_gaussian");
  } else {
    signal_count++;
    gaussian_count++;
  }

  return 0;
}


int ReportTripletEvent(
  float Power, float MeanPower, float period,
  float mid_time_bin, int start_time_bin, int freq_bin,
  int pot_len,const float *PoT, int write_triplet
) {
  TRIPLET_INFO ti;
  triplet triplet;
  int retval=0, i, j;
  double step,norm,index;
  double max_power=0;
  static int * inv;

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  if (!inv) inv = (int*)calloc_a(swi.analysis_cfg.triplet_pot_length, sizeof(int), MEM_ALIGN);

  // triplet info
  ti.score=Power;
  ti.t.peak_power=Power;
  ti.t.mean_power=MeanPower;
  ti.freq_bin=freq_bin;
  ti.time_bin=mid_time_bin+start_time_bin+0.5f;
  ti.t.chirp_rate=ChirpFftPairs[analysis_state.icfft].ChirpRate;
  ti.t.fft_len=ChirpFftPairs[analysis_state.icfft].FftLen;
  ti.bperiod=period;
  ti.t.period=static_cast<float>(period*static_cast<double>(ti.t.fft_len)/swi.subband_sample_rate);
  ti.t.freq=cnvt_bin_hz(freq_bin, ti.t.fft_len);
  double t_offset=(static_cast<double>(mid_time_bin)+start_time_bin+0.5)
      *static_cast<double>(ti.t.fft_len)/
         swi.subband_sample_rate;
  ti.t.detection_freq=calc_detection_freq(ti.t.freq,ti.t.chirp_rate,t_offset);
  ti.t.time=swi.time_recorded+t_offset/86400.0;
  time_to_ra_dec(ti.t.time, &ti.t.ra, &ti.t.decl);

  // Populate the min and max PoT arrays.  These are only used
  // for graphics.
  memset(ti.pot_min,0xff,swi.analysis_cfg.triplet_pot_length*sizeof(int));
  memset(ti.pot_max,0,swi.analysis_cfg.triplet_pot_length*sizeof(int));
  step=static_cast<double>(pot_len)/swi.analysis_cfg.triplet_pot_length;
  ti.scale=static_cast<float>(1.0/step);
  index=0;
  for (i=0;i<pot_len;i++) {
    if (PoT[i]>max_power) max_power=PoT[i];
  }
  norm=255.0/max_power;
  float mtb = mid_time_bin;
  if (pot_len > swi.analysis_cfg.triplet_pot_length) {
    ti.tpotind0_0 = ti.tpotind0_1 = static_cast<int>(((mtb-period)*swi.analysis_cfg.triplet_pot_length)/pot_len);
    ti.tpotind1_0 = ti.tpotind1_1 = static_cast<int>(((mtb)*swi.analysis_cfg.triplet_pot_length)/pot_len);
    ti.tpotind2_0 = ti.tpotind2_1 = static_cast<int>(((mtb+period)*swi.analysis_cfg.triplet_pot_length)/pot_len);
    for (j=0; j<pot_len; j++) {
      i = (j*swi.analysis_cfg.triplet_pot_length)/pot_len;
      if ((PoT[j]*norm)<ti.pot_min[i]) {
        ti.pot_min[i]=static_cast<unsigned int>(floor(PoT[j]*norm));
      }
      if ((PoT[j]*norm)>ti.pot_max[i]) {
        ti.pot_max[i]=static_cast<unsigned int>(floor(PoT[j]*norm));
      }
    }
  } else {
    memset(inv, -1, sizeof(inv));
    for (i=0;i<swi.analysis_cfg.triplet_pot_length;i++) {
      j = (i*pot_len)/swi.analysis_cfg.triplet_pot_length;
      if (inv[j] < 0) inv[j] = i;
      if ((PoT[j]*norm)<ti.pot_min[i]) {
        ti.pot_min[i]=static_cast<unsigned int>(floor(PoT[j]*norm));
      }
      if ((PoT[j]*norm)>ti.pot_max[i]) {
        ti.pot_max[i]=static_cast<unsigned int>(floor(PoT[j]*norm));
      }
    }
    ti.tpotind0_0 = inv[static_cast<int>(mtb-period)];
    ti.tpotind0_1 = inv[static_cast<int>(mtb-period+1)];
    ti.tpotind1_0 = (inv[static_cast<int>(mtb)]+inv[static_cast<int>(mtb+1)])/2;
    ti.tpotind1_1 = (inv[static_cast<int>(mtb+1)]+inv[static_cast<int>(mtb+2)])/2;
    ti.tpotind2_0 = inv[static_cast<int>(mtb+period)];
    if (mtb+period+1 >= pot_len) ti.tpotind2_1 = swi.analysis_cfg.triplet_pot_length-1;
    else ti.tpotind2_1 = inv[static_cast<int>(mtb+period+1)];
  }

  // Update sah_graphics triplet info regardless of whether it is the
  // best thus far.  If a triplet has made it this far, display it.
#ifdef BOINC_APP_GRAPHICS
    if (!nographics()) sah_graphics->ti.copy(&ti);
#endif

  // best thus far ?
  if (ti.score>best_triplet->score) {
    *best_triplet=ti;
  }


  if (write_triplet) {

    if (signal_count > swi.analysis_cfg.max_signals) {
      SETIERROR(RESULT_OVERFLOW,"in ReportTripletEvent");
    }

    retval = outfile.printf("%s", ti.t.print_xml(0,0,1).c_str());

    if (retval < 0) {
      SETIERROR(WRITE_FAILED,"in ReportTripletEvent");
    } else {
      signal_count++;
      triplet_count++;
    }

  }

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  return(retval);
}

int ReportPulseEvent(float PulsePower,float MeanPower, float period,
                     int time_bin,int freq_bin, float snr, float thresh, float *folded_pot,
                     int scale, int write_pulse) {
  PULSE_INFO pi;
  pulse pulse;
  int retval=0, i, len_prof=static_cast<int>(floor(period));
  float step,norm,index,MinPower=PulsePower*MeanPower*scale;

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  // pulse info
  pi.score=snr/thresh;
  pi.p.peak_power=PulsePower-1;
  pi.p.mean_power=MeanPower;
  pi.p.fft_len=ChirpFftPairs[analysis_state.icfft].FftLen;
  pi.p.chirp_rate=ChirpFftPairs[analysis_state.icfft].ChirpRate;
  pi.p.period=static_cast<float>(period*static_cast<double>(pi.p.fft_len)/swi.subband_sample_rate);
  pi.p.snr = snr;
  pi.p.thresh = thresh;
  pi.p.len_prof = len_prof;
  pi.freq_bin=freq_bin;
  pi.time_bin=time_bin;
  pi.p.freq=cnvt_bin_hz(freq_bin, pi.p.fft_len);
  double t_offset=(static_cast<double>(time_bin)+0.5)
       *static_cast<double>(pi.p.fft_len)/
         swi.subband_sample_rate;
  pi.p.detection_freq=calc_detection_freq(pi.p.freq,pi.p.chirp_rate,t_offset);
  pi.p.time=swi.time_recorded+t_offset/86400.0;
  time_to_ra_dec(pi.p.time, &pi.p.ra, &pi.p.decl);

  for (i=0;i<len_prof;i++) {
    if (folded_pot[i]<MinPower) MinPower=folded_pot[i];
  }  
  norm=255.0f/((PulsePower*MeanPower*scale-MinPower));
  
  // Populate the min and max PoT arrays.  These are only used
  // for graphics.
#ifdef BOINC_APP_GRAPHICS
  if (!nographics()) {
    step=static_cast<float>(len_prof)/swi.analysis_cfg.pulse_pot_length;
    index=0;
    for (i=0;i<swi.analysis_cfg.pulse_pot_length;i++) {
      pi.pot_min[i]=255;
      pi.pot_max[i]=0;
      int j;
      for (j=0; j<step; j++) {
        unsigned int pot = static_cast<unsigned int>((folded_pot[static_cast<int>(floor(index))+j]-MinPower)*norm);
        if (pot<pi.pot_min[i]) {
          pi.pot_min[i]=pot;
        }
        if (pi.pot_min[i] >= 256) pi.pot_min[i] = 255; // kludge until we fix the assert failures
        BOINCASSERT(pi.pot_min[i] < 256);
        if (pot>pi.pot_max[i])
          pi.pot_max[i]=pot;
        if (pi.pot_max[i] >= 256) pi.pot_max[i] = 255; // kludge until we fix the assert failures
        BOINCASSERT(pi.pot_max[i] < 256);
      }
      index+=step;
    }
  }
#endif

  // Populate the result PoT if the folded PoT will fit.
  if (pi.p.len_prof < swi.analysis_cfg.pulse_pot_length) {
	pi.p.pot.resize(len_prof);
  	for (i=0;i<len_prof;i++) {
		pi.p.pot[i] = (unsigned char)((folded_pot[i]-MinPower)*norm);
  	}
  } else {
    pi.p.pot.clear();
  }

  // Update gdata pulse info regardless of whether it is the
  // best thus far.  If a pulse has made it this far, display it.
#ifdef BOINC_APP_GRAPHICS
    if (!nographics()) sah_graphics->pi.copy(&pi);
#endif

  // best thus far ?
  if (pi.score>best_pulse->score) {
    *best_pulse=pi;
  }

  if (write_pulse) {

    if (signal_count > swi.analysis_cfg.max_signals) {
      SETIERROR(RESULT_OVERFLOW,"in ReportPulseEvent");
    }

    //for (i=0;i<len_prof;i++) {
//	sprintf(&pi.p.pot[i], "%02x",(int)((folded_pot[i]-MinPower)*norm));
 //   }

    retval = outfile.printf("%s", pi.p.print_xml(0,0,1).c_str());

    if (retval >= 0) {
      outfile.printf("\n");
    }

    if (retval < 0) {
      SETIERROR(WRITE_FAILED,"in ReportPulseEvent");
    } else {
      signal_count++;
      pulse_count++;
    }

  }

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif


  return(retval);
}

