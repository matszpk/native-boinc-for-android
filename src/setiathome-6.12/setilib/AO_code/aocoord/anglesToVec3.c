/*      %W      %G      */
/*
modification history
--------------------
x.xx 25jun95,pjp .. included documentation
*/
#include	<math.h>
#include	<azzaToRaDec.h>
/*******************************************************************************
* anglesToVec3 - convert from angles to 3 vector notation.
*
* DESCRIPTION
* Convert the input angular coordinate system to a 3 vector Cartesian 
* representation. Coordinate system axis are set up so that x points
* toward the direction when priniciple angle equals 0 (ra=0,ha=0,az=0).
* Y is towards increasing angle.
*
*   c1   c2    directions
*
*   ra   dec   x towards vernal equinox,y west, z celestial north.
*	       this is a righthanded system.
*   ha   dec   x hour angle=0, y hour angle=pi/2(west),z=celestial north.
*	       This is a left handed system.
*   az   alt   x north horizon,y east, z transit. 
*	       This is a left handed system.
* This routine evaluates 2 cosines and 2 sines.
*
* RETURNS
* The 3 vector is returned via the pointer pvec3. 
*
* SEE ALSO
* vec3ToAngles, convLib.
*/
void	anglesToVec3
	(	
	  double	c1Rad,		/*coord 1 in radians*/
	  double	c2Rad,		/*coord 2 in radians*/
	  double*	pvec3		/* pnts to a 3 vector for output*/
	)
{
	double	cosC2;

	cosC2=cos(c2Rad);
	pvec3[0] = cos(c1Rad)*cosC2;	 /* x*/
	pvec3[1]= sin(c1Rad)*cosC2;	/* y*/
	pvec3[2]= sin(c2Rad);		/* z*/
	return;
}
