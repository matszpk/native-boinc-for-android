/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include <azzaToRaDec.h>

/*******************************************************************************
* isLeapYear  -  check if a year is a leap year.
*
* Determine whether a year is a leap year in the gregorian calendar.
* Leap years are those years divisible by 4 except those years not ending
* in 00 that are not divisisble by 400 (1900 is not a leap year, 2000 is).
*
* RETURNS
* True if the year is a leap year otherwise FALSE (0).
* 
* SEE ALSO 
* convLib
*/

int     isLeapYear
        (
        int     year            /* years since 0 AD */
        )
{
        if ((year%4)) return(FALSE);    /* not leap since not divisible by 4*/
        if ((year%100)) return(TRUE);   /* leap since not start of century*/
        return(!(year%400));            /* century/400 is a leap, else not*/
}
