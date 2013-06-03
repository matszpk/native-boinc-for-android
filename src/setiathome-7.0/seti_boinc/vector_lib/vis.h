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
// $Id: vis.h,v 1.1 2004/11/11 00:30:02 korpela Exp $
//
// Specific template instances for VIS supported SIMD routines.  This file
// is included from "simd.h"
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//


static const char trueb[8]={1,1,1,1,1,1,1,1};
static const short truew[4]={1,1,1,1};
static const long trued[2]={1,1};
static const v_i64 falsed=0;
static const v_i64 minus_one=(v_i64)(-1);
static const v_i64 hi_d=(v_i64)0xffffffff00000000LL;
static const v_i64 lo_d=(v_i64)0x00000000ffffffffLL;
static struct _gsr {
   unsigned long align:3,scale:5,not_used:17,irnd:2,im:1,unused:4;
} gsr;
static unsigned long bmask;


//  First lets do the easy operators......
//--------------------- binary operators -------------------------------------

#define vis_deref(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::ptr::operator *() { \
  return reinterpret_cast<simd<t,n>::arrtype &>(*(__m64 *)(v)); \
}

vis_deref(char,8)
vis_deref(signed char,8)
vis_deref(unsigned char,8)
vis_deref(short,4)
vis_deref(unsigned short,4)
vis_deref(long,2)
vis_deref(unsigned long,2)

#define vis_array_deref(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::ptr::operator [](int i) { \
  return reinterpret_cast<simd<t,n>::arrtype &>(*((__m64 *)(v)+i)); \
}

vis_deref(char,8)
vis_deref(signed char,8)
vis_deref(unsigned char,8)
vis_array_deref(short,4)
vis_array_deref(unsigned short,4)
vis_array_deref(long,2)
vis_array_deref(unsigned long,2)

#define vis_commutative_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %3,%2,%1\n" __followup_instr \
      :  "=f" (retval64) \
      :  "%f" (m64.all) , \
         "f" (b.m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define vis_noncomm_binop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %2,%3,%1\n" __followup_instr \
      :  "=f" (retval64) \
      :  "f" (m64.all) , \
         "f" (b.m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define vis_unop(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype REF simd<t,n>::arrtype::operator __op() const  \
{ \
  register __m64 retval64; \
  __asm__ ( __instr " %2,%1\n" __followup_instr \
      :  "=f" (retval64) \
      :  "f" (m64.all) \
  ); \
  return reinterpret_cast<simd<t,n>::arrtype & >(retval64); \
}

#define vis_assign(t,n,__op,__instr,__followup_instr) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op( \
    const simd<t,n>::arrtype &b) \
{ \
   __asm__ volatile ( "" \
             : "=f" (m64.all)   \
	     : "f" (b.m64.all)  \
   ); \
   return *static_cast<simd<t,n>::arrtype *>(this); \
}

//--------------------- addition -------------------------------------

vis_commutative_binop(short,4,+,"fpadd16","")
vis_commutative_binop(unsigned short,4,+,"fpadd16","")
vis_commutative_binop(long,2,+,"fpadd32","")
vis_commutative_binop(unsigned long,2,+,"fpadd32","")

//---------------------- multiplication ------------------------------
  
inline simd<short,4>::arrtype REF simd<short,4>::arrtype::operator *( 
    const simd<t,n>::arrtype &b)
{
    register __m64 retval64,high,low;
    __asm__ ( "fmul8sux16 %3,%4,%1\n"
	      "\tfmul8ulx16 %3,%4,%2\n"
	      "\tfpadd16 %1,%2,%3\n"
	: "=f" (retval), "=f" (high), 
	  "=f" (low)
	: "%f" (m64.all),
	  "f" (b.m64.all)
    );
    return reinterpret_cast<simd<t,n>::arrtype &>(retval64); 
}

//---------------------- subtraction ---------------------------------
vis_noncomm_binop(short,4,-,"fpsub16","")
vis_noncomm_binop(unsigned short,4,-,"fpsub16","")
vis_noncomm_binop(long,2,-,"fpsub32","")
vis_noncomm_binop(unsigned long,2,-,"fpsub32","")

