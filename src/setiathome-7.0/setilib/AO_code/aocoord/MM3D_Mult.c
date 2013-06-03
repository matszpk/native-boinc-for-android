/*      %W      %G      */
#include <azzaToRaDec.h>

/*******************************************************************************
* MM3D_Mult - mulitply matrix by matrix (3x3 matrices, double precision)
*
* DESCRIPTION
*
* The routine does a 3 by 3 matrix multiply of m2 * m1 returning the result
* via the pointer pmresult. The matrix elements are double precision.
* If concatanating matrices, m2 and m1 to apply on vector v1,
* m1 would be applied first to v1 then m2.
*
* The routine is written so that in place mulitplication (m1=m1*m2) is legal
* (the result is buffered during the multiplication stage)
*
*      M2           M1              M2 X M1
*  (a0 a1 a2)   (b0 b1 b2)   (a0b0+a1b3+a2b6  ... ...)
*  (a3 a4 a5) X (b3 b4 b5)  =(...     ...         ...)
*  (a6 a7 a8)   (b6 b7 b8)   (... ...  a6b2+a7b5+a8b8)
*
* RETURN
* The product matrix is returned via the pointer mresult. The function
* has no return value.
*/
void    MM3D_Mult
        (
        double*         m2,     /* matrix 2*/
        double*         m1,     /* matrix 1*/
        double*         mresult /* result matrix returned here*/
        )
{
        double  mtmp[9];        /* to hold result*/
        double  *ptmp,*pm1,*pm2;
        double  tmp;
        int     irow,icol,m2off;
 
        ptmp=mtmp;
        for (irow=0;irow<3;irow++){
            m2off=irow*3;               /* offset m2. moves across a row*/
           for (icol=0;icol<3;icol++){
                /*
                * loop m2 across 1 row (inc by 1) m1 down a col (inc by 3)
                * i've unrolled the loop
                */
                pm1=m1 + icol;          /* start m1, moves down a col*/
                pm2=m2 + m2off;         /* start for m2 matrix move across row*/                tmp=(*pm2++) * (*pm1);
                pm1+=3;
                tmp+=(*pm2++) * (*pm1);
                pm1+=3;
                *ptmp++ = tmp + ((*pm2) * (*pm1));
           }
        }
        for (ptmp=mtmp;ptmp<mtmp+9;)*mresult++=*ptmp++;
        return;
}
