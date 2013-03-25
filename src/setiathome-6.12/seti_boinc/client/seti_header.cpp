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
 * $Id: seti_header.cpp,v 1.22.2.6 2007/05/31 22:03:11 korpela Exp $
 *
 */

#include "sah_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <cmath>

#include "diagnostics.h"
#include "util.h"
#include "s_util.h"

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "seti.h"
#include "analyze.h"
#include "timecvt.h"
#include "parse.h"
#include "../db/db_table.h"
#include "../db/schema_master.h"
#include "seti_header.h"
#include "analyzePoT.h"
#include "analyzeReport.h"
#include "chirpfft.h"

// Write a SETI work unit header to a file

char *receivers[]={"invalid","synthetic","ao1420"};
char *datatypes[]={"invalid","ascii","encoded","sun_binary"};

SETI_WU_INFO::SETI_WU_INFO(const workunit &w) :
    track_mem<SETI_WU_INFO>("SETI_WU_INFO"),
    data_class(0),
    start_ra(w.group_info->data_desc.start_ra),
    start_dec(w.group_info->data_desc.start_dec),
    end_ra(w.group_info->data_desc.end_ra),
    end_dec(w.group_info->data_desc.end_dec),
    true_angle_range(w.group_info->data_desc.true_angle_range),
    time_recorded(w.group_info->data_desc.time_recorded_jd),
    subband_center(w.subband_desc.center),
    subband_base(w.subband_desc.base),
    subband_sample_rate(w.subband_desc.sample_rate),
    fft_len(w.group_info->splitter_cfg->fft_len),
    ifft_len(w.group_info->splitter_cfg->ifft_len),
    subband_number(w.subband_desc.number),
    receiver_cfg(w.group_info->receiver_cfg),
    nsamples(w.group_info->data_desc.nsamples),
    bits_per_sample(w.group_info->recorder_cfg->bits_per_sample),
    position_history(w.group_info->data_desc.coords.begin()),
    num_positions(w.group_info->data_desc.coords.size()),
    analysis_cfg(w.group_info->analysis_cfg),
    beam_width(w.group_info->receiver_cfg->beam_width),
wu(&w) {
  if (!strcmp(w.group_info->splitter_cfg->data_type,"ascii")) data_type = DATA_ASCII;
  else if (!strcmp(w.group_info->splitter_cfg->data_type,"encoded")) data_type = DATA_ENCODED;
  else if (!strcmp(w.group_info->splitter_cfg->data_type,"sun_binary")) data_type = DATA_SUN_BINARY;
  splitter_version=(int)floor(w.group_info->splitter_cfg->version)*0x100;
  splitter_version+=(int)((w.group_info->splitter_cfg->version)*0x100) && 0xff;
  angle_range=true_angle_range;
  sprintf(tape_version,"%8.2f",w.group_info->recorder_cfg->version);
}

SETI_WU_INFO::SETI_WU_INFO() :
    track_mem<SETI_WU_INFO>("SETI_WU_INFO"),
    data_class(0),
    start_ra(0),
    start_dec(0),
    end_ra(0),
    end_dec(0),
    true_angle_range(0),
    time_recorded(0),
    subband_center(0),
    subband_base(0),
    subband_sample_rate(0),
    fft_len(0),
    ifft_len(0),
    subband_number(0),
    nsamples(0),
    bits_per_sample(0),
    position_history(),
    num_positions(0),
    beam_width(0) 
{
  data_type = DATA_ASCII;
  splitter_version=0;
  angle_range=0;
  tape_version[0]=0;
}
SETI_WU_INFO::SETI_WU_INFO(const SETI_WU_INFO &s) :
    track_mem<SETI_WU_INFO>("SETI_WU_INFO"),
    data_type(s.data_type),
    data_class(s.data_class),
    splitter_version(s.splitter_version),
    start_ra(s.start_ra),
    start_dec(s.start_dec),
    end_ra(s.end_ra),
    end_dec(s.end_dec),
    angle_range(s.true_angle_range),
    true_angle_range(s.true_angle_range),
    time_recorded(s.time_recorded),
    subband_center(s.subband_center),
    subband_base(s.subband_base),
    subband_sample_rate(s.subband_sample_rate),
    fft_len(s.fft_len),
    ifft_len(s.ifft_len),
    subband_number(s.subband_number),
    receiver_cfg(s.receiver_cfg),
    nsamples(s.nsamples),
    bits_per_sample(s.bits_per_sample),
    position_history(s.position_history),
    num_positions(s.num_positions),
    analysis_cfg(s.analysis_cfg),
    beam_width(s.beam_width),
wu(s.wu) {
  strcpy(tape_version,s.tape_version);
}

int seti_write_wu_header(FILE *file, int output_xml) {

  fprintf(file,"%s",swi.wu->print_xml().c_str());
  return 0;
}

