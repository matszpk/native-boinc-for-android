//#define DEBUG
//______________________________________________________________________________
// Source Code Name  : seti_coord.cc
// Programmer        : Donnelly/Cobb et al
//                   : Project SERENDIP, University of California, Berkeley CA
// Compiler          : GNU gcc
// Date opened       : 10/94
// Purpose           : Functions for manipulating celestial coordinate systems
//
// Modification History
//      10/94          Converted from s3 for usage with s4 system
//                     Modified function co_EqToHorz to take
//                     cos(lat) and sin(lat) as parameters.
//       4/96  jeffc   Added function co_GetBeamRes.
//       12/97 jeffc   Added 3 routines: co_ZenAzToRaDec(),
//                     co_ZenAzCorrectionPreliminary(),
//                     co_ZenAzCorrection()
//   03/05/99  jeffc   ZenAz correction coeffecients now a 2d table.  Added
//                      in 21cm feed corrections.
//______________________________________________________________________________


#include <stdio.h>
#include <math.h>

#include "setilib.h"

#ifndef BOOLEAN
#define BOOLEAN unsigned char
#define TRUE 1
#define FALSE 0
#endif

static const double D2R=(M_PI/180.0);  /* decimal degrees to radians conversion */

static const double SECCON=206264.8062470964; /* for co_Precess */

double Atan2 (double y, double x); /* for co_Precess */

// go from epoch of the day to our standard epoch (see seti_coord.h)
//___________________________________________________________________________________
void eod2stdepoch(double jd, double &ra, double &decl, double stdepoch) {
//___________________________________________________________________________________

    double eod, pos1[3], pos2[3];

    eod = seti_dop_jdtoje(jd);          // eod
    seti_dop_eqxyz(ra, decl, pos1);         // go to retangular
    seti_dop_precess(eod, pos1, stdepoch, pos2);   // precess
    seti_dop_xyzeq(pos2, &ra, &decl);       // go to equatorial

}


//___________________________________________________________________________________
// co_EqToHorz()
// Written by Beppe Sabatini.  Given right ascension, declination and the
// local sidereal time, this function returns zenith angle (the complement of
// horizon altitude) and azimuth (compass point) for ARECIBO.
//___________________________________________________________________________________

void co_EqToHorz(double ra, double dec, double lst,
        double *za, double *azm, double CosLat, double SinLat) {
    double sin_alt, alt;                    /* Sine of Altitude, Altitude */
    double alt_rad;                         /* Altitude in radians */
    double azm_rad;                         /* Azimuth in radians */
    double ha, ha_rad;                      /* Hour Angle, HA in radians */

    ha = lst - ra;
    ha_rad = ha * 15.0 * D2R;        /* convert hour angle to radians */

    dec *= D2R;                             /* convert dec to radians */

    /*
     * Calculate Zenith Angle.  Formula from Practical Astronomy with Your
     * Calculator, by Peter Duffet-Smith.
     */

    sin_alt  = sin(dec) * SinLat +
            cos(dec) * CosLat * cos(ha_rad);
    alt_rad  = asin(sin_alt);
    alt      = alt_rad / D2R;               /* convert radians to degrees */
    *za      = fabs(90.0 - alt);   /* convert altitude to zenith angle */

    /* Calculate Azimuth */

    azm_rad  = acos((sin(dec) - SinLat * sin(alt_rad)) /
            (CosLat * cos(alt_rad)));
    /*
     * If the event is in the east half of the sky, the Azimuth needs to
     * be mirrored around the NS axis.  This compensates for the limited
     * range of the Arccosine.
     */
    if (sin(ha_rad) > 0.0) {
        azm_rad = (2.0 * M_PI) - azm_rad;
    }

    /* The Azimuth is converted from radians to degrees */
    *azm     = (azm_rad / D2R);
}


//__________________________________________________________________________
// co_SkyAngle()
// function written by Jeff Cobb
// This function, given the equatorial coordinates of two points on
// the celestial sphere, will return the angular distance between them.
// Input - right ascension (in decimal hours) and declination (in decimal
//         degrees) of the two points.
// Output - angular separation (in decimal degrees) of the two points.
//__________________________________________________________________________



