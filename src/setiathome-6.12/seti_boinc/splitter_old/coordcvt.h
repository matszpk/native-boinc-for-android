/*
 * coordcvt.h
 *
 * Celestial coordinate conversion routines.
 *
 * $Id: coordcvt.h,v 1.1.2.1 2006/01/13 00:24:49 jeffc Exp $
 *
 */

#ifndef COORDCVT_H
#define COORDCVT_H

double jd_to_lmst(double jd, double longitude);

void horz_to_equatorial(double alt, double azimuth, double lsthour,
                        double lat, double *ra, double *dec);

void telstr_coord_convert(SCOPE_STRING *telstr, double lat, double lon);

#endif

/*
 * $Log: coordcvt.h,v $
 * Revision 1.1.2.1  2006/01/13 00:24:49  jeffc
 * *** empty log message ***
 *
 * Revision 1.1  2003/06/03 00:16:09  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.1  1998/11/02 16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/19  23:03:18  korpela
 * Initial revision
 *
 */
