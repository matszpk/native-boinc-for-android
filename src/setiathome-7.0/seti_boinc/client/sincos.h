// Copyright 2005 Regents of the University of California

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

// seti.C
// $Id: sincos.h,v 1.2.2.2 2005/06/26 19:55:47 korpela Exp $
//


/* Implement the sincos[f] function on machines that don't have it */
#ifndef _SINCOS_H_
#define _SINCOS_H_
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif

/* These are outside of the #ifndefs because on some machines sinf and cosf */
/* are in libstdc++, but are not defined in the system header <math.h>.     */
void sincos(double angle, double *s, double *c);
void sincosf(float angle, float *s, float *c);
float sinf(float angle);
float cosf(float angle);
float atanf(float angle);

#ifndef HAVE_SINCOSF
inline void sincosf(float angle, float *s, float *c) {
/* do we have sincos? If so use it since it's likely to */
/* be faster than separate computation                  */
#ifdef HAVE_SINCOS
  float sd,cd;
  sincos(angle,&sd,&cd);
  *s=(float)sd;
  *c=(float)cd;
/* if we don't have sincos, how about sinf and cosf?    */
#elif defined(HAVE_SINF) && defined(HAVE_COSF)
  *s=sinf(angle);
  *c=cosf(angle);
/* as a last resort, compute the old fashioned way      */
#else
  *s=(float)sin(angle);
  *c=(float)cos(angle);
#endif
}
#endif

#ifndef HAVE_SINCOS
inline void sincos(double angle, double *s, double *c) {
/* this is the obvious implementation.  Is there a      */
/* better trickier way?                                 */
  *s=sin(angle);
  *c=cos(angle);
}
#endif

#ifndef HAVE_SINF
inline float sinf(float angle) {
  return (float)sin(angle);
}
#endif

#ifndef HAVE_COSF
inline float cosf(float angle) {
  return (float)cos(angle);
}
#endif

#ifndef HAVE_ATANF
inline float atanf(float angle) {
  return (float)atan(angle);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