double co_SkyAngle(double first_ra,
        double first_dec,
        double second_ra,
        double second_dec)

{
    double sky_ang;
    double temp;

    first_ra  *= 15.0;            /* convert hours to degrees */
    second_ra *= 15.0;

    first_ra   *= D2R;            /* convert to radians */
    first_dec  *= D2R;
    second_ra  *= D2R;
    second_dec *= D2R;

    /* solve spherical triangle */
    temp = sin(first_dec) * sin(second_dec) +
            cos(first_dec) * cos(second_dec) *
            cos(first_ra - second_ra);
    if (temp > 1) {
        temp = 1;
    } else if (temp < -1) {
        temp = -1;
    }
    sky_ang = acos(temp);


    return (sky_ang /= D2R);      /* convert back to degrees and return */
}

//__________________________________________________________________________
// co_GetBeamRes()
// Function returns the ra_res and dec_res (both in their respective units)
// The returned value for ra_res and dec_res is multiplied times the number
// of beams.  Note that this function can also be used to get the beam width
// in degrees.  Beam width is equal to db_DecRes.
// Parameters:
//   db_DecRes         returned declination resolution (= bw) in degrees
//   db_RaRes          returned right-asencion resolution in hours
//   ld_Freq           frequency in MHz (local oscillator value is usually
//                     passed here)
//   us_TelDiameter    telescope diameter in feet
//   db_NumBeams       number of beams
//__________________________________________________________________________


void co_GetBeamRes(double *db_DecRes,
        double *db_RaRes,
        long     ld_Freq,
        unsigned us_TelDiameter,
        double   db_NumBeams) {
    *db_DecRes = db_NumBeams * 68/(((float) ld_Freq/1000) * us_TelDiameter);
    *db_RaRes = *db_DecRes/15;
}


//__________________________________________________________________________
// co_ZenAzToRaDec()
// function written by Jeff Cobb
// ported to siren (s4) by Jeff Cobb
// Converts zenith angle and azimth to ra and dec
// input: zen and az in decimal degrees
//        lst in decimal hours
// output: ra is in hours, dec is in degrees
//__________________________________________________________________________

void co_ZenAzToRaDec(double zenith_ang,
        double azimuth,
        double lsthour,
        double *ra,
        double *dec,
        double latitude)

{
    double dec_rad, hour_ang_rad;
    double temp;

    zenith_ang *= D2R;      /* decimal degrees to radians */
    azimuth    *= D2R;

    /***** figure and return declination *****/

    dec_rad = asin ((cos(zenith_ang) * sin(latitude*D2R)) +
            (sin(zenith_ang) * cos(latitude*D2R) * cos(azimuth)));

    *dec = dec_rad / D2R;   /* radians to decimal degrees */

    /***** figure and return right ascension *****/

    temp = (cos(zenith_ang) - sin(latitude*D2R) * sin(dec_rad)) /
            (cos(latitude*D2R) * cos(dec_rad));
    if (temp > 1) {
        temp = 1;
    } else if (temp < -1) {
        temp = -1;
    }
    hour_ang_rad = acos (temp);


    if (sin(azimuth) > 0.0) {       /* insure correct quadrant */
        hour_ang_rad = (2 * M_PI) - hour_ang_rad;
    }


    /* to get ra, we convert hour angle to decimal degrees, */
    /* convert degrees to decimal hours, and subtract the   */
    /* result from local sidereal decimal hours.            */
    *ra = lsthour - ((hour_ang_rad / D2R) / 15.0);

    // Take care of wrap situations in RA
    if (*ra < 0.0) {
        *ra += 24.0;
    } else if (*ra >= 24.0) {
        *ra -= 24.0;
    }
}                     /* end zen_az_2_ra_dec */

