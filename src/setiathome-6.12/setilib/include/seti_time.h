//____________________________________________________________ 
// Source Code Name  : seti_time.cc
// Programmer        : Donnelly/Cobb et al
//                   : Project SERENDIP, University of California, Berkeley CA
// Compiler          : GNU gcc
// Date Opened       : 10/94
// Purpose           : header file for seti_time.cc
//____________________________________________________________

#include <string>


#define TIME_DATA_DIRECTORY "/home/seti/ao_dat_files"
#define TAI_UTC_CONV TIME_DATA_DIRECTORY"/tai-utc.dat"
#define UT1_UTC_CONV TIME_DATA_DIRECTORY"/finals.all"

class seconds {
public:
  double t;
  explicit seconds(double d=1) : t(d) {};
  seconds(const seconds &s) : t(s.t) {} ;
  seconds &operator =(const seconds &a) {
    if (this != &a) t=a.t;
    return *this;
  }
  seconds operator +(const seconds &a) const {
    seconds rv(t + a.t);
    return rv;
  }
  seconds operator -(const seconds &a) const {
    seconds rv(t - a.t);
    return rv;
  }
  seconds operator *(const double &a) const {
    seconds rv(t * a);
    return rv;
  }
  double operator /(const seconds &a) const {
    double rv(t / a.t);
    return rv;
  }
  seconds operator /(const double &a) const {
    seconds rv(t / a);
    return rv;
  }
  seconds &operator +=(const seconds &a) {
    return *this=*this+a;
  }
  seconds &operator -=(const seconds &a) {
    return *this=*this-a;
  }
  bool operator >(const seconds &a) {
    return (*this-a).val()>0;
  }
  double val() const { return t; };  // returns the value in seconds
  double uval() const { return t; }; // returns the value in units for 
                                     // which the class is named
};

static inline seconds operator *(const double &a, const seconds &b) {
  return b*a;
}

class minutes : public seconds {
public:
  explicit minutes(double d=1) : seconds(d*60) {} ;
  minutes(const seconds &s) : seconds(s) {} ;
  double uval() const { return t/60; };
};

class hours : public seconds {
public:
  explicit hours(double d=1) : seconds(d*3600) {} ;
  hours(const seconds &s) : seconds(s) {} ;
  double uval() const { return t/3600; };
};

class days : public seconds {
public:
  explicit days(double d=1) : seconds(d*86400) {} ;
  days(const seconds &s) : seconds(s) {} ;
  double uval() const { return t/86400; };
};

static const days JD1970(2440587.5);    /* Time at 0:00 GMT 1970 Jan 1 */
static const days J2000(2451545.0);     /* Julian epoch 2000.0 */
static const days B1900(2415020.31352); /* Besselian epoch 1900.0 */
static const days E1950(2433283.5);     /* Ephemeris epoch 1950.0 */
static const days MJD0(2400000.5);      /* Modified Julian Day 0.0 */
static const days TJD0(2440000.5);      /* Truncated Julian Day 0.0 */
static const days JD0(0.0);             /* Julian Day 0.0 */
static const days gregorian_year(365.2425);       /* Gregorian year in days */
static const days julian_year(365.25);            /* Julian year in days */
static const days besselian_year(365.2421987817); /* Besselian year in days */
static const days sidereal_year(besselian_year);  /* Siderial year in days */
static const days B1950(B1900+50*besselian_year);  /* Besselian epoch 1950 */
static const days B2000(B1900+100*besselian_year); /* Besselian epoch 2000 */

class gregorian_years : public seconds {
public:
  explicit gregorian_years(double d=1) : seconds(d*gregorian_year) {} ;
  gregorian_years(const seconds &s) : seconds(s) {} ;
  double uval() const { return *this/gregorian_year; }
};

class besselian_years : public seconds {
public:
  explicit besselian_years(double d=1) : seconds(d*besselian_year) {} ;
  besselian_years(const seconds &s) : seconds(s) {} ;
  double uval() const { return *this/besselian_year; }
};

class julian_years : public seconds {
public:
  explicit julian_years(double d=1) : seconds(d*julian_year) {} ;
  julian_years(const seconds &s) : seconds(s) {} ;
  double uval() const { return *this/julian_year; }
};

class seti_time {
public:
  typedef enum {
    AST, // Arecibo Standard Time
    UTC, // Coordinated Universal Time
    UT1, // Universal Time
    UT2, // Smoothed Universal Time
    TT,  // Terrestrial Time
    T,   // Besselian date in days
    TAI, // International Atomic Time
    GPS  // GPS Time Base
  } base_t;
private:
  // Time is held internally as the sum of (ultimately) 2 double
  // precision floating point numbers.  The unit (ultimately) is
  // seconds.  
  days epoch_val;  // epoch of the julian date (always starts at midnight)  
  days jd_val;     // days after the epoch (UTC)
  base_t base_val;
public:
  // construct seti_time default (MJD=0 UTC) 
  seti_time() : epoch_val(MJD0), jd_val(0), base_val(UTC) {};
  // construct seti_time from julian date
  explicit seti_time(const days &jd, const days &e=JD0, base_t base=UTC) : 
    epoch_val(days(floor(e.uval())+0.5)), jd_val(jd+(e-epoch_val)),
    base_val(base) {};
  // construct seti_time from unix time_t
  explicit seti_time(time_t t0, double f0=0.0) : 
    epoch_val(JD1970), jd_val(seconds((double)t0+f0)), base_val(UTC) {};
  // construct seti_time from AST
  explicit seti_time(char *AST);
  // copy constructor
  seti_time(const seti_time &a) :
    epoch_val(a.epoch_val), jd_val(a.jd_val), base_val(a.base_val) {};

