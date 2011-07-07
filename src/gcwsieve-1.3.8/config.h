/* config.h -- (C) Geoffrey Reynolds, May 2006.

   Compiler/OS dependant configuration.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __GNUC__
# define attribute(list) __attribute__(list)
#else
# define attribute(list)
#endif

/* Define HAVE_FORK if unistd.h declares fork()
 */
#ifndef HAVE_FORK
# if BOINC
#  define HAVE_FORK 0
# elif defined(_WIN32)
#  define HAVE_FORK 0
# else
#  define HAVE_FORK 1
# endif
#endif

/* Define HAVE_GETRUSAGE if sys/resource.h declares getrusage()
 */
#ifndef HAVE_GETRUSAGE
# if defined(_WIN32)
#  define HAVE_GETRUSAGE 0
# else
#  define HAVE_GETRUSAGE 1
# endif
#endif

/* Define HAVE_GETTIMEOFDAY if sys/time.h declares gettimeofday()
 */
#ifndef HAVE_GETTIMEOFDAY
# if defined(_WIN32)
#  define HAVE_GETTIMEOFDAY 0
# else
#  define HAVE_GETTIMEOFDAY 1
# endif
#endif

/* Define HAVE_SETPRIORITY if sys/resource.h declares setpriority().
 */
#ifndef HAVE_SETPRIORITY
# if defined(_WIN32)
#  define HAVE_SETPRIORITY 0
# else
#  define HAVE_SETPRIORITY 1
# endif
#endif

/* Define HAVE_SETAFFINITY if sched.h declares sched_setaffinity().
 */
#ifndef HAVE_SETAFFINITY
# if defined(_WIN32) || defined(__APPLE__)
#  define HAVE_SETAFFINITY 0
# else
#  define HAVE_SETAFFINITY 1
# endif
#endif

/* Define HAVE_ALARM if signal.h defines SIGALRM
 */
#ifndef HAVE_ALARM
# if defined(_WIN32)
#  define HAVE_ALARM 0
# else
#  define HAVE_ALARM 1
# endif
#endif

/* These definition are missing from mingw32 3.4.5
 */
#if defined(__MINGW32__)
# define ffs __builtin_ffs
# define ffsl __builtin_ffsl
# define ffsll __builtin_ffsll
#endif

/* Define HAVE_FFS if string.h defines ffs, ffsl, ffsll.
 */
#ifndef HAVE_FFS
# if defined(__GNUC__)
#  define HAVE_FFS 1
# else
#  define HAVE_FFS 0
# endif
#endif

/* Define NEED_UNDERSCORE if global assembler symbols need to be prepended
   by '_'.
*/
#ifndef NEED_UNDERSCORE
# if defined(__MINGW32__) || (defined(_MSC_VER) && !defined(_WIN64)) || defined(__APPLE__)
#  define NEED_UNDERSCORE 1
# else
#  define NEED_UNDERSCORE 0
# endif
#endif

/* Define HAVE_MEMALIGN if malloc.h declares a memalign() function.
 */
#ifndef HAVE_MEMALIGN
# if defined(_WIN32) || defined(__APPLE__)
#  define HAVE_MEMALIGN 0
# else
#  define HAVE_MEMALIGN 1
# endif
#endif

/* Define HAVE_MMAP if sys/mman.h declares a mmap() function.
 */
#ifndef HAVE_MMAP
# if defined(_WIN32)
#  define HAVE_MMAP 0
# else
#  define HAVE_MMAP 1
# endif
#endif

/* Define HAVE_MALLOPT if malloc.h declares a mallopt() function.
 */
#ifndef HAVE_MALLOPT
# if defined(_WIN32) || defined(__APPLE__)
#  define HAVE_MALLOPT 0
# else
#  define HAVE_MALLOPT 1
# endif
#endif

#endif /* _CONFIG_H */
