/*      %W      %G      */
#include    <azzaToRaDec.h>
#include	<mathCon.h>
/******************************************************************************
* meanEqToEcl_A - angle of eclipitc from mean equatorial equator.
*
* DESCRIPTION
*
* Return the angle from the mean equatorial equator to the ecliptic in radians.
* This angle is used when computing the nutation matrix and when going from
* mean equatorial coordinates (no nutation applied yet) to ecliptic coordinates
* prior to nutation.
*
* The ecliptic angle is computed at the start of the modified julian day.
*
* RETURNS
* The angle is returned in radians as a double.
*
* REFERENCE
* Astronomical Almanac 1992, page B18,B20
* Astronomical Almanac 1984, page S23-S25 for nutation series coefficients.
*/
double  meanEqToEcl_A
        (
         int    mjd,             /* modified julian day */
         double ut1Frac           /* fraction of a day since ut 0hrs */
        )
{
        double  t;              /* fractions of century*/
        double  obliqRd;
 
        t= JUL_CEN_AFTER_J2000(mjd,ut1Frac);
        obliqRd=(23.439291 + t*(-.0130042 + t*(-.00000016 + .000000504*t))) *
                C_DEG_TO_RAD;
        return(obliqRd);
}
