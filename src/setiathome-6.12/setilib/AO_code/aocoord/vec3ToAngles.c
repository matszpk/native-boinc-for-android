/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<math.h>
#include	<mathCon.h>
/*******************************************************************************
* vec3ToAngles - convert 3 vector to angles
*
* Convert the input 3 vector to angles (radians).
* If posC1 is TRUE, then the first angle will always be returned as a positive
* angle (for hour angle system you may want to let the ha be negative ).
*
* The coordinate systems are:
*   c1 - ra, ha, azimuth
*   c2 - dec,dec, altitude
*
* RETURNS
* The function has no return value.
* 
* NOTE 
* This routine uses atan2 and asin.
*/
void    vec3ToAngles
        (
          double*       pvec3,     /* 3 vector for input*/
          int           c1Pos,     /* true --> always return c1 > 0.*/
          double*       pc1,       /* return 1st coord in radians*/
          double*       pc2        /* return 2nd coord in radians*/
        )
{
        double  c1Rad;

        c1Rad=atan2(pvec3[1],pvec3[0]); /* atan y/x */
        /*
        * atan2 returns -pi to pi. If user requested, make sure we return
        * 0 to 2pi;
        */
        if (c1Pos && (c1Rad < 0.)){
           c1Rad = c1Rad+C_2PI;
        }
        *pc1=c1Rad;
        *pc2=asin(pvec3[2]);            /* asin (z)*/
        return;
}
