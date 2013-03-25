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

#ifndef _SQLINT8_H_
#define _SQLINT8_H_

#include "sah_config.h"
#include <limits.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
typedef int64_t sqlint8_t;
#ifdef PRIdLEAST64
  #define INT8_FMT PRIdLEAST64
#else
  #if SIZEOF_LONG_INT >=8
    #define INT8_FMT "ld"
  #else
    #define INT8_FMT "lld"
  #endif
#endif
#define INT8_PRINT_CAST(x) x
#define INT8_USING_INT64_T


#elif SIZEOF_LONG_INT >= 8
typedef long sqlint8_t;
#define INT8_FMT "ld"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_LONG

#elif defined(LLONG_MAX) || defined(HAVE_LONG_LONG)
typedef long long sqlint8_t;
#define INT8_FMT "lld"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_LONG_LONG

#elif defined(USE_MYSQL) && !defined(CLIENT)
#include "my_config.h"
#include "global.h"
typedef longlong sqlint8_t;
#define INT8_FMT "lld"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_LONGLONG

#elif defined(_INTEGRAL_MAX_BITS) && (_INTEGRAL_MAX_BITS >= 64)
typedef _int64 sqlint8_t;
#define INT8_FMT "lld"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_INT64

#elif defined(USE_INFORMIX)
#include "int8.h"
#include "sqlhdr.h"
#include <string>
#include <iostream>

class _ifx_int8 {
  public:
    _ifx_int8(ifx_int8 a) : val(a) {};
    _ifx_int8(const char *s) {ifx_int8cvasc(const_cast<char *>(s),strlen(s),&val);};
    _ifx_int8(double d) {ifx_int8cvdbl(d,&val);};
    _ifx_int8(long l) {ifx_int8cvlong(l,&val);}; 
    _ifx_int8(int l=0) {ifx_int8cvlong(static_cast<long>(l),&val);}; 
    operator const char *() const ;
    operator long() const ;
  private:
    ifx_int8 val;
    static char pval[20];
};


inline _ifx_int8::operator const char *() const {
   ifx_int8toasc(const_cast<ifx_int8 *>(&val),pval,20); 
   pval[19]=0; 
   return pval;
}

inline _ifx_int8::operator long() const {
   long l;
   ifx_int8tolong(const_cast<ifx_int8 *>(&val),&l); 
   return l;
}

typedef _ifx_int8 sqlint8_t;
#define INT8_FMT "s"
#define INT8_PRINT_CAST(x) static_cast<const char *>(x)
#define INT8_USING_IFX

#elif defined(HAVE_LONG_DOUBLE)
typedef long double sqlint8_t;
#define INT8_FMT "lf21.0"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_LONG_DOUBLE

#else
typedef double sqlint8_t;
#define INT8_FMT "f16.0"
#define INT8_PRINT_CAST(x) x
#define INT8_USING_DOUBLE
#endif

#ifndef HAVE_ATOLL
inline sqlint8_t atoll(const char *p) {
  sqlint8_t rv=0;
  sqlint8_t sign=1;
  while (isspace(*(p++))) /* do nothing */ ;
  if (*p=='-') {
    sign=-1;
    p++;
  }
  while (isdigit(*(p++))) {
    rv*=10;
    rv+=(*(p-1)-'0');
  }
  rv*=sign;
  return rv;
}
#endif

#if defined(INT8_USING_INT64) || defined(INT8_USING_IFX)
inline std::ostream &operator <<(std::ostream &o, const sqlint8_t &p) {
  char buf[32];
  sprintf(buf,"%"INT8_FMT,INT8_PRINT_CAST(p));
   o << buf ;
   return o;
}

inline std::istream &operator >>(std::istream &i, sqlint8_t &p) {
   char c=' ';
   sqlint8_t sign=1;
   p=0;
   while (isspace(c) && !i.eof()) i.get(c);
   if (c=='-') {
     sign=-1;
     if (!i.eof()) i.get(c);
   }
   while (isdigit(c) && !i.eof()) {
     p*=10;
     p+=(c-'0');
	 i.get(c);
   }
   if (!i.eof()) i.putback(c);
   p*=sign;
   return i;
}
#endif
#endif
