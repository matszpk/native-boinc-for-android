/*
 * testAzZaToRaDec - test azza to radec routine. 
 *
 * SYNTAX:
 *  testAzZaToRaDec {-n} {-l toloop} az za yyyy mm dd hh mm ss 
 *
 * ARGS:
 *  az 	float	azimuth (feed) to use for computations
 *  za 	float	azimuth (feed) to use for computations
 *  yyyy int    4 digit year to start at
 *  mm   int    2 digit mm   to start at
 *  dd   int    2 day of month   to start at .. 1 to 12
 *  hh   int    hour of day  to start at
 *  mm   int    min  of day  to start at
 *  ss   int    sec of day to start at
 *
 * OPTIONS: 
 *  -n use current time to start at. ignore yyyy mm dd hh mm ss 
 *  -l toloop number of seconds from start to compute ra,dec. default is 1
 *
 * DESCRIPTION:
 * 	Test the azzaToRaDec routine. Input az,za, and starting time. It will
 *then compute the ra,dec J2000 position for this time. The routine
 *is setup for arecibo. The times should be AST time (actually it uses
 *the define LOCAL_TIME_TO_UTC_HRS  (positive for west longitude to convert
 *from input time to utc).
 *
 * 	The time az,za ra,dec is output to standard out.
 *
 *NOTES:
 *1. No model correction is currently done.
 *2. It uses the starting time to figure out which  utcToUt1  equation to
 *   used (see utcInfoInp routine). Eventually they will insert another
 *   leap second and the file utcToUt1.dat will have to be updated..
*/

#include	<stdlib.h>
#include	<stdio.h>
#include	<time.h>
#include	<sys/time.h>
#include	<azzaToRaDec.h>

/*
 * here is the offset to go local hours  to utc hours
*/

#define	LOCAL_TIME_TO_UTC_HRS   4.
/*
 * program setup info stored in test_info struct
*/

typedef struct { 
		double	az;		/* az of feed position degrees*/
		double	za;		/* za of feed degrees*/
	    int	    toloop; /* seconds to loop over*/
		int	stYear;		/* 4 digits*/
		int	stMon;      /* count 1 to 12*/
		int	stDay;      /* count 1 .. lastday of month*/
		double stDayFrac; /* fraction of day*/
	} TEST_INFO;
/*
 * prototypes
*/
void processargs(int argc,char **argv,TEST_INFO *ptI);


