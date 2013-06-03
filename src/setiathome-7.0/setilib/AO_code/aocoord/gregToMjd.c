/*      %W      %G      */
#include <azzaToRaDec.h>

/*******************************************************************************
* gregToMjd - gregorian date to modified julian date
*
* Convert from a UT gregorian date to a modified julian date. This routine
* can be used from 1901 thur 2100.
*
* COMPUTATION
*
* The julian calendar has 3 years of 365 days followed by 1 year of 366 days.
* The gregorian calendar is the same as the julian calendar up to 1542.
* oct 4th 1542 is followed by oct 15 1542 in the gregorian calendar. After
* this date, there is 3 years of 365 days followed by 1 year of 366 days,
* except that years ending in 00 are not leap year unless they are divisible
* by 400 : 1700,1800,1900 are not leap years while 2000 is a leap year.
* By definition, the mjd is the julian date - 2400000.5
*
* epochs:
*
*  When using AD, BC notation,
*  1 AD is preceeded by 1 BC.
*  It is also possible to use AD and negative numbers. So the year prior to
*  1 AD would be 0 AD (or 1 BC) and the year prior to that would be -1 AD
*  or (2BC).
*
*  The julian day numbers are measured from 12noon greenwhich jan 1 4713 Bc.
*  4713Bc is -4712 Ad and 0 AD is a leap year. There are an integral number of
*  4 year cycles from the start of the julian calendar until 0 AD.
*  So jan 1 0 AD 12noon greenwich is julian dayno:
*
*       4712*365.25= 1721058.
*  gregorian 0 Grenwhich 0 would occur .5 days earlier at midnite on 0 AD.
*
*  To go from gregorian date to modified julian date compute the julian
*  daynumber from 0AD given the m,d,y input then make the corrections due to
*  switching from gregorian date to modified julian date.
*
*  a. The correction for gregorian to modified julian are:
*
*     -10 days pope gregory thru out 1542.
*     -3  days 1700,1800,1900 were not gregorian leap years (this is why
*                 the routine fails outside of 1900 to  2100 .
*              (note: correction added so will work back to 1542...)
*     -.5 days since the julian day starts .5 days after the gregorian
*         (after correcting to 0AD, gregorian starts at midnite and
*          julian starts .5 days  after).
*     +1721058  to get you from 0AD back to 4713BC
*     -2400000.5  to go from julian date to modified julian date.
*
*  D. the constant to add is:
*     -3 -10 - 2400000.5 + 1721058 - .5 = -678956
*
*  E. to compute the day number you could use:
*
*       nd= (int)(y*365.25) + daysFrom start of year
*
*     This will fail since the leap year cycle from 0AD is:
*     366,365,365,365.
*     If we end up with an integral number of 4 years + 1 year, the last year
*     is a leap year and the 365.25*366 doesn't give us the extra day we need.
*     It will work if we rearrange the leap year cycles to have:
*      365,365,365,366. leap year then falls on the last year of the
*     cycle and we can catch this with our day of year computatation. To make
*     this correction, use (year-1) and add 366 days (since the 1st year
*     0 AD had 366 days.
*
*     The final problem is that daynumbers are measure from jan 1 4713. So
*     365 days later puts you on the first day of the next year. But jan 1
*     of the next year also adds 1 day from our day of year computation so
*     we need to subtract 1. so the day of year computation should be:
*       nd = (int)((y-1)*365.25 + 366 + dayofyear - 1
*     jan
*
*       mjd = (int)((year-1)*365.25) + 365  - 678956
*
* RETURNS
* The modified julian date
*
* WARNING
* This routine is valid from 1542 thru 2100. After 2100 adjust the constant
* GREG_TO_MJD_OFFSET  by -1 (in COMPUTATION   -3 goes to  -4).
* If you believe all my hand waving about why the constants are there,
* i've also got a bridge to sell you.
*
* NOTE
* put in test so should work 1542 thru 2100
*
* SEE ALSO
* gregToMjdDno for a faster routine if you have the day number.
*
*/
int     gregToMjd
        (
        int     day,            /* gregorian day of month */
        int     month,          /* gregorian month of year ( 1 to 12) */
        int     year            /* gregorian years since 0 AD */
        )
{
	int	ival;
	ival=( (int)((year-1)*365.25) + dmToDayNo(day,month,year)
                + GREG_TO_MJD_OFFSET + 365);
	if (year <= 1900)ival++;
	if (year <= 1800)ival++;
	if (year <= 1700)ival++;
	return(ival);
}
