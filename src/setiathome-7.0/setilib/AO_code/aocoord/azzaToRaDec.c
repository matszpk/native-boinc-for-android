/*******************************************************************************
* azzaToRaDec - convert az,za to ra dec J2000
*
* DESCRIPTION
*
* Given az,za, mjd and utcFraction of day, compute the corresponding
* ra,dec J2000 coordinates.  Prior to calling this routine you must
* call azzaToRaDecInit once to initialize the AZZA_TO_RADEC_INFO structure.
* This struct holds information on the observatory position, going
* utc to ut1, pointing  model information, and how often to update the 
* precession, nutation matrix.
*
* RETURN
* The J2000 ra,dec corresponding to the requested az,za, and time. The
* data is returned as radians.
*
*NOTE:
* The azFd is the azimuth of the feed. This is 180 degrees from the azimuth
*of the source. For the gregorian dome this is the same as the encoder
*azimuth. For the carriage house, the azFd is encoderAzimuth-180.
*
*  The model correction is not yet implemented.
*/ 

/*	includes	*/
#include	<stdio.h>
#include	<string.h>
#include	<azzaToRaDec.h>
#include	<mathCon.h>
#include	<math.h>

void  azzaToRaDec
      (
	double	        azFd,			/* feed azimuth*/
	double		    za,         /* encoder za*/
    int             mjd,        /* modified julian date*/
	double			utcFrac,	/* fraction of utc day*/
	int				ofDate, /* if not zero return coord of date*/
	AZZA_TO_RADEC_INFO *pinfo,
    double         *praJ2_rd,  /* ra  j2000 or ofDate, radians*/
    double         *pdecJ2_rd  /* dec j2000 or ofDate, radians*/
	) 
{
	double  azEl_V[3];
    double  ut1Frac;                 /* ut1 fraction of a day*/
	double	raDApp_V[3],haAppD_V[3];	/* 3 vectors*/
	double  raDTrue_V[3],raDJ2000_V[3];
	double	lst;			/* localmean sidereal time*/
	double	last;			/* local apparent sidereal time*/
	double	aber_V[3];					/* aberation vector*/
	/*
	 * change azFd,za to az/el 3 vectors
	*/
	anglesToVec3((azFd+180.)*C_DEG_TO_RAD,(90.-za)*C_DEG_TO_RAD,azEl_V);
	azElToHa_V(azEl_V,pinfo->obsPosI.latRd,haAppD_V);
	ut1Frac=utcFrac + utcToUt1(mjd,utcFrac,&pinfo->utcI);
	if (ut1Frac >= 1.) {
       	ut1Frac-=1.;
       	mjd++;
    }
    else if (ut1Frac < 0.) {
           ut1Frac+=1.;
           mjd--;
    }
    /*	
	 * go from mjdUt to local  mean sidereal time 
	*/
    lst=ut1ToLmst(mjd,ut1Frac,pinfo->obsPosI.wLongRd);
	last=lst + pinfo->precNutI.eqEquinox;	/* local apparent sidereal time*/
	/*
	 * go ha/dec to  ra/dec apparent
	*/
	haToRaDec_V(haAppD_V,last,raDApp_V);
	/*
 	 * compute the annual aberation subtract from curr ra/dec
     * and then renormalize vector
	*/
    aberAnnual_V(mjd,ut1Frac,aber_V);/* compute the aberration offset*/
	VV3D_Sub(raDApp_V,aber_V,raDTrue_V);
	V3D_Normalize(raDTrue_V,raDTrue_V);/* re normalize it */
	/*
 	 *if not coord OfDate request,  apply inverse nutation,prec to go to j2000
	*/
	if (!ofDate) {
		precNut(mjd,ut1Frac,FALSE,&pinfo->precNutI,raDTrue_V,raDJ2000_V);
	} else {
		memcpy(raDJ2000_V,raDTrue_V,sizeof(raDTrue_V));	/* return of date*/
	}
	/*
 	* convert vectors back to angles
	*/
	vec3ToAngles(raDJ2000_V,TRUE,praJ2_rd,pdecJ2_rd);	/* go back to angles*/

	return;
}
