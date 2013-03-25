//__________________________________________________________________________
// Source Code Name  : seti_coord.h
// Programmer        : Donnelly/Cobb et al
//                   : Project SERENDIP, University of California, Berkeley CA
// Compiler          : GNU gcc
// Purpose           : header file for s3coord.cc
//__________________________________________________________________________


// static const double AOLATITUDE_RADS=0.320334327; /* observatory - arecibo */
// static const double AOLATITUDE_DEGS=18.3538056;
// static const double AOLONGITUDE_DEGS=(-66.75522222);

// Updated by EK to match phil's position 10/24/06
// The change is 7.6 arcsec, so no big deal
// These are the astronomical lat and lon, not geodetic coordinates
// so changes to the telescope can change them.
static const double AOLATITUDE_RADS=0.320336761284;
static const double AOLATITUDE_DEGS=18.35394444444;
static const double AOLONGITUDE_DEGS=(-66.75300000000);

const double stdepoch = 2000;  // our standard analysis epoch
    
#ifndef BOOLEAN
#define BOOLEAN unsigned char
#define TRUE 1
#define FALSE 0
#endif

long long co_radeclfreq2npix(double ra, double decl, double freq);
void co_npix2radeclfreq(double &ra, double &decl, double &freq, long long npix);
void co_radecl2pix(double ra, double decl, long &pix);
void co_qpix2radecl(double &ra, double &decl, long pix);
void eod2stdepoch(double jd, double &ra, double &decl, double stdepoch);
void co_EqToHorz(double ra, 
                 double dec, 
                 double lst, 
                 double *za, 
                 double *azm,
                 double CosLat, 
                 double SinLat);

double co_SkyAngle(double first_ra,
                 double first_dec,
                 double second_ra,
                 double second_dec);

void co_GetBeamRes(double * db_DecRes,
                   double * db_RaRes,
                   long     ld_Freq,
                   unsigned us_TelDiameter,
                   double   db_NumBeams);

void co_ZenAzToRaDec(double zenith_ang,
                     double azimuth,
                     double lsthour,
                     double *ra,
                     double *dec,
		     double latitude=AOLATITUDE_DEGS);

void co_ZenAzCorrectionPreliminary(double *zen, 
                                   double *az);

bool co_ZenAzCorrection(receiver_config &ReceiverConfig,
                        double *zen,
                        double *az,
			double rot_angle=0);

bool co_ZenAzCorrection(telescope_id TiD,
                        double *zen,
                        double *az,
			double rot_angle=0);

double co_GetSlewRate(double Ra1, double Dec1, double Jd1,
                      double Ra2, double Dec2, double Jd2); 

double wrap(double Val, long LowBound, long HighBound, BOOLEAN LowInclusive);

void            co_EqToXyz(double ra, double dec, double * xyz);

void            co_XyzToEq(double xyz[], double *ra, double *dec);

void            co_Precess(double e1, double * pos1, double e2, double * pos2);

double Atan2 (double y, double x); /* for co_Precess */
