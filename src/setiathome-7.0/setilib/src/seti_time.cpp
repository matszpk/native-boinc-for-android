/*
 * Source Code Name  : seti_time.cc
 * Programmer        : Donnelly/Cobb et al
 *                   : Project SERENDIP, University of California, Berkeley CA
 * Compiler          : GNU gcc
 * time functions for serendip 4 off line programs
 * -----------------------------------------------------------------------------
 * modification history:
 *   converted to s4  by chuck 8/94
 *   -brendan 12/94 - added tm_s3HmsToDec() function
 *   -brendan 03/95 - changed int variables and function types to long
 *   -scott   06/95 - added tm_CompareDate() function
 *   -jeffc   12/97 - changed tm_AstToUt() to make day go to 1 on year
 *                    wrap.  It was going to 0.  (Why???)
 *                    Cloned tm_AstToUt() to to make a version that handles
 *                    a double hour.
 *                    Added 2 routines to transform betwee 2 and 4 digit
 *                    years: tm_TwoDigitYear() and tm_CentYear(). These
 *                    good in terms of Y2K until 2090.
 *   -jeffc   6/98    Added month table stMonths[].
 *   -mattl   8/98    Fixed discrepancies in month abbreviations in tm_MonToI()
 * -----------------------------------------------------------------------------
 */

#define  S4TIME

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "setilib.h"


// Month abbreviations indexed by month number (xx is a dummy for
// month 0)
const char *stMonths[13] = {"xx",
        "ja", "fe", "mr", "ap", "my", "jn",
        "jl", "au", "se", "oc", "no", "dc"
                           };
// Conversions we'll need from the various time bases...
std::map<seti_time,seconds> ut1_utc_conv;
struct tai_conv {
    double  a0;
    double  a1;
    double  d0;
};

std::map<seti_time,tai_conv> tai_utc_conv;

void read_time_conversions() {
    if (tai_utc_conv.size() == 0)  {
        tai_conv temp;
        char buf[256];
        FILE *in=fopen(TAI_UTC_CONV,"r");
        while (in && fgets(buf,255,in)) {
            double jd;
            // TAI-UTC=a0+(mjd-d0)*a1
            // a1=0 after 1 jan 1972 (introduction of leap seconds)
            sscanf(buf+17,"%lf",&jd);
            sscanf(buf+39,"%lf",&(temp.a0));
            sscanf(buf+60,"%lf",&(temp.d0));
            sscanf(buf+70,"%lf",&(temp.a1));
            tai_utc_conv[seti_time(days(jd),JD0)]=temp;
        }
        temp.a0=0;
        temp.d0=0;
        temp.a1=0;
        tai_utc_conv[seti_time()]=temp;
        fclose(in);
    }
    if (ut1_utc_conv.size() == 0) {
        char buf[256];
        FILE *in=fopen(UT1_UTC_CONV,"r");
        memset(buf,0,256);
        while (in && fgets(buf,255,in)) {
            double mjd, diff;
            diff=0;
            sscanf(buf+7,"%8lf",&mjd);
            // use Bulletin B values if available.
            if (!sscanf(buf+154,"%11lf",&diff)) {
                sscanf(buf+58,"%10lf",&diff);
            }
            if (diff!=0) {
                ut1_utc_conv[seti_time(days(mjd),MJD0)]=seconds(diff);
            }
            ut1_utc_conv[seti_time()]=seconds(0.0);
            memset(buf,0,256);
        }
        fclose(in);
    }
}

seti_time::seti_time(char *ast_string) {
    // AST arrives as YYDDDHHMMSShh
    long year,doy,hour,min,sec,centi_sec;
    double dhour, julday;
    struct DateStruct sdate;
    seti_time *temp_time;


    sscanf(ast_string, "%2ld%3ld%2ld%2ld%2ld%2ld",  &year,
            &doy,
            &hour,
            &min,
            &sec,
            &centi_sec);
    tm_AstToUt(&year, &doy, &hour);
    tm_DayToMon(doy-1, year, &sdate);
    dhour = hour + min/60.0 + sec/3600.0 + centi_sec/360000.0;
    julday =  tm_ToJul(sdate.month, sdate.day, tm_CentYear(sdate.year), dhour);

    temp_time = new seti_time(days(julday));
    *this = *temp_time;
    delete temp_time;
}


