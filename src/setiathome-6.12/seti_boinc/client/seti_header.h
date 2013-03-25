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

/*
 *
 * $Id: seti_header.h,v 1.7.2.5 2007/05/31 22:03:11 korpela Exp $
 *
 */

#ifndef SETI_HEADER_H
#define SETI_HEADER_H

#ifndef _WIN32
#include <time.h>
#include <vector>
#endif

#include "timecvt.h"
#include "db/db_table.h"
#include "db/schema_master.h"
#undef USE_MYSQL

#define SRC_SYNTHETIC 1
#define SRC_ARECIBO_1420 2
#define NUM_SRCS 3

#define DATA_ASCII  1
#define DATA_ENCODED 2
#define DATA_SUN_BINARY 3

#define MAX_SCOPE_STRINGS 32
#define MAX_NUM_FFTS       64
#define MAX_CFFT_PARAMS     2

// Variables of type ChirpFftTable_t (usually an array of such
// variables) indicate a range of chirp rates and the FFT
// lenghts (spectral resolutions) that we  wish to process within
// said chirp range.
typedef struct {
  double   MaxChirpRate;
  // Upper bound for this range.
  // Lower bound is MaxChirpRate of the previous
  // ChirpFftTable_t element.  Lower bound
  // of first ChirpFftTable_t element is assumed to be zero.
  unsigned char  DoFft[MAX_NUM_FFTS];         // Boolean - do this FFT length while in
  //   this chirp range
}
ChirpFftTable_t;


struct SCOPE_STRING : public track_mem<SCOPE_STRING> {
  TIME st;
  double alt, az;
  double ra, dec;
  SCOPE_STRING() : track_mem<SCOPE_STRING>("SCOPE_STRING") {}
};

struct SETI_WU_INFO : public track_mem<SETI_WU_INFO> {
  int data_type;
  int data_class;
  int splitter_version;
  double start_ra;  // 0 .. 24
  double start_dec;  // -90 .. 90
  double end_ra;
  double end_dec;
  double angle_range;  // in degrees (only for 0.1 degree beam)
  double true_angle_range;  // in degrees
  double time_recorded; // in Julian form
  double subband_center; // Hz This parameter is deprecated in favor of
  //    subband_base
  double subband_base;        // center freq of bin 0.
  double subband_sample_rate;
  int fft_len;
  int ifft_len;
  int subband_number;
  receiver_config receiver_cfg;
  unsigned long nsamples;
  unsigned int bits_per_sample;
  std::vector<coordinate_t>::const_iterator position_history;
  size_t num_positions;         // number_of_positions in history array
  char tape_version[16];
  analysis_config analysis_cfg;
  int num_fft_lengths;
  int analysis_fft_lengths[32];
  double beam_width;
  ChirpFftTable_t chirp_fft_table[MAX_CFFT_PARAMS];
  const workunit *wu;
  SETI_WU_INFO();
  SETI_WU_INFO(const workunit &w);
  SETI_WU_INFO(const SETI_WU_INFO &s);
};

extern int seti_write_wu_header(FILE*, int);
extern int seti_write_wu_header(FILE*, int, SETI_WU_INFO swi);
extern int seti_parse_wu_header(FILE*);
extern int seti_parse_wu_header(FILE*, SETI_WU_INFO &swi);

extern float cnvt_fftlen_hz(int);
extern double cnvt_bin_hz(int, int);
extern double cnvt_bin_hz_offset(int, int);
extern float cnvt_fftind_time(int, int);
extern float cnvt_gauss_offset_time(int offset);
extern int cnvt_hz_bin(float freq, float sample_rate, int nbins);
extern double calc_detection_freq(double freq, double chirp_rate, double
    t_offset);

#endif

