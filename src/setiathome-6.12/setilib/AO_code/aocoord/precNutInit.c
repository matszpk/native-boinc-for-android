/*      %W      %G      */
#include        <azzaToRaDec.h>
#include		<string.h>
/******************************************************************************
* precNutInit - initialize PRECNUT_INFO structure
*
* DESCRIPTION
*
*Initialize the PRECNUT_INFO structure. This routine sets the 
*step size for how often to update the precession, nutation matrix.
*This structure is used by precNut(). You must call this routine
*once before calling precNut.
*
*RETURNS
* The resulting  precessed/nutated vector in  poutVec.
*
*REFERENCE
* Astronomical Almanac 1992, page B18.
*/
void  precNutInit
        (
		 double		    deltaMjd,	/* how often to recompute*/
		 PRECNUT_INFO   *pI 			/* prec/nutation info*/
        )
{
		memset((char*)pI,0,sizeof(PRECNUT_INFO));
		pI->deltaMjd=deltaMjd;
		pI->mjd=-1000000;		/* force update first time*/
		return;
}