seconds seti_time::ast_utc_diff() const {
    return hours(-4.0);
}

seconds seti_time::tai_utc_diff() const {
// before 1972 TAI and UTC drifted freely.  After 1972 the difference
// is defined to be the cumulative number of leap seconds.
    if (tai_utc_conv.size() == 0) {
        read_time_conversions();
    }
    seti_time temp(*this);
    temp.base(UTC);
    // find the time conversion prior to this time.
    std::map<seti_time,tai_conv>::iterator i(tai_utc_conv.upper_bound(temp));
    i--;
    tai_conv conv(i->second);
    seconds diff(conv.a0+(temp.mjd().uval()-conv.d0)*conv.a1);
    return diff;
}

seconds seti_time::tt_utc_diff() const {
    // terrestrial time has a constant offset from TAI
    return seconds(32.184)+tai_utc_diff();
}

seconds seti_time::gps_utc_diff() const {
    // GPS time also has a constant offset from TAI, GPS and UTC were equal in
    // 1980.
    return tai_utc_diff()-seconds(19.0);
}

seconds seti_time::ut1_utc_diff() const {
    // UT1 and UTC drift from each other, but are brought closer together when
    // leap seconds are added.  They are never more than a second apart.
    if (ut1_utc_conv.size() == 0) {
        read_time_conversions();
    }
    seti_time temp(*this);
    temp.base(UTC);
    // need to be sure we never extrapolate across a leap second.
    std::map<seti_time,seconds>::iterator i(ut1_utc_conv.upper_bound(temp)),j;
    j=i;
    j--;
    std::pair<seti_time,seconds> lower(*j), upper;
    if (i!=ut1_utc_conv.end()) {
        upper=*i;
        if (upper.second > (lower.second+seconds(0.95))) {
            upper.second-=seconds(1.0);  // the upper point is on the wrong side of a leap
            // second.
        }
    } else {
        upper=lower;
        j--;
        lower=*j;
        if (upper.second > (lower.second+seconds(0.95))) {
            lower.second+=seconds(1.0);  // the lower point is on the wrong side of a leap
            // second
        }
    }
    // now interpolate between the two points.
    seconds diff(
        (upper.second-lower.second)*((temp-lower.first)/(upper.first-lower.first))
    );
    return diff;
}

// stub
seconds seti_time::ut2_utc_diff() const {
    return hours(0);
}

seconds seti_time::t_utc_diff() const {
    // Bessellian date in days since B2000, usually used for precession calcs;
    seti_time t(*this);
    t.base(UTC);
    t.epoch(B2000);
    return t-*this;
}

// seti_time::epoch() for returning or changing the epoch.
days seti_time::epoch(const days &e) {
    days diff(e-epoch_val);
    epoch_val+=diff;
    jd_val-=diff;
    diff=days(floor(epoch_val.uval())+0.5-epoch_val.uval());
    epoch_val+=diff;
    jd_val-=diff;
    return epoch_val;
}

// change the time base
seti_time::base_t seti_time::base(const seti_time::base_t &b) {
    if (base_val == b) {
        return base_val;
    }
    switch (base_val) {
    case UTC:  break;
    case AST:  *this+=ast_utc_diff();
        base_val=UTC;
        break;
    case UT1:  *this+=ut1_utc_diff();
        base_val=UTC;
        break;
    case UT2:  *this+=ut2_utc_diff();
        base_val=UTC;
        break;
    case TT:   *this+=tt_utc_diff();
        base_val=UTC;
        break;
    case T:    *this+=t_utc_diff();
        base_val=UTC;
        break;
    case TAI:  *this+=tai_utc_diff();
        base_val=UTC;
        break;
    case GPS:  *this+=gps_utc_diff();
        base_val=UTC;
        break;
    }
    switch (b) {
    case UTC:  break;
    case AST:  *this-=ast_utc_diff();
        base_val=AST;
        break;
    case UT1:  *this-=ut1_utc_diff();
        base_val=UT1;
        break;
    case UT2:  *this-=ut2_utc_diff();
        base_val=UT2;
        break;
    case TT:   *this-=tt_utc_diff();
        base_val=TT;
        break;
    case T:    *this-=t_utc_diff();
        base_val=T;
        break;
    case TAI:  *this-=tai_utc_diff();
        base_val=TAI;
        break;
    case GPS:  *this-=gps_utc_diff();
        base_val=GPS;
        break;
    }
    return base_val;
}

