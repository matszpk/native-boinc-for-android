/* ________________________________________________________________________
 Author : Jeff Cobb, after Chuck Donnelly, after Walter Herrick, 
          after Jerry Hudson, after Navy code
 source name: seti_doppler.cc
 compiler   : gcc
___________________________________________________________________________
 modification history:
   version 1.0 finished 5/24/01
___________________________________________________________________________
This set of routines together calculate frequency for the barycentric
reference frame.  There are 2 directly callable functions:

		seti_dop_FreqAtBaryCenter()
		seti_dop_FreqOffsetAtBaryCenter()

This is ancient code, passed down through generations of serendiper's.
What I (Jeff) did was form the callable functions above such that the
remainder of the code did not need any modification other that ANSI
prototyping.  The function FreqAtBaryCenter() used to be the main()
function.  The main() function used to contain alot of code for 
calculating date and time from the dbase record.  Julian day is now
passed in, so I got rid of 2 support functions, xdatejd() and leapyear().
This code will be exported to sister seti projects (eg seti@home).  In
order to avoid name collisions, I prepended each function name with
"seti_dop_".
---------------------------------------------------------------------------
*/
//#define DEBUG
#include	<stdio.h>
#include	<string.h>
#include	<math.h>
#include	<ctype.h>
#include 	"setilib.h"

#define		CADKS	1731.456829	/* Converts AUDAY to KMSEC */
#define		C	299792.458	/* Km/sec Speed of Light */
#define		TWOPI	(2 * M_PI)
#define		STR	206264806.2470964
#define		RTD	57.295779513082321
#define		CDR	1.745329252e-2	/* Converts degrees to radians */
#define		R	1296000
#define		OBL	23.43929111	/* Obliquity of Ecliptic, Degrees, from
					23:26:21.448, for epoch J2000, I.A.U.
					(1976) */
#define		T0	2451545		/* J2000.0*/
#define		SECCON	206264.8062470964
#define		JAN_0_1600	2305446.5

double seti_dop_FreqOffsetAtBaryCenter(double DetectionFreq, double JulDay, double Ra, double Dec, 
                                    double Epoch, telescope_id ReceiverID) {

	return(-(DetectionFreq - seti_dop_FreqAtBaryCenter(DetectionFreq, JulDay,
                                                Ra, Dec,
                                                Epoch, ReceiverID)));
}

