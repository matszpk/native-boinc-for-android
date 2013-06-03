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
* fmtHmsD - format hours minutes seconds(double) for output.
*
* DESCRIPTION
*
* Write hours, minutes, seconds as a formatted string into a character array.
* Place leading zeros in each field, separate fields by ":", and let user
* specify the number of digits in the seconds fraction.  The string is
* returned as
*
*   [-/ ]hh:mm:ss.sss
*
* Negative numbers have a leading - while non negative numbers have a space.
*
* cformatted string must point to a buffer that has already been allocated and
* can hold at all the requested characters plus a NULL terminator.
* 
* The input hours, min, sec should be for a non-negative angle with 
* sgn set to +/- 1 depending on whether the initial angle was >=0 or < 0. 
*
* RETURN
* The formated string is returned via the pointer cformatted.
*
* SEE ALSO
* fmtHmsI,fmtRdToHmsD, setSgn, convLib
*/
void    fmtHmsD
        (
         int    sgn,           /* +/-1 depending on angle >=0,< 0*/
         int    hour,		/* hour*/
         int    min,		/* minutes*/
         double sec,		/* seconds*/
         int    secFracDig,     /* number of digits on secs fraction*/
         char   *cformatted     /* return formatted string here*/
        )
{
        char c;
        double mval;

/*
 *      we need to round seconds to .5*least significant digit
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
               hour+=1;         /* don't correct hours for 24, since no day*/
            }
        }
        c=(sgn<0)?'-':' ';
        sprintf(cformatted,"%c%02d:%02d:%0*.*f",c,hour,min,
                ((secFracDig>0)?3+secFracDig:2),secFracDig,sec);
	return;
}
