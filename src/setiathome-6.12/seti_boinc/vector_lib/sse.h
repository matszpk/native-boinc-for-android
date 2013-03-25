// Copyright 2004 Regents of the University of California
//
// libSIMD++ is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.
//
// libSIMD++ is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along
// with libSIMD++; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// $Id: sse.h,v 1.11.2.1 2007/03/22 00:04:00 korpela Exp $
//
// Specific template instances for SSE supported SIMD routines.  This file
// is included from "simd.h"
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//

#include "mmx.h"

#ifdef __GNUC__
// register int retval128 __asm__("xmm7") __attribute__ ((mode(V4SF)));
#endif

static const float zeros[4]={0,0,0,0};
static const float ones[4]={1,1,1,1};
static const float twos[4]={2,2,2,2};
static const float threes[4]={3,3,3,3};
static const float halves[4]={0.5,0.5,0.5,0.5};
typedef enum {
  eq=0, 
  lt, 
  leq, 
  gt, 
  geq, 
  unordered, 
  ne, 
  nlt, 
  nleq, 
  ngt,
  ngeq, 
  ordered
} __sse_cond_code;

static const unsigned long cmpps_results[16]={
  0x00000000,
  0x00000001,
  0x00000100,
  0x00000101,
  0x00010000,
  0x00010001,
  0x00010100,
  0x00010101,
  0x01000000,
  0x01000001,
  0x01000100,
  0x01000101,
  0x01010000,
  0x01010001,
  0x01010100,
  0x01010101
};

//  First lets do the easy operators......
//--------------------- binary operators-------------------------------------
#define sse_deref(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::ptr::operator *() { \
      return reinterpret_cast<simd<t,n>::arrtype REF>(*(__m128 *)(v)); \
}

sse_deref(float,4)

#define sse_array_deref(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::ptr::operator [](int i) { \
      return reinterpret_cast<simd<t,n>::arrtype REF >(*((__m128 *)(v)+i)); \
}

sse_array_deref(float,4)

