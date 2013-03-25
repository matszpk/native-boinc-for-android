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
// $Id: generics.h,v 1.9.2.1 2007/03/22 00:03:59 korpela Exp $
//
// Specific template instances for SSE supported SIMD routines.  This file
// is included from "simd.h"
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//
//

template <typename T, size_t N>
inline typename simd<T,N>::arrtype &simd<T,N>::ptr::operator *() { 
    return (*(this->v)); 
}


#define single_binary_op(t,__op) \
inline simd<t,1>::arrtype REF simd<t,1>::arrtype::operator __op(const \
       simd<t,1>::arrtype &b) const { \
  register t tmp; \
  tmp=this->v[0] __op b.v[0]; \
  return reinterpret_cast<simd<t,1>::arrtype &>(tmp); \
} 

#define single_binary_fltops(t) \
single_binary_op(t,+) \
single_binary_op(t,-) \
single_binary_op(t,*) \
single_binary_op(t,/) 

#define single_binary_intops(t) \
single_binary_fltops(t) \
single_binary_op(t,&) \
single_binary_op(t,^) \
single_binary_op(t,|) \
single_binary_op(t,%) \
single_binary_op(t,<<) \
single_binary_op(t,>>) 

single_binary_intops(char)
single_binary_intops(unsigned char)
single_binary_intops(signed char)
single_binary_intops(short)
single_binary_intops(unsigned short)
single_binary_intops(long)
single_binary_intops(unsigned long)
single_binary_fltops(float)
single_binary_fltops(double)
#undef single_binary_op
#undef single_binary_fltops
#undef single_binary_intops


#define binary_operator(__op) \
template <typename T, const size_t N> \
inline typename simd<T,N>::arrtype REF simd<T,N>::arrtype::operator __op(const \
        typename simd<T,N>::arrtype &b) const { \
   typename simd<T,N>::arrtype tmp; \
  *typename simd<T,N/2>::ptr((void *)&tmp)=*typename simd<T,N/2>::ptr((void *)&v) \
                             __op *typename simd<T,N/2>::ptr((void *)&(b.v)); \
  *(typename simd<T,N/2>::ptr((void *)&tmp)+1)=*(typename simd<T,N/2>::ptr((void *)&v)+1) \
                             __op *(typename simd<T,N/2>::ptr((void *)&(b.v))+1); \
  return tmp; \
}


binary_operator(+) 
binary_operator(-) 
binary_operator(*) 
binary_operator(/) 
binary_operator(&) 
binary_operator(^) 
binary_operator(|) 
binary_operator(%) 
binary_operator(<<) 
binary_operator(>>) 
#undef binary_operator

#define single_bool_op(t,__op,__op_type) \
inline simd<pbool,1>::arrtype REF simd<t,1>::arrtype::operator __op(const \
       simd<__op_type,1>::arrtype &b) const { \
  register pbool tmp; \
  tmp=this->v[0] __op b.v[0]; \
  return reinterpret_cast<simd<pbool,1>::arrtype &>(tmp); \
} 

#define single_bool_ops(t) \
single_bool_op(t,&&,t) \
single_bool_op(t,||,t) \
single_bool_op(t,==,t) \
single_bool_op(t,!=,t) \
single_bool_op(t,<,t) \
single_bool_op(t,>,t) \
single_bool_op(t,<=,t) \
single_bool_op(t,>=,t) 
single_bool_ops(char)
single_bool_ops(unsigned char)
single_bool_ops(signed char)
single_bool_ops(short)
single_bool_ops(unsigned short)
single_bool_ops(long)
single_bool_ops(unsigned long)
single_bool_ops(float)
single_bool_ops(double)
#undef single_bool_ops
#undef single_bool_op

#define bool_operator(__op,__op_type) \
template <typename T, const size_t N> \
inline typename simd<pbool,N>::arrtype REF simd<T,N>::arrtype::operator __op(const \
        typename simd<__op_type,N>::arrtype &b) const { \
  typename simd<pbool,N>::arrtype tmp; \
  *typename simd<pbool,N/2>::ptr((void *)&tmp)=*typename simd<T,N/2>::ptr((void *)&v) \
                             __op *typename simd<__op_type,N/2>::ptr((void *)&(b.v)); \
  *(typename simd<pbool,N/2>::ptr((void *)&tmp)+1)=*(typename simd<T,N/2>::ptr((void *)&v)+1) \
                             __op *(typename simd<__op_type,N/2>::ptr((void *)&(b.v))+1); \
  return reinterpret_cast<typename simd<pbool,N>::arrtype &>(tmp); \
}
bool_operator(&&,T) 
bool_operator(||,T) 
bool_operator(==,T) 
bool_operator(!=,T) 
bool_operator(<,T) 
bool_operator(>,T) 
bool_operator(<=,T) 
bool_operator(>=,T) 
#undef bool_operator