//-----------------------------AND--------------------------------
vis_commutative_binop(char,8,&,"fand","")
vis_commutative_binop(signed char,8,&,"fand","")
vis_commutative_binop(unsigned char,8,&,"fand","")
vis_commutative_binop(short,4,&,"fand","")
vis_commutative_binop(unsigned short,4,&,"fand","")
vis_commutative_binop(long,2,&,"fand","")
vis_commutative_binop(unsigned long,2,&,"fand","")
vis_commutative_binop(long long,1,&,"fand","")
vis_commutative_binop(unsigned long long,1,&,"fand","")

//-----------------------------OR--------------------------------
vis_commutative_binop(char,8,|,"for","")
vis_commutative_binop(signed char,8,|,"for","")
vis_commutative_binop(unsigned char,8,|,"for","")
vis_commutative_binop(short,4,|,"for","")
vis_commutative_binop(unsigned short,4,|,"for","")
vis_commutative_binop(long,2,|,"for","")
vis_commutative_binop(unsigned long,2,|,"for","")
vis_commutative_binop(long long,1,|,"for","")
vis_commutative_binop(unsigned long long,1,|,"for","")

//-----------------------------XOR--------------------------------
vis_commutative_binop(char,8,^,"fxor","")
vis_commutative_binop(signed char,8,^,"fxor","")
vis_commutative_binop(unsigned char,8,^,"fxor","")
vis_commutative_binop(short,4,^,"fxor","")
vis_commutative_binop(unsigned short,4,^,"fxor","")
vis_commutative_binop(long,2,^,"fxor","")
vis_commutative_binop(unsigned long,2,^,"fxor","")
vis_commutative_binop(long long,1,^,"fxor","")
vis_commutative_binop(unsigned long long,1,^,"fxor","")

//-----------------------------NOT--------------------------------
vis_unop(char,8,~,"fnot1","");
vis_unop(unsigned char,8,~,"fnot1","");
vis_unop(signed char,8,~,"fnot1","");
vis_unop(short,4,~,"fnot1","");
vis_unop(unsigned short,4,~,"fnot1","");
vis_unop(long,2,~,"fnot1","");
vis_unop(unsigned long,2,~,"fnot1","");
vis_unop(long long,1,~,"fnot1","");
vis_unop(unsigned long long,1,~,"fnot1","");

//-----------------------------negative--------------------------------
vis_binop_const(short,4,-,"fpsub16","",falsed);
vis_binop_const(unsigned short,4,-,"fpsub16","",falsed);
vis_binop_const(long,2,-,"fpsub32","",falsed);
vis_binop_const(unsigned long,2,-,"fpsubd32","",falsed);

//--------------------- assignment -----------------------------------
vis_assign(char,8,=,"","")
vis_assign(unsigned char,8,=,"","")
vis_assign(signed char,8,=,"","")
vis_assign(short,4,=,"","")
vis_assign(unsigned short,4,=,"","")
vis_assign(long,2,=,"","")
vis_assign(unsigned long,2,=,"","")
vis_assign(float,2,=,"","")
vis_assign(long long,1,=,"","")
vis_assign(unsigned long long,1,=,"","")
vis_assign(double,1,=,"","")

vis_assign(char,8,&=,"fand","")
vis_assign(unsigned char,8,&=,"fand","")
vis_assign(signed char,8,&=,"fand","")
vis_assign(short,4,&=,"fand","")
vis_assign(unsigned short,4,&=,"fand","")
vis_assign(long,2,&=,"fand","")
vis_assign(unsigned long,2,&=,"fand","")
vis_assign(long long,1,&=,"fand","")
vis_assign(unsigned long long,1,&=,"fand","")

vis_assign(char,8,|=,"for","")
vis_assign(unsigned char,8,|=,"for","")
vis_assign(signed char,8,|=,"for","")
vis_assign(short,4,|=,"for","")
vis_assign(unsigned short,4,|=,"for","")
vis_assign(long,2,|=,"for","")
vis_assign(unsigned long,2,|=,"for","")
vis_assign(long long,1,|=,"for","")
vis_assign(unsigned long long,1,|=,"for","")

vis_assign(char,8,^=,"fxor","")
vis_assign(unsigned char,8,^=,"fxor","")
vis_assign(signed char,8,^=,"fxor","")
vis_assign(short,4,^=,"fxor","")
vis_assign(unsigned short,4,^=,"fxor","")
vis_assign(long,2,^=,"fxor","")
vis_assign(unsigned long,2,^=,"fxor","")
vis_assign(long long,1,^=,"fxor","")
vis_assign(unsigned long long,1,^=,"fxor","")

