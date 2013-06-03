/*
 *  include file for pointing transformations library
*/
#ifndef INCazzaToRaDech
#define INCazzaToRaDech

#define     OK       0
#define     ERROR   (-1)

#if     !defined(NULL)
#define NULL            0
#endif

#if     !defined(FALSE) || (FALSE!=0)
#define FALSE           0
#endif

#if     !defined(TRUE) || (TRUE!=1)
#define TRUE            1
#endif

#define FILES_DIR         "/home/seti/files"
#define FILE_OBS_POS      FILES_DIR"/obsPosition.dat"
#define FILE_UTC_TO_UT1   FILES_DIR"/utcToUt1.dat"

/*   to go from greg day to julian day 1901 thru 2100. see gregToMjd() */

#define GREG_TO_MJD_OFFSET  -678956



#define MJD_OFFSET         2400000.5

#define VEL_OF_LIGHT_MPS   299792458.
#define VEL_OF_LIGHT_KMPS  299792.458
#define EARTH_RADIUS_EQUATORIAL_M 6378140.
#define TAI_TO_TDT         32.184
#define JULDATE_J2000       2451545.0
#define JULDAYS_IN_CENTURY  36525
#define AU_SECS             499.004782
#define AUPERDAY_TO_VOVERC      AU_SECS/86400.
#define SOLAR_TO_SIDEREAL_DAY   1.00273790935
#define SIDEREAL_TO_SOLAR_DAY    .997269566330
#define JUL_CEN_AFTER_J2000(mjd,utFrac) \
    ((mjdToJulDate(mjd,utFrac) - JULDATE_J2000)/(double)JULDAYS_IN_CENTURY)

/*  typedefs    */

typedef int STATUS;

/* ------------------------------------------------------
 *  data structures
*/



/*  precession/nutation matrix  and inverses*/

typedef struct {
    double  precN_M[9]; /* precces/nutate  j2000 to date*/
    double  precNI_M[9];/* precces/nutate date to j2000*/
    double  eqEquinox;  /* eq of equinox.meanSidTm--> appSidTm*/
    double  ut1Frac;    /* last computation above */
    double  deltaMjd;   /* for recomputing matrices*/
    int     mjd;        /* last computation above */
    int     fill;
} PRECNUT_INFO;
/*
 *  observatory position info. loaded by obsPosInp()
*/
typedef struct  {
    double  latRd;      /* lattiude in radians*/
    double  wLongRd;    /* west longitude*/
    int hoursToUtc; /* hours to get to utc*/
    int     fill;
} OBS_POS_INFO;
/*
 *      The UTC_INFO struct read in from utcInfoInp tells how to go utc to ut1
*/
typedef struct {
    int     year;                   /* for now the year info 4 digits */
    int     mjdAtOff;               /* mod julian date of offset */
    double  offset;                 /* ms offset at midnite of offset */
    double  rate;                   /* in millisecs/day */
} UTC_INFO;
/*
 * struc holding all the info
*/
typedef struct {
    PRECNUT_INFO    precNutI;   /* precession, nutation info*/
    OBS_POS_INFO    obsPosI;    /* observatory position info*/
    UTC_INFO        utcI;       /* utc to ut1 info*/
} AZZA_TO_RADEC_INFO;

/* ---------------------------*/

/*  function prototypes  done */

void   aberAnnual_V(int mjd,double ut1Frac,double *pv);
void   anglesToVec3(double c1Rad,double c2Rad,double *pvec3);
void   azElToHa_V(double *pazEl,double latitude,double *phaDec);
int    azzaToRaDecInit(int dayNum,int year,AZZA_TO_RADEC_INFO *pinfo);
void   azzaToRaDec(double azFd,double za,int mjd,double utcFrac,int ofDate,
        AZZA_TO_RADEC_INFO *pinfo,double *praJ2_rd,double *pdecJ2_rd);
double dms3_rad(int sgn,int d,int m,double s);
int    dmToDayNo(int day,int mon,int year);
void   fmtDms(int sign,int deg,int min,double sec,int secFracDig,
        char *cformatted);
void   fmtHmsD(int sign,int hour,int min,double sec,int secFracDig,
        char   *cformatted);
void   fmtRdToDms(double dmsRd,int secFracDig,char *cformatted);
void   fmtRdToHmsD(double hmsRd,int secFracDig,char *cformatted);
void   fmtSMToHmsD(double secMidnite,int secFracDig,char *cformatted);
int    gregToMjd(int day,int month,int year);
int    gregToMjdDno(int dayNo,int year);
void   haToAzEl_V(double *phaDec,double latRd,double *pazEl);
void   haToRaDec_V(double *phaDec,double lst,double *praDec);
double hms3_rad(int sgn,int ih,int im,double  s);
int    isLeapYear(int year);
double mjdToJulDate(int mjd,double ut1Frac);
STATUS obsPosInp(char *inpFile,OBS_POS_INFO *pobsPosInfo);
void   M3D_Transpose(double *m,double *mtr);
double meanEqToEcl_A(int mjd,double ut1Frac);
void   MM3D_Mult(double *m2,double *m1,double *mresult);
void   MV3D_Mult(double *m,double *v,double *vresult);
void   nutation_M(int mjd,double ut1Frac,double *pm,double *peqOfEquinox);
void   precJ2ToDate_M(int mjd,double ut1Frac,double *pm);
void   precNut(int mjd,double ut1Frac,int jToCur,PRECNUT_INFO *pI,
        double *pinpVec,double *poutVec);
void   precNutInit(double deltaMjd,PRECNUT_INFO *pI);
void   rad_dms3(double rad,int *sgn,int *dd,int *mm,double *ss);
void   rad_hms3(double rad,int *sgn,int *hh,int *mm,double *ss);
void   rotationX_M(double thetaRd,int rightHanded,double *pm);
void   rotationY_M(double thetaRd,int rightHanded,double *pm);
void   rotationZ_M(double thetaRd,int rightHanded,double *pm);
void   secMid_hms3(double secs,int *hh,int *mm,double  *ss);
void   setSign(int *phourDeg,int *psign);
int    truncDtoI(double d);
double ut1ToLmst(int mjd,double ut1Frac,double westLong);
int    utcInfoInp(int dayNum,int year,char *inpFile,UTC_INFO *putcI);
double utcToUt1(int mjd,double utcFrac,UTC_INFO *putcI);
void   V3D_Normalize(double *v1,double *vr);
void   vec3ToAngles(double *pvec3,int c1Pos,double *pc1,double *pc2);
void   VV3D_Sub(double *v1,double *v2,double *vr);
/*
 * errors
*/

/*  utcInfoInp.c */

#define S_pnt_BAD_UTC_TO_UT1_FILENAME                  1
#define S_pnt_UTC_TO_UT1_DATE_BEFORE_FILE_DATES        2
#define S_pnt_UTC_TO_UT1_NO_DATES_IN_FILE              3
/*
 * obsPosInfo.c
*/
#define S_pnt_BAD_OBS_POSITION_FILENAME                4


#endif  /*INCazzaToRaDech */
