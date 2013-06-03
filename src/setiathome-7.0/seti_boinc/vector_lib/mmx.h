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
// $Id: mmx.h,v 1.7.2.1 2007/03/22 00:03:59 korpela Exp $
//
// Specific template instances for MMX supported SIMD routines.  This file
// is included from "simd.h"
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//

#define FP_STACK "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)"
#define MMX_REGS "mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7"

#ifdef __GNUC__
//register int retval64 __asm__("mm7") __attribute__ ((mode(V8QI)));
//register unsigned long retval32 __asm__("ebx");
//register unsigned short retval16 __asm__("bx");
#endif

static const char trueb[8]={1,1,1,1,1,1,1,1};
static const short truew[4]={1,1,1,1};
static const long trued[2]={1,1};
static const v_i64 falsed=0;
static const v_i64 minus_one=(v_i64)(-1);
static const v_i64 hi_d=(v_i64)0xffffffff00000000LL;
static const v_i64 lo_d=(v_i64)0x00000000ffffffffLL;

//  First lets do the easy operators......
//--------------------- binary operators -------------------------------------

#define mmx_deref(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::ptr::operator *() { \
  return reinterpret_cast<simd<t,n>::arrtype &>(*(__m64 *)(v)); \
}

mmx_deref(char,8)
mmx_deref(unsigned char,8)
mmx_deref(signed char,8)
mmx_deref(short,4)
mmx_deref(unsigned short,4)
mmx_deref(long,2)
mmx_deref(unsigned long,2)

#define mmx_array_deref(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::ptr::operator [](int i) { \
  return reinterpret_cast<simd<t,n>::arrtype &>(*((__m64 *)(v)+i)); \
}

mmx_array_deref(char,8)
mmx_array_deref(unsigned char,8)
mmx_array_deref(signed char,8)
mmx_array_deref(short,4)
mmx_array_deref(unsigned short,4)
mmx_array_deref(long,2)
mmx_array_deref(unsigned long,2)

