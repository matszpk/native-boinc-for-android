/*      %W      %G      */
#include	<math.h>
#include <azzaToRaDec.h>
#include	<mathCon.h>

/******************************************************************************
* precJ2ToDate_M - matrix to precess from J2000 to current coordinates
*
* DESCRIPTION
*
* Return matrix that can be used to go from J2000 equatorial rectangular
* coordinates to  mean equatorial coordinates of  date (ra/dec). For
* equatorial rectangular coordinates, x points to equinox, z points north,
* and y points 90 to x increasing to the east.
*
* The input consists of the modified julian date as an integer and the
* fraction of a day since 0UT.
* The matrix is stored in col major order (increment over colums most rapidly).
*
* RETURNS
* The resulting  matrix is returned via the pointer pm.
*
* REFERENCE
* Astronomical Almanac 1992, page B18.
*/
void    precJ2ToDate_M
        (
         int            mjd,            /* modified julian date */
         double         ut1Frac,         /* frac of day past UT 0 */
         double*        pm              /* pointer to 3 by 3 double matrix*/
        )
{
        double  xiRd,zeRd,thRd;         /* mean equinox of date angles*/
        double  T;                      /* julian centuries past j2000*/
        double  cosXi,sinXi,cosZe,sinZe,cosTh,sinTh;;
 
        T= JUL_CEN_AFTER_J2000(mjd,ut1Frac);
        xiRd = T*(.6406161 + T*( .0000839 + (T*.0000050)))*C_DEG_TO_RAD;
        zeRd = T*(.6406161 + T*( .0003041 + (T*.0000051)))*C_DEG_TO_RAD;
        thRd = T*(.5567530 - T*( .0001185 + (T*.0000116)))*C_DEG_TO_RAD;
 
        cosXi=cos(xiRd);
        sinXi=sin(xiRd);
        cosZe=cos(zeRd);
        sinZe=sin(zeRd);
        cosTh=cos(thRd);
        sinTh=sin(thRd);
 
        /* row 0        */
 
        pm[0]= cosXi*cosTh*cosZe - sinXi*sinZe;
        pm[1]=-sinXi*cosTh*cosZe - cosXi*sinZe;
        pm[2]=      -sinTh*cosZe;
 
        /* row 1        */
 
        pm[3]= cosXi*cosTh*sinZe + sinXi*cosZe;
        pm[4]=-sinXi*cosTh*sinZe + cosXi*cosZe;
        pm[5]=      -sinTh*sinZe;
	
        /* row 2        */
 
        pm[6]= cosXi*sinTh;
        pm[7]=-sinXi*sinTh;
        pm[8]=       cosTh;
        
        return;
}

