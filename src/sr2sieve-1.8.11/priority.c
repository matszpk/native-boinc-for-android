/* priority.c -- (C) Geoffrey Reynolds, February 2007.

   Set process priority and CPU affinity.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifdef __GNUC__
#define _GNU_SOURCE 1
#endif

#include "sr5sieve.h"
#include "config.h"


#if HAVE_SETPRIORITY
#include <sys/resource.h>
# ifndef PRIO_MAX
# define PRIO_MAX 10
# endif
# ifndef PRIO_MIN
# define PRIO_MIN -10
# endif
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__HAIKU__)
#include <OS.h>
#endif

/* priority level: -2  lowest (idle)
                   -1  low
                    0  normal
                    1  high
                    2  highest
*/
void set_process_priority(int level)
{
#if HAVE_SETPRIORITY
  if (level <= -2)
    setpriority(PRIO_PROCESS,0,PRIO_MAX);
  else if (level == -1)
    setpriority(PRIO_PROCESS,0,PRIO_MAX/2);
  else if (level == 0)
    setpriority(PRIO_PROCESS,0,0);
  else if (level == 1)
    setpriority(PRIO_PROCESS,0,PRIO_MIN/2);
  else
    setpriority(PRIO_PROCESS,0,PRIO_MIN);
#elif defined(_WIN32)
  if (level <= -2)
  {
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
  }
  else if (level == -1)
  {
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
  }
  else if (level == 0)
  {
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
  }
  else if (level == 1)
  {
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
  }
  else /* if (level >= 2) */
  {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
  }
#elif defined(__HAIKU__)
  if (level <= -2)
    set_thread_priority(find_thread(NULL), B_LOWEST_ACTIVE_PRIORITY);
  else if (level == -1)
    set_thread_priority(find_thread(NULL), B_LOW_PRIORITY);
  else if (level == 0)
    set_thread_priority(find_thread(NULL), B_NORMAL_PRIORITY);
  else if (level == 1)
    set_thread_priority(find_thread(NULL), B_DISPLAY_PRIORITY);
  else /*if (level >= 2) */
    set_thread_priority(find_thread(NULL), B_URGENT_DISPLAY_PRIORITY);
#else
 /* Do nothing. */
#endif
}


#if HAVE_SETAFFINITY
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

void set_cpu_affinity(int cpu_number)
{
#if HAVE_SETAFFINITY
  if (cpu_number < CPU_SETSIZE)
  {
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu_number,&set);

    sched_setaffinity(getpid(),sizeof(cpu_set_t),&set);
  }
#elif defined(_WIN32)
  if (cpu_number < 32)
  {
    // SetProcessAffinityMask(GetCurrentProcess(), 1<<cpu_number);
    SetThreadAffinityMask(GetCurrentThread(), 1<<cpu_number);
  }
#else
  /* Do nothing */
#endif
}
