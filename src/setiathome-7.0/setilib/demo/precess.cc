#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    double jd, ra, decl, new_ra, new_decl, minute_float, second_float;
    int hrdeg_int, minute_int;

    char ra_str[64];
    char decl_str[64];
    char new_ra_str[64];
    char new_decl_str[64];

printf(">>>>> %d %s %s %s\n", argc, argv[1], argv[2], argv[3]);

    jd   = atof(argv[1]);
    tm_HmsToDec(argv[2], &ra);
    tm_HmsToDec(argv[3], &decl);

    new_ra = ra;
    new_decl = decl;

    eod2stdepoch(jd, new_ra, new_decl, stdepoch); 

    hrdeg_int       = (int)(ra*15);
    minute_float    = (ra*15 - hrdeg_int) * 60 ;
    minute_float    = minute_float < 0.0 ? 0.0 : minute_float;
    minute_int      = (int)minute_float;
    second_float    = (minute_float - minute_int) * 60 ;
    sprintf(ra_str, "(%lf)%02d:%02d:%07.4lf", ra, hrdeg_int, minute_int, second_float);

    hrdeg_int       = (int)(new_ra*15);
    minute_float    = (new_ra*15 - hrdeg_int) * 60 ;
    minute_float    = minute_float < 0.0 ? 0.0 : minute_float;
    minute_int      = (int)minute_float;
    second_float    = (minute_float - minute_int) * 60 ;
    sprintf(new_ra_str, "(%lf)%02d:%02d:%07.4lf", new_ra, hrdeg_int, minute_int, second_float);

    hrdeg_int       = (int)decl;
    minute_float    = (new_decl - hrdeg_int) * 60 ;
    minute_float    = minute_float < 0.0 ? 0.0 : minute_float;
    minute_int      = (int)minute_float;
    second_float    = (minute_float - minute_int) * 60 ;
    sprintf(decl_str, "%02d:%02d:%07.4lf", hrdeg_int, minute_int, second_float);

    hrdeg_int       = (int)new_decl;
    minute_float    = (new_decl - hrdeg_int) * 60 ;
    minute_float    = minute_float < 0.0 ? 0.0 : minute_float;
    minute_int      = (int)minute_float;
    second_float    = (minute_float - minute_int) * 60 ;
    sprintf(new_decl_str, "%02d:%02d:%07.4lf", hrdeg_int, minute_int, second_float);

    printf("%s %s at %lf becomes %s %s  at %lf\n", 
            ra_str, decl_str, jd, new_ra_str, new_decl_str, stdepoch);
}