double seti_dop_FreqAtBaryCenter(double DetectionFreq, double JulDay, double Ra, double Dec, 
                              double Epoch, telescope_id ReceiverID) {

  double		csq = C * C;	 	// C**2 
  double		jd;		 	// Julian date 
  double		ut;		 	// Universal Time (UT1) 
  double		x[3];		 	// Unit vector pointing to object 
  double		ve[3], vb[3], vt[3]; 	// Velocity of Earth, Barycenter, Topo
  double		u[3];		 	// Total velocity of Observer 
  double		nu;		 	// Doppler shifted frequency (GHz) 
  double        	Elevation;
  int			k;		 	// Vector x, y, z component index 
  receiver_config 	ReceiverConfig;  	// Lat, Long, beamwidth, etc

  seti_cfg_GetReceiverConfig(ReceiverID, &ReceiverConfig);

#ifdef DEBUG
printf("ID = %ld  Name = %s  Lat = %lf  Lon = %lf  El = %lf  Dia = %lf  BW = %lf\n",
                ReceiverConfig.s4_id,
                ReceiverConfig.name,
                ReceiverConfig.latitude,
                ReceiverConfig.longitude,
                ReceiverConfig.elevation,
                ReceiverConfig.diameter,
                ReceiverConfig.beam_width);
#endif


   // We must get values into the units required by this code
   nu = DetectionFreq/1e6;				// nu in MHz	 
   if(JulDay - (int)JulDay >= 0.5) {			// jd is for 0.0hr, UT
        ut = (JulDay - (int)JulDay - 0.5) * 24.0;	// ut is an hour offset from jd
        jd = (int)JulDay + 0.5;
    } else {
        ut = (JulDay - (int)JulDay + 0.5) * 24.0;
        jd = (int)JulDay - 0.5;
    }
    Elevation = ReceiverConfig.elevation / 1000.0;	// elevation in Km

    // do calculations
    seti_dop_eqxyz(Ra, Dec, x);					// unit vector
    seti_dop_earthv(jd + ut / 24, ve);				// earth velocity vector
    seti_dop_baryv(jd + ut / 24, vb);				// barycenter velocity vector
    seti_dop_topov(seti_dop_lmst(jd, ut, -1.0*ReceiverConfig.longitude), 
                ReceiverConfig.latitude, Elevation, vt);  // observer diurnal motion
    for (k = 0; k < 3; k++)				// observer velocity vector
      u[k] = CADKS * (ve[k] - vb[k]) + vt[k];
    seti_dop_precess(seti_dop_jdtoje(jd), u, Epoch, u);			// precess to desired epoch
    for (k = 0; k < 3; k++)		 		// velocity of object as seen
      u[k] = -u[k];					//   from observer's frame 

#ifdef DEBUG
    fprintf(stderr, "jd = %lf  ut = %lf\n", jd, ut);
    fprintf(stderr, "x  = %lf %lf %lf\n", x[0], x[1], x[2]);
    fprintf(stderr, "ve = %lf %lf %lf\n", ve[0], ve[1], ve[2]);
    fprintf(stderr, "vb = %lf %lf %lf\n", vb[0], vb[1], vb[2]);
    fprintf(stderr, "vt = %lf %lf %lf\n", vt[0], vt[1], vt[2]);
    fprintf(stderr, "u  = %lf %lf %lf\n", u[0], u[1], u[2]);
    fprintf(stderr, "nu before = %lf\n", nu);
#endif

    nu = nu * sqrt(1 - seti_dop_dot(u, u) / csq) / (1 - seti_dop_dot(u, x) / C);  // find relativistic doppler shift

#ifdef DEBUG
    fprintf(stderr, "nu after = %lf\n", nu);
#endif

    return(nu*1e6);					// return barycentric frequency in Hz
}

/*******************************************************************************
                                    JDTOJE

 Convert Julian Date to Julian Epoch.
*******************************************************************************/

double		seti_dop_jdtoje(double jd)
  {
  return (2000 + (jd - 2451545) / 365.25);
  }

/*******************************************************************************

                                    EQXYZ

  Convert equatorial RA, DEC into X, Y, Z rectangular coordinates, with unit
  radius vector.  X axis points to Vernal Equinox, Y to 6 hours R.A., Z to
  the north celestial pole.  The coordinate system is right-handed.
*******************************************************************************/

void		seti_dop_eqxyz(double ra, double dec, double * xyz)
  {
  double	alpha, delta;		/* Radian equivalents of RA, DEC */
  double	cdelta;

  alpha = M_PI * ra / 12;		/* Convert to radians.*/
  delta = M_PI * dec / 180;
  cdelta = cos(delta);
  xyz[0] = cdelta * cos(alpha);
  xyz[1] = cdelta * sin(alpha);
  xyz[2] = sin(delta);
  }

/*************************************************************************
Programmer's note:
The following was converted from file xyzeq.pas of the effie system.

                         XYZEQ

Convert x,y,z to equatorial coordinates Ra, Dec. It is assumed that
x,y,z are equatorial rectangular coordinates with z pointing north, x
to the vernal equinox, ans y toward 6 hours Ra.
**************************************************************************/

void seti_dop_xyzeq(double xyz[], double *ra, double *dec)
  {
  double r,rho,x,y,z;
  x = xyz[0];
  y = xyz[1];
  z = xyz[2];
  rho = sqrt(x * x + y * y);
  r = sqrt(rho * rho + z * z);
  if (r < 1.0e-20)
    {
    *ra = 0.0;
    *dec = 0.0;
    }
  else
    {
    *ra = Atan2 (y,x) * 12.0 / M_PI;
    if (fabs(rho) > fabs(z))
      *dec = atan(z/rho) * 180.0 / M_PI;
    else
      {
      if (z > 0.0)
        *dec = 90.0 - atan(rho/z) * 180.0 / M_PI;
      else
        *dec = -90.0 - atan(rho/z) * 180.0 / M_PI;
      }
    }
  }