//__________________________________________________________________________
// co_ZenAzCorrectionPreliminary()
// function written by Matt Lebofsky
// Corrects elevation angle using the formula:
// ZA  = ENC - 3 arc minutes - [ 3 arc minutes * (ENC{in degrees}/20degrees) ]
// where ZA = true zenith angle
// and   ENC = encoder reading of zenith angle (ENC is from 0 to +20 degrees).
// The azimuth only gets corrected if > 360, if so then az=az-360
// input: zen and az (pointers) in decimal degrees
// output: zen and az corrected in place in decimal degrees
//__________________________________________________________________________


void co_ZenAzCorrectionPreliminary(double *zen, double *az)

{
    double three_arc_minutes = 0.05;

    /* First correct azimuth - If az > 360, then subtract 360 */

    *az = *az > 360.0 ? *az - 360.0 : *az;

    /* Now correct zenith - see formula above */

    *zen = *zen - three_arc_minutes - ( three_arc_minutes * (*zen / 20.0));

}                      /* end co_Zen_Az_Correction */


//__________________________________________________________________________
// co_ZenAzCorrection(unsigned char TiD)
//__________________________________________________________________________
bool co_ZenAzCorrection(telescope_id TID,
        double *zen, double *az, double rot_angle) {
    static receiver_config ReceiverConfig;
    if (ReceiverConfig.s4_id != TID) {
        char buf[256];
        sprintf(buf,"where s4_id=%d",TID);
        ReceiverConfig.fetch(buf);
    }
    return co_ZenAzCorrection(ReceiverConfig, zen, az, rot_angle);
}