// seti_time::utc() returns UTC in hours past midnight.
hours seti_time::utc() const {
    seti_time temp(*this);
    temp.base(UTC);
    double h=fmod(temp.hrs().uval(),24.0);
    if (h<0) {
        h+=24;
    }
    return hours(h);
}

// seti_time::ut1() returns UT1 in hours past midnight.  Warning, can return
// numbers less than 0 and greater than 24.
hours seti_time::ut1() const {
    hours h(utc()+ut1_utc_diff());
    return h;
}

days seti_time::mjd() const {
    return jd_val+(epoch_val-MJD0);
}

days seti_time::jd()  const {
    return jd_val+(epoch_val-JD0);
}

days seti_time::mjd(const days &j) {
    *this=seti_time(j,MJD0);
    return mjd();
}

days seti_time::jd(const days &j) {
    *this=seti_time(j,JD0);
    return jd();
}

double seti_time::julian() const {
    return(jd().uval());
}

std::string seti_time::printjd() const {
    char s[32];
    //sprintf(s, "%lf", jd().uval());
    sprintf(s, "%lf", julian());
    std::string jd_string(s);
    return(jd_string);
}

void seti_time::print_internals() {
    printf("epoch_val %lf jd_val %lf base_val %d\n", epoch_val.uval(), jd_val.uval(), base_val);
}


/*______________________________________________________________
 * tm_DateToI()
 * Takes dbase string "date" and returns day, month, and year
 *______________________________________________________________
 */

void tm_DateToI(char *date, long *day, long *month, long *year) {
    char day_str[3];
    char year_str[3];
    char *strptr;

    strptr = date;
    memcpy(day_str, strptr, 2);
    day_str[2] = '\0';
    memcpy(year_str, (strptr += 4), 2);
    year_str[2] = '\0';

    *day = atol(day_str);
    *month = tm_MonToI(date);
    *year = atol(year_str);
}


/*__________________________________________________________________
 * tm_HmsToDec()
 * function converts a string in the format hhmmss.s to a decimal
 * hours double
 *__________________________________________________________________
 */

void tm_HmsToDec(char *time_str,double *dec_time) {
    long hours,mins,secs,tenth_sec;

    sscanf(time_str,"%2ld%2ld%2ld%1ld",&hours,&mins,&secs,&tenth_sec);
    *dec_time = hours + (double) mins/60 + (double) secs/3600 +
            (double) tenth_sec/36000;
}


/*__________________________________________________________________
 * tm_s3HmsToDec()
 * function converts a string in the format hhmmss to a decimal
 * hours float. Used by io_GetNextRecord to populate a data
 * structure from a record type 'B' (s3 data).
 *__________________________________________________________________
 */

void tm_s3HmsToDec(char *time_str,double *dec_time) {
    long hours,mins,secs;

    sscanf(time_str,"%2ld%2ld%2ld",&hours,&mins,&secs);
    *dec_time = hours + (double) mins/60 + (double) secs/3600;
}


/*______________________________________________________________
 * tm_DbaseToLst()
 * written by Jeff Cobb
 * function converts time and date in dbas format to
 * local sidereal time.  dbas time assumed to be
 * universal time.
 *______________________________________________________________
 */

