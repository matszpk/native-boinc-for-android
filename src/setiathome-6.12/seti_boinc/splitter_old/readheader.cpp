/*
 *  Functions for reading tape and work unit headers
 *
 *  This file currently assumes that the reciever is at Arecibo.
 *
 * $Id: readheader.cpp,v 1.1.2.1 2006/01/13 00:24:58 jeffc Exp $
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "splitter.h"
#include "splitparms.h"
#include "splittypes.h"
#include "message.h"
#include "timecvt.h"
#include "coordcvt.h"

/* Read a tape header at buffer into a tapeheader_t */
int read_tape_header(char *buffer, tapeheader_t *header)
{
  static char *receivers[]={"","synthetic","ao1420"};
  int done=0,len;
  int pos=0,i;
  char tmpstr[256];
  double dummy;
  unsigned int firstdata;

  do {
    len=strlen(buffer+pos)+1;
    if (!strncmp(buffer+pos,"EOH=",4)) {
      done=1;
    } else if (!strncmp(buffer+pos,"NAME=",5)) {
       strncpy(tmpstr,buffer+pos+5,36);
       i=0;
       while(!isalnum(tmpstr[i])) i++;
       strncpy(header->name,tmpstr+i,36);
    } else if (!strncmp(buffer+pos,"RCDTYPE=",8)) {
       sscanf(buffer+pos+8,"%d",&(header->rcdtype));
    } else if (!strncmp(buffer+pos,"FRAMESEQ=",9)) {
       sscanf(buffer+pos+9,"%lu",&(header->frameseq));
    } else if (!strncmp(buffer+pos,"DATASEQ=",8)) {
       sscanf(buffer+pos+8,"%lu",&(header->dataseq));
    } else if (!strncmp(buffer+pos,"NUMRINGBUFS=",12)) {
       sscanf(buffer+pos+12,"%d",&(header->numringbufs));
    } else if (!strncmp(buffer+pos,"NUMDISKBUFS=",12)) {
       sscanf(buffer+pos+12,"%d",&(header->numdiskbufs));
    } else if (!strncmp(buffer+pos,"MISSED=",7)) {
       sscanf(buffer+pos+7,"%d",&(header->missed));
    } else if (!strncmp(buffer+pos,"AST=",4)) {
       sscanf(buffer+pos+4,"%2d%3d%2d%2d%2d%2d",
	 &(header->st.y), &(header->st.d), &(header->st.h),
	 &(header->st.m), &(header->st.s), &(header->st.c));
	 header->st.tz=AST;
    } else if (!strncmp(buffer+pos,"TELSTR=",7)) {
       sscanf(buffer+pos+7,"%2d%3d%2d%2d%2d %lf %lf %lf",
	 &(header->telstr.st.y), &(header->telstr.st.d), 
	 &(header->telstr.st.h), &(header->telstr.st.m), 
	 &(header->telstr.st.s), &(header->telstr.az), 
	 gregorian?&(header->telstr.alt):&dummy,
	 gregorian?&dummy:&(header->telstr.alt));
	 header->telstr.alt=90.0-header->telstr.alt;
	 header->telstr.st.tz=AST;
	 header->telstr.st.c=0;
    } else if (!strncmp(buffer+pos,"RECEIVER=",9)) {
       i=1;
       while ((!strstr(buffer+pos+9,receivers[i])) && (++i<NUM_SRCS)) ;
       if (i==NUM_SRCS) {
          message("Unknown receiver:");
	  message(buffer+pos+9);
	  message("\n");
	  header->source=0;
       } else {
	  header->source=i;
       }
    } else if (!strncmp(buffer+pos,"CENTERFREQ=",11)) {
       sscanf(buffer+pos+11,"%lf",&(header->centerfreq));
       header->centerfreq*=1e6;
    } else if (!strncmp(buffer+pos,"SAMPLERATE=",11)) {
       sscanf(buffer+pos+11,"%lf",&(header->samplerate));
       header->samplerate*=1e6;
    } else if (!strncmp(buffer+pos,"VER=",4)) {
       strncpy(&(header->version[0]),&(buffer[pos+4]),16);
    } else if (len>1) {
       sprintf(tmpstr,"Unknown header field: %40s\n",buffer+pos);
       message(tmpstr);
    }
    pos+=len;
  } while (!done && (pos<TAPE_HEADER_SIZE));
/* Right now were only doing this for one observatory. (Arecibo)
 * May want to change list later to a switch statement on the reciever 
 */

  st_time_convert(&(header->st));
  header->st.jd-=(float)RECORDER_BUFFER_SAMPLES/header->samplerate/86400;
  st_time_convert(&(header->telstr.st));
  telstr_coord_convert(&(header->telstr),ARECIBO_LAT,ARECIBO_LON);

/*
 * Fix a bug in recorder versions prior to 1.30
 */

 if (atof(&(header->version[0]))<1.299) {
   header->centerfreq-=2.0;
 }

/*
 * Check for blank tape
 */
  firstdata=*(unsigned int *)(buffer+TAPE_HEADER_SIZE);
  if (!(firstdata & 0x55555555) || !(firstdata & 0xaaaaaaaa) ||
       ((firstdata & 0x55555555) == 0x55555555) ||
       ((firstdata & 0xaaaaaaaa) == 0xaaaaaaaa)) {
    header->missed++;
    sprintf(tmpstr,"Possible data problem...data[0] = 0x%x\n",firstdata);
    message(tmpstr);
  }

  return(1);
}

/* Read a work unit header from a FILE into a wuheader_t */
//int read_wu_header(FILE *file, wuheader_t *header) {
/* to be implemented. Don't need it yet. */
//  return(0);
//}

int parse_tape_headers(unsigned char *tapebuffer, tapeheader_t *tapeheaders) {
 int i;

 for (i=0;i<TAPE_FRAMES_IN_BUFFER;i++) {
   read_tape_header((char *)tapebuffer+i*TAPE_FRAME_SIZE, &(tapeheaders[i]));
 }
 return(1);
}

/*
 * $Log: readheader.cpp,v $
 * Revision 1.1.2.1  2006/01/13 00:24:58  jeffc
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:40  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:48  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.3  2003/06/05 15:52:47  korpela
 *
 * Fixed coordinate bug that was using the wrong zenith angle from the telescope
 * strings when handling units from the gregorian.
 *
 * Revision 1.2  2003/06/03 01:01:17  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:16:16  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.9  1999/06/07 21:00:52  korpela
 * *** empty log message ***
 *
 * Revision 2.8  1999/03/05  01:47:18  korpela
 * Added data_class field.
 *
 * Revision 2.7  1999/02/22  21:49:42  korpela
 * Fixed bug in bug fix.
 *
 * Revision 2.6  1999/02/22  21:48:41  korpela
 * Fixed frequency bug in recorders prior to v1.3
 *
 * Revision 2.5  1998/11/13  23:58:52  korpela
 * Modified for move of name field between structures.
 *
 * Revision 2.4  1998/11/02  21:34:19  korpela
 * Fixed azimuth error.
 *
 * Revision 2.3  1998/11/02  21:20:58  korpela
 * Modified for (internal) integer receiver ID.
 *
 * Revision 2.2  1998/11/02  18:39:38  korpela
 * Changed location of timecvt.h
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.4  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.3  1998/10/27  00:57:14  korpela
 * Bug fixes.
 *
 * Revision 1.2  1998/10/15  19:16:38  korpela
 * Corrected syntax errors.
 *
 * Revision 1.1  1998/10/15  19:05:33  korpela
 * Initial revision
 *
 *
 */