/*******************************************************************************
                                    EARTHV

  Find the heliocentric rectangular components of Earth's velocity.  Use
  EARTHXYZ.
*******************************************************************************/

void		seti_dop_earthv(double jd, double *  v)
  {
  double	dt;		/* Time interval over which to evaluate V */
  double	x1[3], x2[3];	/* Position of Earth at times T1, T2 */
  int		k;		/* X, Y, or Z component */
  //void		earthxyz();

  dt = 0.2;
  seti_dop_earthxyz(jd - dt / 2, x1);
  seti_dop_earthxyz(jd + dt / 2, x2);
  for (k = 0; k < 3; k++)
    v[k] = (x2[k] - x1[k]) / dt;
  }

/*******************************************************************************
                                   EARTHXYZ
   Calculate X, Y, Z for Earth.
   Adapted from George Kaplan's "APPLE" programs, U.S. Naval Observatory.
   Modified form of Newcomb's theory (Tables of the Sun, 1898).  Only the
   largest periodic perturbations are evaluated, and Van Flandern's
   expressions for the fundamental arguments (Improved Mean Elements for the 
   Earth and Moon (1981)) are used.  The absolute accuracy is of the order
   1" for epochs near J2000.  Kaplan's program in turn was adapted
   from program "IAUSUN" by P.M. Janiczek, U.S.N.O.

   Position is with respect to mean ecliptic and equator of date.
*******************************************************************************/

