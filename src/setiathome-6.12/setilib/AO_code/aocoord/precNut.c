/*      %W      %G      */
#include <azzaToRaDec.h>

/******************************************************************************
* precNut - perform precession and nutation.
*
* DESCRIPTION
*
*Perform the J2000 to date or Date to J2000 precession and nutation on the
*input vecotr pinpVec (normalized 3 vector coordinates). The return
*value is in poutVec. The direction is determined by the jToCur variable 
*(true --> j2000 to date).
*
*The matrices are passed in via the PRECNUT_INFO struct. This structure 
*contains the precNutation matrix (and it's inverse) precomputed as well as
*the mjd and ut1Frac when the computation was performed. It also includes
*a deltaMjd, that tells the routine how often to automatically recompute 
*the matrices. If the requested time is outside the delta, then this 
*routine will automatically recompute the matrices and update the 
*computation times.
*
*The user should have called precNutInit once before calling this routine
*to initialize the delta values in the PRECNUT_INFO struct.
*
*If pinpVec or poutVec is NULL, the routine will check for recomputing
*the matrices without performing any operations on the vectors (so you
*can precompute the matrices outside of any fast loop). The operations
*can be done in place (pinpVec == poutVec).
*
*The user must allocate the space for pI,pinpVec[3], and poutVec[3]. 
*
*RETURNS
* The resulting  precessed/nutated vector in  poutVec.
*
*REFERENCE
* Astronomical Almanac 1992, page B18.
*History:
*18feb04 - bug in code to decide when we had to recompute the matrices.
*          With the bug is was always recomputing the matrix (which
*		   was ok, just that it was overkill).
*/
void  precNut
        (
         int            mjd,            /* modified julian date */
         double         ut1Frac,        /* fraction of day since UT 0 */
		 int			jToCur,         /* true--> J200 to date, else inverse*/
		 PRECNUT_INFO   *pI,			/* prec/nutation info*/
         double         *pinpVec,       /* input  vector 3 doubles*/
         double         *poutVec        /* output vector 3 doubles*/
        )
{
	    double	deltaMjd;		/* absolute value*/
		double  precM[9],nutM[9];
	    /*
		 * see if we have to update the  matrices
		 * deltaMjd=(mjd + ut1Frac)-pI->deltaMjd;  before 18feb04
		*/
		deltaMjd=(mjd + ut1Frac)-(pI->mjd+pI->ut1Frac);
		if (deltaMjd<0.)deltaMjd=-deltaMjd;
		if (deltaMjd >= pI->deltaMjd){
			pI->mjd		=mjd;		/* save when we recomputed*/
			pI->ut1Frac =ut1Frac;
			/* j2000 to date, prec then nutate */
        	precJ2ToDate_M(mjd,ut1Frac,precM);
        	nutation_M(mjd,ut1Frac,nutM,&pI->eqEquinox);
			MM3D_Mult(nutM,precM,pI->precN_M);	/* concatenate*/
			/*
			 * date to J2000 nutatate then precess. The inverses are the 
			 * tranposes, and Transposition can be done in place
			*/
        	M3D_Transpose(precM,precM);           /* get inverse*/
        	M3D_Transpose(nutM,nutM);           /* get inverse*/
			MM3D_Mult(precM,nutM,pI->precNI_M);	/* concatenate*/
		}
		if ((pinpVec != NULL)&&(poutVec != NULL)){
			if (jToCur){
			   MV3D_Mult(pI->precN_M,pinpVec,poutVec);
			}
			else {
			   MV3D_Mult(pI->precNI_M,pinpVec,poutVec);
			}
		}
        return;
}
