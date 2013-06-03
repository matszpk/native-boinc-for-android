/*	%W	%G	*/
#include <azzaToRaDec.h>

/*******************************************************************************
* MV3D_Mult - multiply matrix by vector ((3x3)*(3) double precision)
*
* DESCRIPTION
* 
* The routine does a 3 by 3 matrix multiply on a 3 vector (M*v) giving a
* 3 vector as the result. The  elements are double  precision.
*
* The routine is written so that in place mulitplication (v=m*v) is legal
* (the result is buffered during the multiplication stage).
*
*      M2         V                 M X V
*   (a0 a1 a2)   (b0 )   (a0b0+a1b1+a2b2)
*   (a3 a4 a5) X (b1 )  =(...           )
*   (a6 a7 a8)   (b2 )   (a6b0+a7b1+a8b2)
* 
* RETURN none
*/
void    MV3D_Mult
        (
        double*         m,      /* matrix */
        double*         v,      /* vector */
        double*         vresult /* result matrix returned here */
        )
{
        double  vtmp[3];        /* to hold result.(only use 1st two)*/
        double  *pvtmp,*pv,*pm;
        double  tmp;
 
            pvtmp=vtmp;
            /*
            *  x component
            */
            pv=v;
	    pm=m;
            tmp=(*pm++) * (*pv++);
            tmp+=(*pm++) * (*pv++);
            *pvtmp++ = (*pm++) * (*pv++) + tmp;
            /*
            *  y component
            */
            pv=v;
            tmp=(*pm++) * (*pv++);
            tmp+=(*pm++) * (*pv++);
            *pvtmp = (*pm++) * (*pv++)  + tmp;
            /*
            *  z component
            */
            pv=v;
            tmp=(*pm++) * (*pv++);
            tmp+=(*pm++) * (*pv++);
            vresult[2]=(*pm) * (*pv) + tmp;
            vresult[1]=vtmp[1];
            *vresult  =*vtmp;
        return;
}