void		seti_dop_earthxyz(double dj, double * xyz)
  {
                        /* TABLES OF THE PERTURBATIONS */
  static int	x[46][8] =
    /* PERTURBATIONS BY VENUS */
    /*            J   I     VC    VS   RHOC  RHOS     BC    BS */
               {{-1,  0,    33,  -67,   -85,  -39,    24,  -17},
                {-1,  1,  2353,-4228, -2062,-1146,    -4,    3},
                {-1,  2,   -65,  -34,    68,  -14,     6,  -92},
                {-2,  1,   -99,   60,    84,  136,    23,   -3},
                {-2,  2, -4702, 2903,  3593, 5822,    10,   -6},
                {-2,  3,  1795,-1737,  -596, -632,    37,  -56},
                {-3,  3,  -666,   27,    44, 1044,     8,    1},
                {-3,  4,  1508, -397,  -381,-1448,   185, -100},
                {-3,  5,   763, -684,   126,  148,     6,   -3},
                {-4,  4,  -188,  -93,  -166,  337,     0,    0},
                {-4,  5,  -139,  -38,   -51,  189,   -31,   -1},
                {-4,  6,   146,  -42,   -25,  -91,    12,    0},
                {-5,  5,   -47,  -69,  -134,   93,     0,    0},
                {-5,  7,  -119,  -33,   -37,  136,   -18,   -6},
                {-5,  8,   154,    0,     0,  -26,     0,    0},
                {-6,  6,    -4,  -38,   -80,    8,     0,    0},
    /* PERTURBATIONS BY MARS */
    /*            J   I      V    VS   RHOC  RHOS      B    BS */
                { 1, -1,  -216, -167,   -92,  119,     0,    0},
                { 2, -2,  1963, -567,  -573,-1976,     0,   -8},
                { 2, -1, -1659, -617,    64, -137,     0,    0},
                { 3, -3,    53, -118,  -154,  -67,     0,    0},
                { 3, -2,   396, -153,   -77, -201,     0,    0},
                { 4, -3,  -131,  483,   461,  125,     7,    1},
                { 4, -2,   526, -256,    43,   96,     0,    0},
                { 5, -4,    49,   69,    87,  -62,     0,    0},
                { 5, -3,   -38,  200,    87,   17,     0,    0},
                { 6, -4,  -104, -113,  -102,   94,     0,    0},
                { 6, -3,   -11,  100,   -27,   -4,     0,    0},
                { 7, -4,   -78,  -72,   -26,   28,     0,    0},
                { 9, -5,    60,  -15,    -4,  -17,     0,    0},
                {15, -8,   200,  -30,    -1,   -6,     0,    0},
    /* PERTURBATIONS BY JUPITER */
    /*            J   I     VC    VS   RHOC  RHOS     BC    BS */
                { 1, -2,  -155,  -52,   -78,  193,     7,    0},
                { 1, -1, -7208,   59,    56, 7067,    -1,   17},
                { 1,  0,  -307,-2582,   227,  -89,    16,    0},
                { 1,  1,     8,  -73,    79,    9,     1,   23},
                { 2, -3,    11,   68,   102,  -17,     0,    0},
                { 2, -2,   136, 2728,  4021, -203,     0,    0},
                { 2, -1,  -537, 1518,  1376,  486,    13,  166},
                { 3, -3,  -162,   27,    43,  278,     0,    0},
                { 3, -2,    71,  551,   796, -104,     6,   -1},
                { 3, -1,   -31,  208,   172,   26,     1,   18},
                { 4, -3,   -43,    9,    13,   73,     0,    0},
                { 4, -2,    17,   78,   110,  -24,     0,    0},
    /* PERTURBATIONS BY SATURN */
    /*            J   I     VC    VS   RHOC  RHOS     BC    BS */
                { 1, -1,   -77,  412,   422,   79,     1,    6},
                { 1,  0,    -3, -320,     8,   -1,     0,    0},
                { 2, -2,    38, -101,  -152,  -57,     0,    0},
                { 2, -1,    45, -103,  -103,  -44,     0,    0}};

  double	t;	/*Time, centuries, from 1900 Jan 0*/
  double	tp;	/*Time, years, from 1850 Jan 0*/
  double	t20;	/*Time, centuries, from 2000 Jan 0*/
  double	lu;	/*Unperturbed mean longitude, radians*/
  double	m;	/*Mean anomaly, affected by long per. perturb., rads*/
  double	r0;	/*Unperturbed radius, AU*/
  double	logr0;	/*log (R0)*/
  double	v;	/*Equation of the center, radians*/
  double	epsilon; /*Mean obliquity of the ecliptic, radians*/
  double	lm;	/*Mean longitude of Moon, radians*/
  double	mm;	/*Mean anomaly of Moon, radians*/
  double	um;	/*Lunar mean argument of latitude, radians*/
  double	omegam;	/*Mean longitude of lunar ascending node, radians*/
  double	gammam;	/*Mean longitude of Moon's perigee, radians*/
  double	rp;	/*Perturbed radius vector, AU*/
  double	lambda;	/*Ecliptic long. WRT mean ecliptic & equinox of date*/
  double	beta;	/*Ecliptic lat.   "    "      "    "     "    "   " */
  double	gv, gm, gj, gs; /*Perturbing mean anomalies of Venus, Mars,
			 Jup., Sat.*/
  double	dl;	/*Perturbation in longitude, milliarcsec*/
  double	dr;	/*Perturbation in radius, units of 10**-9 AU*/
  double	db;	/*Perturbation in latitude, milliarcsec*/
  double	dg;	/*Perturbation in mean anomaly, milliarcsec*/
  double	d;	/*Mean elongation of Moon from the Sun*/
  double	sino, coso; /*Sine, cosine of obliquity*/
  double	sinl, cosl; /*Sine, cosine of Longitude*/
  double	sinb, cosb; /*Sine, cosine of Latitude*/
  double	arg;	/*Argument for computation of perturbations; anomalies*/
  double	ss, cs;	/*Sine, cosine of ARG*/
  int		k;	/*Index for perturbation tables*/
  //double	modreal();

    /* PART II   NECESSARY PRELIMINARIES */
  t  = (dj - 2415020) / 36525;
  tp = (dj - 2396758) / 365.25;
  t20= (dj - 2451545) / 36525;


    /* PART III  COMPUTATION OF ELLIPTIC ELEMENTS AND SECULAR TERMS*/

    /* VAN FLANDERN'S EXPRESSIONS FOR MEAN ELEMENTS */

  lu = 1009677.850 + (100 * R + 2771.27 + 1.089 * t20) * t20;
  lu = seti_dop_modreal(lu * 1000 / STR, (double) TWOPI);
  m = 1287099.804 + (99 * R + 1292581.224 + (-0.577 - 0.012 * t20) * t20) * t20;
  m = seti_dop_modreal(m * 1000 / STR, (double) TWOPI);
  epsilon = 84381.448 + (-46.8150 + (-0.00059 + 0.001813 * t20) * t20) * t20;
  epsilon *= 1000 / STR;


    /* PART IV   LUNAR TERMS */

    /* VAN FLANDERN'S EXPRESSIONS FOR MEAN ELEMENTS */

  lm = 785939.157 + (1336 * R + 1108372.598 +
    (-5.802 + 0.019 * t20) * t20) * t20;
  lm = seti_dop_modreal(lm * 1000 / STR, (double) TWOPI);
  omegam = 450160.280 + (-5 * R - 482890.539 +
    (7.455 + 0.008 * t20) * t20) * t20;
  omegam = seti_dop_modreal(omegam * 1000 / STR, (double) TWOPI);
  gammam = 300072.424 + (11 * R + 392449.965 +
    (-37.112 - 0.045 * t20) * t20) * t20;
  gammam = seti_dop_modreal(gammam * 1000 / STR, (double) TWOPI);

    /* DERIVED ARGUMENTS */
  mm = lm - gammam;
  um = lm - omegam;
  mm = seti_dop_modreal(mm, (double) TWOPI);
  um = seti_dop_modreal(um, (double) TWOPI);

    /* MEAN ELONGATION */
  d = lm - lu;

    /* COMBINATIONS OF ARGUMENTS AND THE PERTURBATIONS */
  d = seti_dop_modreal(d, (double) TWOPI);
  arg = d;
  dl =  6469 * sin(arg) +  13 * sin(3 * arg);
  dr = 13390 * cos(arg) +  30 * cos(3 * arg);

  arg = d + mm;
  arg = seti_dop_modreal(arg, (double) TWOPI);
  dl +=  177 * sin(arg);
  dr +=  370 * cos(arg);

  arg = d - mm;
  arg = seti_dop_modreal(arg, (double) TWOPI);
  dl -=  424 * sin(arg);
  dr -= 1330 * cos(arg);

  arg = 3. * d - mm;
  arg = seti_dop_modreal(arg, (double) TWOPI);
  dl +=   39 * sin(arg);
  dr +=   80 * cos(arg);

  arg = d + m;
  arg = seti_dop_modreal(arg, (double) TWOPI);
  dl -=   64 * sin(arg);
  dr -=  140 * cos(arg);

  arg = d - m;
  arg = seti_dop_modreal(arg, (double) TWOPI);
  dl +=  172 * sin(arg);
  dr +=  360 * cos(arg);

  um = seti_dop_modreal(um, (double) TWOPI);
  arg = um;
  db =   576 * sin(arg);

    /* PART V    COMPUTATION OF PERIODIC PERTURBATIONS */

    /* THE PERTURBING MEAN ANOMALIES */

  gv  = 0.19984020e+1 + 0.1021322923e+2 * tp;
  gm  = 0.19173489e+1 + 0.3340556174e+1 * tp;
  gj  = 0.25836283e+1 + 0.5296346478e+0 * tp;
  gs  = 0.49692316e+1 + 0.2132432808e+0 * tp;
  gv  = seti_dop_modreal(gv, (double) TWOPI);
  gm  = seti_dop_modreal(gm, (double) TWOPI);
  gj  = seti_dop_modreal(gj, (double) TWOPI);
  gs  = seti_dop_modreal(gs, (double) TWOPI);


    /* MODIFICATION OF FUNDAMENTAL ARGUMENTS */

    /* APPLICATION OF THE JUPITER-SATURN GREAT INEQUALITY
      TO JUPITER'S MEAN ANOMALY */

  gj = gj + 0.579904067e-2 * sin(5 * gs - 2 * gj +
    1.1719644977 - 0.397401726e-3 * tp);
  gj = seti_dop_modreal(gj, (double) TWOPI);

    /* LONG PERIOD PERTURBATIONS OF MEAN ANOMALY */

    /*            ARGUMENT IS ( 4 MARS - 7 EARTH + 3 VENUS ) */
  dg = 266 * sin(0.555015 + 2.076942 * t) +
    /*            ARGUMENT IS ( 3 JUPITER - 8 MARS + 4 EARTH ) */
    6400 * sin(4.035027 + 0.3525565 * t) +
    /*            ARGUMENT IS ( 13 EARTH - 8 VENUS ) */
    (1882 - 16 * t) * sin(0.9990265 + 2.622706 * t);


    /* COMPUTATION OF THE EQUATION OF THE CENTER */

    /* FORM PERTURBED MEAN ANOMALY */

  m += dg / STR;
  m = seti_dop_modreal(m, (double) TWOPI);
  v =        sin(    m) * (6910057   - (17240 + 52 * t) * t) +
             sin(2 * m) * (  72338   -    361 * t) +
             sin(3 * m) * (   1054   -      1 * t);

    /* THE UNPERTURBED RADIUS VECTOR */

  logr0     =                30570   -    150 * t -
             cos(    m) * (7274120   - (18140 + 50 * t) * t) -
             cos(2 * m) * (  91380   -    460 * t) -
             cos(3 * m) * (   1450   -     10 * t);
  r0 = pow((double) 10, 1e-9 * logr0);

    /* SELECTED PLANETARY PERTURBATIONS FROM NEWCOMB'S THEORY FOLLOW */

    /* PERTURBATIONS BY VENUS */
  for (k = 0; k < 16; k++)
    {
    /* ARGUMENT J * VENUS +   I * EARTH */
    arg = x[k][0] * gv + x[k][1] * m;
    arg = seti_dop_modreal(arg, (double) TWOPI);
    cs  = cos(arg);
    ss  = sin(arg);
    dl  += (x[k][2] * cs  + x[k][3] * ss);
    dr  += (x[k][4] * cs  + x[k][5] * ss);
    db  += (x[k][6] * cs  + x[k][7] * ss);
    }

    /* PERTURBATIONS BY MARS */
  for ( ; k < 30; k++)
    {
    /* ARGUMENT  J * MARS +   I * EARTH */
    arg = x[k][0] * gm + x[k][1] * m;
    arg = seti_dop_modreal(arg, (double) TWOPI);
    cs  = cos(arg);
    ss  = sin(arg);
    dl  += (x[k][2] * cs  + x[k][3] * ss);
    dr  += (x[k][4] * cs  + x[k][5] * ss);
    db  += (x[k][6] * cs  + x[k][7] * ss);
    }

    /* PERTURBATIONS BY JUPITER */
  for ( ; k < 42; k++)
    {
    /* ARGUMENT J*JUPITER +   I * EARTH */
    arg = x[k][0] * gj + x[k][1] * m;
    arg = seti_dop_modreal(arg, (double) TWOPI);
    cs  = cos(arg);
    ss  = sin(arg);
    dl  += (x[k][2] * cs  + x[k][3] * ss);
    dr  += (x[k][4] * cs  + x[k][5] * ss);
    db  += (x[k][6] * cs  + x[k][7] * ss);
    }

    /* PERTURBATIONS BY SATURN */
  for ( ; k < 46; k++)
    {
    /* ARGUMENT J*SATURN  +   I * EARTH */
    arg = x[k][0] * gs + x[k][1] * m;
    arg = seti_dop_modreal(arg, (double) TWOPI);
    cs  = cos(arg);
    ss  = sin(arg);
    dl  += (x[k][2] * cs  + x[k][3] * ss);
    dr  += (x[k][4] * cs  + x[k][5] * ss);
    db  += (x[k][6] * cs  + x[k][7] * ss);
    }


    /* PART VI   COMPUTATION OF ECLIPTIC AND EQUATORIAL COORDINATES */

  rp = r0 * pow((double) 10, 1e-9 * dr);
  lambda = (dl + dg + v) / STR + lu;
  lambda = seti_dop_modreal(lambda, (double) TWOPI);
  beta = db / STR;
  sino = sin(epsilon);
  coso = cos(epsilon);
  sinl = sin(lambda);
  cosl = cos(lambda);
  sinb = sin(beta);
  cosb = cos(beta);
  xyz[0] = -rp * (cosb * cosl);
  xyz[1] = -rp * (cosb * sinl * coso - sinb * sino);
  xyz[2] = -rp * (cosb * sinl * sino + sinb * coso);
                                        /* "-" because we are computing
                                        for Earth, not the Sun*/
  }

