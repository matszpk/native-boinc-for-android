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

// $Id: seti.h,v 1.19.2.27 2007/08/10 00:38:49 korpela Exp $
#ifndef _SETI_H
#define _SETI_H

#ifdef _WIN32
#include "boinc_win.h"
#endif
#include <vector>
#include "boinc_api.h"

extern APP_INIT_DATA app_init_data;

#include "analyze.h"
#ifndef AP_CLIENT
#include "seti_header.h"
#endif
#include "malloc_a.h"

// Define 64 bit integer types.
#ifdef HAVE_INTTYPES_H  
// Most gnu platforms
#include <inttypes.h>
typedef int64_t sh_sint8_t;
typedef uint64_t sh_uint8_t;

#ifdef PRId64
// If print formats are defined
#define SINT8_FMT "%"PRId64
#define SINT8_FMT_CAST(x) (x)
#define UINT8_FMT "%"PRIu64
#define UINT8_FMT_CAST(x) (x)
#else 
// play it safe.  It'll work through 49 bits at least.
#define SINT8_FMT "%20.0f"
#define SINT8_FMT_CAST(x) static_cast<double>(x)
#define UINT8_FMT "%20.0f"
#define UINT8_FMT_CAST(x) static_cast<double>(x)
#endif

#elif SIZEOF_LONG_INT >= 8 
// other L64
typedef long sh_sint8_t;
typedef unsigned long sh_uint8_t;
#define SINT8_FMT "%ld"
#define SINT8_FMT_CAST(x) (x)
#define UINT8_FMT "%lu"
#define UINT8_FMT_CAST(x) (x)

#elif defined(LLONG_MAX) || defined(HAVE_LONG_LONG)
// systems with long long, but no inttypes.h
typedef long long sh_sint8_t;
typedef unsigned long long sh_uint8_t;
// again play it safe
#define SINT8_FMT "%20.0f"
#define SINT8_FMT_CAST(x) static_cast<double>(x)
#define UINT8_FMT "%20.0f"
#define UINT8_FMT_CAST(x) static_cast<double>(x)

#elif defined(HAVE__INT64) || \
      (defined(_INTEGRAL_MAX_BITS) && (_INTEGRAL_MAX_BITS >= 64))
// Probably, but not necessarily MSC.
typedef _int64 sh_sint8_t;
typedef unsigned _int64 sh_uint8_t;
#ifdef _MSC_VER
// leave it to microsoft to be different.
#define SINT8_FMT "%I64d"
#define SINT8_FMT_CAST(x) (x)
#define UINT8_FMT "%I64u"
#define UINT8_FMT_CAST(x) (x)
#else
#define SINT8_FMT "%20.0f"
#define SINT8_FMT_CAST(x) static_cast<double>(x)
#define UINT8_FMT "%20.0f"
#define UINT8_FMT_CAST(x) static_cast<double>(x)
#endif

#elif defined(HAVE_LONG_DOUBLE)
typedef long double sh_sint8_t;
typedef long double sh_uint8_t;
#define SINT8_FMT "%20.0lf"
#define SINT8_FMT_CAST(x) (x)
#define UINT8_FMT "%20.0lf"
#define UINT8_FMT_CAST(x) (x)

#else
typedef double sh_sint8_t;
typedef double sh_uint8_t;
#define SINT8_FMT "%20.0f"
#define SINT8_FMT_CAST(x) static_cast<double>(x)
#define UINT8_FMT "%20.0f"
#define UINT8_FMT_CAST(x) static_cast<double>(x)

#endif // HAVE_INTTYPES_H

// make a consistent int32_t
#ifndef HAVE_INT32_T
#ifdef HAVE__INT32
typedef _int32 int32_t;
#else
typedef long int32_t;
#endif
#endif


#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

// Adjustement to flops counter to make FLOPS include floating point loads and
// stores.
extern double LOAD_STORE_ADJUSTMENT; 
extern double SETUP_FLOPS;


typedef float sah_complex[2];

struct ANALYSIS_STATE {
  sah_complex *data;
  sah_complex *savedWUData;    // Save the original WU data
  int npoints;
  int icfft;
  int PoT_freq_bin;   // where we are in PoT analysis for this icfft
  // ... will be -1 if no PoT analysis in progress
  int PoT_activity;
  int doing_pulse;
  double FLOP_counter;
};

