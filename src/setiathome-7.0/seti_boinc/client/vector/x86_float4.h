// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend
// this exception to your version of the file, but you are not obligated to
// do so. If you do not wish to do so, delete this exception statement from
// your version.

// $Id: x86_float4.h,v 1.1.2.4 2007/07/16 15:36:57 korpela Exp $
//

// This file is empty is __i386__ is not defined
//
#ifndef _X86_FLOAT4_H_
#define _X86_FLOAT4_H_

#include "sah_config.h"

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_AMD64)

#include "x86_ops.h"

#ifndef M_PI
   #define M_PI 3.14159265358979323846
#endif

struct float4;
struct const_float4;

extern const_float4 SS1, SS2, SS3, SS4, CC1, CC2, CC3;
extern const_float4 SS1F, SS2F, SS3F, SS4F, CC1F, CC2F, CC3F;
extern const_float4 ZERO, ONE, TWO, THREE, RECIP_TWO;
extern const_float4 RECIP_THREE, RECIP_FOUR, RECIP_FIVE;
extern const_float4 RECIP_PI, RECIP_TWOPI, RECIP_HALFPI;
extern const_float4 R_NEG;
// 2^23 (used by quickRound)
extern const_float4 TWO_TO_23;
extern const_float4 INDGEN[2];

ALIGNED(static const int sign_bits[4],16)={INT_MIN, INT_MIN, INT_MIN, INT_MIN};
ALIGNED(static const int other_bits[4],16)={INT_MAX, INT_MAX, INT_MAX, INT_MAX};
#define SIGN_BITS (*(__m128i *)sign_bits)
#define OTHER_BITS (*(__m128i *)other_bits)



struct float4 {
       float4() {};
       float4(const __m128 b) { m=b; };
       float4(const float &fval) { 
#ifdef USE_INTRINSICS
               m=_mm_load1_ps(&fval);
#elif defined(__GNUC__)
               __asm__ ( "shufps $0,%0,%0\n":"=x" (m):"0" (fval) ); 
#endif
       };
       float4(const float &f1, const float &f2, const float &f3, const float &f4)
                    { v[0]=f1; v[1]=f2; v[2]=f3; v[3]=f4; };
       float4(const float4 &b) { m=b.m; };
       float4(const const_float4 &b) { m=((float4 *)&b)->m; };
       inline float4 operator +(const float4 &f) const {
              register float4 rv;
#ifdef USE_INTRINSICS
              rv.m=_mm_add_ps(m,f.m);
#elif defined(__GNUC__)
              __asm__ ( "addps %2,%0" : "=x" (rv.m) : "0" (m), "xm" (f.m) );
#endif
              return  rv;
       };
       inline float4 operator -(const float4 &f) const {
              register float4 rv;
#ifdef USE_INTRINSICS
	          rv.m=_mm_sub_ps(m,f.m);
#elif defined(__GNUC__)
              __asm__ ( "subps %2,%0" : "=x" (rv.m) : "0" (m), "xm" (f.m) );
#endif
              return  rv;
       };
       inline float4 operator *(const float4 &f) const {
              register float4 rv;
#ifdef USE_INTRINSICS
	      rv.m=_mm_mul_ps(m,f.m);
#elif defined(__GNUC__)
              __asm__ ( "mulps %2,%0" : "=x" (rv.m) : "0" (m), "xm" (f.m) );
#endif
	      return  rv;
       };
       inline float4 operator |(const __m128i &b) const {
              register float4 rv;
#ifdef USE_INTRINSICS
	          rv.m=_mm_or_ps(*(__m128 *)&b,m);
#elif defined(__GNUC__)
              __asm__ ( "orps %2,%0" : "=x" (rv.m) : "0" (b), "xm" (m));
#endif
	      return rv;
       };
       inline float4 operator &(const __m128i &b) const {
              register float4 rv;
#ifdef USE_INTRINSICS
	      rv.m=_mm_and_ps(*(__m128 *)&b,m);
#elif defined(__GNUC__)
              __asm__ ( "andps %2,%0" : "=x" (rv.m) : "0" (b), "xm" (m));
#endif
	      return rv;
       };
       inline float4 &operator =(const float4 &b) {
              m=b.m;
	      return *this;
       };
       inline float4 &operator *=(const float4 &b) {
#ifdef USE_INTRINSICS
	      m=_mm_mul_ps(m,b.m);
#elif defined(__GNUC__)
              __asm__ ( "mulps %2,%0" : "=x" (m) : "0" (m), "xm" (b.m) );
#endif
	      return *this;
       }
       inline float4 &operator +=(const float4 &b) {
#ifdef USE_INTRINSICS
	      m=_mm_add_ps(m,b.m);
#elif defined(__GNUC__)
              __asm__ ( "addps %2,%0" : "=x" (m) : "0" (m), "xm" (b.m) );
#endif
	      return *this;
       }
       inline float4 &operator -=(const float4 &b) {
#ifdef USE_INTRINSICS
	      m=_mm_sub_ps(m,b.m);
#elif defined(__GNUC__)
              __asm__ ( "subps %2,%0" : "=x" (m) : "0" (m), "xm" (b.m) );
#endif
	      return *this;
       }
       inline float4 &operator /=(const float4 &b) {
#ifdef USE_INTRINSICS
	      m=_mm_div_ps(m,b.m);
#elif defined(__GNUC__)
              __asm__ ( "divps %2,%0" : "=x" (m) : "0" (m), "xm" (b.m) );
#endif
	      return *this;
       }
       inline operator __m128() {return m;};
       inline operator __m128i() {return *(__m128i *)&m; };
       inline float4 abs() const {
              // clear the sign bits
              return *this & OTHER_BITS;
       }; 
       
