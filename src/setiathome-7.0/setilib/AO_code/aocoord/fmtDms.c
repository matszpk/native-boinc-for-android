/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<stdio.h>
#include	<math.h>
#include    <azzaToRaDec.h>
/*******************************************************************************
* fmtDms - format degree minutes seconds(double) for output.
*
* DESCRIPTION
* Write degrees, minutes, seconds as a formatted string into a character array.
* Degrees are blank filled to the left while minutes and seconds are zero
* filled to the left. The fields are separated by ":" and
* the user specifies the number of digits in the seconds fraction.
* The string is returned as:
*
*   [-/ ]ddd:mm:ss.sss
*
* negative numbers have a leading "-" while non-negative numbers have a leading
* space.
*
* cformatted string must point to a buffer that has already been allocated and
* can hold all the requested characters plus a NULL terminator.
*
* The input deg, min, secs should correspond to a non-negative angle with
* sgn set to +/- 1 to specify whether the true angle is positive or negative.
*
* RETURN
* The formated string is returned via the pointer cformatted.
*
* SEE ALSO
* fmtRdToDms, setSgn, convLib
*/
void    fmtDms
        (
         int    sgn,            /* of the input value*/
         int    deg,		/* degrees */
         int    min,		/* minutes*/
         double sec,		/* seconds*/
         int    secFracDig,     /* number of digits on secs fraction*/
         char   *cformatted     /* return formatted string here*/
        )
{
        char c;
        double mval;
 
	/* 
	 * compute a rounding value. 1/2 of last digit to keep
	*/
        if (secFracDig > 0){
          mval=pow(10.,(double)(-secFracDig))*.5;
        }
        else {
           mval=.5;
        }
        if ((mval+sec)>=60.){
            sec=sec-60. + mval;
            min+=1;
            if (min >=60){
               min-=60;
               deg+=1;          /* don't correct degrees for 360*/
            }
        }
        
        c=(sgn<0)?'-':' ';
        sprintf(cformatted,"%c%3d:%02d:%0*.*f",c,deg,min,
                ((secFracDig>0)?3+secFracDig:2),secFracDig,sec);
	return;
}
