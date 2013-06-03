/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include    <azzaToRaDec.h>
#include	<stdio.h>
/*******************************************************************************
* fmtSMToHmsD - format seconds from midnite to hms.sss for output.
*
* DESCRIPTION
* Convert seconds from midnite to hours,minutes, seconds (double and then
* write hours, minutes, seconds as a formatted string into a character array.
* Place leading zeros in each field, separate fields by ":". The user
* specifies the number of digits to the right of the seconds.
* The string is  returned as :
*
*   hh:mm:ss.nnn
*
* cformatted string must point to a buffer that has already been allocated and
* can hold at all the requested characters plus a NULL terminator.
*
* RETURN
* The formated string is returned via the pointer cformatted.
*
* SEE ALSO
* fmtHmsD,  convLib
*/
void    fmtSMToHmsD
	(
	 	double	secMidnite,	/* seconds from midnite*/
		int secFracDig, /* number of digits to right of the secs*/
        char   *cformatted     /* return formatted string here*/
    )
{
	int	hour,min;
	double sec;

	secMid_hms3(secMidnite,&hour,&min,&sec);
	fmtHmsD(1,hour,min,sec,secFracDig,cformatted);
    return;
}