vis_assign(short,4,+=,"fpadd16","")
vis_assign(unsigned short,4,+=,"fpadd16","")
vis_assign(long,2,+=,"fpadd32","")
vis_assign(unsigned long,2,+=,"fpadd32","")

vis_assign(short,4,-=,"fpsub16","")
vis_assign(unsigned short,4,-=,"fpsub16","")
vis_assign(long,2,-=,"fpsub32","")
vis_assign(unsigned long,2,-=,"fpsub32","")

//vis_assign(short,4,*=,"pmullw","")
//vis_assign(unsigned short,4,*=,"pmullw","")

// unfortunately logical ops require more fiddling
// ------------- generic logical ops
//
#define vis_logical_op(t,n,__op,__op_t,__instr,__conv_ops, __out_spec, __outvar) \
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

#define vis_logical_byteop(t,__op,__op_t,__instr) \
   vis_logical_op(t,8,__op,__op_t,__instr, \
    "pand _trueb,%1\n" \
    "\tmovq %1,%0",          \
    "=y",retval64)

#define vis_logical_wordop(t,__op,__op_t,__instr) \
   vis_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpand _trueb,%1\n" \
    "\tmovd %1,%0", \
    "=r",retval32)

#define vis_logical_dwordop(t,__op,__op_t,__instr) \
   vis_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpacksswb %1,%1\n" \
    "\tpand _trueb,%1\n" \
    "\tmovd %1,%0",          \
    "=a",retval16)

#define vis_logical_notbyteop(t,__op,__op_t,__instr) \
   vis_logical_op(t,8,__op,__op_t,__instr, \
    "pandn _trueb,%1\n" \
    "\tmovq %1,%0",          \
    "=y",retval64)

#define vis_logical_notwordop(t,__op,__op_t,__instr) \
   vis_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpandn _trueb,%1\n" \
    "\tmovd %1,%0", \
    "=r",retval32)

#define vis_logical_notdwordop(t,__op,__op_t,__instr) \
   vis_logical_op(t,4,__op,__op_t,__instr, \
    "packssdw %1,%1\n" \
    "\tpacksswb %1,%1\n" \
    "\tpandn _trueb,%1\n" \
    "\tmovd %1,%0",          \
    "=a",retval16)

//-----------------------------EQUALS--------------------------------
  
vis_logical_byteop(char,==,char,"pcmpeqb")
vis_logical_byteop(unsigned char,==,unsigned char, "pcmpeqb")
vis_logical_byteop(signed char,==,signed char, "pcmpeqb")

vis_logical_wordop(short,==,short,"pcmpeqw")
vis_logical_wordop(unsigned short,==,unsigned short,"pcmpeqw")

vis_logical_dwordop(long,==,long,"pcmpeqd")
vis_logical_dwordop(unsigned long,==,unsigned long,"pcmpeqd")

//-----------------------------NOT EQUALS--------------------------------
  
vis_logical_notbyteop(char,!=,char, "pcmpeqb")
vis_logical_notbyteop(unsigned char,!=,unsigned char, "pcmpeqb")
vis_logical_notbyteop(signed char,!=,signed char, "pcmpeqb")

vis_logical_notwordop(short,!=,short,"pcmpeqw")
vis_logical_notwordop(unsigned short,!=,unsigned short,"pcmpeqw")

vis_logical_notdwordop(long,!=,long,"pcmpeqd")
vis_logical_notdwordop(unsigned long,!=,unsigned long,"pcmpeqd")

//-----------------------------GREATER THAN--------------------------------
#if (CHAR_MIN<0)
vis_logical_byteop(char,>,char, "pcmpgtb")
#endif
vis_logical_byteop(signed char,>,signed char, "pcmpgtb")

vis_logical_wordop(short,>,short,"pcmpgtw")

vis_logical_dwordop(long,>,long,"pcmpgtd")


//-----------------------------LESS THAN OR EQUAL----------------------------
#if (CHAR_MIN<0)
vis_logical_notbyteop(char,<=,char, "pcmpgtb")
#endif
vis_logical_notbyteop(signed char,<=,signed char, "pcmpgtb")

vis_logical_notwordop(short,<=,short,"pcmpgtw")

