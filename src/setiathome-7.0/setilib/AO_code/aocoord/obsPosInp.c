/*      %W      %G      */
/*
modification history
--------------------
x.xx 24nov93, pjp .. started
*/
#include	<azzaToRaDec.h>
#include	<stdio.h>
#include    <errno.h>

extern	int	errno;

/******************************************************************************
* obsPosInp - input the observatories position.
*
*DESCRIPTION
*
*Read the observatories position in from  the obsLocation.dat file.
*The OBS_POS_INFO structure will return the information holding the 
*observatories positions.
*
*The user can specify the default  file by setting the filename to
*blank or NULL. The file format is specified in pnt/Files/obsLocation.dat.
*
*RETURNS
*OK or ERROR if the  file can't be read.
*
*SEE ALSO
*pntXformLib.h
*
*/

STATUS 	obsPosInp 
	(
	char		*inpFile,	/* null, blank --> use default*/
	OBS_POS_INFO *pobsPosInfo	/* return data here*/
	)
{
	FILE*		fp;		/* for read */
	char	 	inpline[132];	/* input data here */
	char*		fileName;
	int	        ic1,ic2;	/* first two coord*/
	double		dc3;		/* 3rd coord*/
	int		gotLat,gotLong;
	int		sgn;
	int		hours;

	gotLat=gotLong=FALSE;
	hours=0;
	fp=NULL;
	if (inpFile == NULL) {
	   fileName=FILE_OBS_POS;
	}
	else if (inpFile[0]== ' ') {
	   fileName=FILE_OBS_POS;
	}
	else{
	   fileName=inpFile;		/* use what they passed in */
	}
	
	if ((fp=fopen(fileName,"r"))==NULL) {
	   fprintf(stderr,"obsPosInp:Can not open file:%s. errno:%d\n",
			fileName,errno);
	   errno= S_pnt_BAD_OBS_POSITION_FILENAME  ;
	   goto errout;
	}
	/*
	*  loop till we find the lattitude, wlong.
	*/
	while (fgets(inpline,132,fp)!=NULL){
	   if (inpline[0]!= '#'){
	     if (sscanf(inpline,"%d%d%lf",&ic1,&ic2,&dc3) == 3){
	       if (!gotLat){		/* lattitude comes first*/
	          /*
	           * format was deg, min, secs(double)
	          */
	 	  setSign(&ic1,&sgn);
          pobsPosInfo->latRd= dms3_rad(sgn,ic1,ic2,dc3);
		  gotLat=TRUE;
	       }
	       else {
	         /*
		  * west long is hours/mins/secs of time
	         */
   		  hours=ic1;
	 	  setSign(&ic1,&sgn);
		  pobsPosInfo->wLongRd=hms3_rad(sgn,ic1,ic2,dc3);
		  gotLong=TRUE;
		  break;
	       }
	     }
	   }
	}
	if ( !(gotLat && gotLong)) {
	   fprintf(stderr,"obsPosInp:Didn't find Lat/Wlong lines in file %s\n",
			fileName);
	   goto errout;
	}
	pobsPosInfo->hoursToUtc = hours;	/* ao is + 4*/
	if (fp != NULL) fclose(fp);
	return(OK);

errout:	if (fp != NULL) fclose(fp);	
        return(ERROR);
}
