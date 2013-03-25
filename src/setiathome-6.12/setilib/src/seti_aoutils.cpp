#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "setilib.h"
extern "C" {
#include "azzaToRaDec.h"
}

static const double AST_TO_UTC_HOURS=4;

//=======================================================
double seti_ao_timeMS2unixtime(long timeMs, time_t now) {
//=======================================================
// converts millisecs past midnight to a unix time_t.
// Time is often given as millisecs past midnight in AO
// scramnet structures.
// The "now" variable needs to be unix time ahead of 
// timeMs by less that 24 hours.  The realtime code can
// just pass the result of time().  Backend code code
// typically will use the SysTime included with each scramnet
// structure.

    double secs_past_midnight, encoder_read_secs_past_midnight,fnow;
    struct tm * now_tm;

    // craft a tm structure with values corresponding to encoder 
    // read time don't use localtime() because it will mess up
    // when data is read on pacific time computers 
    // by correcting for the PST-UT difference
    fnow=now;
    fnow-=AST_TO_UTC_HOURS*3600;                 // translate UTC to AST
    now=(time_t)floor(fnow);
    now_tm = gmtime(&now);  
    secs_past_midnight = now_tm->tm_hour*3600 + now_tm->tm_min*60 + now_tm->tm_sec;
    encoder_read_secs_past_midnight = (timeMs * 0.001);
#if 0
    if(secs_past_midnight < encoder_read_secs_past_midnight) {
        fnow -= secs_past_midnight + 86400;      // back up to midnight and then back up a day
    } else {
#endif
        fnow -= secs_past_midnight;              // just back up to midnight
#if 0
    }                                           // now now is midnight on the day of the encoder r
#endif
    fnow += encoder_read_secs_past_midnight;     // now now as the unix time of the encoder reading
    fnow+=AST_TO_UTC_HOURS*3600;                // translate AST to UTC

    return(fnow);
}


//=======================================================
void seti_AzZaToRaDec(double Az, double Za, double coord_time, double &Ra, double &Dec) {
//=======================================================
// This calls AO Phil's code.
// Any desired model correction must be done prior to calling this routine.

    AZZA_TO_RADEC_INFO  azzaI; /* info for aza to ra dec*/

    const double d2r =  0.017453292;
    int dayNum, i_mjd;
    int ofDate = 1;     // tells azzaToRaDec() to return coords of the date, ie not precessed
    double utcFrac;
    struct tm * coord_tm;
    time_t lcoord_time=(time_t)floor(coord_time);
    double fcoord_time=coord_time-floor(coord_time);

    coord_tm = gmtime(&lcoord_time);                 

    // arithmetic needed by AO functions
    coord_tm->tm_mon += 1;
    coord_tm->tm_year += 1900;
    dayNum = dmToDayNo(coord_tm->tm_mday,coord_tm->tm_mon,coord_tm->tm_year);
    i_mjd=gregToMjd(coord_tm->tm_mday, coord_tm->tm_mon, coord_tm->tm_year);
    utcFrac=(coord_tm->tm_hour*3600 + coord_tm->tm_min*60 + coord_tm->tm_sec + fcoord_time)/86400.;
    if (utcFrac >= 1.) {
        i_mjd++;
        utcFrac-=1.;
    }

    // call the AO functions
    if (azzaToRaDecInit(dayNum, coord_tm->tm_year, &azzaI) == -1) exit(1);
    // subtract 180 from Az because Phil wants dome azimuth
    azzaToRaDec(Az-180.0, Za, i_mjd, utcFrac, ofDate, &azzaI, &Ra, &Dec);

    // Ra to hours and Dec to degrees
    Ra = (Ra / d2r) / 15;
    Dec = Dec / d2r;

    // Take care of wrap situations in RA
    while (Ra < 0) Ra += 24;
    Ra = fmod(Ra,24);
}

