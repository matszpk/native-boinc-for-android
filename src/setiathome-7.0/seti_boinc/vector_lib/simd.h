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
// $Id: simd.h,v 1.10.2.1 2007/03/22 00:03:59 korpela Exp $
//
// Original revision: 28-Jul-2004, Eric J. Korpela
//
  
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cstring>

// HOW TO ALIGN
#if defined(__GNUC__)
#define ALIGN_DEF(x) 
#define ALIGN_ATTRIBUTE(x) __attribute__ ((aligned(x)))
#if defined(USE_MMX) || defined(USE_SSE) || \
    defined(USE_SSE2) || defined(USE_3DNOW)
#define REF &
#else 
#define REF
#endif
typedef float __m128 __attribute__ ((mode(V4SF)));
typedef int __m64 __attribute__ ((mode(V4HI)));
typedef unsigned long __m32;
typedef unsigned short __m16;
typedef unsigned char __m8;
#define PURE_FUNCTION __attribute__ ((pure))
#define HAVE__M128
#define HAVE__M64
#elif defined(_MSC_VER)
#define REF
#define ALIGN_DEF(x) __declspec(align( x )) 
#define ALIGN_ATTRIBUTE(x) 
#define PURE_FUNCTION 
#else
#define REF
#define ALIGN_DEF(x)
#define ALIGN_ATTRIBUTE(x)
#define PURE_FUNCTION 
#endif


struct s_m128 {
#ifdef HAVE__M128
  __m128 m128;
#elif (SIZEOF__LONG_DOUBLE)==16
  long double m128
#elif defined(HAVE_UINT64_T)
  uint64_t m128[2];
#elif defined(HAVE_LONG_LONG)
  unsigned long long m128[2];
#elif defined(HAVE___INT64)
  __int64 m128[2];
#endif
} ALIGN_ATTRIBUTE(16);

typedef unsigned char pbool;


typedef union {
  __m8 all ALIGN_ATTRIBUTE(1);
  unsigned char c[1] ALIGN_ATTRIBUTE(1);
} vec_m8;

typedef union {
  __m16 all ALIGN_ATTRIBUTE(2);
  unsigned short s[1] ALIGN_ATTRIBUTE(2);
  unsigned char c[2] ALIGN_ATTRIBUTE(2);
  vec_m8 v8[2] ALIGN_ATTRIBUTE(2);
} vec_m16;

typedef union {
  __m32 all ALIGN_ATTRIBUTE(4);
  float f[1] ALIGN_ATTRIBUTE(4);
  unsigned long l[4/sizeof(long)] ALIGN_ATTRIBUTE(4);
  unsigned int i[4/sizeof(int)] ALIGN_ATTRIBUTE(4);
  unsigned short s[8/sizeof(short)] ALIGN_ATTRIBUTE(4);
  unsigned char c[4] ALIGN_ATTRIBUTE(4);
  vec_m8 v8[4] ALIGN_ATTRIBUTE(4);
  vec_m16 v16[2] ALIGN_ATTRIBUTE(4);
} vec_m32;

#if defined(HAVE_UNIT64_T)
typedef uint64_t v_i64;
#elif defined(HAVE_LONG_LONG)
typedef unsigned long long v_i64;
#elif defined(HAVE__INT64)
typedef unsigned _int64 v_i64;
#else
typedef unsigned long v_i64[2];
#endif

typedef union {
  __m64 all ALIGN_ATTRIBUTE(8);
  v_i64 ll[1] ALIGN_ATTRIBUTE(8);
  double d[8/sizeof(double)] ALIGN_ATTRIBUTE(8);
  float f[8/sizeof(float)] ALIGN_ATTRIBUTE(8);
  unsigned long l[8/sizeof(long)] ALIGN_ATTRIBUTE(8);
  unsigned int i[8/sizeof(int)] ALIGN_ATTRIBUTE(8);
  unsigned short s[8/sizeof(short)] ALIGN_ATTRIBUTE(8);
  unsigned char c[8] ALIGN_ATTRIBUTE(8);
  vec_m8 v8[8] ALIGN_ATTRIBUTE(8);
  vec_m16 v16[4] ALIGN_ATTRIBUTE(8);
  vec_m32 v32[2] ALIGN_ATTRIBUTE(8);
} vec_m64; 

