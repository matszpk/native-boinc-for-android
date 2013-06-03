/*      %W      %G      */
#include	<math.h>
#include    <azzaToRaDec.h>
#include	<mathCon.h>

/*******************************************************************************
* haToAzEl_V - hour angle dec to azimuth elevation (3 vector)
*
* DESCRIPTION
*
* Transform from  from an hour angle dec system to an azimuth elevation system.
* These are the source azimuth and elevation.
* The  inputs are double precision 3 vectors.
*
* The returned 3 vector has z pointing at zenith, y pointing east (az=90),
* and x pointing to north horizon (az=0). It is a left handed system.
*
* RETURNS
* The resulting azEl 3 vector is returned via the pointer pazEl. The function
* returns void.
*/
void    haToAzEl_V
        (
         double*        phaDec,  /* hour angle dec  3 vector */
         double         latRd,   /* latitude in radians*/
         double*        pazEl    /* returned azEl 3vector */
        )
{
        double  costh,sinth,th;
		double  temp;				/* for in place computation*/
 
        th=latRd-C_PI_D_2;
        costh=cos(th);
        sinth=sin(th);
		temp=phaDec[0];
        pazEl[0]=  -(costh*phaDec[0])             -(sinth * phaDec[2]);
        pazEl[1]=                      -phaDec[1]                 ;
        pazEl[2]=  -(sinth*temp)                  +(costh * phaDec[2]);
        return;
}
