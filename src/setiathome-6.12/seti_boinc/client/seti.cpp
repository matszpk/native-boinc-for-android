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

// seti.C
// $Id: seti.cpp,v 1.60.2.19 2007/08/10 00:38:49 korpela Exp $
//
// SETI-specific logic for reading/writing checkpoint files
// and parsing WU files.
//
// Files:
// state.txt
// the last chirp rate / FFT len finished, if any
// outfile.txt
// The append-only result file
//
// What finally gets sent back as the result file is:
// - the generic result header, with the final CPU time filled in
// - the SETI-specific part of the WU header
// - the contents of outfile.txt

#include "sah_config.h"

#include <cstdio>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "boinc_api.h"
#include "str_util.h"
#include "str_replace.h"

#include "diagnostics.h"
#include "parse.h"
#include "filesys.h"
#include "s_util.h"
#include "seti_header.h"
#include "analyze.h"
#include "analyzeFuncs.h"
#include "analyzeReport.h"
#include "analyzePoT.h"
#include "malloc_a.h"
#include "chirpfft.h"
#include "worker.h"

#include "seti.h"

#define DATA_ASCII 1
#define DATA_ENCODED 2
#define DATA_SUN_BINARY 3

APP_INIT_DATA app_init_data;

ANALYSIS_STATE analysis_state;
static int wrote_header;
double LOAD_STORE_ADJUSTMENT=2.85;
double SETUP_FLOPS=1.42e+10/2.85;

SPIKE_INFO::SPIKE_INFO() : track_mem<SPIKE_INFO>("SPIKE_INFO"),s(), score(0), bin(0), fft_ind(0) {}
 
SPIKE_INFO::~SPIKE_INFO() {}

SPIKE_INFO::SPIKE_INFO(const SPIKE_INFO &si) : track_mem<SPIKE_INFO>("SPIKE_INFO"),
s(si.s), score(si.score), bin(si.bin), fft_ind(si.fft_ind) {}

SPIKE_INFO &SPIKE_INFO::operator =(const SPIKE_INFO &si) {
  if (&si != this) {
    s=si.s;
    score=si.score;
    bin=si.bin;
    fft_ind=si.fft_ind;
  }
  return *this;
}


GAUSS_INFO::GAUSS_INFO() : track_mem<GAUSS_INFO>("GAUSS_INFO"),
g(), score(0), display_power_thresh(0), bin(0), fft_ind(0) {
}

GAUSS_INFO::~GAUSS_INFO() {}

GAUSS_INFO::GAUSS_INFO(const GAUSS_INFO &gi) : track_mem<GAUSS_INFO>("GAUSS_INFO"),
g(gi.g), score(gi.score), bin(gi.bin), fft_ind(gi.fft_ind) {
}

GAUSS_INFO &GAUSS_INFO::operator =(const GAUSS_INFO &gi) {
  if (&gi != this) {
    g=gi.g;
    score=gi.score;
    bin=gi.bin;
    fft_ind=gi.fft_ind;
  }
  return *this;
}

PULSE_INFO::PULSE_INFO() : track_mem<PULSE_INFO>("PULSE_INFO"), p(), score(0), freq_bin(0), time_bin(0) {

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    pot_min = (unsigned int *)calloc_a(swi.analysis_cfg.pulse_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_min == NULL) SETIERROR(MALLOC_FAILED, "new PULSE_INFO pot_min == NULL");
    pot_max = (unsigned int *)calloc_a(swi.analysis_cfg.pulse_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_max == NULL) SETIERROR(MALLOC_FAILED, "new PULSE_INFO pot_max == NULL");

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}


PULSE_INFO::~PULSE_INFO() { 
// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    free_a(pot_min); 
    free_a(pot_max); 

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}


PULSE_INFO::PULSE_INFO(const PULSE_INFO &pi) : track_mem<PULSE_INFO>("PULSE_INFO"),  
p(pi.p), score(pi.score), freq_bin(pi.freq_bin), time_bin(pi.time_bin) {

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    pot_min = (unsigned int *)calloc_a(swi.analysis_cfg.pulse_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_min == NULL) SETIERROR(MALLOC_FAILED, "copied PULSE_INFO pot_min == NULL");
    pot_max = (unsigned int *)calloc_a(swi.analysis_cfg.pulse_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_max == NULL) SETIERROR(MALLOC_FAILED, "copied PULSE_INFO pot_max == NULL");

    memcpy(pot_min,pi.pot_min,(swi.analysis_cfg.pulse_pot_length * sizeof(unsigned int)));
    memcpy(pot_max,pi.pot_max,(swi.analysis_cfg.pulse_pot_length * sizeof(unsigned int)));

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}