/*******************************************************************************
                                     BARYV

  Compute the velocity of the Solar System Barycenter with respect to the Sun.
  See BARYXYZ for other comments.
*******************************************************************************/

void		seti_dop_baryv(double jd, double * vb)
  {
static double
/* Planet:   Jupiter     Saturn      Uranus      Neptune                    */
  pm[] =  {  1047.355,   3498.5,    22869,      19314    },/*Reciprocal Mass*/
  pa[] =  {  5.202803,   9.538843,  19.181951,  30.057779},/*Semimajor Axis*/
  pl[] =  {  0.5999,     0.8728,    5.4714,     5.3269   },/*Longitude at J0*/
  pn[] =  {1.450216e-3,5.839824e-4,2.047627e-4,1.04390e-4};/*Mean daily motion*/

  double	sine, cose;		/* Sine, cosine of obliquity */
  double	l;			/* Longitude, radians */
  double	sinl, cosl;		/* Sine, cosine of longitude */
  double	tmass;			/* Total mass of Sun + All planets */
  double	f;			/* Weighting factor for position, vel.*/
  int		p;			/* Planet */
  double	xdot, ydot, zdot;	/* Rect components of planet's vel.*/
  double	t;			/* Days since J0 */
  //double	jdtoje(), modreal();
  //void		precess();

  t = jd - T0;
  tmass = 1;
  for (p = 5; p <= 8; p++)
    tmass += 1 / pm[p - 5];
  sine = sin(OBL * TWOPI / 360);
  cose = cos(OBL * TWOPI / 360);
  vb[0] = 0;
  vb[1] = 0;
  vb[2] = 0;
  for (p = 5; p <= 8; p++)
    {
    l = pl[p - 5] + t * pn[p - 5];
    l = seti_dop_modreal(l, (double) TWOPI);
    sinl = sin(l);
    cosl = cos(l);
    xdot = -pa[p - 5] * pn[p - 5] * sinl;
    ydot =  pa[p - 5] * pn[p - 5] * cosl * cose;
    zdot =  pa[p - 5] * pn[p - 5] * cosl * sine;
    f = 1 / (pm[p - 5] * tmass);
    vb[0] += xdot * f;
    vb[1] += ydot * f;
    vb[2] += zdot * f;
    }
  seti_dop_precess((double) 2000, vb, seti_dop_jdtoje(jd), vb);
  }