       inline float4 sign() const {
              // return the sign bits
              return *this & SIGN_BITS;
       }; 
       inline float4 round() const {
              // add and subtract the largest exactly 
              // representable integer value of the same sign
              register float4 s(sign());
              register float4 a(s | (float4)TWO_TO_23);
              a=(*this+a)-a;
              return a;
       };
       inline float4 ceil() const {
              return (*this+RECIP_TWO).round();
       }
       inline float4 floor() const {
              return (*this-RECIP_TWO).round();
       }
       inline float4 mod(const float4 &x) const {
              register float4 nx(*this*x.recip());
              nx-=nx.round();  // [-0.5,0.5)
              nx*=x;          // [-x/2,x/2)
              return nx;
       }       
       inline float4 round_pos() const {
              return ((*this+TWO_TO_23)-TWO_TO_23);
       };
       inline float4 round_neg() const {
              return ((*this-TWO_TO_23)+TWO_TO_23);
       }; 
       inline void sincos(float4 &s, float4 &c) const {
              register float4 x(*this*RECIP_TWOPI);
              x-=x.round();
              register float4 x2(x*x);           
              register float4 t1(x2*SS4+SS3);
              register float4 t2(x2*CC3+CC2);
              t1*=x2;
              t2*=x2;
              t1+=SS2;
              t2+=CC1;
              t1*=x2;
              t2*=x2;
              t1+=SS1;
              c=t2+ONE;
              s=t1*x;
       }      
       inline float4 sin() const {
              register float4 x(*this*RECIP_TWOPI);
              x-=x.round();
              register float4 x2(x*x);
              return ((((x2*SS4+SS3)*x2+SS2)*x2+SS1)*x);    
       }
       inline float4 cos() const {
              register float4 x(*this*RECIP_TWOPI);
              x-=x.round();
              x*=x;
              return (((x*CC3+CC2)*x+CC1)*x+ONE);
       }
       template <int p0, int p1, int p2, int p3> 
       inline float4 shuffle(float4 b) const {
#ifdef USE_INTRINSICS
         return _mm_shuffle_ps(m,b.m,_MM_SHUFFLE(p3,p2,p1,p0));
#elif defined(__GNUC__)
         register float4 rv;
         __asm__ ( "shufps  %3,%2,%0" : "=x" (rv) : "0" (m), "x" (b.m), "i" (p0 | (p1 << 2) | (p2<<4) | (p3 << 6)) );
         return rv;
#endif
       }
       inline float4 rsqrt() const {
       }
       inline float4 sqrt() const {
       }
       inline float4 recip() const {
       }
       union {
             ALIGNED(__m128 m,16);
             float v[4];
             float f;
       };
} ; 

struct const_float4 : public float4 {
  public:
    inline const_float4() : float4() {};
    inline const_float4(const __m128 b) : float4() { m=b; };
    inline const_float4(const float &fval) : float4() {
      v[0]=fval; v[1]=fval; v[2]=fval; v[3]=fval;
    }
    inline const_float4(const float &f1, const float &f2, const float &f3, const float &f4)
       { v[0]=f1; v[1]=f2; v[2]=f3; v[3]=f4; };
    inline const_float4(const float4 &b) { m=b.m; };
    inline operator float4() const { return *this; };
    inline operator __m128() {return m;};
    inline operator __m128i() {return *(__m128i *)&m; };
};

#endif
#endif

