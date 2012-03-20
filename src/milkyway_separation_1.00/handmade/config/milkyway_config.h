/*
 *  Copyright (c) 2010-2011 Matthew Arsenault
 *  Copyright (c) 2010-2011 Rensselaer Polytechnic Institute
 *
 *  This file is part of Milkway@Home.
 *
 *  Milkway@Home is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MILKYWAY_CONFIG_H_
#define _MILKYWAY_CONFIG_H_

#define BOINC_APPLICATION 1
#define DOUBLEPREC 1
#define MW_ENABLE_DEBUG 0

#define HAVE_EXP10 0
#define HAVE_EXP2 0
#define HAVE_SINCOS 0
#define HAVE_FMAX 1
#define HAVE_FMIN 1

#define HAVE_UNISTD_H 1
#define HAVE_WINDOWS_H 0
#define HAVE_DIRECT_H 0
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_MALLOC_H 1
#define HAVE_FLOAT_H 1
#define HAVE_FPU_CONTROL_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_ASPRINTF 1
#define HAVE_POSIX_MEMALIGN 0
#define HAVE_MEMALIGN 1
#define HAVE__ALIGNED_MALLOC 0
#define HAVE___MINGW_ALIGNED_MALLOC 0

#define HAVE_CLOCK_GETTIME 1
#define HAVE_MACH_ABSOLUTE_TIME 0
#define HAVE_GETTIMEOFDAY 1

/* C99 Restrict.

   For MSVC, just defining restrict to __restrict doesn't work because
   its preprocessor is stupid, and for some reason replaces the
   restrict in __declspec(restrict) in the MSVC standard headers.
 */
#ifdef _MSC_VER
  #define RESTRICT __restrict
  #define FUNC_NAME __FUNCTION__
  #define inline __inline
#else
  #ifdef __cplusplus
    #define RESTRICT __restrict__
  #else
    #define RESTRICT restrict
  #endif /* __cplusplus */
  #define FUNC_NAME __func__
#endif /* _MSC_VER */


#ifndef _MSC_VER
  #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    #define HOT __attribute__((hot))
  #else
    #define HOT
  #endif

  #define CONST_F __attribute__((const))
  #define ALWAYS_INLINE __attribute__((always_inline))
  #define INTERNAL __attribute__((visibility("internal")))
  #define WEAK __attribute__((weak))
#else
  #define HOT
  #define CONST_F
  #define ALWAYS_INLINE
  #define INTERNAL

  /* Only sort of */
  #define WEAK __declspec(selectany)
#endif /* _MSC_VER */


#ifndef _MSC_VER
  #define _GNU_SOURCE 1
#endif

/* __GNUC_GNU_INLINE__ only defined in 4.1.3+ */
#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__) && !defined(__GNUC_GNU_INLINE__)
  #define MW__GNUC_GNU_INLINE__ 1
#endif


#if defined(__GNUC_GNU_INLINE__) || defined(MW__GNUC_GNU_INLINE__)
  #define OLD_GCC_EXTERNINLINE extern
#else
  /* Assume C99 inline semantics */
  #define OLD_GCC_EXTERNINLINE
#endif /* __GNUC_GNU_INLINE__ */


#if DOUBLEPREC
  #define PRECSTRING "double"
#else
  #define PRECSTRING "float"
#endif

#if DISABLE_DENORMALS
  #define DENORMAL_STRING " no denormals"
#else
  #define DENORMAL_STRING ""
#endif


/* x86_64 seems to always give i386 even on OS X 64 */

#if defined(__x86_64__) || defined(_M_X64)
  #define ARCH_STRING "x86_64"
  #define MW_IS_X86_64 1
  #define MW_IS_X86 1
#elif defined(__i386__) || defined(_M_IX86)
  #define ARCH_STRING "x86"
  #define MW_IS_X86_32 1
  #define MW_IS_X86 1
#elif defined(__powerpc__) || defined(__ppc__) || defined(_M_PPC)
  #define ARCH_STRING "PPC"
#elif defined(__ppc64__)
  #define ARCH_STRING "PPC64"
#elif defined(__arm__) || defined(__arm)
  #define ARCH_STRING "ARM"
#elif defined(__sparc)
  #define ARCH_STRING "SPARC"
#elif defined(__mips64)
  #define ARCH_STRING "MIPS64"
#elif defined(__mips)
  #define ARCH_STRING "MIPS"
#else
  #define ARCH_STRING "Unknown architecture"
  #warning "Unknown architecture!"
#endif


#if DOUBLEPREC
  #ifdef __SSE2__
    #define HAVE_SSE_MATHS 1
  #endif
#else
  #ifdef __SSE__
    #define HAVE_SSE_MATHS 1
  #endif
#endif /* DOUBLEPREC */

#ifdef _MSC_VER
/* __declspec(align) is shit. If you use it on some typedef,
   and use that type as a function parameter it errors. */
  #define MW_ALIGN __attribute__((aligned))
  #define MW_ALIGN_V(x) __declspec(align(x))
  #define MW_ALIGN_TYPE
  #define MW_ALIGN_TYPE_V(x)
#else
  #define MW_ALIGN __attribute__((aligned))
  #define MW_ALIGN_V(x) __attribute__((aligned(x)))
  #define MW_ALIGN_TYPE __attribute__((aligned))
  #define MW_ALIGN_TYPE_V(x) __attribute__((aligned(x)))
#endif /* _MSC_VER */

/* Can use x87 or some non-x86 math */
#if !defined(__APPLE__) && !MW_IS_X86_64
  #define HAVE_OTHER_MATH_X87 1
#endif

#define MILKYWAY_SYSTEM_NAME "Linux"


#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
  #pragma error_messages (off, E_CANT_SET_NONDEF_ALIGN_FUNC)
  #pragma error_messages (off, E_CANT_SET_NONDEF_ALIGN_AUTO)
  #pragma error_messages (off, E_CANT_SET_NONDEF_ALIGN_PARAM)
#endif

#endif /* _MILKYWAY_CONFIG_H_ */

