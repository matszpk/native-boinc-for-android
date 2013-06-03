
// Copyright (c) 1999-2006 Regents of the University of California
//
// FFTW: Copyright (c) 2003,2006 Matteo Frigo
//       Copyright (c) 2003,2006 Massachusets Institute of Technology
//
// fft8g.[cpp,h]: Copyright (c) 1995-2001 Takya Ooura

// This program is free software; you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the 
// Free Software Foundation; either version 2, or (at your option) any later
// version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions
// as an alternative to FFTW and distribute a linked executable and 
// source code.  You must obey the GNU General Public License in all 
// respects for all of the code used other than the FFT library itself.  
// Any modification required to support these libraries must be distributed 
// under the terms of this license.  If you modify this program, you may extend 
// this exception to your version of the program, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.  Please be aware that FFTW is not covered by this exception, 
// therefore you may not use FFTW in any derivative work so modified without 
// permission of the authors of FFTW.

// $Id: hires_timer.h,v 1.1.2.4 2006/12/14 22:22:00 korpela Exp $
//
#include "sah_config.h"
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#elif defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_HRTIME_T) && defined(HAVE_GETHRTIME)
typedef hrtime_t tick_t;
#elif defined(HAVE_INT_FAST64_T)
typedef int_fast64_t tick_t;
#elif defined(HAVE_INT64_T)
typedef int64_t tick_t;
#elif defined(HAVE__INT64)
typedef _int64 tick_t;
#elif defined(HAVE_LONG_LONG)
typedef long long tick_t;
#else
typedef double tick_t;
#endif

typedef double second_t;

class hires_timer {
  public:
    hires_timer();
    tick_t ticks();     // current time value in ticks
    tick_t delta() const {return ticks_min_incr;};
    inline second_t seconds() {return (second_t)(ticks()*period);};
    inline second_t frequency() const {return 1.0/period;};
    inline second_t resolution() const {return (second_t)ticks_min_incr*period;};
    inline second_t start() { 
      register second_t tmp=seconds(); 
      do {
	start_time=seconds();
      } while ((start_time-tmp)<resolution()); 
      return start_time; 
    };
    second_t elapsed() { return seconds()-start_time; };
    second_t stop() { register second_t tmp=elapsed(); start_time=0; return tmp; };
  private:
    tick_t rollover;
    tick_t last_ticks;
    second_t start_time;
    static second_t period;
    static tick_t ticks_min_incr;
    static second_t increment;
    static int use_fallback;
    tick_t find_increment();
    second_t find_frequency();
    void calibrate();
};


