/*      %W      %G      */
#include    <azzaToRaDec.h>

/*******************************************************************************
* azElToHa_V - azimuth elevation to hour angle dec (3 vector)
*
* DESCRIPTION
*
* Transform from  from an azimuth elevation system to an hour angle dec system.
* The  inputs are double precision 3 vectors.
*
* RETURNS
* The resulting  haDec 3 vector is returned via the pointer phaDec. The function* returns void.
*/
void    azElToHa_V
        (
         double*        pazEl, /* hour angle dec  3 vector */
         double         latitude,/* in degrees */
         double*        phaDec   /* returned azEl 3vector */
        )
{
        haToAzEl_V(pazEl,latitude,phaDec);
        return;
}
