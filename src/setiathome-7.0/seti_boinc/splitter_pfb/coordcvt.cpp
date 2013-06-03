/*
 * coordcvt.c
 *
 * Celestial coordinate conversion routines.
 *
 * $Id: coordcvt.cpp,v 1.2.4.1 2006/12/14 22:24:37 korpela Exp $
 *
 */

#include "sah_config.h"
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"

#include <seti_tel.h>
#include <seti_cfg.h>
#include <seti_coord.h>

#include "splitparms.h"
#include "splittypes.h"
#include "coordcvt.h"

extern int gregorian;

namespace SPLITTER {

double jd_to_lmst(double jd, double longitude) {
/*
 * converts from julian date/time to local mean siderial time
 *
 */

  double tu;    /* time, in centuries from Jan 0, 2000      */
  double tusqr;
  double tucb;
  double g;     /* derrived from tu, used to calculate gmst */
  double gmst;  /* Greenwich Mean Sidereal Time             */
  double lmst;  /* Local Mean Sidereal Time                 */
  double jd0=floor(jd+0.5)-0.5;             /* jd at noon GMT */
  double ddtime=fmod((jd-jd0)*24,24);       /* GMT hours      */

  tu     = (jd0 - 2451545.0)/36525;
  tusqr  = tu * tu;
  tucb   = tusqr * tu;

/* computation of the gmst at ddtime UT */

  g   = 24110.54841 + 8640184.812866 * tu + 0.093104 * tusqr - 6.62E-6 * tucb;

  for (gmst = g; gmst < 0; gmst += 86400) ;

  gmst /= 3600;    /* GMST at 0h UT */
  gmst += (1.0027379093 * ddtime);  /* GMST at utime UT */
  
  gmst=fmod(gmst,24);

/* computation of lmst given gmst and longitude */

  lmst = gmst + ((longitude/360) * 24);
 
  if (lmst < 0)
      lmst += 24;
  lmst = fmod(lmst,24);
  return(lmst);    
}


void horz_to_equatorial(double alt, double azimuth, double lsthour,
                        double lat, double *ra, double *dec) {
   double dec_rad, hour_ang_rad, zenith_ang=90.0-alt;
   double temp;

   zenith_ang*=(M_PI/180.0);
   azimuth*=(M_PI/180.0);

   dec_rad = asin ((cos(zenith_ang) * sin(lat*M_PI/180)) +
		    (sin(zenith_ang) * cos(lat*M_PI/180) * cos(azimuth)));

  *dec = dec_rad * 180.0/M_PI;   /* radians to decimal degrees */

  temp = (cos(zenith_ang) - sin(lat*M_PI/180) * sin(dec_rad)) /
                       (cos(lat*M_PI/180) * cos(dec_rad));
  if (temp > 1)
    temp = 1;
  else if (temp < -1)
    temp = -1;
  hour_ang_rad = acos (temp);

  if (sin(azimuth) > 0.0)         /* insure correct quadrant */
    hour_ang_rad = (2 * M_PI) - hour_ang_rad;
 
 
                 /* to get ra, we convert hour angle to decimal degrees, */
                 /* convert degrees to decimal hours, and subtract the   */
                 /* result from local sidereal decimal hours.            */
  *ra = lsthour - ((hour_ang_rad * 180.0/M_PI) / 15.0);

  // Take care of wrap situations in RA
  if (*ra < 0.0)
    *ra += 24.0;
  *ra=fmod(*ra,24);
}                     
#define D2R  0.017453292

void AltAzCorrection(double *alt, double *az) {
  double zen=90-*alt;
  co_ZenAzCorrection((telescope_id)(gregorian?AOGREG_1420:AO_1420),&zen,az);
  *alt=90-zen;
}

}

void telstr_coord_convert(SCOPE_STRING *telstr, double lat, double lon) {
 
  double lmst=SPLITTER::jd_to_lmst(telstr->st.jd,lon);

  SPLITTER::AltAzCorrection(&(telstr->alt),&(telstr->az));
  SPLITTER::horz_to_equatorial(telstr->alt, telstr->az, lmst, lat, &(telstr->ra),
		     &(telstr->dec));
}

/*
 * $Log: coordcvt.cpp,v $
 * Revision 1.2.4.1  2006/12/14 22:24:37  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/11 18:53:37  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/29 20:35:33  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.3  2003/06/05 19:01:45  korpela
 *
 * Fixed coordiate bug that allowed RA>24 hours.
 *
 * Revision 1.2  2003/06/05 15:52:46  korpela
 *
 * Fixed coordinate bug that was using the wrong zenith angle from the telescope
 * strings when handling units from the gregorian.
 *
 * Revision 1.1  2003/06/03 00:16:09  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.1  2003/05/20 16:01:54  eheien
 * *** empty log message ***
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.3  1999/03/05 01:47:18  korpela
 * New zenith angle correction.
 *
 * Revision 2.2  1999/02/22  22:21:09  korpela
 * Fixed half-day error.
 *
 * Revision 2.1  1998/11/02 16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/27  00:48:48  korpela
 * Bug fixes.
 *
 * Revision 1.1  1998/10/19  23:02:43  korpela
 * Initial revision
 *
 *
 */