double tm_DbasToLst(char *aststr, char *dbasdate) {
    /* will hold substrings of the 2 input strings */
    char asthr_str   [3];
    char astmin_str  [3];
    char astsec_str  [3];
    char dbasday_str [3];
    char dbasmon_str [3];      /* will be changed from letters to digits */
    char dbasyr_str  [3];

    struct DateStruct sdate; /* type def in SIREN.h  - used by daytomon */

    long sec_i, min_i, hour_i, mday_i, yday_i, month_i, year_i;

    double ut, lst;

    char *strptr;

    /* substring the input */
    strptr = aststr;
    strncpy(asthr_str, strptr, 2);
    asthr_str[2] = '\0';
    strncpy(astmin_str, (strptr += 2), 2);
    astmin_str[2] = '\0';
    strncpy(astsec_str, (strptr += 2), 2);
    astsec_str[2] = '\0';
    strptr = dbasdate;
    strncpy(dbasday_str, strptr, 2);
    dbasday_str[2] = '\0';
    strncpy(dbasmon_str, (strptr += 2), 2);
    dbasmon_str[2] = '\0';
    strncpy(dbasyr_str, (strptr += 2), 2);
    dbasyr_str[2] = '\0';

    /* month from letters to digits */
    sprintf(dbasmon_str, "%02ld", tm_MonToI(dbasdate));

    /* need time and date as integers */
    sec_i   = atol(astsec_str);
    min_i   = atol(astmin_str);
    hour_i  = atol(asthr_str);
    mday_i  = atol(dbasday_str);
    month_i = atol(dbasmon_str);
    year_i  = atol(dbasyr_str);
    year_i  = tm_CentYear(year_i);  // add century

    /* get day number of year (ast) */
    yday_i = tm_DayMonToYrDay(mday_i, month_i, year_i);

    /* removed this call, it assumed data were AST */
    /* get universal time from atlantic standard time */
    /**  ast_2_ut(&year_i, &yday_i, &hour_i);
    ***/

    /* get local sidereal time from universal time and telescope location */
    tm_DayToMon(yday_i-1, year_i, &sdate);  /* get month & day back */
    ut  = (double)(hour_i + (double)(min_i / 60.0) + (double)(sec_i / 3600.0));
    lst = tm_UtToLst(ut, AOLONGITUDE_DEGS, sdate.month, sdate.day, year_i);

    /* finally... */
    return lst;
}


/*________________________________________________________________________
 * tm_MonToI()
 * function takes a pointer to a date in the dbas format and returns an
 * integer in the range from one to twelve indicating the month
 * of the date.
 *________________________________________________________________________
 */

long tm_MonToI(char *fdate) {
    fdate += 2;    /* set to point to month field */
    if (strncmp(fdate,"ja",2) == 0) {
        return(1);
    }
    if (strncmp(fdate,"fe",2) == 0) {
        return(2);
    }
    if (strncmp(fdate,"mr",2) == 0) {
        return(3);
    }
    if (strncmp(fdate,"ap",2) == 0) {
        return(4);
    }
    if (strncmp(fdate,"my",2) == 0) {
        return(5);
    }
    if (strncmp(fdate,"jn",2) == 0) {
        return(6);
    }
    if (strncmp(fdate,"jl",2) == 0) {
        return(7);
    }
    if (strncmp(fdate,"au",2) == 0 || strncmp(fdate,"ag",2) == 0) {
        return(8);
    }
    if (strncmp(fdate,"se",2) == 0 || strncmp(fdate,"sp",2) == 0) {
        return(9);
    }
    if (strncmp(fdate,"oc",2) == 0) {
        return(10);
    }
    if (strncmp(fdate,"no",2) == 0 || strncmp(fdate,"nv",2) == 0) {
        return(11);
    }
    if (strncmp(fdate,"de",2) == 0 || strncmp(fdate,"dc",2) == 0) {
        return(12);
    }
    return(0); /* return 0 on error */
}


/*
 *________________________________________________________________________
 * tm_UtToLst()
 * function converts universal time to local sidereal time
 * given the telescope coordinates mon day and year.
 * day and month parameters start from 1. E.g.,
 * if it is june 5th, then day is 5 month is 6
 *________________________________________________________________________
 */

double tm_UtToLst(double ddtime, double longitude,
        long mon, long day, long year)