typedef union {
  __m128 all ALIGN_ATTRIBUTE(16);
#ifdef HAVE__M128
  __m128 m128 ALIGN_ATTRIBUTE(16);
#endif
#ifdef HAVE__M128D
  __m128d m128d ALIGN_ATTRIBUTE(16);
#endif
#ifdef HAVE__M128D
  __m128i m128i ALIGN_ATTRIBUTE(16);
#endif
#if defined(HAVE_UINT128_T)
  uint128_t u128 ALIGN_ATTRIBUTE(16);
#elif defined(HAVE___INT128)
  __int128 u128 ALIGN_ATTRIBUTE(16);
#endif
#if defined(HAVE_UINT64_T)
        inline operator __m128() const { return *(reinterpret_cast<__m128 *>(this)) };
  uint64_t ll[2] ALIGN_ATTRIBUTE(16);
#elif defined(HAVE_LONG_LONG)
  unsigned long long ll[2] ALIGN_ATTRIBUTE(16);
#elif defined(HAVE___INT64)
  __int64 ll[2] ALIGN_ATTRIBUTE(16);
#endif
#if defined(HAVE_LONG_DOUBLE) 
#if (SIZEOF_LONG_DOUBLE>8)
  long double ld[1] ALIGN_ATTRIBUTE(16);
#elif (SIZEOF_LONG_DOUBLE == 8)
  long double ld[2] ALIGN_ATTRIBUTE(16);
#endif
#endif
  double d[16/sizeof(double)] ALIGN_ATTRIBUTE(16);
  float f[16/sizeof(float)] ALIGN_ATTRIBUTE(16);
  unsigned long l[16/sizeof(long)] ALIGN_ATTRIBUTE(16);
  unsigned int i[16/sizeof(int)] ALIGN_ATTRIBUTE(16);
  unsigned short s[16/sizeof(short)] ALIGN_ATTRIBUTE(16);
  unsigned char c[16] ALIGN_ATTRIBUTE(16);
  vec_m8 v8[16] ALIGN_ATTRIBUTE(16);
  vec_m16 v16[8] ALIGN_ATTRIBUTE(16);
  vec_m32 v32[4] ALIGN_ATTRIBUTE(16);
  vec_m64 v64[2] ALIGN_ATTRIBUTE(16);
} vec_m128;   




// Generic definitions

template <typename t1, typename t0>
inline t1 &interpret_as(t0 &a) { return reinterpret_cast<t1 &>(a); }

template <typename T, const size_t N>
class simd {
  public:
    class ptr;
    class arrtype;
    class arrtype {
      public: 
	const size_t size(T*N);
	typedef T array[N] ALIGN_ATTRIBUTE(size);
        union {
          array v;
	  vec_m8 m8;
          vec_m16 m16;
          vec_m32 m32;
          vec_m64 m64;
          vec_m128 m128;
        };
        arrtype(vec_m128 &b) : m128(b) {};
        arrtype(vec_m64 &b) : m64(b) {};
        arrtype(vec_m32 &b) : m32(b) {};
        arrtype(vec_m16 &b) : m16(b) {};
        arrtype(vec_m8 &b) : m8(b) {};
        arrtype(__m128 &b) : m128(reinterpret_cast<vec_m128 &>(b)) {};
	arrtype(const typename simd<pbool,N>::arrtype &b);
	arrtype() {} ;
	arrtype(const T &m);
        void prefetch() const PURE_FUNCTION;
        void prefetchw() const PURE_FUNCTION;

