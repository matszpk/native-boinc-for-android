#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "setilib.h"

// Precesses from J2000 to a year specified by the user.
// Input: ra in degrees, dec in degrees
// Output: ra in hours, dec in degrees
int main(int argc, char ** argv) {
   
    if(argc != 2) {
      printf("Usage: precess2 <output year>\n");
      exit(-1);
    } 
    float jd_output = atof(argv[1]);   // First arg is output jd  (Should be 1993?)
    float jd_J2000 = 2451545.0;        // By definition

    char infile_name[20] = "psr_list.txt";
    FILE* infile = fopen(infile_name, "r");

    // Variables to input from psr list file
    int num; 
    float RAJD, DECJD, P0, DM, W50, S1400, C1;
    char PSRJ[20];

    // Variables for handling ra and dec    
    double ra, decl, new_ra, new_decl, minute_float, second_float;
    int hrdeg_int, minute_int;
    char ra_str[64];
    char decl_str[64];
    char new_ra_str[64];
    char new_decl_str[64];
   
    printf("Coordinates of pulsars precessed to epoch jd = %f\n\n", jd_output);

    while(1) { 

    // Scan a line from the file
    // DECJD is dec in degrees.  RAJD is ra in degrees.  Epoch is J2000
    if(fscanf(infile, "%d %s %f %f %f %f %f %f %f\n",
		   &num, &PSRJ, &RAJD, &DECJD, &P0, &DM, &W50, &S1400, &C1) == EOF)
   	break;
    ra = RAJD / 15.0;
    decl = DECJD; 

    new_ra = ra;
    new_decl = decl;
    // eod2stdepoch(float jd, float ra, float dec, float stdepoch)
    // input:
    // 		jd = starting julian date
    //          ra = starting ra in hours
    //          dec = starting dec in degrees
    //          stdepoch = ending julian date
    // output:
    //          ra = ending ra in hours, float
    //          dec = ending dec in degrees
    eod2stdepoch(jd_J2000, new_ra, new_decl, jd_output); 

    /*
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
    */

    printf("%f degrees ra, %f degrees dec at JD %lf becomes\n%f hours ra, %f degrees dec at %lf\n\n", 
            RAJD, DECJD, jd_J2000, new_ra, new_decl, jd_output);
    }
}

