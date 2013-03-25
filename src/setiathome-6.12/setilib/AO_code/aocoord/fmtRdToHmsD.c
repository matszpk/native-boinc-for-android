/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
/*******************************************************************************
* fmtRdToHmsD - format radians to hours minutes seconds for output.
*
* DESCRIPTION
* Convert radians to hours, minutes, secs and then format the variables for
* output in a character array. Place leading zeros in each field, separate
* fields by ":", and let user specify the number of digits in the seconds
* fraction.  The string is  returned as:
*
*   [-/ ]hh:mm:ss.sss
*
* Negatative numbers have a leading "-" while non-negative numbers have a
* leading space.
*
* cformatted string must point to a buffer that has already been allocated and
* can hold at all the requested characters plus a NULL terminator.
*
* RETURN
* The formated string is returned via the pointer cformatted.
*
* SEE ALSO
* rad_hms3, fmtHmsD, convLib.
*
*/
void    fmtRdToHmsD
        (
         double hmsRd,          /* radian value to format*/
         int    secFracDig,     /* number of digits on secs fraction*/
         char   *cformatted     /* return formatted string here*/
        )
{
        int     sgn,h,m;
        double  s;
 
        rad_hms3(hmsRd,&sgn,&h,&m,&s);
        fmtHmsD(sgn,h,m,s,secFracDig,cformatted);
        return;
}
