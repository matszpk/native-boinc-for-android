/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
#include	<mathCon.h>
/*******************************************************************************
* rad_hms3 - convert from radians to hours, minutes, secs (double).
*
* DESCRIPTION
*
* Convert from radians to hours, minutes, secs(double). Hours, minutes are
* integer while seconds is a double.
* The angle is returned as a non-negative angle with sgn set to +/-1
* for the correct sign.
*
* RETURNS
* The function returns nothing.
*
* SEE ALSO
* dms3_rad, setSgn, convLib.h
*/

void    rad_hms3
	(
	 double rad,    /* input radians*/
         int *sgn,      /* return sign +/- 1*/
         int *hh,       /* return hours*/
         int *mm,       /* return minutes*/
         double *ss     /* return seconds*/
	)
{
/*
 *      convert  radians to ihours imin Dsecs ...
 *      always return hms positive. return sign in sgn +/- 1
 *      (gets around  -1 to 0 problem
*/
        double     xh,xm;

        xh=rad*C_RAD_TO_HOURS;
        *sgn= xh < 0. ? -1:1;
        xh *= (*sgn);
        *hh = truncDtoI(xh);
        xm = (xh - (*hh))*60.;
        *mm = truncDtoI(xm);
        *ss= (xm - (*mm))*60.;
        return;
}
