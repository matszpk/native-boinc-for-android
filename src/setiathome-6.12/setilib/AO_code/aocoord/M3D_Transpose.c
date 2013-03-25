/*      %W      %G      */
#include <azzaToRaDec.h>

/*******************************************************************************
* M3D_Transpose - transpose a 3x3 double matrix
*
* DESCRIPTION
*
* The routine transposes a 3 by 3 matrix of doubles. This swaps the rows and
* the columns.
*
* The routine is written so that in place transposistion  is legal.
*
*
* RETURN
* Transposed matrix is returned via the pointer mtr.
*/
void    M3D_Transpose
        (
         double*        m,              /* matrix to transpose*/
         double*        mtr             /* transposed matrix*/
        )
{
        double  t;
#define   ROW0   0
#define   ROW1   3
#define   ROW2   6
        /*
        *   diaganols
        */
        mtr[ROW0+0]=m[ROW0+0];
        mtr[ROW1+1]=m[ROW1+1];
        mtr[ROW2+2]=m[ROW2+2];
        /*
        *       0,1   and 1,0
        */
                  t=   m[ROW0+1];
        mtr[ROW0+1]=   m[ROW1+0];
        mtr[ROW1+0]=   t;
        /*
        *       0,2   and 2,0
        */
                  t=   m[ROW0+2];
        mtr[ROW0+2]=   m[ROW2+0];
        mtr[ROW2+0]=   t;
        /*
        *       1,2   and 2,1
        */
                  t=   m[ROW1+2];
        mtr[ROW1+2]=   m[ROW2+1];
        mtr[ROW2+1]=   t;
        return;
}
