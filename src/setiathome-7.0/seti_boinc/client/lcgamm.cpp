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

/*
 * $Id: lcgamm.cpp,v 1.9.2.11 2007/05/31 22:03:10 korpela Exp $
 */
#include "sah_config.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <crtdbg.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits.h>
#include <limits>
#include <time.h>  // delete me!

#include "diagnostics.h"

#define ITMAX 10000  // Needs to be a few times the sqrt of the max. input to lcgf
#define FP_EPS 1.0e-35

/*  Log of the complete gamma function  */
static double gammln(double a) {

  double x,y,tmp,ser;
  static double cof[6]={76.18009172947146,-86.50532032941677,
                        24.01409824083091,-1.231739572450155,
                        0.1208650973866179e-2,-0.5395239384953e-5};
  int j;

  y=x=a;
  tmp=x+5.5;
  tmp -= (x+0.5)*log(tmp);
  ser=1.000000000190015;
  for (j=0;j<=5;j++) ser += cof[j]/++y;
  return (double)(-tmp+log(2.5066282746310005*ser/x));
}

/* Log of the compliment of the incomplete gamma function
 * log(1-P(a,x)) valid only for (a+1)<x
 */

double lcgf(double a, double x) {
  int i;
  const double EPS=std::numeric_limits<double>::epsilon();
  const double FPMIN=std::numeric_limits<double>::min()/EPS;
  double an,b,c,d,del,h,gln=gammln(a);

  // assert(x>=(a+1));
  BOINCASSERT(x>=(a+1));
  b=x+1.0-a;
  c=1.0f/FPMIN;
  d=1.0/b;
  h=d;
  for (i=1;i<=ITMAX;i++) {
    an = -i*(i-a);
    b += 2.0;
    d=an*d+b;
    if (fabs(d)<FPMIN) d=FPMIN;
    c=b+an/c;
    if (fabs(c)<FPMIN) c=FPMIN;
    d=1.0/d;
    del=d*c;
    h*=del;
    if (fabs(del-1.0)<EPS) break;
  }
  // assert(i<ITMAX);
  BOINCASSERT(i<ITMAX);
  return (float)(log(h)-x+a*log(x)-gln);
}

float lcgf(float a, float x, long& timecalled, long& numcalls) {
 numcalls++; 
 static double mytime=0;
 long begintime = clock(); 
 float rv=lcgf(a,x);
 long endtime = clock();
 mytime+=(double)(endtime-begintime)/CLOCKS_PER_SEC;
 if (endtime < begintime) {
   mytime+=UINT_MAX;
 }
 timecalled=(long)mytime;
 return rv;
}

static float dlcgf(float a, float x) {
  return (lcgf(a,x+0.1f)-lcgf(a,x-0.1f))/0.2f;
}

/* Solve a log(1-P(a,x))=y for x given the value of a and
 * allowed fractional error in y of frac_err
 */
float invert_lcgf(float y,float a, float frac_err) {
  int j;
  float df,dx,dxold,f,fh,fl;
  float temp,xh,xl,rts;

  xh=(a+1.5f);
  xl=(float)(a-2.0*y*sqrt(a));
  fl=lcgf(a,xl)-y;
  fh=lcgf(a,xh)-y;

  BOINCASSERT(fl<=0);
  BOINCASSERT(fh>=0);

  rts=0.5f*(xh+xl);
  dxold=(float)fabs(xh-xl);
  dx=dxold;
  f=lcgf(a,rts)-y;
  df=dlcgf(a,rts);
  for (j=1;j<=ITMAX;j++) {
    if ((((rts-xh)*df-f)*((rts-xl)*df-f) >= 0.0)
        || (fabs(2.0*f)>fabs(dxold*df))) {
      dxold=dx;
      dx=0.5f*(xh-xl);
      rts=xl+dx;
      if ((xl==rts) || (xh==rts)) return rts;
    } else {
      dxold=dx;
      dx=f/df;
      temp=rts;
      rts-=dx;
      if (temp==rts) return rts;
    }
    f=lcgf(a,rts)-y;
    df=dlcgf(a,rts);
    if (fabs(f)<fabs(frac_err*y)) return rts;
    if (f<0.0)
      xl=rts;
    else
      xh=rts;
  }
  BOINCASSERT(ITMAX);
  return 0.0;
}

