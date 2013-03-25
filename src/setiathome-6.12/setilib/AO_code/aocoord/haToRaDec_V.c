/*      %W      %G      */
#include		<math.h>
#include <azzaToRaDec.h>


/*******************************************************************************
* haToRaDec_V - hour angle/dec to right ascension (of date)/dec
*
* DESCRIPTION
*
* Transform from  from an hour angle dec system to a right ascension (of date)
* dec system. The  inputs are double precision 3 vectors. The lst is passed
* in in radians.
* If the lst is the mean sidereal time , then the ra/dec and hour angle should
* be the mean positions. If lst is the apparent sidereal time then the
* ra/dec, hour angle should be the apparent (or true with nutation) positions.
*
* The transformation is simlar to the raDecToHa except that we reflect
* first (left handed to right handed system) before we do the rotation in 
* the opposite direction.
*
* RETURNS
* The resulting  raDec 3 vector is returned via the pointer praDec. The function* returns void.
*/
void    haToRaDec_V
        (
         double*        phaDec, /* hour angle dec  3 vector*/
         double         lst,    /* local sidereal time in radians*/
         double*        praDec   /* returned ra dec (date) 3vector */
        )
{
        double  coslst,sinlst;
		double  temp;
 
        coslst=cos(-lst);
        sinlst=sin(-lst);
		temp=phaDec[0];			/* for in place operation*/
        praDec[0] =   (phaDec[0] * coslst) + (-phaDec[1] * sinlst);
        praDec[1] =   (-(temp    * sinlst) + (-phaDec[1] * coslst));
        praDec[2] =   (phaDec[2]);
        return;
}