/*   double ddtime;      time in decimal digits
 *   double longitude;   longitude in decimal degrees, negative if west,
 *                       positive if east.
 *   long mon, day, year; month, day and year as long integers
 */
{
    double tu;          /* time, in centuries, from Jan 0, 2000 */
    double tusqr;
    double tucb;
    double g;          /* derived from tu, used to calculate gmst */
    double gmst;       /* Greenwich Mean Sidereal Time */
    double jd;         /* Julian date */
    double lmst;       /* Local Mean Sidereal Time */

    /* computation of the julian date given the day, month, year */

    jd  = (367 * year) - ((7 * (year + ((mon + 9)/12)))/4) + (275 * mon)/9 + day;
    jd  += 1721013.5;

    tu     = (jd - 2451545.0)/36525;
    tusqr  = tu * tu;
    tucb   = tusqr * tu;

    /* computation of the gmst at ddtime UT */

    g   = 24110.54841 + 8640184.812866 * tu + 0.093104 * tusqr - 6.62E-6 * tucb;

    for (gmst = g; gmst < 0; gmst += 86400) {
        ;
    }

    gmst /= 3600;    /* GMST at 0h UT */

    gmst += (1.0027379093 * ddtime);  /* GMST at utime UT */

    /* this is the *wrong* way to fix this, as gmst may be > 48 */
    /*   if (gmst > 24) gmst -= 24; */
    /* ..instead use fmod: */

    gmst = fmod(gmst,24);

    /* computation of lmst given gmst and longitude */

    lmst = gmst + ((longitude/360) * 24);

    /* this is also some wrong code - first bring lmst above 0, */
    /* then do an fmod - note that a fmod on a negative number */
    /* yeilds a negative answer, hence why we bring lmst above 0 first */
    /*     if (lmst < 0) lmst += 24; */
    /*     else if (lmst > 24) lmst -= 24; */
    /*     else lmst = lmst; */

    while (lmst < 0) {
        lmst += 24;
    }
    lmst = fmod(lmst,24);

    return(lmst);
}


/*________________________________________________________________________
 * tm_DayToMon()
 * function takes a day number and a year
 * number and returns the month number
 *________________________________________________________________________
 */

void tm_DayToMon(long day, long year, struct DateStruct *sdate) {
    long numdays;
    sdate->year = year;
    numdays = dysize((int) year);
    if (numdays == 365) { /* normal year */
        if (day < 31) {    /* jan 0..30 */
            sdate->month = 1;
            sdate->day = day+1;
        } else if (day < 59) { /* feb */
            sdate->month = 2;
            sdate->day = day-30;
        } else if (day < 90) { /* mar */
            sdate->month = 3;
            sdate->day = day-58;
        } else if (day < 120) { /* apr */
            sdate->month = 4;
            sdate->day = day-89;
        } else if (day < 151) { /* may */
            sdate->month = 5;
            sdate->day = day-119;
        } else if (day < 181) { /* jun */
            sdate->month = 6;
            sdate->day = day-150;
        } else if (day < 212) { /* jul */
            sdate->month = 7;
            sdate->day = day-180;
        } else if (day < 243) { /* aug */
            sdate->month = 8;
            sdate->day = day-211;
        } else if (day < 273) { /* sep */
            sdate->month = 9;
            sdate->day = day-242;
        } else if (day < 304) { /* oct */
            sdate->month = 10;
            sdate->day = day-272;
        } else if (day < 334) { /* nov */
            sdate->month = 11;
            sdate->day = day-303;
        } else if (day < 365) { /* dec */
            sdate->month = 12;
            sdate->day = day-333;
        } else {
            sdate->month = 0;
            sdate->day = 0;
        }
    } else {            /* leap year */
        if (day < 31) {    /* jan 0..30 */
            sdate->month = 1;
            sdate->day = day+1;
        } else if (day < 60) { /* feb */
            sdate->month = 2;
            sdate->day = day-30;
        } else if (day < 91) { /* mar */
            sdate->month = 3;
            sdate->day = day-59;
        } else if (day < 121) { /* apr */
            sdate->month = 4;
            sdate->day = day-90;
        } else if (day < 152) { /* may */
            sdate->month = 5;
            sdate->day = day-120;
        } else if (day < 182) { /* jun */
            sdate->month = 6;
            sdate->day = day-151;
        } else if (day < 213) { /* jul */
            sdate->month = 7;
            sdate->day = day-181;
        } else if (day < 244) { /* aug */
            sdate->month = 8;
            sdate->day = day-212;
        } else if (day < 274) { /* sep */
            sdate->month = 9;
            sdate->day = day-243;
        } else if (day < 305) { /* oct */
            sdate->month = 10;
            sdate->day = day-273;
        } else if (day < 335) { /* nov */
            sdate->month = 11;
            sdate->day = day-304;
        } else if (day < 366) { /* dec */
            sdate->month = 12;
            sdate->day = day-334;
        } else {
            sdate->month = 0;
            sdate->day = 0;
        }
    }
}