#define single_ubool_op(t,__op) \
inline simd<pbool,1>::arrtype REF simd<t,1>::arrtype::operator __op() const \
{ \
  register pbool tmp; \
  tmp= __op v[0]; \
  return reinterpret_cast<simd<pbool,1>::arrtype &>(tmp); \
}

#define single_ubool_ops(t) \
single_ubool_op(t,!)
single_ubool_ops(char)
single_ubool_ops(unsigned char)
single_ubool_ops(signed char)
single_ubool_ops(short)
single_ubool_ops(unsigned short)
single_ubool_ops(long)
single_ubool_ops(unsigned long)
single_ubool_ops(float)
single_ubool_ops(double)
#undef single_ubool_ops
#undef single_ubool_op

#define ubool_operator(__op) \
template <typename T, const size_t N> \
inline typename simd<pbool,N>::arrtype REF simd<T,N>::arrtype::operator __op() const { \
  typename simd<pbool,N>::arrtype tmp; \
  *simd<pbool,N/2>::ptr((void *)&tmp)= __op *simd<T,N/2>::ptr((void *)this); \
  *(simd<pbool,N/2>::ptr((void *)&tmp)+1)= __op *(simd<T,N/2>::ptr((void *)this)+1); \
  return tmp; \
}
ubool_operator(!) 
#undef ubool_operator

#define single_unary_op(t,__op) \
inline simd<t,1>::arrtype REF simd<t,1>::arrtype::operator __op() const \
{ \
  register t tmp; \
  tmp= __op v[0]; \
  return reinterpret_cast<simd<t,1>::arrtype &>(tmp); \
}

#define single_unary_fltops(t) \
single_unary_op(t,-) 

#define single_unary_intops(t) \
single_unary_fltops(t) \
single_unary_op(t,~)
single_unary_intops(char)
single_unary_intops(unsigned char)
single_unary_intops(signed char)
single_unary_intops(short)
single_unary_intops(unsigned short)
single_unary_intops(long)
single_unary_intops(unsigned long)
single_unary_fltops(float)
single_unary_fltops(double)
#undef single_unary_op
#undef single_unary_fltops
#undef single_unary_intops

#define unary_operator(__op) \
template <typename T, const size_t N> \
inline typename simd<T,N>::arrtype REF simd<T,N>::arrtype::operator __op() const \
{ \
  typename simd<T,N>::arrtype tmp; \
  *simd<T,N/2>::ptr((void *)&tmp)=__op *simd<T,N/2>::ptr((void *)this); \
  *(simd<T,N/2>::ptr((void *)&tmp)+1)=__op *(simd<T,N/2>::ptr((void *)this)+1); \
  return reinterpret_cast<typename simd<T,N>::arrtype &>(tmp); \
}

unary_operator(-)
unary_operator(~)
#undef unary_operator

#define single_assignment_op(t,__op) \
inline simd<t,1>::arrtype &simd<t,1>::arrtype::operator __op(const \
    simd<t,1>::arrtype &b) \
{ \
  v[0] __op b.v[0]; \
  return *this; \
}
#define single_assignment_fltops(t) \
single_assignment_op(t,=) \
single_assignment_op(t,+=) \
single_assignment_op(t,-=) \
single_assignment_op(t,*=) \
single_assignment_op(t,/=) 
#define single_assignment_intops(t) \
single_assignment_fltops(t) \
single_assignment_op(t,&=)  \
single_assignment_op(t,^=)  \
single_assignment_op(t,|=)  \
single_assignment_op(t,%=)  \
single_assignment_op(t,<<=) \
single_assignment_op(t,>>=)
single_assignment_intops(char)
single_assignment_intops(unsigned char)
single_assignment_intops(signed char)
single_assignment_intops(short)
single_assignment_intops(unsigned short)
single_assignment_intops(long)
single_assignment_intops(unsigned long)
single_assignment_fltops(float)
#if !defined(USE_MMX) && !defined(USE_SSE) && !defined(USE_SSE2) && !defined(USE_3DNOW)
single_assignment_fltops(double)
#endif
#undef single_assignment_op
#undef single_assignment_fltops
#undef single_assignment_intops