bool co_ZenAzCorrection(receiver_config &ReceiverConfig,
        double *zen, double *az, double rot_angle) {
    double CosAz, SinAz, SinZen, Cos2Az, Sin2Az, Cos3Az, Sin3Az, Cos6Az, Sin6Az, SinZen2,
           Cos3Imb, Sin3Imb, AzRadians, ZenRadians, ZenCorrection, AzCorrection;

    if ((ReceiverConfig.center_freq == 0) && !ReceiverConfig.fetch()) {
        return false;
    }

    if (isnan(ReceiverConfig.array_az_ellipse)) {
        ReceiverConfig.array_az_ellipse=0;
    }
    if (isnan(ReceiverConfig.array_za_ellipse)) {
        ReceiverConfig.array_za_ellipse=0;
    }
    if (isnan(ReceiverConfig.array_angle)) {
        ReceiverConfig.array_angle=0;
    }

#ifdef DEBUG
    static int printed=0;
    int i;
    if (!printed) {
        fprintf(stderr, "%d\n", TiD);
        fprintf(stderr, "ID = %ld  Name = %s  Lat = %lf  Lon = %lf  El = %lf  Dia = %lf  BW = %lf\n",
                ReceiverConfig.s4_id,
                ReceiverConfig.name,
                ReceiverConfig.latitude,
                ReceiverConfig.longitude,
                ReceiverConfig.elevation,
                ReceiverConfig.diameter,
                ReceiverConfig.beam_width);

        for (i=0; i < ReceiverConfig.zen_corr_coeff.size(); i++) {
            fprintf(stderr, "%lf ", ReceiverConfig.zen_corr_coeff[i]);
        }
        fprintf(stderr, "\n");
        for (i=0; i < ReceiverConfig.az_corr_coeff.size(); i++) {
            fprintf(stderr, "%lf ", ReceiverConfig.az_corr_coeff[i]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "array_az_ellipse=%lf array_za_ellipse=%lf array_angle=%lf",
                ReceiverConfig.array_az_ellipse,ReceiverConfig.array_za_ellipse,
                ReceiverConfig.array_angle
               );
        printed++;
    }
#endif


// Start of port of AO Phil's IDL routine modeval.pro
    AzRadians  = *az  * D2R;
    ZenRadians = *zen * D2R;

    CosAz  = cos(AzRadians);
    SinAz  = sin(AzRadians);
    Cos2Az = cos(2.0 * AzRadians);
    Sin2Az = sin(2.0 * AzRadians);
    Cos3Az = cos(3.0 * AzRadians);
    Sin3Az = sin(3.0 * AzRadians);
    Cos6Az = cos(6.0 * AzRadians);
    Sin6Az = sin(6.0 * AzRadians);

    SinZen  = sin(ZenRadians);
    SinZen2 = SinZen * SinZen;

    Cos3Imb = sin(ZenRadians - 0.1596997627) * Cos3Az;    // 0.159... = 9.15 deg.  This is related
    Sin3Imb = sin(ZenRadians - 0.1596997627) * Sin3Az;    // to the balance of dome and CH. (via Phil)

    ZenCorrection = ReceiverConfig.zen_corr_coeff[ 0]             +
            ReceiverConfig.zen_corr_coeff[ 1] * CosAz     +
            ReceiverConfig.zen_corr_coeff[ 2] * SinAz     +
            ReceiverConfig.zen_corr_coeff[ 3] * SinZen    +
            ReceiverConfig.zen_corr_coeff[ 4] * SinZen2   +
            ReceiverConfig.zen_corr_coeff[ 5] * Cos3Az    +
            ReceiverConfig.zen_corr_coeff[ 6] * Sin3Az    +
            ReceiverConfig.zen_corr_coeff[ 7] * Cos3Imb   +
            ReceiverConfig.zen_corr_coeff[ 8] * Sin3Imb   +
            ReceiverConfig.zen_corr_coeff[ 9] * Cos2Az    +
            ReceiverConfig.zen_corr_coeff[10] * Sin2Az    +
            ReceiverConfig.zen_corr_coeff[11] * Cos6Az    +
            ReceiverConfig.zen_corr_coeff[12] * Sin6Az;

    AzCorrection  = ReceiverConfig.az_corr_coeff[ 0]                +
            ReceiverConfig.az_corr_coeff[ 1] * CosAz        +
            ReceiverConfig.az_corr_coeff[ 2] * SinAz        +
            ReceiverConfig.az_corr_coeff[ 3] * SinZen       +
            ReceiverConfig.az_corr_coeff[ 4] * SinZen2      +
            ReceiverConfig.az_corr_coeff[ 5] * Cos3Az       +
            ReceiverConfig.az_corr_coeff[ 6] * Sin3Az       +
            ReceiverConfig.az_corr_coeff[ 7] * Cos3Imb      +
            ReceiverConfig.az_corr_coeff[ 8] * Sin3Imb      +
            ReceiverConfig.az_corr_coeff[ 9] * Cos2Az       +
            ReceiverConfig.az_corr_coeff[10] * Sin2Az       +
            ReceiverConfig.az_corr_coeff[11] * Cos6Az       +
            ReceiverConfig.az_corr_coeff[12] * Sin6Az;
// end of port of modeval.pro

// Now correct for the offsets of the beams in the arrays.
// Arrays are distributed on an ellipse.
//  compute the az,za offsets to add to the az, za coordinates
//  these are great circle.
//  1. With zero rotation angle,  the th generated is counter clockwise.
//     The offsets in the azalfaoff.. table were generated from using
//     this orientation of the angle.
//  2. The rotangl of the array is positive clockwise (since that is the
//     way the floor rotates when given a positive number). We use a
//     minus sign since it is opposite of the angle used to generate the
//     offset array above.
//  3. After computing the angle of a beam and projecting it onto the
//     major, minor axis of the ellipse, the values are subtracted
//     from pixel 0 az,za. In other words, the positive direction is the
//     same as we've already used above for our correction, so we can just
//     add it to our correction.

    double posrot=(ReceiverConfig.array_angle-rot_angle)*D2R;
    AzCorrection+=ReceiverConfig.array_az_ellipse*cos(posrot);
    ZenCorrection+=ReceiverConfig.array_za_ellipse*sin(posrot);

    *zen -= ZenCorrection / 3600.0;               // Correction is in arcsec.
    *az  -= (AzCorrection  / 3600.0) / sin(*zen * D2R);   // Correction is in arcsec.

    // Sometimes the azimuth is not where the telescope is
    // pointing, but rather where it is physically positioned.
    *az    += ReceiverConfig.az_orientation;

    if (*zen < 0.0) {                             // Have we corrected zen over
        //  the zenith?
        *zen = 0.0 - *zen;                          // If so, correct zen and
        *az += 180.0;                               //   swing azimuth around to
    }

    *az = wrap(*az, 0L, 360L, TRUE);              // az to 0 - 360 degrees
    return true;
}

//__________________________________________________________________________
double wrap(double Val, long LowBound, long HighBound, BOOLEAN LowInclusive) {
//__________________________________________________________________________

    if (LowInclusive) {
        while (Val >= HighBound) {
            Val -= HighBound;
        }
        while (Val <   LowBound) {
            Val += HighBound;
        }
    } else {
        while (Val >  HighBound) {
            Val -= HighBound;
        }
        while (Val <=  LowBound) {
            Val += HighBound;
        }
    }
    return Val;
}


//__________________________________________________________________________
double co_GetSlewRate(double Ra1, double Dec1, double Jd1,
        double Ra2, double Dec2, double Jd2)
//__________________________________________________________________________
{
    if (Jd1 != Jd2) {
        return(co_SkyAngle(Ra1, Dec1, Ra2, Dec2) / SECDIFF(Jd1, Jd2));
    } else {
        return(0.0);
    }
}

/*******************************************************************************
  Convert equatorial RA, DEC into X, Y, Z rectangular coordinates, with unit
  radius vector.  X axis points to Vernal Equinox, Y to 6 hours R.A., Z to
  the north celestial pole.  The coordinate system is right-handed.
*******************************************************************************/

void            co_EqToXyz(double ra, double dec, double *xyz) {
    double        alpha, delta;           /* Radian equivalents of RA, DEC */
    double        cdelta;

    alpha = M_PI * ra / 12;               /* Convert to radians.*/
    delta = M_PI * dec / 180;
    cdelta = cos(delta);
    xyz[0] = cdelta * cos(alpha);
    xyz[1] = cdelta * sin(alpha);
    xyz[2] = sin(delta);
}

/*************************************************************************
Convert x,y,z to equatorial coordinates Ra, Dec. It is assumed that
x,y,z are equatorial rectangular coordinates with z pointing north, x
to the vernal equinox, ans y toward 6 hours Ra.
**************************************************************************/

void            co_XyzToEq(double xyz[], double *ra, double *dec) {
    double r,rho,x,y,z;
    x = xyz[0];
    y = xyz[1];
    z = xyz[2];
    rho = sqrt(x * x + y * y);
    r = sqrt(rho * rho + z * z);
    if (r < 1.0e-20) {
        *ra = 0.0;
        *dec = 0.0;
    } else {
        *ra = Atan2 (y,x) * 12.0 / M_PI;
        if (fabs(rho) > fabs(z)) {
            *dec = atan(z/rho) * 180.0 / M_PI;
        } else {
            if (z > 0.0) {
                *dec = 90.0 - atan(rho/z) * 180.0 / M_PI;
            } else {
                *dec = -90.0 - atan(rho/z) * 180.0 / M_PI;
            }
        }
    }
}

/*******************************************************************************
                                    PRECESS

Precess equatorial rectangular coordinates from one epoch to another.  Adapted
from George Kaplan's "APPLE" package (U.S. Naval Observatory). Positions are
referred to the mean equator and equinox of the two respective epochs.  See
pp. 30--34 of the Explanatory Supplement to the American Ephemeris; Lieske, et
al. (1977) Astron. & Astrophys. 58, 1--16; and Lieske (1979) Astron. &
Astrophys. 73, 282--284.
*******************************************************************************/

void            co_Precess(double e1, double *pos1, double e2, double *pos2) {
    double        tjd1;   /* Epoch1, expressed as a Julian date */
    double        tjd2;   /* Epoch2, expressed as a Julian date */
    double        t0;     /* Lieske's T (time in cents from Epoch1 to J2000) */
    double        t;      /* Lieske's t (time in cents from Epoch1 to Epoch2) */
    double        t02;    /* T0 ** 2 */
    double        t2;     /* T ** 2 */
    double        t3;     /* T ** 3 */
    double        zeta0;  /* Lieske's Zeta[A] */
    double        zee;    /* Lieske's Z[A] */
    double        theta;  /* Lieske's Theta[A] */
    /* The above three angles define the 3 rotations
    required to effect the coordinate transformation. */
    double        czeta0; /* cos (Zeta0) */
    double        szeta0; /* sin (Zeta0) */
    double        czee;   /* cos (Z) */
    double        szee;   /* sin (Z) */
    double        ctheta; /* cos (Theta) */
    double        stheta; /* sin (Theta) */
    double        xx,yx,zx, xy,yy,zy, xz,yz,zz;
    /* Matrix elements for the transformation. */
    double        vin[3]; /* Copy of input vector */
    //double      jetojd();

    vin[0] = pos1[0];
    vin[1] = pos1[1];
    vin[2] = pos1[2];
    tjd1 = tm_JulianEpochToJulianDate(e1);
    tjd2 = tm_JulianEpochToJulianDate(e2);
    t0 = (tjd1 - 2451545) / 36525;
    t = (tjd2 - tjd1) / 36525;
    t02 = t0 * t0;
    t2 = t * t;
    t3 = t2 * t;
    zeta0 = (2306.2181 + 1.39656 * t0 - 0.000139 * t02) * t +
            (0.30188 - 0.000344 * t0) * t2 +
            0.017998 * t3;
    zee = (2306.2181 + 1.39656 * t0 - 0.000139 * t02) * t +
            (1.09468 + 0.000066 * t0) * t2 +
            0.018203 * t3;
    theta = (2004.3109 - 0.85330 * t0 - 0.000217 * t02) * t +
            (-0.42665 - 0.000217 * t0) * t2 -
            0.041833 * t3;
    zeta0 /= SECCON;
    zee /= SECCON;
    theta /= SECCON;
    czeta0 = cos(zeta0);
    szeta0 = sin(zeta0);
    czee = cos(zee);
    szee = sin(zee);
    ctheta = cos(theta);
    stheta = sin(theta);

    /* Precession rotation matrix follows */
    xx = czeta0 * ctheta * czee - szeta0 * szee;
    yx = -szeta0 * ctheta * czee - czeta0 * szee;
    zx = -stheta * czee;
    xy = czeta0 * ctheta * szee + szeta0 * czee;
    yy = -szeta0 * ctheta * szee + czeta0 * czee;
    zy = -stheta * szee;
    xz = czeta0 * stheta;
    yz = -szeta0 * stheta;
    zz = ctheta;

    /* Perform rotation */
    pos2[0] = xx * vin[0] + yx * vin[1] + zx * vin[2];
    pos2[1] = xy * vin[0] + yy * vin[1] + zy * vin[2];
    pos2[2] = xz * vin[0] + yz * vin[1] + zz * vin[2];
}

/*-----------------------------------------------------------------------------
                                    ATAN2

  Two-argument Arc-Tangent.  Finds polar angle for radius, given x, y.
-----------------------------------------------------------------------------*/

double Atan2 (double y, double x)      /* x, y in rectangular coordinates*/
/* returns Polar Angle, radians*/
{
    double  arg;

    if ((fabs(x) == 0.0) && (fabs(y) == 0.0)) {
        arg = 0.0;
    } else {
        if (x > 0) {
            if (x > fabs(y)) {
                arg = atan(y/x);
            } else if (y > 0.0) {
                arg = 0.5 * M_PI - atan(x/y);
            } else {
                arg = -0.5 * M_PI - atan(x/y);
            }
        } else {
            if (fabs(x) > fabs(y)) {
                arg = M_PI + atan (y/x);
            } else if (y > 0.0) {
                arg = 0.5*M_PI - atan (x/y);
            } else {
                arg = -0.5*M_PI - atan (x/y);
            }
        }
    }
    if (arg < 0.0) {
        arg += 2.0*M_PI;
    }
    return(arg);
}