#define sse_commutative_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) const  \
{ \
        register __m128 retval128; \
	__asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=x" (retval128) \
	:  "%0" (m128.all) , \
	   "xm" (b.m128.all) \
	); \
	return reinterpret_cast<simd<t,n>::arrtype REF >(retval128); \
}

#define sse_noncomm_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) const  \
{ \
        register __m128 retval128; \
	__asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=x" (retval128) \
	:  "0" (m128.all) , \
	   "xm" (b.m128.all) \
	); \
	return reinterpret_cast<simd<t,n>::arrtype REF >(retval128); \
}

#define sse_binop_const(t,n,__op,__instr,__followup_instr, __const_operand) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op() const  \
{ \
    register __m128 retval128; \
    __asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=x" (retval128) \
	:  "0" (__const_operand) , \
           "xm" (m128.all) \
    ); \
    return reinterpret_cast<simd<t,n>::arrtype REF >(retval128); \
}

#define sse_assign(t,n,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator =( \
	const simd<t,n>::arrtype &b) \
{ \
	__asm__ ( "/* nada */" \
	:  "=x" (m128.all) \
	:  "0" (b.m128.all) \
	); \
	return *this; \
}

#define sse_assign_op(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) \
{ \
	__asm__ ( __instr " %1,%0\n" __followup_instr \
	:  "=x" (m128.all) \
	:  "xm" (b.m128.all) \
	); \
	return *this; \
}

#define sse_comp_op(t,n,__op,__op_mnem) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator __op(const \
    simd<t,n>::arrtype &b) const \
{ \
  register __m32 retval32; \
  __asm__ ( "cmpps %3,%2,%1\n" \
            "\tmovmskps %1,%0\n" \
	    "\tmovl %4(%0),%0\n" \
             : "=r" (retval32) \
	     : "%x" (m128.all), \
	       "xm" (b.m128.all), \
	       "i" (__op_mnem),  \
	       "m" (cmpps_results) \
  ); \
  return reinterpret_cast<simd<pbool,n>::arrtype REF>(retval32); \
}

sse_commutative_binop(float,4,+,"addps","")
sse_commutative_binop(float,4,*,"mulps","")

sse_noncomm_binop(float,4,-,"subps","")
sse_noncomm_binop(float,4,/,"divps","")

sse_binop_const(float,4,-,"subps","",zeros)

sse_assign(float,4,"movaps","")
sse_assign_op(float,4,+=,"addps","")
sse_assign_op(float,4,*=,"mulps","")
sse_assign_op(float,4,-=,"subps","")
sse_assign_op(float,4,/=,"divps","")

sse_comp_op(float,4,==,eq)
sse_comp_op(float,4,>,gt)
sse_comp_op(float,4,<,lt)
sse_comp_op(float,4,>=,geq)
sse_comp_op(float,4,<=,leq)
sse_comp_op(float,4,!=,ne)

//-----------------------------sqrt--------------------------------

#define sse_sqrtps_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::sqrt() \
  const \
{ \
  register __m128 retval128; \
  __asm__ ( "sqrtps %1,%0\n" \
             : "=x" (retval128)  \
	     : "xm" (m128.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype &>(retval128); \
}

sse_sqrtps_macro(float,4)

//-----------------------------reciprocal--------------------------------


#define sse_rcpps_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::aprx_recip() \
  const \
{ \
  register __m128 retval128; \
  __asm__ ( "rcpps %1,%0\n" \
             : "=x" (retval128) \
	     : "xm" (m128.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval128); \
}
sse_rcpps_macro(float,4)

//-----------------------------reciprocal sqrt--------------------------------

#define sse_rsqrtps_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::aprx_rsqrt() \
  const \
{ \
  register __m128 retval128; \
  __asm__ ( "rsqrtps %1,%0\n" \
             : "=x" (retval128)  \
	     : "xm" (m128.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval128); \
}
sse_rsqrtps_macro(float,4)

//-----------------------------max--------------------------------

#define sse_maxps_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::max(const simd<t,n>::arrtype &b) \
  const \
{ \
  register __m128 retval128; \
  __asm__ ( "maxps %2,%1\n" \
             : "=x" (retval128)  \
	     : "%0" (m128.all), \
	       "xm" (b.m128.all)  \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval128); \
}
sse_maxps_macro(float,4)

//-----------------------------min--------------------------------

#define sse_minps_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::min(const simd<t,n>::arrtype &b) \
  const \
{ \
  register __m128 retval128; \
  __asm__ ( "minps %2,%1\n" \
             : "=x" (retval128)  \
	     : "%0" (m128.all), \
	       "xm" (b.m128.all)  \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval128); \
}
sse_minps_macro(float,4)

//-----------------------------shuffle---------------------------------------

#define sse_shuffle_macro(t,n) \
template <const long long order> \
inline typename simd<t,n>::arrtype REF shuffle(const typename simd<t,n>::arrtype &v)\
{ \
    register __m128 retval128; \
    __asm__ ("shufps %2,%1,%0\n" : \
	     "=x" (retval128) : \
	     "xm" (v.m128.all), \
	     "i" (((order&0x3000)>>12)| \
		  ((order&0x0300)>>6)| \
	           (order&0x0030)| \
	           ((order&0x0003)<<6)), \
	     "0"  (v.m128.all) \
	); \
    return reinterpret_cast<simd<t,n>::arrtype REF >(retval128); \
}

sse_shuffle_macro(float,4)
sse_shuffle_macro(long,4)
sse_shuffle_macro(unsigned long,4)

//-----------------------------interleave-----------------------------------
#define sse_interleave_macro(t,n) \
template <const long long order> \
inline typename simd<t,n>::arrtype REF interleave( \
    const typename simd<t,n>::arrtype &a, \
    const typename simd<t,n>::arrtype &b) \
{ \
    register __m128 retval128; \
    __asm__ ("shufps %3,%2,%0\n" : \
	     "=x" (retval128) :\
	     "0" (a.m128.all), \
	     "xm" (b.m128.all), \
	     "i" (((order & 0x3000)>>12)| \
		  ((order & 0x0300)>> 6)| \
	          ((order & 0x0030)    )| \
	          ((order & 0x0003)<<6 ))  \
	); \
    return reinterpret_cast<simd<t,n>::arrtype REF >(retval128); \
}

sse_interleave_macro(float,4)  

//-----------------------------ancillary-------------------------------------
  
#define sse_start_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_start() { \
}
sse_start_macro(float,4)

#ifndef USE_3DNOW
#define EMMS "emms\n"
#else
#define EMMS "femms\n"
#endif

#define sse_end_macro(t,n)  \
inline volatile  void  simd<t,n>::simd_mode_finish() { \
    __asm__(EMMS \
	    :  \
	    :  \
	    : FP_STACK,MMX_REGS \
	   ); \
}
sse_end_macro(float,4)

template <typename T, unsigned int N>
inline void simd<T,N>::arrtype::prefetch() const {
    __asm__ ( "prefetch %0" 
	      : 
	      : "m" (*(reinterpret_cast<char *>(this)+64)));
}

template <typename T, unsigned int N>
inline void simd<T,N>::arrtype::prefetchw() const {
    __asm__ ( "prefetch %0" 
	      : 
	      : "m" (*(reinterpret_cast<char *>(this)+64)));
}


/*
 *
 * $Log: sse.h,v $
 * Revision 1.11.2.1  2007/03/22 00:04:00  korpela
 * *** empty log message ***
 *
 * Revision 1.11  2004/12/08 00:19:23  korpela
 * Fixed a signedness mismatch in pulsefind.cpp.
 * Let win-config.h know that "glut.h" exists.
 * Reduced windows includes to only "windows.h" in bmplib.cpp
 * Added some prefetch support to the experimental vector template library.
 *
 *
 * Revision 1.1  2004/09/17 01:42:36  korpela
 * initial checkin of test code.
 *
 *
 */