PULSE_INFO &PULSE_INFO::operator =(const PULSE_INFO &pi) {

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  if (&pi != this) {
    p=pi.p;
    score=pi.score;
    freq_bin=pi.freq_bin;
    time_bin=pi.time_bin;

    memcpy(pot_min,pi.pot_min,(swi.analysis_cfg.pulse_pot_length * sizeof(unsigned int)));
    memcpy(pot_max,pi.pot_max,(swi.analysis_cfg.pulse_pot_length * sizeof(unsigned int)));
  }

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  return *this;
}

TRIPLET_INFO::TRIPLET_INFO() : track_mem<TRIPLET_INFO>("TRIPLET_INFO"),
t(), score(0), bperiod(0), freq_bin(0), tpotind0_0(0), tpotind0_1(0), 
tpotind1_0(0), tpotind1_1(0), tpotind2_0(0), tpotind2_1(0), time_bin(0), 
scale(0) {

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    pot_min = (unsigned int *)calloc_a(swi.analysis_cfg.triplet_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_min == NULL) SETIERROR(MALLOC_FAILED, "new TRIPLET_INFO pot_min == NULL");
    pot_max = (unsigned int *)calloc_a(swi.analysis_cfg.triplet_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_max == NULL) SETIERROR(MALLOC_FAILED, "new TRIPLET_INFO pot_max == NULL");

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}

TRIPLET_INFO::TRIPLET_INFO(const TRIPLET_INFO &ti) : track_mem<TRIPLET_INFO>("TRIPLET_INFO"), t(ti.t), score(ti.score), 

    bperiod(ti.bperiod), freq_bin(ti.freq_bin), 
    tpotind0_0(ti.tpotind0_0), tpotind0_1(ti.tpotind0_1),
    tpotind1_0(ti.tpotind1_0), tpotind1_1(ti.tpotind1_1),
    tpotind2_0(ti.tpotind2_0), tpotind2_1(ti.tpotind2_1), 
    time_bin(ti.time_bin), scale(ti.scale)

{

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    pot_min = (unsigned int *)calloc_a(swi.analysis_cfg.triplet_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_min == NULL) SETIERROR(MALLOC_FAILED, "copied TRIPLET_INFO pot_min == NULL");
    pot_max = (unsigned int *)calloc_a(swi.analysis_cfg.triplet_pot_length, sizeof(unsigned int), MEM_ALIGN);
    if (pot_max == NULL) SETIERROR(MALLOC_FAILED, "copied TRIPLET_INFO pot_max == NULL");
    memcpy(pot_min,ti.pot_min,(swi.analysis_cfg.triplet_pot_length * sizeof(unsigned int)));
    memcpy(pot_max,ti.pot_max,(swi.analysis_cfg.triplet_pot_length * sizeof(unsigned int)));

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}    

TRIPLET_INFO &TRIPLET_INFO::operator =(const TRIPLET_INFO &ti) {

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  if (&ti != this) {
    t=ti.t;
    score=ti.score;
    bperiod=ti.bperiod;
    freq_bin=ti.freq_bin;
    tpotind0_0=ti.tpotind0_0;
    tpotind0_1=ti.tpotind0_1;
    tpotind1_0=ti.tpotind1_0;
    tpotind1_1=ti.tpotind1_1;
    tpotind2_0=ti.tpotind2_0;
    tpotind2_1=ti.tpotind2_1;
    time_bin=ti.time_bin;

    memcpy(pot_min,ti.pot_min,(swi.analysis_cfg.triplet_pot_length * sizeof(unsigned int)));
    memcpy(pot_max,ti.pot_max,(swi.analysis_cfg.triplet_pot_length * sizeof(unsigned int)));
  }

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  return *this;
}


TRIPLET_INFO::~TRIPLET_INFO() { 

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

    free_a(pot_min); 
    free_a(pot_max); 

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

}


