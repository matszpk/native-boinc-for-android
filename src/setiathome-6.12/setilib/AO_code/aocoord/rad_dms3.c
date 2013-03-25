/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
#include	<mathCon.h>
/*******************************************************************************
* rad_dms3 - convert from radians to degrees, minutes, secs (double).
*
* DESCRIPTION
*
* Convert from radians to degrees, minutes, secs(double). Deg, minutes are
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

void    rad_dms3
	(
	 double rad,	/* input radians*/
	 int *sgn,	/* return sign +/- 1*/
	 int *dd,	/* return degrees*/
	 int *mm,	/* return minutes*/
	 double *ss	/* return seconds*/
	)
{
/*
 *      convert  radians to ideg imin Dsecs ...
 *      always return dms positive. return sign in sgn +/- 1
 *      (gets around  -1 to 0 problem
*/
        double     xm,xd;

        xd=rad*C_RAD_TO_DEG;
        *sgn= xd < 0. ? -1:1;
        xd *= (*sgn);
        *dd = truncDtoI(xd);
        xm = (xd - (*dd))*60.;
        *mm = truncDtoI(xm);
        *ss= (xm - (*mm))*60.;
        return;
}
