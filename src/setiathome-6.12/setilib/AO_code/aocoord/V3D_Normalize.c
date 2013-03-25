/*      %W      %G      */
#include    <azzaToRaDec.h>
#include	<math.h>

/*******************************************************************************
* V3D_Normalize - normalize a 3 vector to unit magnitude.
*
* DESCRIPTION
*
* Normalize a 3 vector to unit magnitude. If the vector length is 0, set
* the  vector to 0 magnitude.
* In placed operation is ok.
*
* RETURN
* The normalized vector is returned via the pointer vresult.
*/
void    V3D_Normalize
        (
         double*        v1,     /* vector to normalize */
         double*        vr      /* return result */
        )
{
        double d;
        d=sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
        if (d <=0.) {
          *vr++=0.;
          *vr++=0.;
          *vr++=0.;
        }
        else {
          d=1./d;               /* quicker to multiply*/
          vr[0]= v1[0]*d;
          vr[1]= v1[1]*d;
          vr[2]= v1[2]*d;
        }
        return;
}