extern bool notranspose_flag;
extern bool default_functions_flag;
extern bool verbose;
extern int seti_init_state();
extern int seti_do_work();
extern int result_group_start();
extern int result_group_write_header();
extern int result_group_end();
extern int checkpoint(BOOLEAN force_checkpoint=0);
extern int seti_parse_wu(FILE*, ANALYSIS_STATE&);
extern int parse_state_file(ANALYSIS_STATE& as);
extern void final_report();

extern ANALYSIS_STATE analysis_state;
#ifndef AP_CLIENT
extern SETI_WU_INFO swi;
#endif

#ifdef BOINC_APP_GRAPHICS
#include "reduce.h"
#include "graphics2.h"
#endif

#ifndef AP_CLIENT
struct SPIKE_INFO : public track_mem<SPIKE_INFO> {
public :
 SPIKE_INFO();
  ~SPIKE_INFO();
  SPIKE_INFO(const SPIKE_INFO &si);
  SPIKE_INFO &operator =(const SPIKE_INFO &si);
  spike s;		
  double score;		// calc in ReportEvents()
  int bin;		// assigned in ReportEvents()
  int fft_ind;		// assigned in ReportEvents()
};

struct AUTOCORR_INFO : public track_mem<AUTOCORR_INFO> {
public :
 AUTOCORR_INFO();
  ~AUTOCORR_INFO();
  AUTOCORR_INFO(const AUTOCORR_INFO &si);
  AUTOCORR_INFO &operator =(const AUTOCORR_INFO &si);
  autocorr a;		
  double score;		// calc in ReportEvents()
  int bin;		// assigned in ReportEvents()
  int fft_ind;		// assigned in ReportEvents()
};

struct GAUSS_INFO  : public track_mem<GAUSS_INFO> {
public :
  GAUSS_INFO();
  ~GAUSS_INFO();
  GAUSS_INFO(const GAUSS_INFO &gi);
  GAUSS_INFO &operator =(const GAUSS_INFO &gi);
  gaussian g;
  double score;		// calc in ReportGaussEvent()
  double display_power_thresh;	// calc in gaussfit()
  int bin;		// assigned in ReportGaussEvent()
  int fft_ind;		// assigned in ReportGaussEvent()
  //float * pot;
};

struct PULSE_INFO : public track_mem<PULSE_INFO> {
public :
  PULSE_INFO();
  ~PULSE_INFO();
  PULSE_INFO(const PULSE_INFO &pi);
  PULSE_INFO &operator =(const PULSE_INFO &pi);
  pulse p;
  double score;		// calc in ReportPulseEvent()
  int freq_bin;		// assigned in ReportPulseEvent()
  int time_bin;		// assigned in ReportPulseEvent()
  unsigned int * pot_min;  // Scaled 0-255 for display
  unsigned int * pot_max;  // Scaled 0-255 for display
};

struct TRIPLET_INFO : public track_mem<TRIPLET_INFO> {
public :
  TRIPLET_INFO();
  ~TRIPLET_INFO();
  TRIPLET_INFO(const TRIPLET_INFO &ti);
  TRIPLET_INFO &operator =(const TRIPLET_INFO &ti);
  triplet t;
  double score;		// assigned in ReportTripletEvent()
  double bperiod;	// probably deprecated - remove sometime
  int freq_bin;		// assigned in ReportTripletEvent()
  			// ticks below assigned in ReportTripletEvent()
  int tpotind0_0;	// index into pot_min/pot_max arrays
  int tpotind0_1;	// of start/end of first element of triplet
  int tpotind1_0;	// second element
  int tpotind1_1;
  int tpotind2_0;	// and third element
  int tpotind2_1;
  double time_bin;	// calc in ReportTripletEvent()
  double scale;      	// Scale from PoT bins to TRIPLET_POT_LEN
  unsigned int * pot_min;  // Scaled 0-255 for display
  unsigned int * pot_max;  // Scaled 0-255 for display
};
#endif
#define UNSTDMAX(a,b)  (((a) > (b)) ? (a) : (b))
#define UNSTDMIN(a,b)  (((a) < (b)) ? (a) : (b))

unsigned int pow2(unsigned int num);

#endif
