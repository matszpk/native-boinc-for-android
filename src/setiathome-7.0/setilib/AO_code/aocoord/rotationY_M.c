/*      %W      %G      */
#include    <azzaToRaDec.h>
#include	<math.h>
/*******************************************************************************
* rotationY_M - return rotation matrix about the y axis.
*
* DESCRIPTION
*
* Return a 3 dimensional cartesian rotation matrix about the y axis.
* The rotation angle thetaRd is in radians. When the  variable rightHanded is
* true, theta increases counter clockwise when looking toward zero.
* Righthanded false will give the opposite sense of rotation (for left
*  handed systems). This is a rotation of coordinate systems. To rotate a
*       
* The matrix is stored in col major order (increment over colums most rapidly).
*
* RETURNS
* The resulting  matrix is returned via the pointer pm.
*/
void    rotationY_M
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
        pm[1]=0.;
        pm[2]=-sinth;
 
        pm[3]=0.;
        pm[4]=1.;
        pm[5]=0.;
 
        pm[6]=sinth;
        pm[7]=0.;
        pm[8]=costh;
        return;
}