#define mmx_commutative_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %2,%1\n" __followup_instr \
      :  "=y" (retval64) \
      :  "%0" (m64.all) , \
         "ym" (b.m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define mmx_noncomm_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %2,%1\n" __followup_instr \
      :  "=y" (retval64) \
      :  "0" (m64.all) , \
         "ym" (b.m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define mmx_binop_const(t,n,__op,__instr,__followup_instr, __const_operand) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op() const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %2,%1\n" __followup_instr \
      :  "=y" (retval64) \
      :  "0" (__const_operand) , \
         "ym" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define mmx_assign(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) \
{ \
   __asm__ volatile ( "" \
             : "=y" (m64.all)   \
	     : "0" (b.m64.all)  \
   ); \
   return *static_cast<simd<t,n>::arrtype *>(this); \
}

#define mmx_assign_op(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) \
{ \
   __asm__ ( __instr" %1,%0\n" __followup_instr  \
             : "=ym" (m64.all)   \
	     : "y" (b.m64.all)  \
   ); \
   return *static_cast<simd<t,n>::arrtype *>(this); \
}


//--------------------- addition -------------------------------------

mmx_commutative_binop(char,8,+,"paddb","")
mmx_commutative_binop(signed char,8,+,"paddb","")
mmx_commutative_binop(unsigned char,8,+,"paddb","")
mmx_commutative_binop(short,4,+,"paddw","")
mmx_commutative_binop(unsigned short,4,+,"paddw","")
mmx_commutative_binop(long,2,+,"paddd","")
mmx_commutative_binop(unsigned long,2,+,"paddd","")

//---------------------- multiplication ------------------------------
mmx_commutative_binop(short,4,*,"pmullw","")
mmx_commutative_binop(unsigned short,4,*,"pmullw","")

//---------------------- subtraction ---------------------------------
mmx_noncomm_binop(char,8,-,"psubb","")
mmx_noncomm_binop(signed char,8,-,"psubb","")
mmx_noncomm_binop(unsigned char,8,-,"psubb","")
mmx_noncomm_binop(short,4,-,"psubw","")
mmx_noncomm_binop(unsigned short,4,-,"psubw","")
mmx_noncomm_binop(long,2,-,"psubd","")
mmx_noncomm_binop(unsigned long,2,-,"psubd","")

//-----------------------------AND--------------------------------
mmx_commutative_binop(char,8,&,"pand","")
mmx_commutative_binop(signed char,8,&,"pand","")
mmx_commutative_binop(unsigned char,8,&,"pand","")
mmx_commutative_binop(short,4,&,"pand","")
mmx_commutative_binop(unsigned short,4,&,"pand","")
mmx_commutative_binop(long,2,&,"pand","")
mmx_commutative_binop(unsigned long,2,&,"pand","")
mmx_commutative_binop(long long,1,&,"pand","")
mmx_commutative_binop(unsigned long long,1,&,"pand","")

//-----------------------------OR--------------------------------
mmx_commutative_binop(char,8,|,"por","")
mmx_commutative_binop(signed char,8,|,"por","")
mmx_commutative_binop(unsigned char,8,|,"por","")
mmx_commutative_binop(short,4,|,"por","")
mmx_commutative_binop(unsigned short,4,|,"por","")
mmx_commutative_binop(long,2,|,"por","")
mmx_commutative_binop(unsigned long,2,|,"por","")
mmx_commutative_binop(long long,1,|,"por","")
mmx_commutative_binop(unsigned long long,1,|,"por","")

//-----------------------------XOR--------------------------------
mmx_commutative_binop(char,8,^,"pxor","")
mmx_commutative_binop(signed char,8,^,"pxor","")
mmx_commutative_binop(unsigned char,8,^,"pxor","")
mmx_commutative_binop(short,4,^,"pxor","")
mmx_commutative_binop(unsigned short,4,^,"pxor","")
mmx_commutative_binop(long,2,^,"pxor","")
mmx_commutative_binop(unsigned long,2,^,"pxor","")
mmx_commutative_binop(long long,1,^,"pxor","")
mmx_commutative_binop(unsigned long long,1,^,"pxor","")

//-----------------------------NOT--------------------------------
mmx_binop_const(char,8,~,"pxor","",minus_one);
mmx_binop_const(unsigned char,8,~,"pxor","",minus_one);
mmx_binop_const(signed char,8,~,"pxor","",minus_one);
mmx_binop_const(short,4,~,"pxor","",minus_one);
mmx_binop_const(unsigned short,4,~,"pxor","",minus_one);
mmx_binop_const(long,2,~,"pxor","",minus_one);
mmx_binop_const(unsigned long,2,~,"pxor","",minus_one);
mmx_binop_const(long long,1,~,"pxor","",minus_one);
mmx_binop_const(unsigned long long,1,~,"pxor","",minus_one);

//-----------------------------negative--------------------------------
mmx_binop_const(char,8,-,"psubb","",falsed);
mmx_binop_const(unsigned char,8,-,"psubb","",falsed);
mmx_binop_const(signed char,8,-,"psubb","",falsed);
mmx_binop_const(short,4,-,"psubw","",falsed);
mmx_binop_const(unsigned short,4,-,"psubw","",falsed);
mmx_binop_const(long,2,-,"psubd","",falsed);
mmx_binop_const(unsigned long,2,-,"psubd","",falsed);

//--------------------- assignment -----------------------------------
mmx_assign(char,8,=,"","")
mmx_assign(unsigned char,8,=,"","")
mmx_assign(signed char,8,=,"","")
mmx_assign(short,4,=,"","")
mmx_assign(unsigned short,4,=,"","")
mmx_assign(long,2,=,"","")
mmx_assign(unsigned long,2,=,"","")
mmx_assign(float,2,=,"","")
mmx_assign(long long,1,=,"","")
mmx_assign(unsigned long long,1,=,"","")
mmx_assign(double,1,=,"","")

mmx_assign_op(char,8,&=,"pand","")
mmx_assign_op(unsigned char,8,&=,"pand","")
mmx_assign_op(signed char,8,&=,"pand","")
mmx_assign_op(short,4,&=,"pand","")
mmx_assign_op(unsigned short,4,&=,"pand","")
mmx_assign_op(long,2,&=,"pand","")
mmx_assign_op(unsigned long,2,&=,"pand","")
mmx_assign_op(long long,1,&=,"pand","")
mmx_assign_op(unsigned long long,1,&=,"pand","")

mmx_assign_op(char,8,|=,"por","")
mmx_assign_op(unsigned char,8,|=,"por","")
mmx_assign_op(signed char,8,|=,"por","")
mmx_assign_op(short,4,|=,"por","")
mmx_assign_op(unsigned short,4,|=,"por","")
mmx_assign_op(long,2,|=,"por","")
mmx_assign_op(unsigned long,2,|=,"por","")
mmx_assign_op(long long,1,|=,"por","")
mmx_assign_op(unsigned long long,1,|=,"por","")

mmx_assign_op(char,8,^=,"pxor","")
mmx_assign_op(unsigned char,8,^=,"pxor","")
mmx_assign_op(signed char,8,^=,"pxor","")
mmx_assign_op(short,4,^=,"pxor","")
mmx_assign_op(unsigned short,4,^=,"pxor","")
mmx_assign_op(long,2,^=,"pxor","")
mmx_assign_op(unsigned long,2,^=,"pxor","")
mmx_assign_op(long long,1,^=,"pxor","")
mmx_assign_op(unsigned long long,1,^=,"pxor","")

mmx_assign_op(char,8,+=,"paddb","")
mmx_assign_op(unsigned char,8,+=,"paddb","")
mmx_assign_op(signed char,8,+=,"paddb","")
mmx_assign_op(short,4,+=,"paddw","")
mmx_assign_op(unsigned short,4,+=,"paddw","")
mmx_assign_op(long,2,+=,"paddd","")
mmx_assign_op(unsigned long,2,+=,"paddd","")

mmx_assign_op(char,8,-=,"psubb","")
mmx_assign_op(unsigned char,8,-=,"psubb","")
mmx_assign_op(signed char,8,-=,"psubb","")
mmx_assign_op(short,4,-=,"psubw","")
mmx_assign_op(unsigned short,4,-=,"psubw","")
mmx_assign_op(long,2,-=,"psubd","")
mmx_assign_op(unsigned long,2,-=,"psubd","")

mmx_assign_op(short,4,*=,"pmullw","")
mmx_assign_op(unsigned short,4,*=,"pmullw","")

// unfortunately logical ops require more fiddling
// ------------- generic logical ops
//
#define mmx_logical_op(t,n,__op,__op_t,__instr,__conv_ops, __out_spec, __outvar) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
    const simd<__op_t,n>::arrtype &b) const \
{ \
  register __m64 retval64; \
  register __m32 retval32; \
  register __m16 retval16; \
  __asm__ ( __instr" %2,%1\n\t"__conv_ops"\n" \
	    : __out_spec (__outvar) \
	    : "y" (b.m64.all), \
              "ym" (m64.all) \
      ); \
  return reinterpret_cast<simd<pbool,n>::arrtype &>(__outvar); \
}