int seti_write_wu_header(FILE *file, int output_xml, SETI_WU_INFO &swi) {

  fprintf(file,"%s",swi.wu->print_xml().c_str());
  return 0;
}

SETI_WU_INFO swi;

static workunit *wu;

int seti_parse_wu_header(FILE* f) {
  char buf[256];
  int found=0;
 
  std::string buffer("");
  buffer.reserve(10*1024);

  swi.data_type=0;

  //  Allow old style headers to be parsed correctly.
  // jeffc - need this?
  //swi.fft_len=2048;
  //swi.ifft_len=8;

  do {
    fgets(buf, 256, f);
  } while (!feof(f) && !xml_match_tag(buf,"<workunit_header")) ;
 
  buffer+=buf;

  while (fgets(buf, 256, f) && !xml_match_tag(buf,"</workunit_header")) {
    buffer+=buf;
  }
  buffer+=buf;

  if (wu) delete wu;
  wu=new workunit(buffer);
  SETI_WU_INFO temp(*wu);
  swi=temp;
  found=1;

  if (!swi.data_type || !found || !swi.nsamples) {
    SETIERROR(BAD_HEADER, "!swi.data_type || !found || !swi.nsamples");
  }
  return 0;
}

int seti_parse_wu_header(FILE* f, SETI_WU_INFO &swi) {
  char buf[256];
  int found=0;
  
  std::string buffer("");
  buffer.reserve(10*1024);

  swi.data_type=0;

  //  Allow old style headers to be parsed correctly.
  //swi.fft_len=2048;
  //swi.ifft_len=8;

  do {
    fgets(buf, 256, f);
  } while (!feof(f) && !xml_match_tag(buf,"<workunit_header")) ;
  
  buffer+=buf;

  while (fgets(buf, 256, f) && !xml_match_tag(buf,"</workunit_header")) {
    buffer+=buf;
  }
  buffer+=buf;

  if (wu) delete wu;
  wu=new workunit(buffer);
  SETI_WU_INFO temp(*wu);
  swi=temp;
  found=1;

  if (!swi.data_type || !found || !swi.nsamples) {
    SETIERROR(BAD_HEADER, "!swi.data_type || !found || !swi.nsamples");
  }
  return 0;
}

float cnvt_fftlen_hz(int fft_len) {
  return (float)(swi.subband_sample_rate/fft_len);
}

// If a workunit is generated by frequency splitting
// using a forward fft of length F and F/I inverse ffts of length I. After the 
// forward fft the frequencies of the bins are 0,1,..F/2,-F/2..-2,-1. When you
// consider the finite width of the bins, bin 0 contains frequencies from -1/2
// to 1/2.  When you take the inverse FFT of the first I bins to make a time
// series for the workunit, you get frequencies from -1/2 to I-1/2.  That's why
// the DC bin is not the central frequency of a work unit.
//
// If we switch to the polyphase filter for splitting, the ifft_len will be
// one, which is the case when the DC bin is the same as the central frequency.
double cnvt_bin_hz(int bin, int fft_len) {
  int endpt=fft_len-fft_len/(2*swi.ifft_len);
  if (bin < endpt) {
    return (swi.subband_base+swi.subband_sample_rate*bin/fft_len);
  } else {
    return (swi.subband_base+swi.subband_sample_rate*(bin-fft_len)/fft_len);
  }
}

double cnvt_bin_hz_offset(int bin, int fft_len) {
  return cnvt_bin_hz(bin, fft_len) - swi.subband_base;
}

int cnvt_hz_bin(float freq, float sample_rate, int nbins) {
  if (freq >= 0) {
    return (int)(nbins*freq/sample_rate);
  } else {
    return (int)(nbins/2-(freq/sample_rate));
  }
}

float cnvt_fftind_time(int fft_ind, int fft_len) {
  return (float)(fft_ind*(fft_len/swi.subband_sample_rate));
}

float cnvt_gauss_offset_time(int offset) {
  float dur = (float)(swi.nsamples/swi.subband_sample_rate);
  return (float)(offset*dur/swi.analysis_cfg.gauss_pot_length);
}

// need to properly handle frequency rollover when calculating detection
// frequency
double calc_detection_freq(double freq, double chirp_rate, double t_offset) {
  double fft_ratio=1.0/swi.wu->group_info->splitter_cfg->ifft_len;
  double lowerlim=swi.subband_base-0.5*swi.subband_sample_rate*fft_ratio;
  double upperlim=lowerlim+swi.subband_sample_rate;
  double detection_freq=freq+chirp_rate*t_offset;
  while (detection_freq < lowerlim) detection_freq+=swi.subband_sample_rate;
  while (detection_freq > upperlim) detection_freq-=swi.subband_sample_rate;
  return detection_freq;
}