// Do SETI-specific initialization for a work unit.
// - prepare outfile for appending
// - reset scores
int seti_init_state() {
  analysis_state.icfft= 0;
  analysis_state.PoT_freq_bin = -1;
  analysis_state.PoT_activity = POT_INACTIVE;
  progress = 0.0;
  reset_high_scores();
  std::string path;

  boinc_resolve_filename_s(OUTFILE_FILENAME, path);
  if (outfile.open(path.c_str(), "ab")) SETIERROR(CANT_CREATE_FILE," in seti_init_state()");

  return 0;
}

// This gets called at the start of each chirp/fft pair.
int result_group_start() {
  wrote_header = 0;
  return 0;
}

// This gets called prior to writing an entry in the output file.
// Write the chirp/fft parameters if not done already.

int result_group_write_header() {
  if (wrote_header) return 0;
  wrote_header = 1;
  int retval = outfile.printf( "<ogh ncfft=%d cr=%e fl=%d>\n",
                               analysis_state.icfft,
                               ChirpFftPairs[analysis_state.icfft].ChirpRate,
                               ChirpFftPairs[analysis_state.icfft].FftLen
                             );
  if (retval < 0) SETIERROR(WRITE_FAILED,"in result_group_write_header()");
  return 0;
}


// this gets called at the end of each chirp/fft pair
// Rewrite the state file,
// and write an end-of-group record to log file
// if anything was written for this group.
int result_group_end() {
  int retval=0;

  if (wrote_header) {
    retval = outfile.printf( "<ogt ncfft=%d cr=%e fl=%d>\n",
                             analysis_state.icfft,
                             ChirpFftPairs[analysis_state.icfft].ChirpRate,
                             ChirpFftPairs[analysis_state.icfft].FftLen);
    if (retval < 0) SETIERROR(WRITE_FAILED,"in result_group_end");
  }

  return 0;
}

