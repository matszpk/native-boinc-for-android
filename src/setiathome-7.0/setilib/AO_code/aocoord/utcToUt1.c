/*      %W      %G      */
#include        <math.h>
#include <azzaToRaDec.h>
#include	<mathCon.h>

/*******************************************************************************
* utcToUt1 - return fraction of day needed to go from UTC to UT1.
*
*DESCRIPTION
*
* Return the offset from utc to ut1 as a fraction of a day. The returned
* value (dut1Frac) is defined as ut1Frac=utcFrac + dut1Frac;
* The fraction of a day can be  < 0. 
*   
* The utc to ut1 conversion info is passed in via the structure UTC_INFO.
*
*RETURN
*  The ut1 fraction of a day.
*/
double  utcToUt1 
        (
        int     mjd,            /* modified julian date */
        double  utcFrac,        /* fract of day from 0 UTC */
	UTC_INFO* putcI		/* holds utc to ut1 info */
        )
{
	return(1e-3*C_SEC_TO_FRAC * 
	      (putcI->offset + ((mjd - putcI->mjdAtOff)+utcFrac)*putcI->rate));
}