/*________________________________________________________________________
 * tm_DayMonToYrDay()
 * functions takes day, month, and year and
 * returns day-of-year.
 *________________________________________________________________________
 */

long tm_DayMonToYrDay(long day, long mon, long year) {
    long yday;

    if (dysize((int) year) == 366) /* leap year */
        if (mon > 2) {
            day++;
        }

    switch (mon) {
    case 1  : yday = day;
        break;
    case 2  : yday = day + 31;
        break;
    case 3  : yday = day + 59;
        break;
    case 4  : yday = day + 90;
        break;
    case 5  : yday = day + 120;
        break;
    case 6  : yday = day + 151;
        break;
    case 7  : yday = day + 181;
        break;
    case 8  : yday = day + 212;
        break;
    case 9  : yday = day + 243;
        break;
    case 10 : yday = day + 273;
        break;
    case 11 : yday = day + 304;
        break;
    case 12 : yday = day + 334;
        break;
    default : yday = 0;
    }
    return yday;
}


/*________________________________________________________________________
 * tm_AstToUt()
 * converts from ast to ut
 * assumes days are numbered from 1 to 365 (or 366 on leap year)
 *________________________________________________________________________
 */

void tm_AstToUt(long *year, long *day, long *hour) {
    long numdays;

    (*hour) += 4;

    if (*hour >= 24) {
        (*hour) -= 24;
        (*day)++;
        numdays = dysize((int) *year);
        if (((*day > 365) && (numdays == 365)) || ((*day > 366) && (numdays == 366))) {
            *day=1;
            (*year)++;
            *year = *year > 99 ? *year - 100 : *year;
        }
    }

}

/*________________________________________________________________________
 * tm_AstToUt() - version to handle hour passed as double
 * converts from ast to ut
 * assumes days are numbered from 1 to 365 (or 366 on leap year)
 *________________________________________________________________________
 */

void tm_AstToUt(long *year, long *day, double *hour) {
    long numdays;

    (*hour) += 4.0;

    if (*hour >= 24.0) {
        (*hour) -= 24.0;
        (*day)++;
        numdays = dysize((int) *year);
        if (((*day > 365) && (numdays == 365)) || ((*day > 366) && (numdays == 366))) {
            *day=1;
            (*year)++;
            *year = *year > 99 ? *year - 100 : *year;
        }
    }

}


/*________________________________________________________________________
 * tm_ToJul()
 *  takes month, day, year and decimal hour as arguments and
 *  arguments and returns decimal julian day as a double.
 *  the code used to calculate the int portion of julian day
 *  was lifted from "c users journal", vol 11,
 *  number 2; february, 1993.  it, in turn came from an
 *  algorithm presented in the communications of the acm,
 *  volume 11, number 10; october, 1968.
 *________________________________________________________________________
 */

double tm_ToJul(long month,
        long day,
        long year,
        double hour) {
    long ljul;

    ljul =  (day - 32075L + 1461L *
            (year + 4800L + (month - 14L) / 12L) / 4L +
            367L * (month - 2L - (month - 14L) / 12L * 12L) / 12L -
            3L * ((year + 4900L + (month - 14L) / 12L) / 100L) / 4L);

    return ( (double(ljul) + (hour / 24.0)) - 0.5 );   // -0.5 to make
    // standard jday
}