  // assignment operator
  seti_time &operator =(const seti_time &a) {
    if (this != &a) {
      epoch_val=a.epoch_val;
      jd_val=a.jd_val;
      base_val=a.base_val;
    }
    return *this;
  }

  // subtraction acting on seti_times returns seconds
  seconds operator -(const seti_time &a) const {
    seti_time temp(a);
    if (temp.base_val != base_val) {
      temp.base(base_val);
    }
    return (jd_val-temp.jd_val)+(epoch_val-temp.epoch_val);
  }

  // subtraction acting on seconds returns seti_time
  seti_time operator -(const seconds &s) const {
    return seti_time(jd_val-s,epoch_val,base_val);
  }

  // adding two seti_times doesn't make since does it?  Skip that one.

  // adding seconds to seti_time returns seti_time
  seti_time operator +(const seconds &s) const {
    return seti_time(jd_val+s,epoch_val,base_val);
  }

  seti_time &operator +=(const seconds &s) {
    *this=*this+s;
    return *this;
  }
  seti_time &operator -=(const seconds &s) {
    *this=*this-s;
    return *this;
  }

  // less than operator
  bool operator <(const seti_time &b) const {
    return ((b-*this).uval() > 0);
  }

  // modify the epoch of the seti_time
  days epoch(const days &e);
  // return the epoch of the seti_time
  const days &epoch() const {
    return epoch_val;
  }

  // modify the base of the seti_time
  base_t base(const base_t &b);
  // return the base of a seti_time
  const base_t &base() const {
    return base_val;
  }

  // conversion routines
  seconds ast_utc_diff() const;
  seconds t_utc_diff() const;
  seconds tt_utc_diff() const;
  seconds tai_utc_diff() const;
  seconds ut1_utc_diff() const;  
  seconds ut2_utc_diff() const;
  seconds gps_utc_diff() const;

  // returns time past epoch in various units
  seconds sec() const {return jd_val; };
  hours hrs() const {return jd_val; };
  days dys() const {return jd_val; };

  // returns time of day in hours past midnight for various time bases.
  hours ast() const;
  hours utc() const; 
  hours ut1() const;
  hours ut2() const;
  hours tt() const;
  hours tai() const;
  hours gps() const;

  // returns julian dates.
  days mjd() const; 
  days mjd(const days &j);  // sets date and epoch to the specified mjd
  days jd() const;
  days jd(const days &j);   // sets date and epoch to the specified mjd
  double  julian() const;   // retunrs a standard floating point julian day

  // printing methods
  void print_internals();   // print the internal values - helpful in debugging
  std::string printjd() const;

};  

  


// replacing the old SunOS function....
#define dysize(x) (x%4==0)&&((x%100!=0)||(x%400==0)) ? 366 : 365

#define SECS_TO_DAYS        ((double)1 / (double)86400)
#define SECDIFF(Jd1, Jd2) ((double)(fabs(Jd1 - Jd2) * 86400))

struct DateStruct
       { 
       long month;
       long day;
       long year;
       };

// stMonths[] is defined with data in seti_time.cc
#ifndef S4TIME
extern char * stMonths[]; 
#endif

void tm_HmsToDec(char *time_str,double *dec_time);
void tm_s3HmsToDec(char *time_str,double *dec_time);
long tm_MonToI(char *fdate);
void tm_DayToMon(long day, long year, struct DateStruct *sdate);
long tm_DayMonToYrDay(long day, long mon, long year);
void tm_AstToUt(long *year, long *day, long *hour);
void tm_AstToUt(long *year, long *day, double *hour);
double tm_DbasToLst(char *aststr, char *dbasdate);
double tm_UtToLst(double ddtime, double longitude,
                  long mon, long day, long year);
double tm_ToJul(long month, long day, long year, double hour);
void tm_FromJul(double jday, long *month, long *day, long *year, double *hour);
void tm_DateToI(char *date, long *day, long *month, long *year);
long tm_CompareDate( long *date1mon, long *date1day, long *date1year,
                     long *date2mon, long *date2day, long *date2year);
long tm_CentYear(long TwoDigitYear);
long tm_TwoDigitYear(long CentYear);
void tm_Converts4Time(long u32Time, long *DOY, long *Hour, long *Min, long *Sec,
                      long *Ten);
double tm_JulianEpochToJulianDate(double je);
double tm_JulianDateToJulianEpoch(double jd);