#define mmx_logical_byteop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,8,__op,__op_t,__instr, \
    "pand _trueb,%1\n" \
    "\tmovq %1,%0",          \
    "=y",retval64)

#define mmx_logical_wordop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpand _trueb,%1\n" \
    "\tmovd %1,%0", \
    "=r",retval32)

#define mmx_logical_dwordop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpacksswb %1,%1\n" \
    "\tpand _trueb,%1\n" \
    "\tmovd %1,%0",          \
    "=a",retval16)

#define mmx_logical_notbyteop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,8,__op,__op_t,__instr, \
    "pandn _trueb,%1\n" \
    "\tmovq %1,%0",          \
    "=y",retval64)

#define mmx_logical_notwordop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpandn _trueb,%1\n" \
    "\tmovd %1,%0", \
    "=r",retval32)

#define mmx_logical_notdwordop(t,__op,__op_t,__instr) \
   mmx_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpacksswb %1,%1\n" \
    "\tpandn _trueb,%1\n" \
    "\tmovd %1,%0",          \
    "=a",retval16)

//-----------------------------EQUALS--------------------------------
  
mmx_logical_byteop(char,==,char,"pcmpeqb")
mmx_logical_byteop(unsigned char,==,unsigned char, "pcmpeqb")
mmx_logical_byteop(signed char,==,signed char, "pcmpeqb")

mmx_logical_wordop(short,==,short,"pcmpeqw")
mmx_logical_wordop(unsigned short,==,unsigned short,"pcmpeqw")

mmx_logical_dwordop(long,==,long,"pcmpeqd")
mmx_logical_dwordop(unsigned long,==,unsigned long,"pcmpeqd")

//-----------------------------NOT EQUALS--------------------------------
  