#define multiple_assignment_op(t,n,__big_type,__op) \
inline simd<t,n>::arrtype &simd<t,n>::arrtype::operator __op(const \
    simd<t,n>::arrtype &b) \
{ \
  *reinterpret_cast<__big_type *>(this)  \
    __op *reinterpret_cast<const __big_type *>(&(b.v)); \
  return *this; \
}
multiple_assignment_op(char,4,unsigned long,=) 
multiple_assignment_op(signed char,4,unsigned long,=) 
multiple_assignment_op(unsigned char,4,unsigned long,=) 
multiple_assignment_op(short,2,unsigned long,=) 
multiple_assignment_op(unsigned short,2,unsigned long,=) 
multiple_assignment_op(char,2,unsigned short,=)
multiple_assignment_op(signed char,2,unsigned short,=)
multiple_assignment_op(unsigned char,2,unsigned short,=)
#undef multiple_assignment_op

#define assignment_operator(__op) \
template <typename T, const size_t N> \
inline typename simd<T,N>::arrtype &simd<T,N>::arrtype::operator __op(const \
    typename simd<T,N>::arrtype &b) \
{ \
  *simd<T,N/2>::ptr((void *)this) __op *simd<T,N/2>::ptr((void *)&(b.v)); \
  *(simd<T,N/2>::ptr((void *)this)+1) __op *(simd<T,N/2>::ptr((void *)&(b.v))+1); \
  return *this; \
}

assignment_operator(=)
assignment_operator(+=)
assignment_operator(-=)
assignment_operator(*=)
assignment_operator(/=)
assignment_operator(&=)
assignment_operator(^=)
assignment_operator(|=)
assignment_operator(%=)
assignment_operator(<<=)
assignment_operator(>>=)
#undef assignment_operator

#define _simd_mode_start(t) \
inline volatile void simd<t,1>::simd_mode_start() {}

#define _simd_mode_finish(t) \
inline volatile void simd<t,1>::simd_mode_finish() {}

template <typename T, const size_t N>
inline volatile void simd<T,N>::simd_mode_start() {
  simd<T,N/2>::simd_mode_start();
}

template <typename T, const size_t N>
inline volatile void simd<T,N>::simd_mode_finish() {
  simd<T,N/2>::simd_mode_start();
}

_simd_mode_start(char)
_simd_mode_start(unsigned char)
_simd_mode_start(signed char)
_simd_mode_start(short)
_simd_mode_start(unsigned short)
_simd_mode_start(long)
_simd_mode_start(unsigned long)
_simd_mode_start(float)
_simd_mode_start(double)
#undef simd_mode_start

#define single_joint_op(t,__op,__op_type,__ret_type) \
inline simd<__ret_type,1>::arrtype simd<t,1>::arrtype::operator __op(const \
    __op_type &b) const \
{ \
  register __ret_type tmp; \
  tmp=v[0] __op b; \
  return reinterpret_cast<simd<__ret_type,1>::arrtype &>(tmp); \
}

#define single_joint_fltops(t) \
single_joint_op(t,+,t,t) \
single_joint_op(t,-,t,t) \
single_joint_op(t,*,t,t) \
single_joint_op(t,/,t,t) \
single_joint_op(t,&&,t,pbool) \
single_joint_op(t,||,t,pbool) \
single_joint_op(t,==,t,pbool) \
single_joint_op(t,!=,t,pbool) \
single_joint_op(t,<,t,pbool) \
single_joint_op(t,>,t,pbool) \
single_joint_op(t,<=,t,pbool) \
single_joint_op(t,>=,t,pbool) 

#define single_joint_intops(t) \
single_joint_fltops(t) \
single_joint_op(t,<<,int,t) \
single_joint_op(t,>>,int,t) \
single_joint_op(t,&,t,t) \
single_joint_op(t,^,t,t) \
single_joint_op(t,|,t,t) \
single_joint_op(t,%,t,t)


single_joint_intops(char)
single_joint_intops(unsigned char)
single_joint_intops(signed char)
single_joint_intops(short)
single_joint_intops(unsigned short)
single_joint_intops(long)
single_joint_intops(unsigned long)
single_joint_fltops(float)
single_joint_fltops(double)
#undef single_joint_op
#undef single_joint_fltops
#undef single_joint_intops

#define joint_operator(__op,__op_type,__ret_type) \
template <typename T, const size_t N> \
inline typename simd<__ret_type,N>::arrtype simd<T,N>::arrtype::operator __op(const \
    __op_type &b) const \
{ \
  typename simd<__ret_type,N>::arrtype tmp; \
  *typename simd<__ret_type,N/2>::ptr((void *)&tmp)=*typename simd<T,N/2>::ptr((void *)this) __op b; \
  *(typename simd<__ret_type,N/2>::ptr((void *)&tmp)+1)=*(typename simd<T,N/2>::ptr((void *)this)+1) __op b; \
  return tmp; \
}

