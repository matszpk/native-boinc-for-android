/*      %W      %G      */
#include     <azzaToRaDec.h>

/*******************************************************************************
* mjdToJulDate - convert from mjd utFrac to julian date.
*
*DESCRIPTION
*
* Convert from modified julian day number and fract of UT1 day  to
* julian date.
*
*
*RETURN
*  The julian date as a double.
*
*/
double  mjdToJulDate
        (
        int     mjd,            /* modified julian date integer */
        double  ut1Frac          /* fract of day from 0 UT */
        )
{
        return(mjd + MJD_OFFSET + ut1Frac);
}
