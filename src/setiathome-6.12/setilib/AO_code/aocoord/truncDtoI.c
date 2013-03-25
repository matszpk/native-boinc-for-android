#include	<azzaToRaDec.h>
#include	<math.h>
int	truncDtoI(double d)
{
#if FALSE
/*
 *	truncate double to int (truncate towards 0) 
 *	the fintrz of the 68040 takes 30 usecs since it traps to a 
 *	software routine. this routine takes 2 usecs.
*/
	register int i;
	register int cm;
	register int im;
	
	asm volatile ("fmove%.l fpcr,%0":"=dm" (cm) :);

	im=(cm & 0xff00) | 0x10 ;		/* round toward 0*/

	asm volatile ("fmove%.l %0,fpcr": :"dmi" (im)); /* load fpcr*/

	asm volatile ("fmove%.l %1,%0" : "=d" (i) :"f" (d)); /* trunc*/

	asm volatile ("fmove%.l %0,fpcr": :"dmi" (cm)); /* reload fpcr*/
#endif
/*
 *  trunc seem to be broken in the math.h..
 *  do it the long way with floor
*/
	int i;
	i=(d < 0.)?-(floor(-d)):floor(d);
	return(i);	
}
