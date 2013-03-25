/*      %W      %G      */
#include        <azzaToRaDec.h>
#include	<math.h>
#include	<mathCon.h>

/*******************************************************************************
* aberAnnual_V - return the annual aberration vector offset.
*
* DESCRIPTION
*
* Return the annual aberration vector offset. This vector should be added
* to the true object position to get the apparent position of the object.
*
* Aberration requires the observer to point at a direction different from
* the geometrical position of the object. It is caused by the relative motion
* of the two objects (think of it as having to bend the umbrella toward your
* direction of motion to not get wet..). It is the velocity vector offset of
* of the observer/object from the geometric direction measured in
* an inertial frame.
*
* Complete aberration is called planetary aberration and includes the motion
* of the object and the motion of the observer. For distant objects the
* objects motion can be ignored. Stellar aberration is the portion due to the
* motion of the observer (relative to some inertial frame). It consists of
* secular aberration (motion of the solar system), annual aberration
* (motion of the earth about the sun), and diurnal aberration (caused by
* the rotation of the earth about its axis.
*
* Secular aberration is small and ignored (the solar system is assumed to be
* in  the inertial frame of the stars). The earths velocity about the sun of
* +/- 30km/sec = +/- .0001v/c gives  +/- 20" variation for annual aberration.
* diurnal variation gives a maximum of about +/- .32" at the equator over a day.*
* This routine computes annual aberration using the approximate routines no
* page B17 and C24 of the AA.
*
* The matrix is stored in col major order (increment over colums most rapidly).
*
*
* RETURNS
* The resulting  vector offset is returned via the pointer pv.
*/
void    aberAnnual_V
        (
         int    mjd,            /* modified julian date */
         double ut1Frac,         /* fraction of day from UT 0hrs */
         double* pv             /* return offset vector here */
        )
{
        double L;               /* mean long of sun in degrees, */
        double gRd;             /* mean anomaly of sun in radians*/
        double n;               /* jd - j2000.0 in julian days */
        double lambdaRd;        /* eclipitic longitude see pg C24 AA */
        double C;               /* convert au/day/c to radians*/
        double cosl;            /* cosine lambda*/
        
 
        n=mjdToJulDate(mjd,ut1Frac) - JULDATE_J2000;     /* get julian date */
        L=fmod((280.460 + .9856474*n),360.);
        L=(L<0.)?L+360.:L;
	    gRd=fmod((357.528 + .9856003*n)*C_DEG_TO_RAD,C_2PI);
        gRd=(gRd<0.)?gRd+C_2PI:gRd;
        lambdaRd=(L + 1.915*sin(gRd) + .020*sin(2*gRd))*C_DEG_TO_RAD;
        C= AU_SECS/86400.;
        cosl=cos(lambdaRd);
        /*
        *   the constants below are:
        *    .0172   = k
        *    .0158   = k * cos(eps)
        *    .0068   = k * sin(eps)
        *  in AU/day. k is the constant of aberration: 20.49" and eps is the
        *  ecliptic angle.
        *  C converts from AU/day and divides by c at the same time (since
        *  we want v/c for the correction.
        */
        pv[0]= .0172*C*sin(lambdaRd);
        pv[1]=-.0158*C*cosl;
        pv[2]=-.0068*C*cosl;
        return;
}
