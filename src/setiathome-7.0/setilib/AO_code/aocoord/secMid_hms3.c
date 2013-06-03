/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include        <azzaToRaDec.h>
/*******************************************************************************
* secMid_hms3 - convert secs from midnite to hours,mins,secs (double).
*
*DESCRIPTION
*
*Convert seconds from midnite to hours, minutes, seconds. The value is returned
*modulo 1 day (hours,minutes,seconds will always be values within 1 day).
*The hours and minutes from midnite are returned in hh, mm, and ss. This 
*routine uses double input, h,m int, and sec double.
*
*RETURNS
*The funtion has no return value.
*
*SEE ALSO
*convLib.h
*/

void secMid_hms3
	(
	  double secs,		/* seconds from midnite input*/
	  int *hh,		/* return hours here */
	  int *mm,		/* return minutes here*/
	  double  *ss		/* return seconds here*/
	)
{
	int	isecs;
	double frac;
/*
 *      convert secs from midnite to hh mm ss
*/
   isecs=secs;
   frac=secs-isecs;
   isecs=isecs%86400;
   *hh=isecs/3600;
   *mm= (isecs -((*hh) * 3600))/60;
   *ss=(isecs%60) + frac;
   return;
}
