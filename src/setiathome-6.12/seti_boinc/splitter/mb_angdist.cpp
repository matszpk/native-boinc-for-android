/*
 * angdist.c
 *
 *  Computes angular distance between two lat/lon points
 *
 * $Id: mb_angdist.cpp,v 1.1.2.1 2006/12/14 22:24:39 korpela Exp $
 *
 */

#include "sah_config.h"
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include "seti_header.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"
#include "xml_util.h"
#include "db/app_config.h"


#define DTOR (M_PI/180)

double angdist(double r1, double d1, double r2, double d2) {
  double alpha,dist,d;

  r1*=DTOR;
  r2*=DTOR;
  d1*=DTOR;
  d2*=DTOR;
  alpha=r1-r2;
  dist=sin(d2)*sin(d1)+cos(d1)*cos(d2)*cos(alpha);
  if ((dist*dist)<1) {
    d=sqrt(1.0-dist*dist);
  } else {
    dist=1.0;
    d=0.0;
  }
  dist=atan2(d,dist);
  dist=dist/DTOR;
  return (dist);
}

double angdist(const coordinate_t &a, const coordinate_t &b) {
  return angdist(a.ra*15,a.dec,b.ra*15,b.dec);
}


/*
 * $Log: mb_angdist.cpp,v $
 * Revision 1.1.2.1  2006/12/14 22:24:39  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:37  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:39  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:31  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.2  2003/06/03 01:01:15  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:16:08  korpela
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
 * Revision 1.1  1998/10/27  00:47:33  korpela
 * Initial revision
 *
 *
 */
