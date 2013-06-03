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
 *
 * timecvt.c
 *
 * Time conversion routines.
 *
 * $Id: timecvt.cpp,v 1.6.2.4 2007/02/22 02:28:42 vonkorff Exp $
 *
 */

#include "sah_config.h"
#include "str_util.h"
#include "str_replace.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include "util.h"
#include "s_util.h"
#include "timecvt.h"

void timecvt_start() {}

static void trim_newline(char*p) {
  int n = (int)strlen(p);
  if (n && (p[n-1]=='\n')) {
    p[n-1] = 0;
  }
}

char* jd_string(double jd) {
  static char buf[256], datestr[64];
  time_t tmptime;

  if ( jd > JD0 ) {
    tmptime=jd_to_time_t(jd);
    strlcpy(datestr, asctime(gmtime(&tmptime)), sizeof(datestr));
    trim_newline(datestr);
    sprintf(buf, "%14.5f (%s)", jd, datestr);
  } else {
    sprintf(buf,"%14.5f",jd);
  }
  return buf;
}

// output in human-readable form
//
char* short_jd_string(double jd) {
  static char buf[256];
  time_t tmptime;

  if ( jd > JD0) {
    tmptime=jd_to_time_t(jd);
    strlcpy(buf, asctime(gmtime(&tmptime)),sizeof(buf));
    trim_newline(buf);
  } else {
    strlcpy(buf,"Unknown",sizeof(buf));
  }
  return buf;
}

void st_to_ut(TIME *st) {
  double hour=((double)st->h)+((double)st->m)/60+((double)st->s)/3600
              +((double)st->c)/360000;
  if (st->tz !=0) {
    hour-=st->tz;
    st->tz=0;
    if (hour<0) {
      hour+=24;
      st->d--;
      if (st->d<1) {
        st->y--;
        st->d+=NUM_DAYS(st->y);
      }
    }
    if (hour>=24) {
      hour-=24;
      st->d++;
      if (st->d>NUM_DAYS(st->y)) {
        st->d-=NUM_DAYS(st->y);
        st->y++;
      }
    }
    st->h=(int)hour;
    hour-=st->h;
    hour*=60;
    st->m=(int)hour;
    hour-=st->m;
    hour*=60;
    st->s=(int)hour;
    hour-=st->s;
    hour*=100;
    st->c=(int)hour;
  }
}

double ut_to_jd(long year, long month, long day, double hour) {
  double jd;
  long l=(month-14)/12;
  jd=(double)(day-32075L+1461L*(year+4800L+l)/4+367l*(month-2-l*12)/12-
              3*((year+4900l+l)/100)/4);
  jd+=(hour/24-0.5);
  return(jd);
}

double time_t_to_jd(time_t unix_time) {
  return (((double)unix_time)/SECONDS_PER_DAY+JD0);
}

time_t jd_to_time_t(double jd) {
  return ((time_t)((jd-JD0)*SECONDS_PER_DAY));
}


void st_time_convert(TIME *st) {
  /* Fix Y2K Bug */
  if (st->y < 90) {
    st->y+=2000;
  } else if (st->y < 100) {
    st->y+=1900;
  }

  /* Convert time zome time into UT */
  st_to_ut(st);

  /* Fill in julian date field */
  {
    double hour=((double)st->h)+((double)st->m)/60+((double)st->s)/3600
                +((double)st->c)/360000;
    st->jd=ut_to_jd(st->y,1,st->d,hour);
  }

  /* Fill in unix_time field */
  st->unix_time=jd_to_time_t(st->jd);

}
void timecvt_end() {}

