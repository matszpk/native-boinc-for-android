/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
/*******************************************************************************
* dmToDayNo -  convert from day,month to day number of year.
*
* Pass in the day, month, and gregorian year. Return the day number of the year.
* Jan 1 is dayno 1. The year (which should be 4 digits)
* is needed to determine if the current year is a leap year or not.
*
* RETURNS
* The day number as an integer.
*
* WARNING
* Leap year determination uses the gregorian convention (as opposed to the 
* julian). It will not work for years prior to 1582.
* The month is forced to be between 1 and 12.
*
* SEE ALSO
* dayNoToDm, convLib. 
*
*/
int    dmToDayNo 
        (
        int     day,	        /* day of month */
        int     mon,            /* month of year 1-12 */
        int     year            /* gregorian year (> 1582) */
        )
{
/*
 *      static array holds  days up until start of this month. Double 0 at
 *      start since months start at 1 and c array starts at 0.
 *      1st set is for non leapyeard, second set is for leap years.
*/
        static int dayNoDat[26]=
                {0,0,31,59,90,120,151,181,212,243,273,304,334,
                 0,0,31,60,91,121,152,182,213,244,274,305,335};
	int index;

	if (mon<1)mon=1;
	if (mon>12)mon=12;
	index=(isLeapYear(year))?13:0;	
	return(dayNoDat[index+mon] + day);
}