joint_operator(+,T,T) 
joint_operator(-,T,T) 
joint_operator(*,T,T) 
joint_operator(/,T,T) 
joint_operator(&&,T,pbool)
joint_operator(||,T,pbool) 
joint_operator(==,T,pbool) 
joint_operator(!=,T,pbool) 
joint_operator(<,T,pbool) 
joint_operator(>,T,pbool) 
joint_operator(<=,T,pbool)
joint_operator(>=,T,pbool) 
joint_operator(&,T,T) 
joint_operator(^,T,T) 
joint_operator(|,T,T) 
joint_operator(%,T,T) 
joint_operator(<<,int,T) 
joint_operator(>>,int,T) 
#undef joint_operator

#define single_jointasgn_op(t,__op,__op_type) \
inline simd<t,1>::arrtype & simd<t,1>::arrtype::operator __op(const \
    __op_type &b) \
{ \
  v[0] __op b; \
  return *this; \
}

#define single_jointasgn_fltops(t) \
single_jointasgn_op(t,=,t)  \
single_jointasgn_op(t,+=,t) \
single_jointasgn_op(t,-=,t) \
single_jointasgn_op(t,*=,t) \
single_jointasgn_op(t,/=,t) 

#define single_jointasgn_intops(t) \
single_jointasgn_fltops(t) \
single_jointasgn_op(t,<<=,int) \
single_jointasgn_op(t,>>=,int) \
single_jointasgn_op(t,&=,t)  \
single_jointasgn_op(t,^=,t)  \
single_jointasgn_op(t,|=,t)  \
single_jointasgn_op(t,%=,t) 

single_jointasgn_intops(char)
single_jointasgn_intops(unsigned char)
single_jointasgn_intops(signed char)
single_jointasgn_intops(short)
single_jointasgn_intops(unsigned short)
single_jointasgn_intops(long)
single_jointasgn_intops(unsigned long)
single_jointasgn_fltops(float)
single_jointasgn_fltops(double)
#undef single_jointasgn_op
#undef single_jointasgn_fltops
#undef single_jointasgn_intops

#define jointasgn_operator(__op,__op_type) \
template <typename T, const size_t N> \
inline typename simd<T,N>::arrtype &simd<T,N>::arrtype::operator __op(const \
    __op_type &b) \
{ \
  *simd<T,N/2>::ptr((void *)this) __op b; \
  *(simd<T,N/2>::ptr((void *)this)+1) __op b; \
  return *this; \
}

jointasgn_operator(+=,T)
jointasgn_operator(-=,T)
jointasgn_operator(*=,T)
jointasgn_operator(/=,T)
jointasgn_operator(&=,T)
jointasgn_operator(^=,T)
jointasgn_operator(|=,T)
jointasgn_operator(%=,T)
jointasgn_operator(=,T)
jointasgn_operator(<<=,int)
jointasgn_operator(>>=,int)
#undef jointasgn_operator

#define single_constructor(t) \
inline simd<t,1>::arrtype::arrtype(const t &b) \
{ \
  v[0]=b; \
}

single_constructor(char)
single_constructor(unsigned char)
single_constructor(signed char)
single_constructor(short)
single_constructor(unsigned short)
single_constructor(long)
single_constructor(unsigned long)
single_constructor(float)
single_constructor(double)
#undef single_constructor

template <typename T, const size_t N> 
inline simd<T,N>::arrtype::arrtype(const T &b) 
{ 
  *simd<T,N/2>::ptr((void *)this)=b; 
  *(simd<T,N/2>::ptr((void *)this)+1)=b; 
}

#define pb_assign(t) \
inline simd<t,1>::arrtype::arrtype(const simd<pbool,1>::arrtype &b) { \
  v[0]=b.v[0]; \
} 

pb_assign(char)
pb_assign(unsigned char)
pb_assign(signed char)
pb_assign(short)
pb_assign(unsigned short)
pb_assign(long)
pb_assign(unsigned long)
pb_assign(float)
pb_assign(double)


template <typename T, const size_t N>
inline simd<T,N>::arrtype::arrtype(const typename simd<pbool,N>::arrtype &b) {
  *typename simd<T,N/2>::ptr((void *)this)=*typename simd<pbool,N/2>::ptr((void *)(&(b.v)));
  *(typename simd<T,N/2>::ptr((void *)this)+1)=*(typename simd<pbool,N/2>::ptr((void *)(&(b.v)))+1);
}