int main(int argc,char **argv) 
{
	TEST_INFO  testI;
	AZZA_TO_RADEC_INFO  azzaI; /* info for aza to ra dec*/
	char		cbufRa[80],cbufDec[80],cbufTm[80];
	double		astSec;
	int			dayNum,mjd;
	double      utcFrac;
	double		ra_Rd,dec_Rd;
	int			i;
	double      ut1Off;


	processargs(argc,argv, &testI);	/* get the arguments*/

	dayNum=dmToDayNo(testI.stDay,testI.stMon,testI.stYear);

/*
 *   initialize the azzatoradec conversion.
 *   1. input the utcToUt1 conversion.. use dayNum,stYear to figure out
 *      which equation we use in the uctToUt1.dat file.
 *   2. init the precnut struct.
 *   3. get the observatories position
*/
	if (azzaToRaDecInit(dayNum,testI.stYear,&azzaI) == ERROR) goto errout;
/*
 * compute int mjd and utc fraction of day  for the start
*/
	mjd=gregToMjd(testI.stDay,testI.stMon,testI.stYear);
	utcFrac=testI.stDayFrac + LOCAL_TIME_TO_UTC_HRS/24.;
/*
 * now loop computing the ra,decs as we step 1 second at a time
*/
	for (i=0;i<testI.toloop;i++) {
/*
 * 		in case we crossed utc midnite
*/
		if (utcFrac >= 1.) {
			mjd++;
			utcFrac-=1.;
		}
		ut1Off= utcToUt1(mjd,utcFrac,&azzaI.utcI)*86400.;
/*
 *		convert az,za to ra,dec .. comes back in radians...
*/
        int ofDate = 1; 
		azzaToRaDec(testI.az,testI.za,mjd,utcFrac,ofDate,&azzaI,&ra_Rd,&dec_Rd);
/*
 *  format and output to stdout
*/

		fmtRdToHmsD(ra_Rd,2,cbufRa);	/* radians to hms*/
		fmtRdToDms(dec_Rd,1,cbufDec);
		astSec=((utcFrac -  LOCAL_TIME_TO_UTC_HRS/24.)*86400.); 
		astSec=(astSec < 0.)?86400. - astSec:astSec;
		fmtSMToHmsD(astSec,1,cbufTm);	/* format the ast time hh:mm:ss*/
/*
 *   astTime   ra       dec
 * hh:hh:ss hh:mm:ss  dd:mm:ss
*/
		fprintf(stdout,"%s  %s %s\n",cbufTm,cbufRa,cbufDec);
		utcFrac+=1./86400.;
	}
	exit(0);
errout: exit(-1);
}
/****************************************************************************/
/*   processargs                                                            */
/****************************************************************************/
void    processargs
    (
        int argc,
        char **argv,
        TEST_INFO *ptI
    )
{
/*
        function to process a programs input command line.
        This is a template that can be customized for individual programs
        To use it you should:

        - pass in the parameters that may be changed.
        - edit the case statement below to correspond to what you want.
        - stdio.h must be added for this routine to work

        Don't forget the ** on the arguments coming in (since you want to
        pass back data.
*/
        int getopt();                   /* c lib function returns next opt*/
        int itemp;
        extern int opterr;              /* if 0, getopt won't output err mesg*/
        extern char *optarg;
		extern int optind;              /* after call, ind into argv for next*/

       int c;                          /* Option netter returned by getopt*/
	   int hour,min,sec;
	   int	useNow;
	   int	numArgsNeeded;
	   struct timeval  tv;
	   struct timezone tz;
	   struct tm       *ptm;

        char  *myoptions = "nl:";/* options to search for. :--> needs
*/

        char *USAGE =
"Usage:azzatoradec -n {-l toloop} az za yyyy mm dd hh mm ss";

        opterr = 0;                             /* turn off there message*/
        /*
         * set the defaults
        */
        ptI->toloop=1;
	    ptI->az    =270.;
	    ptI->za    =10.;
		useNow     =0;
/*
        loop over all the options in list
*/
	
       while ((c = getopt(argc,argv,myoptions)) != -1){
          switch (c) {
          case 'n':
                  useNow=1;
                  break;
          case 'l':
                   sscanf(optarg,"%d",&itemp);
                   if (itemp > 0)  ptI->toloop=itemp;
                   break;
          case '?':                     /*if c not in myoptions, getopt rets ?*/
             goto errout;
             break;
          }
        }

		numArgsNeeded=(useNow)?2:8;
		if ((optind + numArgsNeeded) != argc) goto errout;
/*
 * 		get az za,
*/
		ptI->az=atof(argv[optind++]);
		ptI->za=atof(argv[optind++]);

		if (useNow) {
			gettimeofday(&tv,&tz);
			ptm=localtime(&tv.tv_sec);
		    ptI->stYear=ptm->tm_year + 1900;
		    ptI->stMon =ptm->tm_mon  + 1;
		    ptI->stDay =ptm->tm_mday;
			hour= ptm->tm_hour;
			min = ptm->tm_min;
			sec = ptm->tm_sec;
		} else {
		    ptI->stYear=atoi(argv[optind++]);
		    ptI->stMon =atoi(argv[optind++]);
		    ptI->stDay =atoi(argv[optind++]);
			hour = atoi(argv[optind++]);
			min  = atoi(argv[optind++]);
			sec  = atoi(argv[optind++]);
		}
		ptI->stDayFrac=(sec + 60.*(min + 60.*(hour)))/86400.;
        return;
/*
        here if illegal option or argument
*/
errout: fprintf(stderr,"%s\n",USAGE);
        exit(1);
}