/*******************************************************************************
                                     TOPOV

  Calculate rectangular components of observer's diurnal motion.
*******************************************************************************/

void		seti_dop_topov(double lst, double nlat, double elev, double * v)
  {
  double	phi, phi1;	/* Geographic, geocentric latitude, radians */
  double	ae;		/* Equatorial radius of the Earth */
  double	f;		/* Flattening factor for the Earth */
  double	theta;		/* Right ascension of V, radians */
  double	r;		/* Distance from center of the Earth */
  double	vm;		/* Magnitude of velocity */
  double	omega;		/* Angular velocity of rotation, rad./sec */
  double	cphi;		/* cosine (PHI) */
  double	ctheta, stheta;	/* cosine, sine (THETA) */

  ae = 6378.140;		/* A.A., 1986, p. K6 */
  f = 1 / 298.257;		/* " "    "       "  */
  phi = CDR * nlat;
  phi1 = atan((1 - f * f) * tan(phi));
  theta = 15 * CDR * (lst + 6);
  cphi = cos(phi1);
  ctheta = cos(theta);
  stheta = sin(theta);
  r = ae + elev;
  omega = 1.0027379094 * TWOPI / 86400; /*Ang. vel. of Earth, J2000*/
  vm = r * omega;
  v[0] = vm * cphi * ctheta;
  v[1] = vm * cphi * stheta;
  v[2] = 0;
  }

