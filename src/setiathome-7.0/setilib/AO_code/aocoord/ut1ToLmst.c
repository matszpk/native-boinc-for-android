/*      %W      %G      */
#include        <math.h>
#include <azzaToRaDec.h>
#include	<mathCon.h>

/*******************************************************************************
* ut1ToLmst - modified julian date and UT1 fraction to local mean sidereal time
*
*DESCRIPTION
*
* Convert from modified julian day number and fract of UT1 day  to
* local mean sidereal time.
*
* The modified julian date is the julian date - 2400000.5 and is passed in
* as an integer. The time of day is passed in as a fraction of a UT1 day.
* The west longitude of the site in radians is used to go from  GMST to
* LMST.
*
*RETURN
*  The local mean sidereal time in radians as a double is passed packed
* as the function value. It is between [0,2pi).
*
*REFERENCE
*   Taken from page B6 of the aston. almanac 1992.
* reference values for the routine are: jan 1 1992:8621 0. = 6:35:40.9309gmst
*
*WARNINGS
*
* The day fraction must be in UT1 time. Don't pass in the atomic fraction
* of a day (eeco time).
* The sidereal to solar day ratio  is taken from the value for 1992.
*
* SEE ALSO
* lmstToLast
*/
double  ut1ToLmst
        (
        int     mjd,            /* modified julian date integer */
        double  ut1Frac,        /* fract of day from 0 UT */
        double  westLong        /* west longituded of site in radians */
        )
{
        double  Tu;             /* time from date 0 UT to  j2000 in julian
                                   centuries*/
        double  gmstAt0Ut;      /* gmst at 0 hours UT for time of interest*/
        double  dfract;         /* gmst as fract of a day .. [0...1) */
        double  dintp;          /* integer portion ignored*/
        
/*      sidereal/solar day take from  1992 jan 1. Should actually take the
 *      value for each year, but it has changed by less than 1 part in 10**11
 *      in the last 20 years
*/
        /*
        * fraction of julian centuries till j2000
		* TU is measured from 2000 jan 1D12H UT
        */
        Tu=JUL_CEN_AFTER_J2000(mjd,0.); /* do the fraction separately*/
        /*
        * gmst at 0 ut of date in sidereal seconds
        */
        gmstAt0Ut=24110.54841 + Tu*(8640184.812866 +
                                Tu*(.093104        -
                                Tu*(6.2e-6)));
	/*
        *  convert to fraction of a day, add on user fract and throw away
        *  integer part
        */
        dfract=modf((gmstAt0Ut/86400. + ut1Frac*SOLAR_TO_SIDEREAL_DAY -
                     westLong/C_2PI),&dintp);
        /*
        *  make sure 0 to 1.
        */
        if (dfract < 0.) dfract+=1.;
        /*
        *  return as 0 to 2pi
        */
        return(dfract*C_2PI);           /* return 0 to 2 pi*/
}