vis_logical_notdwordop(long,<=,long,"pcmpgtd")

//-----------------------------LESS THAN--------------------------------
#if (CHAR_MIN<0)
vis_logical_byteop(char,<,char, "pcmpltb")
#endif
vis_logical_byteop(signed char,<,signed char, "pcmpltb")

vis_logical_wordop(short,<,short,"pcmpltw")

vis_logical_dwordop(long,<,long,"pcmpltd")

//-----------------------------GREATER THAN OR EQUAL-----------------------
#if (CHAR_MIN<0)
vis_logical_notbyteop(char,>=,char, "pcmpltb")
#endif
vis_logical_notbyteop(signed char,>=,signed char, "pcmpltb")

vis_logical_notwordop(short,>=,short,"pcmpltw")

vis_logical_notdwordop(long,>=,long,"pcmpltd")

//-------------------------LOGICAL OR--------------------------------

#define vis_por_macro(t,n) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator ||(const \
    simd<t,n>::arrtype &b) const \
{ \
  return ((*this)|b)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

vis_por_macro(char,8)
vis_por_macro(unsigned char,8)
vis_por_macro(signed char,8)

vis_por_macro(short,4)
vis_por_macro(unsigned short,4)

vis_por_macro(long,2)
vis_por_macro(unsigned long,2)

//-------------------------LOGICAL AND--------------------------------
#define vis_pand_macro(t,n) \
inline simd<pbool,n>::arrtype &simd<t,n>::arrtype::operator &&(const \
    simd<t,n>::arrtype &b) const \
{ \
  return ((*this)&b)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

vis_pand_macro(char,8)
vis_pand_macro(unsigned char,8)
vis_pand_macro(signed char,8)

vis_pand_macro(short,4)
vis_pand_macro(unsigned short,4)

vis_pand_macro(long,2)
vis_pand_macro(unsigned long,2)

//-----------------------------LOGICAL NOT--------------------------------
#define vis_pnot_macro(t,n) \
inline simd<pbool,n>::arrtype REF simd<t,n>::arrtype::operator !() const \
{ \
  return (*this)!=reinterpret_cast<const simd<pbool,n>::arrtype &>(falsed); \
} 

vis_pnot_macro(char,8)
vis_pnot_macro(unsigned char,8)
vis_pnot_macro(signed char,8)

vis_pnot_macro(short,4)
vis_pnot_macro(unsigned short,4)

vis_pnot_macro(long,2)
vis_pnot_macro(unsigned long,2)
//---------------------------average----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2) || defined(USE_3DNOW)
#define vis_avg_macro(t,n,instr) \
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
vis_avg_macro(unsigned char,8,"pavgusb")
#if (CHAR_MIN==0)
vis_avg_macro(char,8,"pavgusb")
#endif
#elif defined(USE_SSE) || defined(USE_SSE2)
vis_avg_macro(unsigned short,4,"pavgw")
vis_avg_macro(unsigned char,8,"pavgb")
#if (CHAR_MIN==0)
vis_avg_macro(char,8,"pavgb")
#endif
#endif

//---------------------------max----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2)
#define vis_max_macro(t,n,instr) \
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
vis_max_macro(char,8,"pmaxsb")
#else
vis_max_macro(char,8,"pmaxub")
#endif
vis_max_macro(unsigned char,8,"pmaxub")
vis_max_macro(signed char,8,"pmaxsb")
vis_max_macro(short,4,"pmaxsw")
vis_max_macro(unsigned short,4,"pmaxuw")

#endif
//---------------------------min----------------------------------

#if defined(USE_SSE) || defined(USE_SSE2)
#define vis_min_macro(t,n,instr) \
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
vis_min_macro(char,8,"pminsb")
#else
vis_min_macro(char,8,"pminub")
#endif
vis_min_macro(unsigned char,8,"pminub")
vis_min_macro(signed char,8,"pminsb")
vis_min_macro(short,4,"pminsw")
vis_min_macro(unsigned short,4,"pminuw")

#endif

//-----------------------------shuffle---------------------------------------
#if defined(USE_SSE) || defined(USE_SSE2)
#define vis_shuffle_macro(t,n) \
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

vis_shuffle_macro(short,4)
vis_shuffle_macro(unsigned short,4)
#endif
#undef vis_shuffle_macro

#define vis_shuffle_macro(t,n) \
template <const long long order> \
extern typename simd<t,n>::arrtype REF shuffle( \
    const typename simd<t,n>::arrtype &v) ;

#define vis_spec_shuffle_macro(t,n,order,instr) \
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

vis_spec_shuffle_macro(char,8,0x00112233,"punpcklbw");
vis_spec_shuffle_macro(char,8,0x44556677,"punpckhbw");
vis_spec_shuffle_macro(char,4,0x0011,"punpcklbw");
vis_spec_shuffle_macro(char,4,0x4455,"punpckhbw");
vis_spec_shuffle_macro(unsigned char,8,0x00112233,"punpcklbw");
vis_spec_shuffle_macro(unsigned char,8,0x44556677,"punpckhbw");
//vis_spec_shuffle_macro(unsigned char,4,0x0011,"punpcklbw");
//vis_spec_shuffle_macro(unsigned char,4,0x4455,"punpckhbw");
vis_spec_shuffle_macro(signed char,8,0x00112233,"punpcklbw");
vis_spec_shuffle_macro(signed char,8,0x44556677,"punpckhbw");
vis_spec_shuffle_macro(signed char,4,0x0011,"punpcklbw");
vis_spec_shuffle_macro(signed char,4,0x4455,"punpckhbw");

vis_spec_shuffle_macro(short,4,0x0011,"punpcklwd");
vis_spec_shuffle_macro(short,4,0x2233,"punpckhwd");
vis_spec_shuffle_macro(unsigned short,4,0x0011,"punpcklwd");
vis_spec_shuffle_macro(unsigned short,4,0x2233,"punpckhwd");

vis_spec_shuffle_macro(long,2,0x00,"punpckldq");
vis_spec_shuffle_macro(long,2,0x11,"punpckhdq");
vis_spec_shuffle_macro(unsigned long,2,0x00,"punpckldq");
vis_spec_shuffle_macro(unsigned long,2,0x11,"punpckhdq");
vis_spec_shuffle_macro(float,2,0x00,"punpckldq");
vis_spec_shuffle_macro(float,2,0x11,"punpckhdq");

//-----------------------------interleave---------------------------------------
#define vis_spec_interleave_macro(t,n,order,instr,mask) \
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

vis_spec_interleave_macro(signed char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);
vis_spec_interleave_macro(unsigned char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);
vis_spec_interleave_macro(char,8,0x02460246,"packuswb",0x00ff00ff00ff00ffLL);

#undef vis_spec_interleave_macro
#define vis_spec_interleave_macro(t,n,order,instr,shift_inst) \
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

vis_spec_interleave_macro(signed char,8,0x13571357,"packuswb","psraw $8");
vis_spec_interleave_macro(unsigned char,8,0x13571357,"packuswb","psraw $8");
vis_spec_interleave_macro(char,8,1357135746,"packuswb","psraw $8");
vis_spec_interleave_macro(short,4,0x1313,"packssdw","psrad $16");
vis_spec_interleave_macro(unsigned short,4,0x1313,"packssdw","psrad $16");

#undef vis_spec_interleave_macro
#define vis_spec_interleave_macro(t,n,order,instr,shift_inst1,shift_inst2) \
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
vis_spec_interleave_macro(short,4,0x0202,"packssdw","pslld $16","psrad $16");
vis_spec_interleave_macro(unsigned short,4,0x0202,"packssdw","pslld $16","psrad $16");

#undef vis_spec_interleave_macro
#define vis_spec_interleave_two(t,n) \
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

vis_spec_interleave_two(long,2);
vis_spec_interleave_two(unsigned long,2);
vis_spec_interleave_two(float,2);

#define vis_spec_interleave_four(t,n) \
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

vis_spec_interleave_four(long,4);
vis_spec_interleave_four(unsigned long,4);
vis_spec_interleave_four(float,4);

//---------------------------left shift-------------------------------
inline void vis_psllw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psllw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_psllw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator <<(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_psllw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_psllw_macro(short,4)
_vis_psllw_macro(unsigned short,4)
#undef _vis_psllw_macro

inline void vis_pslleqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psllw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_pslleqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator <<=(const int &b) \
{ \
  vis_pslleqw(b,&m64); \
  return *this; \
}
_vis_pslleqw_macro(short,4)
_vis_pslleqw_macro(unsigned short,4)
#undef _vis_pslleqw_macro

inline void vis_pslld(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("pslld %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_pslld_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator <<(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_pslld(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_pslld_macro(long,2)
_vis_pslld_macro(unsigned long,2)
#undef _vis_pslld_macro

inline void vis_pslleqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("pslld %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_pslleqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator <<=(const int &b) \
{ \
  vis_pslleqd(b,&m64); \
  return *this; \
}
_vis_pslleqd_macro(long,2)
_vis_pslleqd_macro(unsigned long,2)
#undef _vis_pslleqd_macro

//---------------------------right unsigned shift-------------------------------
inline void vis_psrlw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrlw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_psrlw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_psrlw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_psrlw_macro(unsigned short,4)
#undef _vis_psrlw_macro

inline void vis_psrleqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrlw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_psrleqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  vis_psrleqw(b,&m64); \
  return *this; \
}
_vis_psrleqw_macro(unsigned short,4)
#undef _vis_psrleqw_macro

inline void vis_psrld(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrld %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_psrld_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_psrld(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_psrld_macro(unsigned long,2)
#undef _vis_psrld_macro

inline void vis_psrleqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrld %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_psrleqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  vis_psrleqd(b,&m64); \
  return *this; \
}
_vis_psrleqd_macro(unsigned long,2)
#undef _vis_psrleqd_macro

//---------------------------right signed shift-------------------------------
inline void vis_psraw(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psraw %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_psraw_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_psraw(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_psraw_macro(short,4)
#undef _vis_psraw_macro

inline void vis_psraeqw(const int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psraw %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_psraeqw_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  vis_psraeqw(b,&m64); \
  return *this; \
}
_vis_psraeqw_macro(short,4)
#undef _vis_psraeqw_macro

inline void vis_psrad(const vec_m64 *a, int b, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrad %2,%1\n"
             : "=y" (c->all)  // outputs into c
	     : "%0" (a->all), // inputs a from memory
	       "iy" (b)  // inputs b from mms register
   );
#endif
}

#define _vis_psrad_macro(t,n) \
inline simd<t,n>::arrtype simd<t,n>::arrtype::operator >>(const int &b) \
  const \
{ \
  vec_m64 tmp; \
  vis_psrad(&m64,b,&tmp); \
  return simd<t,n>::arrtype(tmp); \
}
_vis_psrad_macro(long,2)
#undef _vis_psrad_macro

inline void vis_psraeqd(int a, vec_m64 *c) {
#ifdef __GNUC__
    __asm__ ("psrad %2,%1\n"
	     "\tmovq %1,%0\n"
             : "=m" (c->all)  // outputs into c
	     : "%y" (c->all), // inputs a from memory
	       "iy" (a)  // inputs b from mms register
   );
#endif
}

#define _vis_psraeqd_macro(t,n) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator >>=(const int &b) \
{ \
  vis_psraeqd(b,&m64); \
  return *this; \
}
_vis_psraeqd_macro(long,2)
#undef _vis_psraeqd_macro

//-----------------------------ancillary-------------------------------------
  
#define _vis_start_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_start() { \
}
_vis_start_macro(char,8)
_vis_start_macro(unsigned char,8)
_vis_start_macro(signed char,8)
_vis_start_macro(short,4)
_vis_start_macro(unsigned short,4)
_vis_start_macro(long,2)
_vis_start_macro(unsigned long,2)
_vis_start_macro(float,2)

#ifndef USE_3DNOW
#define EMMS "emms\n"
#else
#define EMMS "femms\n"
#endif

#define _vis_end_macro(t,n) \
inline volatile void  simd<t,n>::simd_mode_finish() { \
    __asm__("emms\n" \
	    :  \
	    :  \
            : MMX_REGS, FP_STACK  \
	   ); \
}
_vis_end_macro(char,8)
_vis_end_macro(unsigned char,8)
_vis_end_macro(signed char,8)
_vis_end_macro(short,4)
_vis_end_macro(unsigned short,4)
_vis_end_macro(long,2)
_vis_end_macro(unsigned long,2)
_vis_end_macro(float,2)
#if !defined(USE_SSE) && !defined(USE_SSE2) && !defined(USE_3DNOW)
_vis_end_macro(float,4)
#endif

/*
 *
 * Revision 1.1  2004/09/17 01:42:36  korpela
 * initial checkin of test code.
 *
 */