mmx_logical_notbyteop(char,!=,char, "pcmpeqb")
mmx_logical_notbyteop(unsigned char,!=,unsigned char, "pcmpeqb")
mmx_logical_notbyteop(signed char,!=,signed char, "pcmpeqb")

mmx_logical_notwordop(short,!=,short,"pcmpeqw")
mmx_logical_notwordop(unsigned short,!=,unsigned short,"pcmpeqw")

mmx_logical_notdwordop(long,!=,long,"pcmpeqd")
mmx_logical_notdwordop(unsigned long,!=,unsigned long,"pcmpeqd")

//-----------------------------GREATER THAN--------------------------------
#if (CHAR_MIN<0)
mmx_logical_byteop(char,>,char, "pcmpgtb")
#endif
mmx_logical_byteop(signed char,>,signed char, "pcmpgtb")

mmx_logical_wordop(short,>,short,"pcmpgtw")

mmx_logical_dwordop(long,>,long,"pcmpgtd")


//-----------------------------LESS THAN OR EQUAL----------------------------
#if (CHAR_MIN<0)
mmx_logical_notbyteop(char,<=,char, "pcmpgtb")
#endif
mmx_logical_notbyteop(signed char,<=,signed char, "pcmpgtb")

mmx_logical_notwordop(short,<=,short,"pcmpgtw")

mmx_logical_notdwordop(long,<=,long,"pcmpgtd")

//-----------------------------LESS THAN--------------------------------
#if (CHAR_MIN<0)
mmx_logical_byteop(char,<,char, "pcmpltb")
#endif
mmx_logical_byteop(signed char,<,signed char, "pcmpltb")

mmx_logical_wordop(short,<,short,"pcmpltw")

mmx_logical_dwordop(long,<,long,"pcmpltd")

//-----------------------------GREATER THAN OR EQUAL-----------------------
#if (CHAR_MIN<0)
mmx_logical_notbyteop(char,>=,char, "pcmpltb")
#endif
mmx_logical_notbyteop(signed char,>=,signed char, "pcmpltb")

mmx_logical_notwordop(short,>=,short,"pcmpltw")

mmx_logical_notdwordop(long,>=,long,"pcmpltd")

//-------------------------LOGICAL OR--------------------------------

#define mmx_por_macro(t,n) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator ||(const \
    simd<t,n>::arrtype &b) const \
{ \
  return ((*this)|b)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

mmx_por_macro(char,8)
mmx_por_macro(unsigned char,8)
mmx_por_macro(signed char,8)

mmx_por_macro(short,4)
mmx_por_macro(unsigned short,4)

mmx_por_macro(long,2)
mmx_por_macro(unsigned long,2)

//-------------------------LOGICAL AND--------------------------------
#define mmx_pand_macro(t,n) \
inline simd<pbool,n>::arrtype &simd<t,n>::arrtype::operator &&(const \
    simd<t,n>::arrtype &b) const \
{ \
  return ((*this)&b)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

mmx_pand_macro(char,8)
mmx_pand_macro(unsigned char,8)
mmx_pand_macro(signed char,8)

mmx_pand_macro(short,4)
mmx_pand_macro(unsigned short,4)

mmx_pand_macro(long,2)
mmx_pand_macro(unsigned long,2)

//-----------------------------LOGICAL NOT--------------------------------
#define mmx_pnot_macro(t,n) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator !() const \
{ \
  return (*this)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

mmx_pnot_macro(char,8)
mmx_pnot_macro(unsigned char,8)
mmx_pnot_macro(signed char,8)

mmx_pnot_macro(short,4)
mmx_pnot_macro(unsigned short,4)

mmx_pnot_macro(long,2)
mmx_pnot_macro(unsigned long,2)
//---------------------------average----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2) || defined(USE_3DNOW)
#define mmx_avg_macro(t,n,instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::avg( \
    const simd<t,n>::arrtype &b) const { \
  register __m64 retval64; \
  __asm__ ( instr" %2,%1\n" \
            : "=y" (retval64) \
            : "%0" (m128.all),\
              "ym" (b.m128.all) \
      ); \
  return reinterpret_cast<simd<t,n>::arrtype &>(retval64); \
}
#endif

#if defined(USE_3DNOW)
mmx_avg_macro(unsigned char,8,"pavgusb")
#if (CHAR_MIN==0)
mmx_avg_macro(char,8,"pavgusb")
#endif
#elif defined(USE_SSE) || defined(USE_SSE2)
mmx_avg_macro(unsigned short,4,"pavgw")
mmx_avg_macro(unsigned char,8,"pavgb")
#if (CHAR_MIN==0)
mmx_avg_macro(char,8,"pavgb")
#endif
#endif

//---------------------------max----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2)
#define mmx_max_macro(t,n,instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::max( \
    const simd<t,n>::arrtype &b) const { \
  register __m64 retval64; \
  __asm__ ( instr" %2,%1\n" \
            : "=y" (retval64) \
            : "%0" (m128.all), \
              "ym" (b.m128.all) \
      ); \
  return reinterpret_cast<simd<t,n>::arrtype &>(retval64); \
}

#if (CHAR_MIN<0)
mmx_max_macro(char,8,"pmaxsb")
#else
mmx_max_macro(char,8,"pmaxub")
#endif
mmx_max_macro(unsigned char,8,"pmaxub")
mmx_max_macro(signed char,8,"pmaxsb")
mmx_max_macro(short,4,"pmaxsw")
mmx_max_macro(unsigned short,4,"pmaxuw")

#endif
//---------------------------min----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2)
#define mmx_min_macro(t,n,instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::min( \
    const simd<t,n>::arrtype &b) const { \
  register __m64 retval64; \
  __asm__ ( instr" %2,%1\n" \
            : "=y" (retval64) \
            : "%0" (m128.all), \
              "ym" (b.m128.all) \
      ); \
  return reinterpret_cast<simd<t,n>::arrtype &>(retval64); \
}

