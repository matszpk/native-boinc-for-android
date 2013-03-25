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
 * timecvt.h
 *
 * Time conversion routines.
 *
 * $Id: timecvt.h,v 1.3.2.2 2005/10/09 18:24:22 korpela Exp $
 *
 */

#ifndef TIMECVT_H
#define TIMECVT_H

#include <ctime>

struct TIME {
  int y,d,h,m,s,c;
  double tz;
  double jd;
  time_t unix_time;
  TIME operator -(const TIME &b) const;
};



#ifdef __MWERKS__
// Note: Although Macintosh time is based on 1/1/1904, the
//MetroWerks Standard Libraries time routines use a base of 1/1/1900.
//#define JD0 2416480.5  /* Time at 0:00 GMT 1904 Jan 1 */
#define JD0 2415020.5  /* Time at 0:00 GMT 1900 Jan 1 */
#else
#define JD0 2440587.5  /* Time at 0:00 GMT 1970 Jan 1 */
#endif

#define SECONDS_PER_DAY 86400

#define ISLEAP(y) ((!((y) % 4) && ((y) % 100)) || !((y) % 400))
#define NUM_DAYS(year) (365+ISLEAP(year))

extern char* jd_string(double jd);
extern char* short_jd_string(double jd);
extern double ut_to_jd(long year, long month, long day, double hour);
extern double time_t_to_jd(time_t unix_time);
extern time_t jd_to_time_t(double jd);
extern void st_time_convert(TIME *st);

inline TIME TIME::operator -(const TIME &b) const {
  TIME r;
  r.tz=tz-b.tz;
  r.jd=jd-b.jd;
  r.unix_time=jd_to_time_t(r.jd);
  if (r.tz != 0) {
    r.jd-=(r.tz/24);
    r.unix_time-=((long)(r.tz)*3600);
    r.tz=0;
  }
  return r;
}

#endif