#define gen_shuffle_macro(t,n) \
template <const int order> \
inline typename simd<t,n>::arrtype REF shuffle( \
    const typename simd<t,n>::arrtype &v) \
{ \
  static t tmp[n];\
  switch (n) {                             \
    case  8:                               \
      tmp[n-5]=v.v[(order>>16) & (n-1)];   \
      tmp[n-6]=v.v[(order>>20) & (n-1)];   \
      tmp[n-7]=v.v[(order>>24) & (n-1)];   \
      tmp[n-8]=v.v[(order>>28) & (n-1)];   \
    case 4:                                \
      tmp[n-3]=v.v[(order>> 8) & (n-1)];   \
      tmp[n-4]=v.v[(order>>12) & (n-1)];   \
    case 2:                                \
      tmp[n-1]=v.v[(order    ) & (n-1)];   \
      tmp[n-2]=v.v[(order>> 4) & (n-1)];   \
      break;                               \
    default:                               \
        register int j=order;              \
	for (register int i=n-1;i>-1;i--) { tmp[i]=v.v[j&(n-1)]; j>>=4; } \
  }                                        \
  return *reinterpret_cast<simd<t,n>::arrtype *>(&tmp); \
} 

gen_shuffle_macro(double,2)
#ifndef USE_SSE
gen_shuffle_macro(float,4)
gen_shuffle_macro(long,4)
gen_shuffle_macro(unsigned long,4)
gen_shuffle_macro(short,4)
gen_shuffle_macro(unsigned short,4)
#endif
gen_shuffle_macro(float,2)
gen_shuffle_macro(long,2)
gen_shuffle_macro(unsigned long,2)
gen_shuffle_macro(char,8)
gen_shuffle_macro(unsigned char,8)
gen_shuffle_macro(signed char,8)
gen_shuffle_macro(char,4)
//gen_shuffle_macro(unsigned char,4)
gen_shuffle_macro(signed char,4)

#define gen_interleave_macro(t,n) \
template <const long long order> \
inline typename simd<t,n>::arrtype REF interleave( \
    const typename simd<t,n>::arrtype &a, const typename simd<t,n>::arrtype & b) \
{ \
  simd<t,n>::arrtype tmp; \
  register long long j=order; \
  for (register int i=n/2-1;i>-1;i--) { tmp.v[i]=a.v[j&(n-1)]; j>>=4; } \
  for (register int i=n-1;i>n/2-1;i--) { tmp.v[i]=b.v[j&(n-1)]; j>>=4; } \
  return tmp; \
}

gen_interleave_macro(char,8)
gen_interleave_macro(unsigned char,8)
gen_interleave_macro(signed char,8)

#define gen_interleave_four(t) \
template <const long long order> \
inline typename simd<t,4>::arrtype REF interleave( \
    const typename simd<t,4>::arrtype &a, const typename simd<t,4>::arrtype & b) \
{ \
    static simd<t,4>::arrtype tmp; \
    tmp.v[0]=a.v[(order>>12)&3]; \
    tmp.v[1]=a.v[(order>>8)&3]; \
    tmp.v[2]=b.v[(order>>4)&3]; \
    tmp.v[3]=b.v[order&3]; \
    return tmp; \
}

#if !defined(USE_SSE2) && !defined(USE_SSE) && !defined(USE_MMX) && \
  !defined(USE_3DNOW)
gen_interleave_four(float)
gen_interleave_four(long)
gen_interleave_four(unsigned long)
#endif

gen_interleave_four(short)
gen_interleave_four(unsigned short)

gen_interleave_four(char)
gen_interleave_four(signed char)

#define gen_interleave_two(t) \
template <const long long order> \
inline typename simd<t,2>::arrtype REF interleave( \
    const typename simd<t,2>::arrtype &a, const typename simd<t,2>::arrtype & b) \
{ \
    simd<t,2>::arrtype tmp; \
    tmp.v[0]=a.v[(order>>4)&1]; \
    tmp.v[1]=b.v[order&1]; \
    return tmp; \
}

#if !defined(USE_MMX) && !defined(USE_SSE) && !defined(USE_SSE2) && \
  !defined(USE_3DNOW) 
gen_interleave_two(long)
gen_interleave_two(unsigned long)
gen_interleave_two(float)
#endif

#if !defined(USE_SSE2)
gen_interleave_two(double)
#endif

