/*      %W      %G      */
#include  <azzaToRaDec.h>

/*******************************************************************************
* gregToMjdDno - gregorian date (as day number)  to modified julian date
*
* Convert from a UTC gregorian date to a modified julian date. This routine
* can be used from 1901 thur 2100. The input is the gregorian day number of
* the year and the gregorian year. Jan 1 is day number 1. 
* This is a faster version of gregToMjd since the dayNumber does not have
* to be computed.
*
* NOTE: 
*   valid oct 1542 to 2100.
* SEE ALSO:
* gregToMjd for a description of the algorithm.
*
*/
int     gregToMjdDno
        (
        int     dayNo,          /* gregorian day of year. jan 1 = 1*/
        int     year            /* gregorian years since 0 AD */
        )
{
	int	ival;
	ival=( (int)((year-1)*365.25) + dayNo + GREG_TO_MJD_OFFSET + 365);
	if (year <= 1900)ival++;
	if (year <= 1800)ival++;
	if (year <= 1700)ival++;
	return(ival);
}
