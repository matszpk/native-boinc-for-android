/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<azzaToRaDec.h>
/******************************************************************************
* setSign - make the hours/degrees positive and set the sign to compensate
*
*DESCRIPTION
* Set the hours or degrees to a positive value and set the sign variable to
* reflect the original sign.
*
* RETURNS
* The hour/degrees are returned as positive and the sign bit is set.
* 
* SEE ALSO
* convLib
*/


void  setSign
	( 
	int* 	phourDeg,	/* hours or degrees*/
	int*	psign 		/* return sign here*/
	)
{
	if (*phourDeg<0){
	    *psign=-1;
	    *phourDeg= - *phourDeg;
	}
	else{
	   *psign=1;
	}
	return;
}
