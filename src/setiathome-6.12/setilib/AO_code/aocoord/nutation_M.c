/*      %W      %G      */
#include	<math.h>
#include    <azzaToRaDec.h>
#include 	<mathCon.h>
/******************************************************************************
* nutation_M - nutation matrix to go from mean position of date to true.
*
* DESCRIPTION
*
* Return nutation matrix that is used to go from mean coordinates of date to
* the true coordinates of date. The nutation matrix corrects for the short
* term periodic motions of the celestial pole (18 year and less....).
* This matrix should be applied after the precession matrix that goes from
* mean position of  epoch to mean position of date. This uses the 1980
* IAU Theory of Nutation.
* The equation of the equinox is also returned. This is the value to take
* you from mean sidereal time to apparent sidereal time (see page B6 of AA).
* It is nut[3] (counting from 0).
*
* Input is the modified julian day as an integer and the fraction of a
* day since UT 0hrs as a double.
*
*
* To create the matrix we:
*   1. rotate from mean equatorial system of date  to ecliptic system about x
*      axis (mean equinox).
*   2. rotate about eclipitic pole by delta psi (change in longitude due to
*      nutation.
*   3. rotate from true eclipitic system back to true equatorial by rotating
*      about x axis (true equinox) by -(eps + delta) eps ( mean obliquity of
*      eclipitic plus nutation contribution).
*
* Since the eclipitic pole  is not affected by nutation (we ignore the
* planetary contribution to the eclptic motion) only the equinox of the
* eclipitic is affected by nutation. When going back from true ecliptic
* to  true equatorial, use eps (avg obliquity) + deleps (obliquity due to
* nutation).
*
* Note that this method is the complete rotation versus the approximate value
* used on B20 of the AE 1992.
*
* The matrix is stored in col major order (increment over colums most rapidly).
*
* RETURNS
* The resulting  matrix is returned via the pointer pm.
*
* REFERENCE
* Astronomical Almanac 1992, page B18,B20
* Astronomical Almanac 1984, page S23-S26 for nutation series coefficients.
*/
void    nutation_M
        (
         int     mjd,           /* modified julian day of date*/
         double  ut1Frac,        /* fraction of day from UT 0 hrs */
         double* pm,            /* return nutation matrix here*/
	 double* peqOfEquinox 	/* equation of equinox in rd */
        )
{
        double  t;              /* date - j2000 in julian centuries*/
        double  obliqRd;        /*obliquity of ecliptic of date relative to
                                  mean equator of date (in radians)*/
        double  delPsiRd;       /* nutation in longitude (eclipitic) */
        double  delEpsRd;       /* nutation in obliquity */
        double  toRad;          /* go 10-4 arcsecs to rad*/
        double  m1[9];
        double  omRd,fRd,dRd,lprRd,lRd; /* see pg S26 AE 1984*/

        obliqRd=meanEqToEcl_A(mjd,ut1Frac);/*get angle mean equat.to eclipitic*/
        t=JUL_CEN_AFTER_J2000(mjd,ut1Frac);/*already did it in meanEq!!!*/
/*
 *  delPsi is the change in ecliptic longitude from the mean position due to
 *  nutation.
 *  delEps is the change in ecliptic obliquity from the mean oblitquity.
 *
 *  the series for delPsi, delEps is take from page S23 of the 1984 AA.
 *  The series has 106 terms. I used terms down to 100*10-4asecs
 *  See page S23-s26 of AA 1984 for details
 *
 *  period       6798.4 3399.2 182.6 365.3 121.7 13.7 27.6 13.6 9.1...   (days)
 *  series terms:  1      2      9    10    11    31  32   33   34 ...
*/
        omRd   =( 450160.280 - t*(   5.*1296000. +  482890.539 +
                               t*(   7.455 + t*.008)))*C_ASEC_TO_RAD;
 
        dRd    =(1072261.307 + t*(1236.*1296000. + 1105601.328 +
                               t*(  -6.891 + t*.019)))*C_ASEC_TO_RAD;
 
        fRd    =( 335778.877 + t*(1342.*1296000. +  295263.137 +
                               t*( -13.257 + t*.011)))*C_ASEC_TO_RAD;
 
        lprRd  =(1287099.804 + t*(  99.*1296000. + 1292581.224 +
                               t*(   -.577 - t*.012)))*C_ASEC_TO_RAD;
        lRd    =( 485866.733 + t*(1325.*1296000. +  715922.633 +
                               t*(  31.310 + t*.064)))*C_ASEC_TO_RAD;
        toRad= 1e-4*C_ASEC_TO_RAD;
        delPsiRd=(
                 (-171996.-174.2*t)*sin(   omRd)        /*6798.4days  term 1*/
             +   (   2062.+   .2*t)*sin(2.*omRd)        /*3399.1 day  term 2*/
             +   ( -13187.-  1.6*t)*sin(2.*(fRd-dRd+omRd))/*182.6 day term 9*/
             +   (   1426.-  3.4*t)*sin(   lprRd)       /*365.3  day  term10*/
             +   (   -517.+  1.2*t)*sin(lprRd+2.*(fRd-dRd+omRd))/*121.7day t11*/             +   (    217.-   .5*t)*sin(-lprRd+2.*(fRd-dRd+omRd))/*365.2d  t12*/             +   (    129.+   .1*t)*sin(omRd+2.*(fRd-dRd))       /*177.8d  t13*/             +   (  -2274.-   .2*t)*sin(2.*(fRd+omRd))  /* 13.7  day  term31*/
             +   (    712.+   .1*t)*sin(lRd)            /* 27.6  day  term32*/
             +   (   -386.-   .4*t)*sin(2.*fRd+omRd)    /* 13.6  day  term33*/
             +   (   -301.        )*sin(lRd+2.*(fRd+omRd))/*9.1  day  term34*/
             +   (   -158.        )*sin(lRd-2.*dRd)       /*31.8 day  term35*/
             +   (    123.        )*sin(-lRd+2.*(fRd+omRd))/*27.1day  term36*/
                 )*toRad;
 
        delEpsRd=(
                 (  92025.+  8.9*t)*cos(   omRd)        /*6798.4days term 1*/
             +   (   -895.+   .5*t)*cos(2.*omRd)        /*3399.1 day  term 2*/
             +   (   5736.-  3.1*t)*cos(2.*(fRd-dRd+omRd))/*182.6 day  term 9*/
             +   (     54.-   .1*t)*cos(   lprRd)       /*365.3  day  term10*/
             +   (    224 -   .6*t)*cos(lprRd+2.*(fRd-dRd+omRd))/*121.7day t11*/             +   (    -95.+   .3*t)*cos(-lprRd+2.*(fRd-dRd+omRd))/*365.2d  t12*/
	     +   (    -70.        )*cos(omRd+2.*(fRd-dRd))       /*177.8d  t13*/
             +   (    977.-   .5*t)*cos(2.*(fRd+omRd))  /* 13.7  day  term31*/
             +   (     -7.        )*cos(lRd)            /* 27.6  day  term32*/
             +   (    200.        )*cos(2.*fRd+omRd)    /* 13.6  day  term33*/
             +   (    129.-   .1*t)*cos(lRd+2.*(fRd+omRd))/*9.1  day  term34*/
                                               /*negligible 31.8 day  term35*/
             +   (    -53.        )*cos(-lRd+2.*(fRd+omRd))/*27.1day  term36*/
                 )*toRad;
 
        /*
        *   rotate mean equatorial to mean eclipitic
        */
        rotationX_M(obliqRd,TRUE,m1);   /* mean equatorial to mean eclipitic*/
        /*
        *  delPsi defined as value to add to mean longitude. This is a
        *  rotation of a vector. The rotation of the coordinate system is
        *  minus this value
        */
        rotationZ_M(-delPsiRd,TRUE,pm);  /* mean ecliptic to true eclipitic*/
        MM3D_Mult(pm,m1,pm);            /* concatenate matrices*/
        /*
        *  looks like delEps is a rotation of the coordinate system and
        *  not just added to a vector (i tried both ways and coordinate
        *  system was closer).
        */
        rotationX_M(-(obliqRd+delEpsRd),TRUE,m1);/* back to true equatorial*/
        MM3D_Mult(m1,pm,pm);            /* concatenate matrices*/
	*peqOfEquinox=cos(obliqRd+delEpsRd)*delPsiRd;
        return;
}
