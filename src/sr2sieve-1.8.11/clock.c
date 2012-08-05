/* clock.c -- (C) Geoffrey Reynolds, May 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include "sr5sieve.h"
#include "config.h"


/* Return the CPU time used by this process to date, in seconds.
   TODO: Make it work when multithreading too.
*/
#if HAVE_GETRUSAGE
#include <sys/resource.h>
static double cpu_seconds(void)
{
  int ret;
  struct rusage r;

  ret = getrusage(RUSAGE_SELF, &r);

  assert(ret == 0);

  return (double)(r.ru_utime.tv_sec + r.ru_stime.tv_sec)
    + (double)(r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1000000;
}
#elif defined(_WIN32)
/* clock() doesn't seem to work in Windows, it just returns the elapsed time
   since the process started. That might be a mingw32 bug, but no harm using
   this code for other Windows compilers just in case.
*/
#include <windows.h>
static double cpu_seconds(void)
{
  FILETIME ft_create, ft_exit, ft_kernel, ft_user;
  ULARGE_INTEGER ns100_kernel, ns100_user;

  GetProcessTimes(GetCurrentProcess(),&ft_create,&ft_exit,&ft_kernel,&ft_user);

  ns100_kernel.u.LowPart = ft_kernel.dwLowDateTime;
  ns100_kernel.u.HighPart = ft_kernel.dwHighDateTime;
  ns100_user.u.LowPart = ft_user.dwLowDateTime;
  ns100_user.u.HighPart = ft_user.dwHighDateTime;

  return (double)(ns100_kernel.QuadPart + ns100_user.QuadPart) / 10000000;
}
#else
/* If clock_t is an unsigned 32 bit type and CLOCKS_PER_SEC is 1 million
   (typical case) then clock() will wrap around every 72 minutes. But as
   long as this function is called at least once every 72 minutes, and not
   called from a signal handler, it will work correctly. We can ensure this
   happens by setting the maximum reporting period to 1 hour in srsieve.h.
*/
#include <time.h>

static double cpu_seconds(void)
{
  static clock_t last_clock = 0;
  static int64_t accumulated_clocks = 0;
  clock_t this_clock = clock();

  assert(this_clock != (clock_t)-1);

  accumulated_clocks += (this_clock - last_clock);
  last_clock = this_clock;

  return (double)accumulated_clocks / CLOCKS_PER_SEC;
}
#endif /* HAVE_RUSAGE */


/* Accumulated CPU time in seconds.
 */
static double acc_cpu_seconds;
double get_accumulated_cpu(void)
{
  return acc_cpu_seconds + cpu_seconds();
}

void set_accumulated_cpu(double seconds)
{
  acc_cpu_seconds = seconds - cpu_seconds();
}


#if HAVE_GETTIMEOFDAY
#include <stdlib.h>
#include <sys/time.h>
uint32_t millisec_elapsed_time(void)
{
  struct timeval elapsed;

  gettimeofday(&elapsed,NULL);

  /* It is OK if this overflows, only the low order bits are needed.
   */
  return (elapsed.tv_sec*1000 + elapsed.tv_usec/1000);
}
#elif defined(_WIN32)
#include <windows.h>
uint32_t millisec_elapsed_time(void)
{
  FILETIME elapsed;
  ULARGE_INTEGER ns100;

  GetSystemTimeAsFileTime(&elapsed);

  ns100.u.LowPart = elapsed.dwLowDateTime;
  ns100.u.HighPart = elapsed.dwHighDateTime;

  return ns100.QuadPart/10000;
}
#else
#include <stdlib.h>
#include <time.h>
uint32_t millisec_elapsed_time(void)
{
  return time(NULL)*1000;
}
#endif /* HAVE_GETTIMEOFDAY */


#if HAVE_GETTIMEOFDAY
#include <stdlib.h>
#include <sys/time.h>
static double elapsed_seconds(void)
{
  struct timeval elapsed;

  gettimeofday(&elapsed,NULL);

  return (double)elapsed.tv_sec + (double)elapsed.tv_usec/1000000;
}
#elif defined(_WIN32)
#include <windows.h>
static double elapsed_seconds(void)
{
  FILETIME elapsed;
  ULARGE_INTEGER ns100;

  GetSystemTimeAsFileTime(&elapsed);

  ns100.u.LowPart = elapsed.dwLowDateTime;
  ns100.u.HighPart = elapsed.dwHighDateTime;

  return (double)ns100.QuadPart/10000000;
}
#else
#include <stdlib.h>
#include <time.h>
static double elapsed_seconds(void)
{
  return (double)time(NULL);
}
#endif /* HAVE_GETTIMEOFDAY */


/* Accumulated elapsed time in seconds, accurate to 1 millisec.
 */
static double acc_seconds;
double get_accumulated_time(void)
{
  return acc_seconds + elapsed_seconds();
}

void set_accumulated_time(double seconds)
{
  acc_seconds = seconds - elapsed_seconds();
}


/* For benchmarking purposes.
 */
#if USE_ASM && defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
/* Use function in misc-i386.S or misc-x86-64.S */
#elif USE_ASM && defined(__GNUC__) && (defined(__ppc64__) || defined(__powerpc64__))
uint64_t timestamp(void)
{
  uint64_t ret;
  asm volatile ("mftb %0" : "=r" (ret) );
  return ret;
}
#elif HAVE_GETTIMEOFDAY
#include <stdlib.h>
#include <sys/time.h>
uint64_t timestamp(void)
{
  struct timeval thistime;
  gettimeofday(&thistime, NULL);
  return thistime.tv_sec * 1000000 + thistime.tv_usec;
}
#else
#include <time.h>
uint64_t timestamp(void)
{
  return clock();
}
#endif