#if (CHAR_MIN<0)
mmx_min_macro(char,8,"pminsb")
#else
mmx_min_macro(char,8,"pminub")
#endif
mmx_min_macro(unsigned char,8,"pminub")
mmx_min_macro(signed char,8,"pminsb")
mmx_min_macro(short,4,"pminsw")
mmx_min_macro(unsigned short,4,"pminuw")

#endif

//-----------------------------shuffle---------------------------------------
#if defined(USE_SSE) || defined(USE_SSE2)
#define mmx_shuffle_macro(t,n) \
template <const long long order> \
inline typename simd<t,n>::arrtype REF shuffle( \
    const typename simd<t,n>::arrtype &v)\
{ \
  register __m64 retval64; \
  __asm__ ("pshufw %2,%1,%0\n" : \
           "=y" (retval64) : \
           "ym" (v.m64.all), \
           "i" (((order&0x3000)>>12)| \
                ((order&0x0300)>>6)| \
                 (order&0x0030)| \
                ((order&0x0003)<<6)) \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

mmx_shuffle_macro(short,4)
mmx_shuffle_macro(unsigned short,4)
#endif
#undef mmx_shuffle_macro

#define mmx_shuffle_macro(t,n) \
template <const long long order> \
extern typename simd<t,n>::arrtype REF shuffle( \
    const typename simd<t,n>::arrtype &v) ;

#define mmx_spec_shuffle_macro(t,n,order,instr) \
template <> \
inline simd<t,n>::arrtype REF shuffle<order>( \
    const simd<t,n>::arrtype &v) \
{ \
  register __m64 retval64; \
  __asm__ (instr" %2,%0\n" : \
           "=y" (retval64) : \
           "0" (v.m64.all) , \
           "ym" (v.m64.all)  \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

mmx_spec_shuffle_macro(char,8,0x00112233,"punpcklbw");
mmx_spec_shuffle_macro(char,8,0x44556677,"punpckhbw");
mmx_spec_shuffle_macro(char,4,0x0011,"punpcklbw");
mmx_spec_shuffle_macro(char,4,0x4455,"punpckhbw");
mmx_spec_shuffle_macro(unsigned char,8,0x00112233,"punpcklbw");
mmx_spec_shuffle_macro(unsigned char,8,0x44556677,"punpckhbw");
//mmx_spec_shuffle_macro(unsigned char,4,0x0011,"punpcklbw");
//mmx_spec_shuffle_macro(unsigned char,4,0x4455,"punpckhbw");
mmx_spec_shuffle_macro(signed char,8,0x00112233,"punpcklbw");
mmx_spec_shuffle_macro(signed char,8,0x44556677,"punpckhbw");
mmx_spec_shuffle_macro(signed char,4,0x0011,"punpcklbw");
mmx_spec_shuffle_macro(signed char,4,0x4455,"punpckhbw");

mmx_spec_shuffle_macro(short,4,0x0011,"punpcklwd");
mmx_spec_shuffle_macro(short,4,0x2233,"punpckhwd");
mmx_spec_shuffle_macro(unsigned short,4,0x0011,"punpcklwd");
mmx_spec_shuffle_macro(unsigned short,4,0x2233,"punpckhwd");

mmx_spec_shuffle_macro(long,2,0x00,"punpckldq");
mmx_spec_shuffle_macro(long,2,0x11,"punpckhdq");
mmx_spec_shuffle_macro(unsigned long,2,0x00,"punpckldq");
mmx_spec_shuffle_macro(unsigned long,2,0x11,"punpckhdq");
mmx_spec_shuffle_macro(float,2,0x00,"punpckldq");
mmx_spec_shuffle_macro(float,2,0x11,"punpckhdq");

//-----------------------------interleave---------------------------------------
#define mmx_spec_interleave_macro(t,n,order,instr,mask) \
template <> \
inline simd<t,n>::arrtype REF interleave<order>( \
    const simd<t,n>::arrtype &a,const simd<t,n>::arrtype &b) \
{ \
  register __m64 retval64,dummy; \
  __asm__ ("pand %4,%0\n" \
           "\tpand %4,%3\n" \
           "\t"instr" %3,%0\n" : \
           "=y" (retval64),"=y" (dummy) : \
           "0" (a.m64.all) , \
           "1" (b.m64.all), \
           "ym" (mask) \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

mmx_spec_interleave_macro(signed char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);
mmx_spec_interleave_macro(unsigned char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);
mmx_spec_interleave_macro(char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);

#undef mmx_spec_interleave_macro
#define mmx_spec_interleave_macro(t,n,order,instr,shift_inst) \
template <> \
inline simd<t,n>::arrtype REF interleave<order>( \
    const simd<t,n>::arrtype &a,const simd<t,n>::arrtype &b) \
{ \
  register __m64 retval64,dummy; \
  __asm__ ( \
           shift_inst",%0\n" \
           "\t"shift_inst",%3\n" \
           "\t"instr" %3,%0\n" : \
           "=y" (retval64),"=y" (dummy) : \
           "0" (a.m64.all) , \
           "1" (b.m64.all) \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

mmx_spec_interleave_macro(signed char,8,0x13571357,"packuswb","psraw $8");
mmx_spec_interleave_macro(unsigned char,8,0x13571357,"packuswb","psraw $8");
mmx_spec_interleave_macro(char,8,1357135746,"packuswb","psraw $8");
mmx_spec_interleave_macro(short,4,0x1313,"packssdw","psrad $16");
mmx_spec_interleave_macro(unsigned short,4,0x1313,"packssdw","psrad $16");

#undef mmx_spec_interleave_macro
#define mmx_spec_interleave_macro(t,n,order,instr,shift_inst1,shift_inst2) \
template <> \
inline simd<t,n>::arrtype REF interleave<order>( \
    const simd<t,n>::arrtype &a,const simd<t,n>::arrtype &b) \
{ \
  register __m64 retval64,dummy; \
  __asm__ ( \
           shift_inst1",%0\n" \
           "\t"shift_inst1",%3\n" \
           "\t"shift_inst2",%0\n" \
           "\t"shift_inst2",%3\n" \
           "\t"instr" %3,%0\n" : \
           "=y" (retval64),"=y" (dummy) : \
           "0" (a.m64.all) , \
           "1" (b.m64.all) \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}
mmx_spec_interleave_macro(short,4,0x0202,"packssdw","pslld $16","psrad $16");
mmx_spec_interleave_macro(unsigned short,4,0x0202,"packssdw","pslld $16","psrad $16");

#undef mmx_spec_interleave_macro
#define mmx_spec_interleave_two(t,n) \
template <unsigned long order> \
inline simd<t,n>::arrtype REF interleave( \
    const simd<t,n>::arrtype &a,const simd<t,n>::arrtype &b) \
{ \
  register __m64 retval64,dummy; \
  __asm__ (  "psrlq %5,%0\n" \
             "\tpsllq %4,%1\n" \
             "\tpand  %1,%0\n" \
           : "=y" (retval64), "=y" (dummy) \
           : "0" (a.m64.all),\
             "y" (b.m64.all),\
             "i" ((order & 1)<<5),   \
             "i" (((order ^ 0x10) & 0x10)<<1) \
          ); \
  return reinterpret_cast<simd<t,n>::arrtype REF >(retval64); \
}

mmx_spec_interleave_two(long,2);
mmx_spec_interleave_two(unsigned long,2);
mmx_spec_interleave_two(float,2);

#define mmx_spec_interleave_four(t,n) \
template <unsigned long order> \
inline simd<t,n>::arrtype REF interleave( \
    const simd<t,n>::arrtype &a,const simd<t,n>::arrtype &b) \
{ \
  register __m64 tmp1,tmp2,tmp3; \
  static const long long lowd=0x00000000ffffffffLL; \
  static const long long highd=0xffffffff00000000LL; \
  static __m64 retval128[2]; \
  switch (order & 0x3000) {          \
    case 0x0000: \
    case 0x1000: \
      tmp1=*reinterpret_cast<const __m64 *>(&a); \
      break; \
    case 0x2000: \
    case 0x3000: \
      tmp1=*(reinterpret_cast<const __m64 *>(&a)+1); \
      break; \
  }    	   \
  switch (order & 0x1000) { \
    case 0x0000: \
      __asm__ ( "pand %2,%0" \
	  : "=y" (tmp1) \
	  : "0" (tmp1), "ym" (lowd)); \
      break; \
    case 0x1000: \
      __asm__ ( "psrlq %2,%0" \
	  : "=y" (tmp1) \
	  : "0" (tmp1), "i" (32)); \
      break; \
  }   \
  switch (order & 0x300) {          \
    case 0x000: \
    case 0x100: \
      tmp2=*reinterpret_cast<const __m64 *>(&a); \
      break; \
    case 0x200: \
    case 0x300: \
      tmp2=*(reinterpret_cast<const __m64 *>(&a)+1); \
      break; \
  }    	   \
  switch (order & 0x100) { \
    case 0x100: \
      __asm__ ( "pand %2,%0" \
	  : "=y" (tmp2) \
	  : "0" (tmp2), "ym" (highd)); \
      break; \
    case 0x000: \
      __asm__ ( "psllq %2,%0" \
	  : "=y" (tmp2) \
	  : "0" (tmp2), "i" (32)); \
      break; \
  }  \
  __asm__ ( "por %2,%0" \
      : "=y" (retval128[0]) \
      : "0" (tmp1), "y" (tmp2)); \
  switch (order & 0x30) {          \
    case 0x00: \
    case 0x10: \
      tmp3=*reinterpret_cast<const __m64 *>(&b); \
      break; \
    case 0x20: \
    case 0x30: \
      tmp3=*(reinterpret_cast<const __m64 *>(&b)+1); \
      break; \
  }    	   \
  switch (order & 0x10) { \
    case 0x00: \
      __asm__ ( "pand %2,%0" \
	  : "=y" (tmp3) \
	  : "0" (tmp3), "ym" (lowd)); \
      break; \
    case 0x10: \
      __asm__ ( "psrlq %2,%0" \
	  : "=y" (tmp3) \
	  : "0" (tmp3), "i" (32)); \
      break; \
  }   \
  switch (order & 0x3) {          \
    case 0x0: \
    case 0x1: \
      tmp2=*reinterpret_cast<const __m64 *>(&b); \
      break; \
    case 0x2: \
    case 0x3: \
      tmp2=*(reinterpret_cast<const __m64 *>(&b)+1); \
      break; \
  }    	   \
  switch (order & 0x1) { \
    case 0x1: \
      __asm__ ( "pand %2,%0" \
	  : "=y" (tmp2) \
	  : "0" (tmp2), "ym" (highd)); \
      break; \
    case 0x0: \
      __asm__ ( "psllq %2,%0" \
	  : "=y" (tmp2) \
	  : "0" (tmp2), "i" (32)); \
      break; \
  }  \
  __asm__ ( "por %2,%0" \
      : "=y" (retval128[1]) \
      : "0" (tmp3), "y" (tmp2)); \
  return reinterpret_cast<simd<t,n>::arrtype &>(retval128); \
}

#if !defined(USE_SSE) && !defined(USE_SSE2)
mmx_spec_interleave_four(long,4);
mmx_spec_interleave_four(unsigned long,4);
mmx_spec_interleave_four(float,4);
#endif

//---------------------------left shift-------------------------------
inline void mmx_psllw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psllw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_psllw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator <<(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_psllw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_psllw_macro(short,4)
_mmx_psllw_macro(unsigned short,4)
#undef _mmx_psllw_macro

inline void mmx_pslleqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psllw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_pslleqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator <<=(const int &b) \
{ \
  mmx_pslleqw(b,&m64); \
  return *this; \
}
_mmx_pslleqw_macro(short,4)
_mmx_pslleqw_macro(unsigned short,4)
#undef _mmx_pslleqw_macro

inline void mmx_pslld(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("pslld %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_pslld_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator <<(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_pslld(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_pslld_macro(long,2)
_mmx_pslld_macro(unsigned long,2)
#undef _mmx_pslld_macro

inline void mmx_pslleqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("pslld %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_pslleqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator <<=(const int &b) \
{ \
  mmx_pslleqd(b,&m64); \
  return *this; \
}
_mmx_pslleqd_macro(long,2)
_mmx_pslleqd_macro(unsigned long,2)
#undef _mmx_pslleqd_macro

//---------------------------right unsigned shift-------------------------------
inline void mmx_psrlw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrlw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_psrlw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_psrlw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_psrlw_macro(unsigned short,4)
#undef _mmx_psrlw_macro

inline void mmx_psrleqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrlw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_psrleqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  mmx_psrleqw(b,&m64); \
  return *this; \
}
_mmx_psrleqw_macro(unsigned short,4)
#undef _mmx_psrleqw_macro

inline void mmx_psrld(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrld %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_psrld_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_psrld(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_psrld_macro(unsigned long,2)
#undef _mmx_psrld_macro

inline void mmx_psrleqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrld %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_psrleqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  mmx_psrleqd(b,&m64); \
  return *this; \
}
_mmx_psrleqd_macro(unsigned long,2)
#undef _mmx_psrleqd_macro

//---------------------------right signed shift-------------------------------
inline void mmx_psraw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psraw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_psraw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_psraw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_psraw_macro(short,4)
#undef _mmx_psraw_macro

inline void mmx_psraeqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psraw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_psraeqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  mmx_psraeqw(b,&m64); \
  return *this; \
}
_mmx_psraeqw_macro(short,4)
#undef _mmx_psraeqw_macro

inline void mmx_psrad(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrad %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _mmx_psrad_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  mmx_psrad(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_mmx_psrad_macro(long,2)
#undef _mmx_psrad_macro

inline void mmx_psraeqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrad %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _mmx_psraeqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  mmx_psraeqd(b,&m64); \
  return *this; \
}
_mmx_psraeqd_macro(long,2)
#undef _mmx_psraeqd_macro

//-----------------------------ancillary-------------------------------------
  
#define _mmx_start_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_start() { \
}
_mmx_start_macro(char,8)
_mmx_start_macro(unsigned char,8)
_mmx_start_macro(signed char,8)
_mmx_start_macro(short,4)
_mmx_start_macro(unsigned short,4)
_mmx_start_macro(long,2)
_mmx_start_macro(unsigned long,2)
_mmx_start_macro(float,2)

#ifndef USE_3DNOW
#define EMMS "emms\n"
#else
#define EMMS "femms\n"
#endif

#define _mmx_end_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_finish() { \
    __asm__("emms\n" \
	    :  \
	    :  \
            : MMX_REGS, FP_STACK  \
	   ); \
}
_mmx_end_macro(char,8)
_mmx_end_macro(unsigned char,8)
_mmx_end_macro(signed char,8)
_mmx_end_macro(short,4)
_mmx_end_macro(unsigned short,4)
_mmx_end_macro(long,2)
_mmx_end_macro(unsigned long,2)
_mmx_end_macro(float,2)
#if !defined(USE_SSE) && !defined(USE_SSE2) && !defined(USE_3DNOW)
_mmx_end_macro(float,4)
#endif

/*
 *
 * Revision 1.1  2004/09/17 01:42:36  korpela
 * initial checkin of test code.
 *
 */

