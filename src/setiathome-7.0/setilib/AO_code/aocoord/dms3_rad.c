/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<mathCon.h>
#include	<azzaToRaDec.h>
/*******************************************************************************
* dms3_rad - convert from degrees, minutes, seconds to radians.
*
* DESCRIPTION
*
* Convert from degrees, minutes, seconds to radians. Degrees, minutes, are 
* integer while seconds is a double. Degrees should also be input as a
* positive number with sgn set to the correct sign of the value
* (+/- 1) (set setSgn routine)..
*
* RETURNS
* The value in radians.
*
* SEE ALSO
* rad_dms3, setSgn, convLib.h
*/

double dms3_rad
	(
	 int    sgn,	/* +/- 1 depending on angle >=,< 0 degrees*/
	 int	d,	/* degrees*/
	 int	m,	/* minutes*/
	 double  s	/* seconds*/
	)
{
/*
 *      	convert from deg min sec to radians.
 *            d,m,s should all be positive
*/
        return((double)sgn * C_2PI*(((double)d)/360. +
                     ((double)m)/21600. +   s/1296000.));
}
