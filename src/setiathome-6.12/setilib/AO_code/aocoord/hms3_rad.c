/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<mathCon.h>
#include    <azzaToRaDec.h>
/******************************************************************************
* hms3_rad - convert from hours, minutes, seconds(double) to radians.
*
* DESCRIPTION
*
* Convert from hours, minutes, seconds(double) to radians. Hours and minutes
* are integer while seconds is a double. The input should be for a 
* non-negative angle with sgn set to +/- 1 to correct for the sign. 
*
* RETURNS
* The value in radians.
*
* SEE ALSO
* rad_hms3, setSgn, convLib.h
*/

double hms3_rad(
	int     sgn,	/* sign for input angle*/
	int	ih,	/* hour*/
	int	im,	/* minute*/
	double  s	/* seconds*/
	)
{
/*      	convert from hour min sec to radians.
*/
        return(C_2PI*sgn*(((double)ih)/24. +
                     ((double)im)/1440. +   s/86400.));
}
