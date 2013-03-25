/*      %W      %G      */
#include <azzaToRaDec.h>

/*******************************************************************************
* VV3D_Sub - subtract two vectors
*
* DESCRIPTION
*          
* Subtract vector V2 from V1. Return result in vresult.
*
* RETURN
* The result is returned via the pointer vresult.
*/
void    VV3D_Sub
        (
         double*        v1,     /* 1st vector */
         double*        v2,     /* 2nd vector */
         double*        vr      /* return result */
        )
{
        *vr++ = (*v1++) - (*v2++);
        *vr++ = (*v1++) - (*v2++);
        *vr++ = (*v1++) - (*v2++);
        return;
}
