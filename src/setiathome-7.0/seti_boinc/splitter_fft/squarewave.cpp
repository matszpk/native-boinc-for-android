#include "sah_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define NUM_FRAMES 200L
#define FRAME_DATA_SIZE (1024L*1024)
#define HEADER_SIZE 1024
#define FILESIZE (NUM_FRAMES*(FRAME_DATA_SIZE+HEADER_SIZE))

char header[HEADER_SIZE];

int main(void) {
  int i;
  int datapos=0;
  int frameseq=0;
  long written=0;
  int on=0;
  struct tm *tm;
  char data;

  char tmpstr[256];
  char telstr[256];
  double time0=(double)time(0);
  time_t clock;

  for (written=0;written<NUM_FRAMES;written++)  {
     int headerpos=0;
     memset(header,0,HEADER_SIZE);
     strcpy(header,"NAME=sqrwave");
     headerpos+=strlen("NAME=sqrwave")+1;
     strcpy(header+headerpos,"RCDTYPE=1");
     headerpos+=strlen("RCDTYPE=1")+1;
     sprintf(tmpstr,"FRAMESEQ=%d",frameseq);
     strcpy(header+headerpos,tmpstr);
     headerpos+=strlen(tmpstr)+1;
     sprintf(tmpstr,"DATASEQ=%d",frameseq++);
     strcpy(header+headerpos,tmpstr);
     headerpos+=strlen(tmpstr)+1;
     clock=floor(time0);
     tm=localtime(&clock);
     sprintf(tmpstr,"AST=%.2d%.3d%.2d%.2d%.2d%.2d",tm->tm_year,tm->tm_yday,tm->tm_hour,tm->tm_min,tm->tm_sec,(int)((time0-floor(time0))*100));
     strcpy(header+headerpos,tmpstr);
     headerpos+=strlen(tmpstr)+1;
     if (!((frameseq-1)%5)) {
       sprintf(telstr,"TELSTR=%.2d%.3d%.2d%.2d%.2d 0.0 0.0 15.0",tm->tm_year,tm->tm_yday,tm->tm_hour,tm->tm_min+(tm->tm_sec>57),(tm->tm_sec+2)%60);
     }
     strcpy(header+headerpos,telstr);
     headerpos+=strlen(telstr)+1;
     strcpy(header+headerpos,"RECEIVER=ao1420");
     headerpos+=strlen("RECEIVER=ao1420")+1;
     strcpy(header+headerpos,"SAMPLERATE=2.5000");
     headerpos+=strlen("SAMPLERATE=2.5000")+1;
     strcpy(header+headerpos,"VER=1.00");
     headerpos+=strlen("VER=1.00")+1;
     strcpy(header+headerpos,"CENTERFREQ=1420.0");
     headerpos+=strlen("CENTERFREQ=1420.0")+1;
     strcpy(header+headerpos,"NUMRINGBUFS=4");
     headerpos+=strlen("NUMRINGBUFS=4")+1;
     strcpy(header+headerpos,"NUMDISKBUFS=2");
     headerpos+=strlen("NUMDISKBUFS=2")+1;
     strcpy(header+headerpos,"EOH=");
     write(stdout->_file, header, HEADER_SIZE);
     for(i=0;i<FRAME_DATA_SIZE;i++) {
       if (!((datapos++)%(100/8))) on=!on;
       data=0xaa*on;
       write(stdout->_file, &data, 1);
     }
     time0+=(1024.0*1024.0*4.0/2.5e6);
   }
}  

