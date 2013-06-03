/* ------------------------------------------------------------------
 Author : Jeff Cobb
 source name: seti_doppler.h
 Contains prototypes for functions in seti_doppler.cc

 Note: Epoch here is the epoch of the coordinates passed to the
       routines.
---------------------------------------------------------------------
*/

double      seti_dop_FreqOffsetAtBaryCenter(double DetectionFreq, double JulDay, double Ra, double Dec,
        double Epoch, telescope_id ReceiverID);
double          seti_dop_FreqAtBaryCenter(double DetectionFreq, double JulDay, double Ra, double Dec,
        double Epoch, telescope_id ReceiverID);
double          seti_dop_jdtoje(double jd);
void            seti_dop_eqxyz(double ra, double dec, double *xyz);
void        seti_dop_xyzeq(double xyz[], double *ra, double *dec);
void            seti_dop_earthv(double jd, double   *v);
void            seti_dop_earthxyz(double dj, double *xyz);
void            seti_dop_baryv(double jd, double *vb);
void            seti_dop_topov(double lst, double nlat, double elev, double *v);
double          seti_dop_lmst(double j, double ut, double wlong);
double          seti_dop_gmst(double j);
void            seti_dop_precess(double e1, double *pos1, double e2, double *pos2);
double          seti_dop_jetojd(double je);
double          seti_dop_modreal(double x, double m);
double          seti_dop_dot(double *x1, double *x2);