/*________________________________________________________________
 * tm_FromJul()
 * takes julian day and pointers to month, day,
 * and year as arguments, and calculates m-d-y form julian day.
 * this code was lifted from "c users journal", vol 11,
 * number 2; february, 1993.  it, in turn came from an
 * algorithm presented in the "communications of the acm",
 * volume 11, number 10; october, 1968.
 *________________________________________________________________
 */

void tm_FromJul(double jday,
        long *month,
        long *day,
        long *year,
        double *hour) {
    long t1, t2, yr, mo, ljul;
    double hr;

    jday += 0.5;                  // +0.5 to translate
    // from standard jday

    hr = jday - (long)jday;
    ljul = (long)(jday - hr);
    *hour = hr * 24.0;                              /* return hour */

    t1 = ljul + 68569L;
    t2 = 4L * t1 / 146097L;
    t1 = t1 - (146097L * t2 + 3L) / 4L;
    yr = 4000L * (t1 + 1) / 1461001L;
    t1 = t1 - 1461L * yr / 4L +31;
    mo = 80L * t1 / 2447L;
    *day = (long)(t1 - 2447L * mo / 80L);          /* return day */
    t1 = mo / 11L;
    *month = (long)(mo + 2L - 12L * t1);           /* return month */
    *year = (long)(100L * (t2 - 49L) + yr +t1);    /* return year */
}


/*____________________________________________________________________

 * tm_CompareDate
 *
 * Function compares date1 to date2.
 *
 * A negative long is returned if date1 is earlier than date2, a
 * positive long is returned if date1 is later than date2, and a
 * zero returned if date1 == date2. All date values must be longs.
 *____________________________________________________________________

 */


long tm_CompareDate( long *date1mon,
        long *date1day,
        long *date1year,
        long *date2mon,
        long *date2day,
        long *date2year)



{


    if (*date1year !=  *date2year) {
        return(*date1year - *date2year);
    }

    if (*date1mon != *date2mon) {
        return(*date1mon - *date2mon);
    }

    if (*date1day != *date2day) {
        return(*date1day - *date2day);
    }

    return(0);

}

//------------------------------------------------------------------------
long tm_CentYear(long TwoDigitYear)
//------------------------------------------------------------------------
{
    return(TwoDigitYear < 90   ?
            TwoDigitYear + 2000 :
            TwoDigitYear + 1900);
}

//------------------------------------------------------------------------
long tm_TwoDigitYear(long CentYear)
//------------------------------------------------------------------------
{
    return(CentYear > 1999 ?
            CentYear - 2000 :
            CentYear - 1900);
}

//--------------------------------------------------------------------
void tm_Converts4Time(long u32Time,
        long *DOY,
        long *Hour,
        long *Min,
        long *Sec,
        long *Ten)
//--------------------------------------------------------------------
// Binary time format:
//      31      23      18     12     6      2     0
//      --------------------------------------------
//      | DOY   | hour  | min  | sec  | 10th | src |
//      --------------------------------------------
//       9 bits  5 bits  6 bits 6 bits 4 bits 2 bits
{

    *DOY  = (0x000001ff) & (u32Time >> 23);
    *Hour = (0x0000001f) & (u32Time >> 18);
    *Min  = (0x0000003f) & (u32Time >> 12);
    *Sec  = (0x0000003f) & (u32Time >>  6);
    *Ten  = (0x0000000f) & (u32Time >>  2);

}

/*******************************************************************************
Programmer's note:
These procedures originally taken directly from doppler.c

                                    JETOJD

  Convert Julian Epoch to a Julian Date.
*******************************************************************************/

double          tm_JulianEpochToJulianDate(double je) {
    return (365.25 * (je - 2000) + 2451545);
}

/*******************************************************************************
                                    JDTOJE

 Convert Julian Date to Julian Epoch.
*******************************************************************************/

double          tm_JulianDateToJulianEpoch(double jd) {
    return (2000 + (jd - 2451545) / 365.25);
}

