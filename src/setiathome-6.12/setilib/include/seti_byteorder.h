// Copyright (c) 1999-2006 Regents of the University of California

// This library is free software; you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published by the 
// Free Software Foundation; either version 2.1, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU Lesser General Public License 
// along with this library; see the file COPYING.  If not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
// MA 02111-1307, USA. or visit http://www.gnu.org/copyleft/lesser.html

#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

/* Update History */
/*******************************************************************************
**
** $Log: seti_byteorder.h,v $
** Revision 1.3  2007/04/18 21:03:51  boincadm
** Fixed bug.  Was checking for define of WORDS_BIG_ENDIAN rather than
** WORDS_BIGENDIAN
**
** Revision 1.2  2007/02/08 22:14:58  korpela
** *** empty log message ***
**
** Revision 1.1  2006/12/14 21:41:20  korpela
** *** empty log message ***
**
**
*******************************************************************************/

#if defined(WORDS_BIGENDIAN) && !defined(FLOAT_WORDS_BIGENDIAN)
#warning The motorola and intel templates do not work when the byte orders of words and floats do not match
#endif

template <typename T>
class motorola {
  public:
    motorola<T>();
//    explicit motorola<T>(const unsigned char *p);
    motorola<T>(const T &t);
    operator T() const;
//    operator unsigned char *() {return &(c[0]);};
  private:
    union {
      unsigned char c[sizeof(T)];
      T native;  // for access on big endian platforms
    };
};

template <typename T>
motorola<T>::motorola() {
  unsigned int i;
  for (i=0;i<sizeof(T);i++) c[i]=0;
}

template <typename T>
motorola<T>::motorola(const T &t) {
#ifdef WORDS_BIGENDIAN
  native=t;
#else
  unsigned int i;
  const char *p=&t;
  for (i=0;i<sizeof(T);i++) c[i]=p[sizeof(T)-i-1];
#endif
}

#if 0
template <typename T>
motorola<T>::motorola<T>(const unsigned char *p) {
#ifdef WORDS_BIGENDIAN
  native=*reinterpret_cast<T *>(p);
#else
  unsigned int i;
  for (i=0;i<sizeof(T);i++) c[i]=p[i];
#endif
}
#endif
template <typename T>
motorola<T>::operator T() const {
#ifdef WORDS_BIGENDIAN
  return native;
#else
  union {
    T t;
    unsigned char c[sizeof(T)];
  } rv;
  unsigned int i;
  for (i=0;i<sizeof(T);i++) rv.c[i]=c[sizeof(T)-i-1];
  return rv.t;
#endif  
}

typedef motorola<double> motorola_double;
typedef motorola<float> motorola_float;
typedef motorola<long> motorola_long;
typedef motorola<unsigned long> motorola_ulong;
typedef motorola<short> motorola_short;
typedef motorola<unsigned short> motorola_ushort;
typedef motorola<int> motorola_int;
typedef motorola<unsigned int> motorola_uint;

template <typename T>
class intel {
  public:
    intel<T>();
//    explicit intel<T>(const unsigned char *p);
    intel<T>(const T &t);
    operator T() const;
//    operator unsigned char *() {return &(c[0]);};
  private:
    union {
      unsigned char c[sizeof(T)];
      T native;  // for access on little endian platforms
    };
};

template <typename T>
intel<T>::intel() {
  unsigned int i;
  for (i=0;i<sizeof(T);i++) c[i]=0;
}

template <typename T>
intel<T>::intel(const T &t) {
#ifndef WORDS_BIGENDIAN
  native=t;
#else
  unsigned int i;
  const char *p=reinterpret_cast<const char *>(&t);
  for (i=0;i<sizeof(T);i++) c[i]=p[sizeof(T)-i-1];
#endif
}

#if 0
template <typename T>
intel<T>::intel<T>(const unsigned char *p) {
#ifndef WORDS_BIGENDIAN
  native=*reinterpret_cast<T *>(p);
#else
  unsigned int i;
  for (i=0;i<sizeof(T);i++) c[i]=p[i];
#endif
}
#endif

template <typename T>
intel<T>::operator T() const {
#ifndef WORDS_BIGENDIAN
  return native;
#else
  union {
    T t;
    unsigned char c[sizeof(T)];
  } rv;
  unsigned int i;
  for (i=0;i<sizeof(T);i++) rv.c[i]=c[sizeof(T)-i-1];
  return rv.t;
#endif  
}

typedef intel<double> intel_double;
typedef intel<float> intel_float;
typedef intel<long> intel_long;
typedef intel<unsigned long> intel_ulong;
typedef intel<short> intel_short;
typedef intel<unsigned short> intel_ushort;
typedef intel<int> intel_int;
typedef intel<unsigned int> intel_uint;

#endif


