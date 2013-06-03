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

#ifndef _X86_OPS_H_
#define _X86_OPS_H_

#if defined(_M_AMD64) || defined(__x86_64__)
// Make sure we use MMX,SSE/2/3 on x86_64
#ifndef USE_MMX
#define USE_MMX 1
#ifndef __MMX__
#define __MMX__
#endif
#endif
#ifndef USE_3DNOW
#define USE_3DNOW 1
#define __3DNOW__ 1
#endif
#ifndef USE_SSE 
#define USE_SSE  1 
#define __SSE__ 1
#endif
#ifndef USE_SSE2
#define USE_SSE2 1
#define __SSE2__ 1
#endif
#ifndef USE_SSE3
#define USE_SSE3 1
#define __SSE3__ 1
#endif
#endif

#if defined(__GNUC__)
#define ALIGN_ATTR(x) __attribute__((aligned(x)))
#define ALIGN_DECL(x)
#else
#define ALIGN_ATTR(x) 
#define ALIGN_DECL(x) __declspec(align(x))
#endif

// How to declare an aligned variable.
#define ALIGNED(decl,x) \
  ALIGN_DECL(x) decl ALIGN_ATTR(x)



#if defined (_WIN64) && defined (_M_AMD64)
#ifdef HAVE_EMMINTRIN_H
#ifdef USE_SSE2
#include <emmintrin.h>
#endif
#endif
#elif defined(__SSE__) || defined(USE_SSE) || defined(__SSE2__)
// Make sure MSC doesn't use slow emulations
#undef _MM_FUNCTIONALITY
#undef _MM2_FUNCTIONALITY
#ifdef HAVE_EMMINTRIN_H
#ifdef USE_SSE2
#if (__GNUC__ < 4) || ((__GNUC__ > 3) && defined(__SSE2__))
#include <emmintrin.h>
#endif
#endif
#endif
#ifdef HAVE_XMMINTRIN_H
#include <xmmintrin.h>
#endif
#endif
 

// We will use the defines __SSE__, __SSE2__ and __SSE3__
// GCC defines these based upon command line args.
#if defined(USE_MMX) && !defined(__MMX__)
#define __MMX__ 1
#endif
#if defined(USE_SSE) && !defined(__SSE__)
#define __SSE__ 1
#endif
#if defined(USE_SSE2) && !defined(__SSE2__)
#define __SSE2__ 1
#endif
#if defined(USE_SSE3) && !defined(__SSE3__)
#define __SSE3__ 1
#endif

#if defined(__MMX__)
// MMX instructions/macros here.
#endif

#if defined(__3DNOW__)

// 3DNOW functions/macros here
#ifdef HAVE_MM3DNOW_H
#include <mm3dnow.h>
#else
// #warning No 3DNow header available...
#endif

#endif

#if defined(__SSE3__)
// PNI specific functions/macros here.
#endif

#if defined(__SSE2__)
// SSE2 specific functions/macros here.
#ifdef _MSC_VER
typedef __m128d x86_m128d;
#else
typedef double x86_m128d __attribute__ ((mode(V2DF))) __attribute__((aligned(16)));
#endif
#endif

#if defined(__SSE__)
// SSE specific functions/macros here.
#ifdef _MSC_VER
typedef __m128 x86_m128;
typedef __m128i x86_m128i;
#else
typedef float x86_m128 __attribute__ ((mode(V4SF))) __attribute__((aligned(16)));
typedef int x86_m128i __attribute__ ((mode(V4SI))) __attribute__((aligned(16)));
#endif

static inline void prefetcht0(const void *v) {
#ifdef USE_INTRINSICS
   _mm_prefetch((const char *)v,_MM_HINT_T0);
#elif defined(__GNUC__)
   __asm__ volatile ( "prefetcht0 %0" : : "m" (*(const char *)v) );
#endif
}

static inline void prefetcht1(const void *v) {
#ifdef USE_INTRINSICS
   _mm_prefetch((const char *)v,_MM_HINT_T1);
#elif defined(__GNUC__)     
   __asm__ volatile ( "prefetcht1 %0" : : "m" (*(const char *)v) );
#endif
}

static inline void prefetcht2(const void *v) {
#ifdef USE_INTRINSICS
   _mm_prefetch((const char *)v,_MM_HINT_T2);
#elif defined(__GNUC__)   
   __asm__ volatile ( "prefetcht2 %0" : : "m" (*(const char *)v) );
#endif
}

static inline void prefetchnta(const void *v) {
#ifdef USE_INTRINSICS
   _mm_prefetch((const char *)v,_MM_HINT_NTA);
#elif defined(__GNUC__) 
   __asm__ volatile ( "prefetchnta %0" : : "m" (*(const char *)v) );
#endif
}

#endif

#ifndef __m128d 
#define __m128d x86_m128d
#endif

#ifndef __m128i
#define __m128i x86_m128i
#endif

#ifndef __m128
#define __m128 x86_m128
#endif

#endif