	inline T &operator [](int i) {return v[i];}
	// binary operators
#define binop(__op,t) \
	arrtype REF operator __op(const arrtype &b) const PURE_FUNCTION; \
        arrtype operator __op(const t &n) const PURE_FUNCTION;
        binop(+,T)
        binop(-,T)
	binop(*,T)
	binop(/,T)
	binop(&,T)
	binop(^,T)
        binop(|,T)
	binop(%,T)
	binop(<<,int)
	binop(>>,int)
#undef binop
#define bool_binop(__op,t) \
        typename simd<pbool,N>::arrtype REF operator __op(const typename \
	    simd<t,N>::arrtype &b) const PURE_FUNCTION; \
        typename simd<pbool,N>::arrtype operator __op(const T &n) const PURE_FUNCTION;
	bool_binop(&&,T)
	bool_binop(||,T)
	bool_binop(==,T)
	bool_binop(!=,T)
	bool_binop(<,T)
	bool_binop(>,T)
	bool_binop(<=,T)
	bool_binop(>=,T)
	// unary operators
        arrtype REF operator +() const { return *this; };
        arrtype REF operator -() const PURE_FUNCTION;
        arrtype REF operator ~() const PURE_FUNCTION;
        typename simd<pbool,N>::arrtype REF operator !() const PURE_FUNCTION;
        //arrtype operator *() const;
        ptr operator &() { return ptr((void *)this); };
	// assignment operators
        arrtype &operator =(const arrtype &b);
        arrtype &operator =(const T &b);
        arrtype &operator +=(const arrtype &b);
        arrtype &operator +=(const T &b);
        arrtype &operator -=(const arrtype &b);
        arrtype &operator -=(const T &b);
        arrtype &operator *=(const arrtype &b);
        arrtype &operator *=(const T &b);
        arrtype &operator /=(const arrtype &b);
        arrtype &operator /=(const T &b);
        arrtype &operator &=(const arrtype &b);
        arrtype &operator &=(const T &b);
        arrtype &operator ^=(const arrtype &b);
        arrtype &operator ^=(const T &b);
        arrtype &operator |=(const arrtype &b);
        arrtype &operator |=(const T &b);
        arrtype &operator %=(const arrtype &b);
        arrtype &operator %=(const T &b);
        arrtype &operator <<=(const arrtype &b);
        arrtype &operator <<=(const int &b);
        arrtype &operator >>=(const arrtype &b);
        arrtype &operator >>=(const int &b);
	// other functions
	arrtype REF sqrt() const PURE_FUNCTION;
	arrtype REF aprx_sqrt() const PURE_FUNCTION;
	arrtype REF rsqrt() const PURE_FUNCTION;
	arrtype REF aprx_rsqrt() const PURE_FUNCTION;
	arrtype REF recip() const PURE_FUNCTION;
	arrtype REF aprx_recip() const PURE_FUNCTION;
	arrtype REF max(const arrtype &b) const PURE_FUNCTION;
	arrtype REF min(const arrtype &b) const PURE_FUNCTION;
	arrtype REF avg(const arrtype &b) const PURE_FUNCTION;
	arrtype REF splat(const int &b) const PURE_FUNCTION;
	// conversion operators
        operator typename simd<pbool,N>::arrtype() const;
	operator __m128() { return *(reinterpret_cast<__m128 *>(this)); } ; 
    };      
    arrtype data;
    static const size_t size;
    static const size_t nbits;
    static volatile void simd_mode_start();
    static volatile void simd_mode_finish();
    class ptr {
      private:
	union {
	  unsigned char *p;
          arrtype *v;
        };
      public:
        inline operator void *() const { return (void *)p; };
        inline void *addr() const { return (void *)p; };
        inline ptr(void *a) : p((unsigned char *)a) {};
        inline ptr operator +(int i) const 
          { return ptr(p+i*(N*sizeof(T))); };
        inline ptr operator -(int i) const 
          { return ptr(p-i*(N*sizeof(T))); };
        inline off_t operator -(simd<T,N>::ptr &pp) const
          { return (p-pp.p)/(N*sizeof(T)); };
        inline ptr &operator ++() 
          { p+=(N*sizeof(T)); return *this; };
        inline ptr operator ++(int i) 
          { register void *z(p); p+=(N*sizeof(T)); return z; };
        inline ptr &operator --() 
          { p-=(N*sizeof(T)); return *this; };
        inline ptr operator --(int i) 
          { register void *z(p); p-=(N*sizeof(T)); return z; };
        inline ptr operator +=(int i)
          { return p+=(i*N*sizeof(T)); };
        inline ptr operator -=(int i)
          { return p-=(i*N*sizeof(T)); };
        inline arrtype &operator *();
        inline const arrtype &operator [](const int n) const { 
          return static_cast<const T &>(*(const arrtype *)(p+n*N*sizeof(T)));
        };
        inline arrtype &operator [](const int n) {
          return static_cast<arrtype &>(*(arrtype *)(p+n*N*sizeof(T)));
        };
    };
};

template <typename t, const size_t n>
const size_t simd<t,n>::size=n*sizeof(t);

template <typename t, const size_t n>
const size_t  simd<t,n>::nbits=n*sizeof(t)*CHAR_BIT;

template <const int order, typename T, size_t N> 
typename simd<T,N>::arrtype &shuffle(const typename simd<T,N>::arrtype &v)
  PURE_FUNCTION;


// generics to fill in the missing stuff
#include "generics.h"
// stuff for specific processors
#ifdef USE_MMX
#include "mmx.h"
#elif defined(USE_SSE)
#include "sse.h"
#elif defined(USE_SSE2)
#include "sse2.h"
#elif defined(USE_3DNOW)
#include "3dnow.h"
#elif defined(USE_ALTIVEC)
#include "altivec.h"
#elif defined(USE_VIS)
#include "vis.h"
#elif defined(USE_VIS2)
#include "vis2.h"
#endif
#include "more_generics.h"


typedef simd<double,2>::arrtype double2;
typedef simd<long long,2>::arrtype longlong2;
typedef simd<unsigned long long,2>::arrtype ulonglong2;
typedef simd<float,4>::arrtype float4;
typedef simd<long,4>::arrtype long4;
typedef simd<unsigned long,4>::arrtype ulong4;
typedef simd<short,8>::arrtype short8;
typedef simd<unsigned short,8>::arrtype ushort8;
typedef simd<char,16>::arrtype char16;
typedef simd<unsigned char,16>::arrtype uchar16;
typedef simd<signed char,16>::arrtype schar16;

typedef simd<float,2>::arrtype float2;
typedef simd<long,2>::arrtype long2;
typedef simd<unsigned long,2>::arrtype ulong2;
typedef simd<short,4>::arrtype short4;
typedef simd<unsigned short,4>::arrtype ushort4;
typedef simd<char,8>::arrtype char8;
typedef simd<unsigned char,8>::arrtype uchar8;
typedef simd<signed char,8>::arrtype schar8;

typedef simd<short,2>::arrtype short2;
typedef simd<unsigned short,2>::arrtype ushort2;
typedef simd<char,4>::arrtype char4;
typedef simd<unsigned char,4>::arrtype uchar4;
typedef simd<signed char,4>::arrtype schar4;

