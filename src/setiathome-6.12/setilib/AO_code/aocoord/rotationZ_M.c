/*      %W      %G      */
#include    <azzaToRaDec.h>
#include        <math.h>

/*******************************************************************************
* rotationZ_M - return rotation matrix about the z axis.
*
* DESCRIPTION
*
* Return a 3 dimensional cartesian rotation matrix about the z axis.
* The rotation angle thetaRd is in radians. When the  variable rightHanded is
* true, theta increases counter clockwise when looking toward zero.
* Righthanded false will give the opposite sense of rotation (for left
* handed systems). This is a rotation of coordinate systems. To rotate a
*
*
* The matrix is stored in col major order (increment over colums most rapidly).
*
* RETURNS
* The resulting  matrix is returned via the pointer pm.
*/
void    rotationZ_M
        (
          double        thetaRd,     /* rotatation angle in radians */
          int           rightHanded, /* true (!0) gives right handed system */
          double*       pm           /* return matrix via this pointer */
        )
{
        double  costh,sinth;
 
        costh=cos(thetaRd);
        sinth=sin(thetaRd);
        sinth=(rightHanded)?sinth:-sinth;
 
        pm[0]=costh;
        pm[1]=sinth;
        pm[2]=0.;
 
        pm[3]=-sinth;
        pm[4]= costh;
        pm[5]=0.;
 
        pm[6]=0.;
        pm[7]=0.;
        pm[8]=1.;
        return;
}
