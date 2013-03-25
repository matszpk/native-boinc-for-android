
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
//

// $Id: hires_timer.cpp,v 1.1.2.6 2006/09/07 00:26:56 korpela Exp $

#include "sighandler.h"
#include "hires_timer.h"
#include "diagnostics.h"
#include <cmath>
#include <time.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_MACH_MACH_TIME_H
#include <mach/mach_time.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSTM_H
#include <sys/systm.h>
#endif
#ifdef HAVE_MACHINE_CPU_H
#include <machine/cpu.h>
#endif

#if _MSC_VER >= 1400
#include "intrin.h"  // include intrinsic assembly functions
#endif

#ifdef _WIN32
#include "windows.h"
#endif


#if defined(_WIN32)
static inline tick_t fallback_ticks() {
  LARGE_INTEGER time;
  FILETIME sysTime;
  GetSystemTimeAsFileTime(&sysTime);
  time.LowPart = sysTime.dwLowDateTime;
  time.HighPart = sysTime.dwHighDateTime;  
  return static_cast<tick_t>(time.QuadPart);    
}
#elif defined(HAVE_GETTIMEOFDAY)
static inline tick_t fallback_ticks() {
  struct timeval tp;
  gettimeofday(&tp,0);
  return static_cast<tick_t>(tp.tv_sec)*1000000+tp.tv_usec;
} 
#else
static inline tick_t fallback_ticks() {
  return static_cast<tick_t>(time(0));
}
#endif

#ifdef _WIN32
static inline tick_t get_ticks() {
  LARGE_INTEGER rv;
  QueryPerformanceCounter(&rv);
  return static_cast<tick_t>(rv.QuadPart);
}
#elif defined(HAVE_HRTIME_T) && defined(HAVE_GETHRTIME)
static inline tick_t get_ticks() { 
  return gethrtime();
}

#elif defined(HAVE_MACH_ABSOLUTE_TIME)
static inline tick_t get_ticks() {
  return mach_absolute_time();
}

#elif defined(HAVE_GET_CYCLECOUNT) 
static inline tick_t get_ticks() {
  return get_cyclecount();
}

#elif defined(HAVE_NANOTIME) 
static inline tick_t get_ticks() {
  struct timespec tsp;
  nanotime(&tsp);
  return static_cast<tick_t>(tsp.tv_sec)*static_cast<tick_t>(1000000000LL)+tsp.tv_nsec;
}

#elif defined(__GNUC__) && defined(__i386__)
static inline tick_t get_ticks() {
  register tick_t rv;
  asm volatile (
        "rdtsc"
        : "=A" (rv)
  );
  return rv;
}

#elif defined(__GNUC__) && defined(__x86_64__) 
static inline tick_t get_ticks() {
  register tick_t rv;
  register unsigned int hi,lo;
  asm volatile (
        "rdtsc"
        : "=a" (lo), "=d" (hi)
        : /* no inputs */
        : /* no clobbers */
  );
  rv=(static_cast<tick_t>(hi)<<32)|lo;
  return rv;
}

#elif defined(__GNUC__) && (defined(__ia64) || defined (__ia64__))
static inline tick_t get_ticks() {
  register tick_t rv;
  asm volatile (
        "mov %0=ar.itc"
        : "=r" (rv)
        : /* no inputs */
        : /* no clobbers */
  );
  return rv;
}

#elif defined(__GNUC__) && defined(__sparc)
static inline tick_t get_ticks() {
  register long lo;
  asm volatile (
#ifdef __sparcv9
        "rd %%tick,%0"
        : "=r" (lo)        
#else
        ".long %1\n"   
	"mov %%g1,%0\n"
	: "=r" (lo)
	: "i" (0x83410000) // rd %tick, %g1
#endif
  );
  return static_cast<tick_t>(lo);
}

#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
static inline tick_t get_ticks() {
  register tick_t rv;
  register unsigned int hi1,hi,lo;
  do {
    asm volatile (
        "mftbu %0\n\t"
        "mftb %1\n\t"
        "mftbu %2\n\t"  /* don't forget to check overflow */
        : "=r" (hi), "=r" (lo), "=r" (hi1)
    );
  } while (hi != hi1);
  rv=(static_cast<tick_t>(hi)<<32)|lo;
  return rv;
}

#elif _MSC_VER >= 1400 && (defined(_M_X64) || defined(_M_AMD64))
static inline tick_t get_ticks() {
  return __rdtsc();
}

#elif defined(HAVE_MICROTIME) 
static inline tick_t get_ticks() {
  struct timeval tsp;
  microtime(&tsp);
  return static_cast<tick_t>(tsp.tv_sec)*static_cast<tick_t>(1000000LL)+tsp.tv_usec;
}

#else
static inline tick_t get_ticks() {
  raise(SIGILL);
  return static_cast<tick_t>(0);
}
#endif

hires_timer::hires_timer() : rollover(0),last_ticks(0),start_time(0.0) {
  if (period==0) {
    install_sighandler();
    if (setjmp(jb)) {
      use_fallback=1;
    } else {
      ticks();
    }
    uninstall_sighandler();
    calibrate();  
  }
}

tick_t hires_timer::ticks() {
  if (use_fallback) {
    tick_t t=fallback_ticks();
    if (t < last_ticks) {
       // Assume the rollover is a power of two
       tick_t this_rollover=2;
       unsigned long long u=last_ticks-t;
       while (u>>=1) this_rollover<<=1;
       rollover+=this_rollover;   
       // unfortunately a rollover probably returns a
       // bogus elapsed time.
    }
    return (last_ticks=fallback_ticks()+rollover);
  } else {
    tick_t t=get_ticks();
    if (t < last_ticks) {
       // Assume the rollover is a power of two
       tick_t this_rollover=2;
       unsigned long long u=last_ticks-t;
       while (u>>=1) this_rollover<<=1;
       rollover+=this_rollover;
    }                                   
    return (last_ticks=get_ticks()+rollover);
  }
}

tick_t hires_timer::find_increment() {
  int i;
  tick_t delta=0;
  for (i=0;i<32;i++) {
    tick_t t1,t2=ticks();
    while ((t1=ticks()) <= t2) ; /* do nothing */
    delta+=(t1-t2);
  }
  return (delta/32);
}

second_t hires_timer::find_frequency() {
  time_t t1,t2=time(0);
  tick_t start,fin;
  // Beware time resets.
  while ((t1=time(0)) <= t2) ; /* do nothing */
  start=ticks();
  while ((t2=time(0)) <= t1) ; /* do nothing */
  fin=ticks();
  return static_cast<double>(fin-start)/(t2-t1);
}

void hires_timer::calibrate() {
  double freq=find_frequency();
  if (freq<=0) freq=find_frequency(); /* in case it rolled over */
  BOINCASSERT(freq > 0);
  period=1.0/freq;
  ticks_min_incr=find_increment();
  increment=period*ticks_min_incr;
  if (increment < 1e-6) {
    increment=1e-6;
    ticks_min_incr=static_cast<tick_t>(floor((second_t)increment/period+0.5));
  }
}
    
second_t hires_timer::period;
tick_t hires_timer::ticks_min_incr;
second_t hires_timer::increment;
int hires_timer::use_fallback;


#ifdef TEST_TIMER
#include <cstdio>

int main(void) {
  hires_timer t;
  t.start();
  printf("frequency:%f resolution:%f seconds:%f\n",t.frequency(),t.resolution(),t.seconds());
  while (t.elapsed() < t.resolution()) ;
  printf("%f elapsed\n",t.stop());
  return 0;
}
#endif