int checkpoint(BOOLEAN force_checkpoint) {

  int retval=0, i, l=xml_indent_level;
  xml_indent_level=0;
  char buf[2048];
  std::string enc_field, str;

  // The user may have set preferences for a long time between
  // checkpoints to reduce disk access.
  if (!force_checkpoint) {
    if (!boinc_time_to_checkpoint()) return CHECKPOINT_SKIPPED;
  }

  fflush(stderr);
  /* should be in this place!
   * in the Android or non-glibc malloc it causes huge system mmap2 overhead
   */
  MFILE state_file;
  
// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  if (state_file.open(STATE_FILENAME, "wb")) SETIERROR(CANT_CREATE_FILE,"in checkpoint()");
  retval = state_file.printf(
             "<ncfft>%d</ncfft>\n"
             "<cr>%e</cr>\n"
             "<fl>%d</fl>\n"
             "<prog>%.8f</prog>\n"
             "<potfreq>%d</potfreq>\n"
             "<potactivity>%d</potactivity>\n"
             "<signal_count>%d</signal_count>\n"
             "<flops>%f</flops>\n"
             "<spike_count>%d</spike_count>\n"
             "<pulse_count>%d</pulse_count>\n"
             "<gaussian_count>%d</gaussian_count>\n"
             "<triplet_count>%d</triplet_count>\n",
             analysis_state.icfft,
             ChirpFftPairs[analysis_state.icfft].ChirpRate,
             ChirpFftPairs[analysis_state.icfft].FftLen,
	     std::min(progress,0.9999999),
             analysis_state.PoT_freq_bin,
             analysis_state.PoT_activity,
             signal_count,
             analysis_state.FLOP_counter,
             spike_count,
             pulse_count,
             gaussian_count,
             triplet_count
           );
  if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");

  // checkpoint the best spike thus far (if any)
  if(best_spike->score) {
    retval = state_file.printf("<best_spike>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    // the spike proper
    str = best_spike->s.print_xml(0,0,1);
    retval = (int)state_file.write(str.c_str(), str.size(), 1);
    // ancillary data
    retval = state_file.printf(
               "<bs_score>%f</bs_score>\n"
               "<bs_bin>%d</bs_bin>\n"
               "<bs_fft_ind>%d</bs_fft_ind>\n",
               best_spike->score,
               best_spike->bin,
               best_spike->fft_ind);
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    retval = state_file.printf("</best_spike>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
  }

  // checkpoint the best gaussian thus far (if any)
  if(best_gauss->score) {
    retval = state_file.printf("<best_gaussian>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    // the gaussian proper 
    str = best_gauss->g.print_xml(0,0,1);
    retval = (int)state_file.write(str.c_str(), str.size(), 1);
    // ancillary data
    retval = state_file.printf(
               "<bg_score>%f</bg_score>\n"
               "<bg_display_power_thresh>%f</bg_display_power_thresh>\n"
               "<bg_bin>%d</bg_bin>\n"
               "<bg_fft_ind>%d</bg_fft_ind>\n",
               best_gauss->score,
               best_gauss->display_power_thresh,
               best_gauss->bin,
               best_gauss->fft_ind);
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    retval = state_file.printf("</best_gaussian>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
  }

  // checkpoint the best pulse thus far (if any)
  // The check for len_prof is a kludge.
  if(best_pulse->score) {
    retval = state_file.printf("<best_pulse>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    // the pulse proper 
    str = best_pulse->p.print_xml(0,0,1);
    retval = (int)state_file.write(str.c_str(), str.size(), 1);
    // ancillary data
    retval = state_file.printf(
               "<bp_score>%f</bp_score>\n"
               "<bp_freq_bin>%d</bp_freq_bin>\n"
               "<bp_time_bin>%d</bp_time_bin>\n",
               best_pulse->score,
               best_pulse->freq_bin,
               best_pulse->time_bin);
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    retval = state_file.printf("</best_pulse>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
  }

  // checkpoint the best triplet thus far (if any)
  if(best_triplet->score) {
    retval = state_file.printf("<best_triplet>\n");
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
    // the triplet proper 
    str = best_triplet->t.print_xml(0,0,1);
    retval = (int)state_file.write(str.c_str(), str.size(), 1);

    // ancillary data
    retval = state_file.printf(
               "<bt_score>%f</bt_score>\n"
               "<bt_bperiod>%f</bt_bperiod>\n"
               "<bt_tpotind0_0>%d</bt_tpotind0_0>\n"
               "<bt_tpotind0_1>%d</bt_tpotind0_1>\n"
               "<bt_tpotind1_0>%d</bt_tpotind1_0>\n"
               "<bt_tpotind1_1>%d</bt_tpotind1_1>\n"
               "<bt_tpotind2_0>%d</bt_tpotind2_0>\n"
               "<bt_tpotind2_1>%d</bt_tpotind2_1>\n"
               "<bt_freq_bin>%d</bt_freq_bin>\n"
               "<bt_time_bin>%f</bt_time_bin>\n"
               "<bt_scale>%f</bt_scale>\n",
               best_triplet->score,
               best_triplet->bperiod,
               best_triplet->tpotind0_0,
               best_triplet->tpotind0_1,
               best_triplet->tpotind1_0,
               best_triplet->tpotind1_1,
               best_triplet->tpotind2_0,
               best_triplet->tpotind2_1,
               best_triplet->freq_bin,
               best_triplet->time_bin,
               best_triplet->scale);
    if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");

   // convert min PoT to chars, encode, and print
   for (i=0; i<swi.analysis_cfg.triplet_pot_length; i++) {
	buf[i] = (unsigned char)best_triplet->pot_min[i];
   }
   enc_field=xml_encode_string(buf, swi.analysis_cfg.triplet_pot_length, _x_csv);
   retval = state_file.printf(
         "<bt_pot_min length=%d encoding=\"%s\">",
		 enc_field.size(), xml_encoding_names[_x_csv]
   );
   if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
   state_file.write(enc_field.c_str(), enc_field.size(), 1);
   retval = state_file.printf("</bt_pot_min>\n");
   if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");

   // convert max PoT to chars, encode, and print
   for (i=0; i<swi.analysis_cfg.triplet_pot_length; i++) {
	buf[i] = (unsigned char)best_triplet->pot_max[i];
   }
   enc_field=xml_encode_string(buf, swi.analysis_cfg.triplet_pot_length, _x_csv);
   state_file.printf(
       "<bt_pot_max length=%d encoding=\"%s\">",
	 enc_field.size(), xml_encoding_names[_x_csv]
   );
   if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
   state_file.write(enc_field.c_str(), enc_field.size(), 1);
   retval = state_file.printf("</bt_pot_max>\n");
   if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
   retval = state_file.printf("</best_triplet>\n");
   if (retval < 0) SETIERROR(WRITE_FAILED,"in checkpoint()");
  }

  // The result (outfile) and state mfiles are now synchronized.
  // Flush them both.
  retval = outfile.flush();
  if (retval) SETIERROR(WRITE_FAILED,"in checkpoint()");
  retval = state_file.flush();
  if (retval) SETIERROR(WRITE_FAILED,"in checkpoint()");
  retval = state_file.close();
  if (retval) SETIERROR(WRITE_FAILED,"in checkpoint()");
  boinc_checkpoint_completed();
  xml_indent_level=l;

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  return 0;
}

// Read the state file and set analysis_state accordingly.
// Note: The state of analysis is saved in two places:
// 1) at the end of processing the data for any given
//    chirp/fft pair.  In this case, the icfft index
//    needs to be set for the *next* chirp/fft pair.
//    If analysis was in this state when saved,
//    PoT_freq_bin will have been set to -1.
// 2) at the end of PoT processing for any given
//    frequency bin (for any given chirp/fft pair).
//    This is indicated by PoT_freq_bin containing
//    something other than -1.  It will contain the
//    frequency bin just completed. In this case, we
//    are *not* finished processing for the current
//    chirp/fft pair and we do not increment icfft to
//    the next pair.  We do however increment PoT_freq_bin
//    to the next frequency bin.
//
int parse_state_file(ANALYSIS_STATE& as) {
  static char buf[8192];
  int ncfft, fl, PoT_freq_bin, PoT_activity;
  double cr=0,flops=0;
  FILE* state_file;

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  ncfft = -1;
  progress = 0.0;
  PoT_freq_bin = -1;
  PoT_activity = POT_INACTIVE;
  state_file = boinc_fopen(STATE_FILENAME, "rb");
  if (!state_file) SETIERROR(FOPEN_FAILED,"in parse_state_file()");

  // main parsing loop
  while (fgets(buf, sizeof(buf), state_file)) {
    if (parse_int(buf, "<ncfft>", ncfft)) continue;
    else if (parse_double(buf, "<cr>", cr)) continue;
    else if (parse_int(buf, "<fl>", fl)) continue;   
    else if (parse_double(buf, "<prog>", progress)) continue;
    else if (parse_int(buf, "<potfreq>", PoT_freq_bin)) continue;
    else if (parse_int(buf, "<potactivity>", PoT_activity)) continue;
    else if (parse_int(buf, "<signal_count>", signal_count)) continue;
    else if (parse_double(buf, "<flops>", flops)) continue;
    else if (parse_int(buf, "<spike_count>", spike_count)) continue;
    else if (parse_int(buf, "<pulse_count>", pulse_count)) continue;
    else if (parse_int(buf, "<gaussian_count>", gaussian_count)) continue;
    else if (parse_int(buf, "<triplet_count>", triplet_count)) continue;
    // best spike
    else if (xml_match_tag(buf, "<best_spike>")) {
      while (fgets(buf, sizeof(buf), state_file)) {
        if (xml_match_tag(buf, "</best_spike>")) break;
        // spike proper
        else if (xml_match_tag(buf, "<spike>")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</spike>")) break;
	    p += strlen(p);
          } 
          best_spike->s.parse_xml(buf);
        }
        // ancillary data
        else if (parse_double(buf, "<bs_score>", best_spike->score)) continue;
        else if (parse_int(buf, "<bs_bin>", best_spike->bin)) continue;
        else if (parse_int(buf, "<bs_fft_ind>", best_spike->fft_ind)) continue;
      } // end while in best_spike
    }  // end if in best_spike

    // best gaussian..
    else if (xml_match_tag(buf, "<best_gaussian>")) {
      while (fgets(buf, sizeof(buf), state_file)) {
        if (xml_match_tag(buf, "</best_gaussian>")) break;
        // gaussian proper
        else if (xml_match_tag(buf, "<gaussian>")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</gaussian>")) break;
	    p += strlen(p);
          } 
          best_gauss->g.parse_xml(buf);
        }
        // ancillary data
        else if (parse_double(buf, "<bg_score>", best_gauss->score)) continue;
        else if (parse_double(buf, "<bg_display_power_thresh>", 
		best_gauss->display_power_thresh)) continue;
        else if (parse_int(buf, "<bg_bin>", best_gauss->bin)) continue ;
        else if (parse_int(buf, "<bg_fft_ind>", best_gauss->fft_ind)) continue;
      }  // end while in best_gaissian
    }  // end if in best_gaussian

    // best pulse
    else if (xml_match_tag(buf, "<best_pulse>")) {
      while (fgets(buf, sizeof(buf), state_file)) {
        if (xml_match_tag(buf, "</best_pulse>")) break;
        // pulse proper
        else if (xml_match_tag(buf, "<pulse>")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</pulse>")) break;
	    p += strlen(p);
          } 
          best_pulse->p.parse_xml(buf);
        }
        // ancillary data
        else if (parse_double(buf, "<bp_score>", best_pulse->score)) continue;
        else if (parse_int(buf, "<bp_freq_bin>", best_pulse->freq_bin)) continue;
        else if (parse_int(buf, "<bp_time_bin>", best_pulse->time_bin)) continue;
      }  // end while in best_pulse
    }  // end if in best_pulse

    // best triplet
    else if (xml_match_tag(buf, "<best_triplet>")) {
      while (fgets(buf, sizeof(buf), state_file)) {
        if (xml_match_tag(buf, "</best_triplet>")) break;
        // triplet proper
        else if (xml_match_tag(buf, "<triplet>")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</triplet>")) break;
	    p += strlen(p);
          } 
          best_triplet->t.parse_xml(buf);
        }
        // ancillary data
        else if (parse_double(buf, "<bt_score>", best_triplet->score)) continue;
        else if (parse_double(buf, "<bt_bperiod>", best_triplet->bperiod)) continue;
        else if (parse_int(buf, "<bt_tpotind0_0>", best_triplet->tpotind0_0)) continue;
        else if (parse_int(buf, "<bt_tpotind0_1>", best_triplet->tpotind0_1)) continue;
        else if (parse_int(buf, "<bt_tpotind1_0>", best_triplet->tpotind1_0)) continue;
        else if (parse_int(buf, "<bt_tpotind1_1>", best_triplet->tpotind1_1)) continue;
        else if (parse_int(buf, "<bt_tpotind2_0>", best_triplet->tpotind2_0)) continue;
        else if (parse_int(buf, "<bt_tpotind2_1>", best_triplet->tpotind2_1)) continue;
        else if (parse_int(buf, "<bt_freq_bin>", best_triplet->freq_bin)) continue;
        else if (parse_double(buf, "<bt_time_bin>", best_triplet->time_bin)) continue;
        else if (parse_double(buf, "<bt_scale>", best_triplet->scale)) continue;
 	else if (xml_match_tag(buf, "<bt_pot_min")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</bt_pot_min")) break;
	    p += strlen(p);
          } 
	  std::vector<unsigned char> pot_min(
	    xml_decode_field<unsigned char>(buf,"bt_pot_min")
	  );
	  int i;
	  for (i=0; i<swi.analysis_cfg.triplet_pot_length; i++) {
	    best_triplet->pot_min[i] = pot_min[i];
	  }
	}  // end Min PoT
 	else if (xml_match_tag(buf, "<bt_pot_max")) {
	  char *p = buf + strlen(buf);
          while(fgets(p, sizeof(buf)-(int)strlen(buf), state_file)) {
            if (xml_match_tag(buf, "</bt_pot_max")) break;
	    p += strlen(p);
          } 
	  std::vector<unsigned char> pot_max(
	    xml_decode_field<unsigned char>(buf,"bt_pot_max")
	  );
	  int i;
	  for (i=0; i<swi.analysis_cfg.triplet_pot_length; i++) {
	    best_triplet->pot_max[i] = pot_max[i];
	  }
	}  // end Max PoT
      }  // end while in best_triplet
    }  // end if in best_triplet

  }  // end main parsing loop

  fclose(state_file);

  analysis_state.FLOP_counter=flops;
  reload_graphics_state();	// so we can draw best_of signals

  // Adjust for restart - go 1 step beyond the checkpoint.
  as.PoT_activity = PoT_activity;
  if(PoT_freq_bin == -1) {
    as.icfft        = ncfft+1;
    as.PoT_freq_bin = PoT_freq_bin;
  } else {
    as.icfft        = ncfft;
    as.PoT_freq_bin = PoT_freq_bin+1;
  }

// debug possible heap corruption -- jeffc
#ifdef _WIN32
BOINCASSERT(_CrtCheckMemory());
#endif

  return 0;
}

// On entry, analysis_state.data points to malloced data
// and outfile is not open.
// On exit, these are freed and closed
int seti_do_work() {
  int retval=0;

  retval = seti_analyze(analysis_state);
  free_a(analysis_state.data);
  analysis_state.data = 0;

  if( swi.data_type == DATA_ENCODED ) {
      free_a(analysis_state.savedWUData);
      analysis_state.savedWUData = 0;
  }

  return retval;
}

// on success, swi.data points to malloced data.
int seti_parse_data(FILE* f, ANALYSIS_STATE& state) {
  unsigned long nbytes, nsamples,samples_per_byte;
  sah_complex *data;
  unsigned long i;
  char *p, buf[256];
  sah_complex *bin_data=0;
  int retval=0;
  FORCE_FRAME_POINTER;

  nsamples = swi.nsamples;
  samples_per_byte=(8/swi.bits_per_sample);
  data = (sah_complex *)malloc_a(nsamples*sizeof(sah_complex), MEM_ALIGN);
  bin_data = (sah_complex *)malloc_a(nsamples*sizeof(sah_complex), MEM_ALIGN);
  if (!data) SETIERROR(MALLOC_FAILED, "!data");
  if (!bin_data) SETIERROR(MALLOC_FAILED, "!bin_data");

  switch(swi.data_type) {
    case DATA_ASCII:
      for (i=0; i<nsamples; i++) {
        p = fgets(buf, 256, f);
        if (!p) {
          SETIERROR(READ_FAILED,"in seti_parse_data");
        }

        sscanf(buf, "%f%f", &data[i][0], &data[i][1]);
      }
      break;
    case DATA_ENCODED:
    case DATA_SUN_BINARY:
      try {
        int nread;
        std::string tmpbuf("");
        fseek(f,0,SEEK_SET);
        nbytes = (nsamples/samples_per_byte);
        tmpbuf.reserve(nbytes*3/2);
        while ((nread=(int)fread(buf,1,sizeof(buf),f))) {
          tmpbuf+=std::string(&(buf[0]),nread);
        }
        std::vector<unsigned char> datav(
           xml_decode_field<unsigned char>(tmpbuf,"data") 
        );
	
        memcpy(bin_data,&(datav[0]),datav.size());
        if (datav.size() < nbytes) throw BAD_DECODE;
      } catch (int i) {
          retval=i;
          if (data) free_a(data);
          if (bin_data) free_a(bin_data);
          SETIERROR(i,"in seti_parse_data()");
      }
      bits_to_floats((unsigned char *)bin_data, data, nsamples);
      memcpy(bin_data,data,nsamples*sizeof(sah_complex));
      state.savedWUData = bin_data;
      break;
/*
#if 0
      nbytes = (nsamples/4);
      bin_data = (unsigned char*)malloc_a((nbytes+2)*sizeof(unsigned char), MEM_ALIGN);
      if (!bin_data) SETIERROR(MALLOC_FAILED, "!bin_data");
      retval = read_bin_data(bin_data, nbytes, f);
      if (retval) {
          if (data) free_a(data);
          if (bin_data) free_a(bin_data);
          SETIERROR(retval,"from read_bin_data()");
      }
      bits_to_floats(bin_data, data, nsamples);
      state.savedWUData = bin_data;
      break;
#endif
*/
  }
  state.npoints = nsamples;
  state.data = data;

#ifdef BOINC_APP_GRAPHICS
  if (sah_graphics) {
      strlcpy(sah_graphics->wu.receiver_name,swi.receiver_cfg.name,255);
      sah_graphics->wu.s4_id = swi.receiver_cfg.s4_id;
      sah_graphics->wu.time_recorded = swi.time_recorded;
      sah_graphics->wu.subband_base = swi.subband_base;
      sah_graphics->wu.start_ra = swi.start_ra;
      sah_graphics->wu.start_dec = swi.start_dec;
      sah_graphics->wu.subband_sample_rate = swi.subband_sample_rate;
      sah_graphics->ready = true;
  }
#endif

  return 0;
}

int seti_parse_wu(FILE* f, ANALYSIS_STATE& state) {
  int retval=0;
  retval = seti_parse_wu_header(f);
  if (retval) SETIERROR(retval,"from seti_parse_wu_header()");
  return seti_parse_data(f, state);
}

void final_report() {
  fprintf(stderr,"\nFlopcounter: %f\n\n", analysis_state.FLOP_counter);
  fprintf(stderr,"Spike count:    %d\n", spike_count);
  fprintf(stderr,"Pulse count:    %d\n", pulse_count);
  fprintf(stderr,"Triplet count:  %d\n", triplet_count);
  fprintf(stderr,"Gaussian count: %d\n", gaussian_count);
  fflush(stderr);
}