/*******************************************************************************
                                      LMST

  Compute local mean sidereal time.  Note:  the Julian date J should be given
  as an integer plus 0.5, representing JD for 0h U.T.
*******************************************************************************/

double		seti_dop_lmst(double j, double ut, double wlong)
  {
  double	tu;		/* Centuries of U.T. since J2000 */
  double	ratio;		/* Ratio of solar day / sidereal day */
  double	lstime;		/* Local Sidereal Time, hours */
  //double	gmst(), modreal();

  tu = (j - T0) / 36525;
  ratio = 1.002737909350795 + 5.9006e-11 * tu - 5.9e-15 * tu * tu;
  lstime = seti_dop_gmst(j) + ratio * ut - wlong / 15;

//fprintf(stderr, "JulDay = %lf  j = %lf  ut = %lf  lmst = %lf\n", JulDay, j, ut, modreal(lstime, (double) 24));
  return (seti_dop_modreal(lstime, (double) 24));
  }

/*******************************************************************************
                                      GMST

  Compute Greenwich mean sidereal time for 0 hours U.T. on a given date.
  Date must be expressed as an integral Julian day number + 0.5.  From the
  Astronomical Almanac, U.S.N.O.
*******************************************************************************/

double		seti_dop_gmst(double j)
  {
  double		tu;		/* Centuries since epoch J2000.0 */
  double		gmstime;	/* Greenwich Mean Sidereal Time, hrs */

  tu = (j - T0) / 36525;
  gmstime = (24110.54841 + 8640184.812866 * tu + 0.093104 * tu * tu -
    6.2e-6 * tu * tu * tu) / 86400;

  return (24 * seti_dop_modreal(gmstime, (double) 1));
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

void		seti_dop_precess(double e1, double * pos1, double e2, double * pos2)
  {
  double	tjd1;	/* Epoch1, expressed as a Julian date */
  double	tjd2;	/* Epoch2, expressed as a Julian date */
  double	t0;	/* Lieske's T (time in cents from Epoch1 to J2000) */
  double	t;	/* Lieske's t (time in cents from Epoch1 to Epoch2) */
  double	t02;	/* T0 ** 2 */
  double	t2;	/* T ** 2 */
  double	t3;	/* T ** 3 */
  double	zeta0;	/* Lieske's Zeta[A] */
  double	zee;	/* Lieske's Z[A] */
  double	theta;	/* Lieske's Theta[A] */
                        /* The above three angles define the 3 rotations
                        required to effect the coordinate transformation. */
  double	czeta0;	/* cos (Zeta0) */
  double	szeta0;	/* sin (Zeta0) */
  double	czee;	/* cos (Z) */
  double	szee;	/* sin (Z) */
  double	ctheta;	/* cos (Theta) */
  double	stheta;	/* sin (Theta) */
  double	xx,yx,zx, xy,yy,zy, xz,yz,zz;
			/* Matrix elements for the transformation. */
  double	vin[3];	/* Copy of input vector */
  //double	jetojd();

  vin[0] = pos1[0];
  vin[1] = pos1[1];
  vin[2] = pos1[2];
  tjd1 = seti_dop_jetojd(e1);
  tjd2 = seti_dop_jetojd(e2);
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

/*******************************************************************************
                                    JETOJD

  Convert Julian Epoch to a Julian Date.
*******************************************************************************/

double		seti_dop_jetojd(double je)
  {
  return (365.25 * (je - 2000) + 2451545);
  }

/*******************************************************************************

                                    MODREAL

  Real-valued modulo function.
*******************************************************************************/

double		seti_dop_modreal(double x, double m)
  {
  int		y;
  double	r;
  double	modr;
  int		lambda;
  double	epsilon;

  epsilon = 1e-15 * m;
  if (fabs(x / m) <= 32767)
    {
    y = int(x / m);
    modr = x - m * y;
    }
  else
    {
    r = 32767 * m;
    lambda = int(x / r);
    y = int((x - lambda * r) / m);
    modr = x - lambda * r - m * y;
    }
  if (modr < 0)
    modr += m;
  if (modr > m - epsilon)
    modr = 0;

  return modr;
  }

/*******************************************************************************

                                       DOT

  Take the inner ("dot") product between two vectors.
*******************************************************************************/

double		seti_dop_dot(double * x1, double * x2)
  {
  return (x1[0] * x2[0] + x1[1] * x2[1] + x1[2] * x2[2]);
  }

