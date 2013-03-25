/* azzaToRaDecInit - initialize for azaToRaDec processing*/

/*
SYNOPSIS
azzaToRaDecInit  - initialize for azzaToRaDec processing

DESCRIPTION
	Call this routine once to initialize the data structures for
converting from az,za to ra,Dec J2000 for arecibo data. The routine
will input the required data files (obs position and initialize the
precession/nutation structure.

	The daynumber of year, and year will be used find the correct
utc to ut1 conversion to use. If you want the most recent conversion
just put a large number for the year (eg 9999).

NOROUTINES
*/
/*	includes	*/

#include	<azzaToRaDec.h>

/*	defines		*/


int azzaToRaDecInit (
		int dayno, 			/* daynumber of year for utcInfo*/
		int year,           /* year for utcinfo*/
      AZZA_TO_RADEC_INFO *pinfo	/* return setup info here*/
	)
{
	int mjd;		
	double	nutM[9];
    /*
	 *  get observatory location			
	*/
	if (obsPosInp(" ",&pinfo->obsPosI) != OK) goto errout;
    /*
	 *  initialize preccession, nutation/
	*/
	precNutInit(1.,&pinfo->precNutI);	 /* update once a day*/
	/*
	 *  get utc to ut1 equation valid for dayno, year
	*/
	if (utcInfoInp(dayno,year,NULL,&pinfo->utcI) != OK) goto errout;
	/*
	 *  Call nutation matrix once. We need to get the equation of the
	 *  equinox. It is normally returned when you precess. In the
	 *  az,za to ra,dec we need to use it before we call the 
	 *  precession routine.
	*/
	mjd=gregToMjdDno(dayno,year);
	nutation_M(mjd,0.,nutM,&pinfo->precNutI.eqEquinox);


    return(OK);
errout: return(ERROR);
}
