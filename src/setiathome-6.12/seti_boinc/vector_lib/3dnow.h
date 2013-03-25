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
// $Id: 3dnow.h,v 1.3 2004/10/28 21:15:36 korpela Exp $
//
// Specific template instances for 3DNow! supported SIMD routines.  This file
// is included from "simd.h"
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//

#include "mmx.h"

static const float zeros[4]={0,0};
static const float ones[4]={1,1};

//  First lets do the easy operators......
//--------------------- binary operators-------------------------------------
#define tdn_deref(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::ptr::operator *() { \
      return reinterpret_cast<simd<t,n>::arrtype REF>(*(__m64 *)(v)); \
}

tdn_deref(float,2)

#define tdn_array_deref(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::ptr::operator [](int i) { \
      return reinterpret_cast<simd<t,n>::arrtype REF >(*((__m64 *)(v)+i)); \
}

tdn_array_deref(float,2)

#define tdn_commutative_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) const  \
{ \
        register __m64 retval64; \
	__asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=y" (retval64) \
	:  "%0" (m64.all) , \
	   "ym" (b.m64.all) \
	); \
	return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

#define tdn_noncomm_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) const  \
{ \
        register __m64 retval64; \
	__asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=y" (retval64) \
	:  "0" (m64.all) , \
	   "ym" (b.m64.all) \
	); \
	return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

#define tdn_binop_const(t,n,__op,__instr,__followup_instr, __const_operand) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op() const  \
{ \
    register __m64 retval64; \
    __asm__ ( __instr " %2,%1\n" __followup_instr \
	:  "=y" (retval64) \
	:  "0" (__const_operand) , \
           "ym" (m64.all) \
    ); \
    return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

#define tdn_assign_op(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) \
{ \
	__asm__ ( __instr " %1,%0\n" __followup_instr \
	:  "=y" (m64.all) \
	:  "ym" (b.m64.all) \
	); \
	return *this; \
}

#define tdn_comp_op(t,n,__op,instr) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator __op(const \
    simd<t,n>::arrtype &b) const \
{ \
  register __m32 retval32; \
  register __m16 retval16; \
  __asm__ ( instr" %2,%1\n" \
            "\tpackssdw %1,%1\n" \
            "\tpacksswb %1,%0\n" \
	    "\tpandn _trueb,%0\n" \
             : "=r" (retval16) \
	     : "%y" (m64.all), \
	       "ym" (b.m64.all) \
  ); \
  return reinterpret_cast<simd<pbool,n>::arrtype REF>(retval32); \
}

tdn_commutative_binop(float,2,+,"pfadd","")
tdn_commutative_binop(float,2,*,"pfmul","")

tdn_noncomm_binop(float,2,-,"pfsub","")

tdn_binop_const(float,2,-,"pfsub","",zeros)

tdn_assign_op(float,2,+=,"pfadd","")
tdn_assign_op(float,2,*=,"pfmul","")
tdn_assign_op(float,2,-=,"pfsub","")

tdn_comp_op(float,2,==,"pfcmpeq")
tdn_comp_op(float,2,>=,"pfcmpge")
tdn_comp_op(float,2,>,"pfcmpgt")

//-----------------------------division----------------------------------
#define tdn_divop(t,n,__op) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
	const simd<t,n>::arrtype &b) const  \
{ \
        register __m64 intermed,retval64; \
	__asm__ ( "pfrcp %3,%1\n" \
	          "\punpckldq %3,%3\n" \
	          "\tpfrcit1 %1,%3\n" \
	          "\tpfrcit2 %1,%3\n" \
	          "\tpfmul %3,%0\n" \
	:  "=y" (retval64),"=y" (intermed) \
	:  "0" (m64.all) , \
	   "y" (b.m64.all) \
        ); \
	return reinterpret_cast<simd<t,n>::arrtype &>(retval64); \
}

tdn_divop(float,2,/)

//-----------------------------sqrt--------------------------------
   
inline simd<float,2>::arrtype REF simd<float,2>::arrtype::sqrt() const {
  return (*this)*rsqrt();
}

inline simd<float,2>::arrtype REF simd<float,2>::arrtype::aprx_sqrt() const  {
  return (*this)*aprx_rsqrt();
}

//-----------------------------reciprocal--------------------------------


#define tdn_pfrcp_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::aprx_recip() \
  const \
{ \
  register __m64 retval64; \
  __asm__ ( "pfrcp %1,%0\n" \
             : "=y" (retval64) \
	     : "ym" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}

tdn_pfrcp_macro(float,2)

#define tdn_recip_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::recip() \
  const \
{ \
  register __m64 intermed,retval64; \
  __asm__ ( "pfrcp %0,%1\n" \
	    "\punpckldq %0,%0\n" \
	    "\tpfrcpit1 %1,%0\n" \
	    "\tpfrcpit2 %1,%0\n" \
            : "=y" (retval64), "=y" (intermed) \
            : "0" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}
tdn_recip_macro(float,2)

//-----------------------------reciprocal sqrt--------------------------------

#define tdn_rsqrt_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::rsqrt() \
  const \
{ \
  register __m64 retval64,intermed; \
  __asm__ ( "pfrsqrt %2,%1\n" \
            "\tmovq %1,%0\n" \
            "\tfpmul %1,%1\n" \
            "\tpunpckldq %2,%2\n" \
            "\tpfrsqit1 %2,%1" \
            "\tpfrcpit2 %0,%1" \
             : "=y" (intermed), "=y" (retval64)  \
	     : "y" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}
tdn_rsqrt_macro(float,4)

#define tdn_pfrsqrt_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::aprx_rsqrt() \
  const \
{ \
  register __m64 retval64; \
  __asm__ ( "pfrsqrt %1,%0\n" \
             : "=y" (retval64)  \
	     : "ym" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}
tdn_pfrsqrt_macro(float,4)

//-----------------------------max--------------------------------

#define tdn_pfmax_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::max(const simd<t,n>::arrtype &b) \
  const \
{ \
  register __m64 retval64; \
  __asm__ ( "pfmax %2,%1\n" \
             : "=y" (retval64)  \
	     : "%0" (m64.all), \
	       "ym" (b.m64.all)  \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}
tdn_pfmax_macro(float,2)

//-----------------------------min--------------------------------

#define tdn_pfmin_macro(t,n) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::min(const simd<t,n>::arrtype &b) \
  const \
{ \
  register __m64 retval64; \
  __asm__ ( "pfmin %2,%1\n" \
             : "=y" (retval64)  \
	     : "%0" (m64.all), \
	       "ym" (b.m64.all)  \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype REF>(retval64); \
}
tdn_pfmin_macro(float,2)

//-----------------------------shuffle---------------------------------------


//-----------------------------interleave-----------------------------------


//-----------------------------ancillary-------------------------------------
  
#define tdn_start_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_start() { \
}
tdn_start_macro(float,4)

#define tdn_end_macro(t,n)  \
inline volatile  void  simd<t,n>::simd_mode_finish() { \
    __asm__ volatile (EMMS \
	    :  \
	    :  \
	    : FP_STACK,MMX_REGS \
	   ); \
}
tdn_end_macro(float,4)

template <typename T, unsigned int N>
inline void simd<T,N>::arrtype::prefetch() const {
  __asm__ ( "prefetch %0" 
      : 
      : "m" (*(reinterpret_cast<char *>(this)+32)));
}

template <typename T, unsigned int N>
inline void simd<T,N>::arrtype::prefetchw() const {
  __asm__ ( "prefetch %0" 
      : 
      : "m" (*(reinterpret_cast<char *>(this)+32)));
}

/*
 *
 * $Log: 3dnow.h,v $
 * Revision 1.3  2004/10/28 21:15:36  korpela
 * Added prefetch support.
 *
 * Revision 1.1  2004/09/29 16:42:02  korpela
 * *** empty log message ***
 *
 *
 */

