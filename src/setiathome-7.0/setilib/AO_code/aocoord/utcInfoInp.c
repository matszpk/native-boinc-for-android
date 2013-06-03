/*      %W      %G      */ 
/*
modification history
--------------------
x.xx 29nov93, pjp .. started
x.xx 22jun95, pjp .. switched to utcToUt1.dat file
x.xx 28apr98, pjp .. offset is now for daynumber, not jan1
x.xx 23dec98, pjp .. <pjp001>typo was overwriting daynum with daynum0
					 affected years that had more than on entry.
*/
#include	<azzaToRaDec.h>
#include	<stdio.h>
#include    <errno.h>

/*	defines		*/


/*	external 	*/

extern	int	errno;

/******************************************************************************
* utcInfoInp - read utc to ut1 info from file.
*
*DESCRIPTION
*
*Read the utc to ut1 information from the utcToUt1.dat file.
*The UTC_INFO structure will return the information needed to go from utc to
*UT1. the routine utcToUt1 converts from utc to ut1 using the information
*read in here into the UTC_INFO structure. The conversion algorithm is:
*
*  utcToUt1= ( offset + ((mjd - mjdAtOffset)+utcFrac)*rate
*
*The offset, rate, data are input from the utcToUt1.dat file.
* 
*The user can specify the default  file by setting the filename to
*blank or NULL. The file format is specified in vw/etc/Pnt/utcToUt1.dat.
*
*The user passes in the year (4 digits) and the daynumber (jan 1 == 1). The
*utcToUt1.dat file will be searched for the greatest year and daynumber that is 
*less than or equal to the date passed in. If all of the values are after the
*requested year/daynumber,  the earliest value in the file will be used and
*and error will be returned. 
*
*NOTE: The year, daynumber,offset,rates in the file are utc/mjd based values.
*      New values are added to the file whenever a leap second occurs. 
*      The addition of the leap second in the irig clock must be synchronized
*	   with the new value read from the file (irig jumps 1 sec, offset jumps
*	   back 1 sec). We normally update the irig clock at midnite ast so you
*      should pass in an ast year/daynumber so that it will move to the 
*      new value at 0 hours ast when the clock changes.
*
*      The pointing program reads this file at startup. For a change to
*      occur, you must stop then restart pointing.
*
*RETURNS
*OK or ERROR if the  file can't be read or all the year/daynumbers are after
*the value passed in. 
*
*SEE ALSO
*pntXformLib.h
*
*/

int   	utcInfoInp
	(
	int	   dayNum,		/* ast daynumber (see NOTE above)*/
	int	   year,        /* use closest value <= this year/daynum*/
	char*	   inpFile,	/* name of file to read, blank/NULL for DEF */
	UTC_INFO*  putcI	/* hold info to go utc to ut1 */
	)
{
	FILE*		fp;		/* for read */
	char	 	inpline[132];	/* input data here */
	char*		fileName;
	int	        dayNumInp,dayNumCur;/* from file*/
	int		yearInp,yearCur;/* from file*/
	double		offsetInp,rateInp;	/* read in*/
	double		offsetCur,rateCur;	/* read in*/
	int		year0,dayNum0;	/* earliest year/daynumber*/
	double		offset0,rate0;
	int		istat;

	fp=NULL;
	dayNumCur=yearCur=-1;
	rateCur=offsetCur=0;
	dayNum0=year0=999999;
	offset0=rate0=0.;
	if (inpFile == NULL) {
	   fileName=FILE_UTC_TO_UT1;
	}
	else if (inpFile[0]== ' ') {
	   fileName=FILE_UTC_TO_UT1;
	}
	else{
	   fileName=inpFile;		/* use what they passed in */
	}
	
	if ((fp=fopen(fileName,"r"))==NULL) {
	   fprintf(stderr,"utcInfoInp:Can not open file:%s. errno:%d\n",
			fileName,errno);
	   errno= S_pnt_BAD_UTC_TO_UT1_FILENAME;
	   goto errout;
	}
	/*
	*  loop till we find a the year/daynumber to use
	*/
	while (fgets(inpline,132,fp)!=NULL){
	   if (inpline[0]!= '#'){
	     if (sscanf(inpline,"%d%d%lf%lf",&yearInp,&dayNumInp,&offsetInp,
			 &rateInp) == 4){
			/*
 		 	* if date <= one passed in
			*/
	    	if  (yearInp <= year){
		    	if ((yearInp < year) || (dayNumInp <= dayNum)){
	        		/*
		     		* if date > what we currently have
		    		*/
		    		if (yearInp >= yearCur){
			   			if ((yearInp > yearCur) || (dayNumInp >= dayNumCur)){
		       				yearCur=yearInp;
		       				dayNumCur=dayNumInp;
		       				rateCur  =rateInp;
		       				offsetCur=offsetInp;
		   				}
			   		}
		    	}
			}
	    	if (yearInp <= year0){
				if ((yearInp < year0) || (dayNumInp < dayNum0)){
		       		year0=yearInp;
#if FALSE
		       		dayNum=dayNumInp; <pjp001>
#endif
		       		dayNum0=dayNumInp;
		       		offset0=offsetInp;
		       		rate0  =rateInp;
		    	}
	   		}
	   }
	 }
   }
	istat=OK;
	if (yearCur == -1) {
		fprintf(stderr,
		     "utcInfoInp:all dates in utcTout1.dat > %d	%d",
			year,dayNum);
		errno=S_pnt_UTC_TO_UT1_DATE_BEFORE_FILE_DATES;
		istat=ERROR;
	        if (year0 < 9999) {
		     /* 
		      * use 1st value
		    */
		     yearCur=year0;
		     dayNumCur=dayNum0;
		     rateCur=rate0;
		     offsetCur=offset0;
		     fprintf(stderr," Using 1st date:%d\n",year0);
	        }
		else  {
		     errno=S_pnt_UTC_TO_UT1_NO_DATES_IN_FILE;
		     fprintf(stderr,"\n");
		     goto errout;
		}
	}
	putcI->year=yearCur;
	putcI->mjdAtOff=gregToMjdDno(dayNumCur,putcI->year);/* 27apr98 change*/
	putcI->offset=offsetCur;
	putcI->rate  =rateCur;
 	if (fp != NULL) fclose(fp);	
	return(istat);

errout:	if (fp != NULL) fclose(fp);	
        return(ERROR);
}
