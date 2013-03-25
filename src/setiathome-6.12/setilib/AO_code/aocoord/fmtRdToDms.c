/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
/*******************************************************************************
* fmtRdToDms - format radians to degree minutes seconds(double) for output.
*
* DESCRIPTION
* Convert radians to  degrees, minutes, seconds and then output as formatted
* string into a character array.
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
* can hold at all the requested characters plus a NULL terminator.
*
* RETURN
* The formated string is returned via the pointer cformatted.
*
* SEE ALSO
* rad_dms3, fmtDms, convLib.
*/
void    fmtRdToDms
        (
         double  dmsRd,         /* radians */
         int    secFracDig,     /* number of digits on secs fraction*/
         char   *cformatted     /* return formatted string here*/
        )
{
        int     deg,min;
        double  sec;
        int     sgn;
        
        rad_dms3(dmsRd,&sgn,&deg,&min,&sec);
        fmtDms(sgn,deg,min,sec,secFracDig,cformatted);
        return;
}
