/* readtape.c
 *
 * Functions for reading tapes.
 *
 * $Id: readtape.cpp,v 1.3.4.4 2007/06/07 20:01:52 mattl Exp $
 *
 */

#include "sah_config.h"
#undef USE_MYSQL
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>

#include "util.h"
#include "splitparms.h"
#include "splitter.h"
#include "message.h"
#include "readheader.h"
#include "readtape.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"
#include "str_util.h"
#include "str_replace.h"

int is_tape;

static tape tape_info;

int current_record;
static int tape_fd;
struct mtget tape_status;

static void update_checkpoint() {
  FILE *file=fopen("rcd.chk","w");
  if (file) {
    fprintf(file,"%d\n",current_record);
    fclose(file);
  }
}

int read_checkpoint() {
  FILE *file=fopen("rcd.chk","r");
  int retval=0;
  if (file) {
    fscanf(file,"%d",&retval);
    fclose(file);
  }
  return(retval);
}

int tape_busy() {
/* Check if tape is busy.  If so return 1 else return 0 
 */
  if (is_tape) {
    if (ioctl(tape_fd,MTIOCGET,&tape_status) != -1) {
      return (tape_status.mt_dsreg);
    } else {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to get tape status\n");
      return (1);
    }
  } else {
    return (0);
  }
}

int tape_eject() {
/* Rewind and eject the tape.  Return 1 if sucessful else return 0 
 */
  struct mtop op={MTOFFL,1};

  close(tape_fd);
  if (is_tape) {
    while (tape_busy()) sleep(1);
    if (ioctl(tape_fd,MTIOCTOP,&op)==-1) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to eject tape\n");
      return(0);
    } else {
      return(1);
    }
  } else {
    return(1);
  }
}


int tape_rewind() {
/* Rewind to beginning of tape. Return 1 if sucessful, 0 if failure 
 */
  struct mtop op={MTREW, 1};
  if (is_tape) {
    while (tape_busy()) sleep(1);
    if (ioctl(tape_fd,MTIOCTOP,&op)!=-1) {
      current_record=0;
      update_checkpoint();
    } else {
      current_record=-1;
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Tape rewind failed\n");
    }
  } else {
    if (lseek(tape_fd, 0, SEEK_SET)!=-1) {
      current_record=0;
      update_checkpoint();
    } else {
      current_record=-1;
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"File rewind failed\n");
    }
  }
  return (!current_record);
}



int open_tape_device(char *device) {
/* opens a device, checks if it is a tape device.  If so, sets is_tape
 * flag. If not, it resets the flag.  All tape routines check this flag
 * so routines will work for both tapes and files
 */


  int fd, errcnt=0;
  struct mtop op={MTNOP,1};

  if ((fd=open(device, O_RDONLY|0x2000, 0777))!=-1) {
    tape_fd=fd;
    if (ioctl(tape_fd,MTIOCGET,&tape_status) != -1)  {
      is_tape = 1;
    } else {
      is_tape = 0;
    }
    while (!norewind && !tape_rewind() && errcnt<10 ) errcnt++;
    if (!nodb && resumetape) {
      tape_rewind();
      fill_tape_buffer(tapebuffer,TAPE_RECORDS_IN_BUFFER);
      parse_tape_headers(tapebuffer, &(tapeheaders[0]));
      if (!tape_info.id) {
        if (!tape_info.fetch(std::string("where name=\'")+tapeheaders->name+"\'")) {
          tape_info.start_time=tapeheaders->st.jd;
          tape_info.last_block_time=tapeheaders->st.jd;
          tape_info.last_block_done=tapeheaders->frameseq;
          strlcpy(tape_info.name,tapeheaders->name,sizeof(tape_info.name));
          tape_info.insert();
	} 
      } 
      startblock=tape_info.last_block_done/TAPE_FRAMES_PER_RECORD;
    }  
    if (norewind) {
      startblock=MAX(read_checkpoint()-TAPE_RECORDS_IN_BUFFER,0);
      tape_rewind();
    }
    if (startblock) {
      return select_record(startblock);
    }  
    return (1);
  } else {
      perror( NULL );
    return (0);
  }
}

int select_record(int record_number) {
/* seeks to a specific record number returns 1 if successful */
  int diff;
  struct mtop op;
  char tmpstr[100];
  off64_t off,offset;

  if ((current_record<0) && !tape_rewind()) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Tape error: position lost and unable to rewind!\n");
    return(0);
  }

  diff=record_number-current_record;

  if (is_tape) {
    if (diff==0) return (1);
    if (diff>0) {
      op.mt_op=MTFSR;
      op.mt_count=diff;
    } else {
      op.mt_op=MTBSR;
      op.mt_count=diff;
    }
    if (ioctl(tape_fd,MTIOCTOP,&op)==-1) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Tape error: unable to position to record %d errno=%d\n",
	  record_number,errno);
      current_record=-1;
      return(0);
    } else {
      current_record+=diff;
      update_checkpoint();
      return(1);
    }
 } else {
   current_record=record_number;
   update_checkpoint();
   offset = record_number;
   offset *= TAPE_RECORD_SIZE;
   //fprintf( stderr, "offset: %lld", offset );
   off=lseek64(tape_fd,offset,SEEK_SET) ;
   //fprintf( stderr, "off: %lld", offset );
   return (off != -1);
 }
}

int tape_read_record(char *buffer) {
  int bytesread=0;
  int i;

  while (tape_busy()) sleep(1);
  
  do {
    i=read(tape_fd,buffer+bytesread,TAPE_RECORD_SIZE);
    if (i>0) bytesread+=i;
  } while ((bytesread<TAPE_RECORD_SIZE) && (i>0));

  if (i==0) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"End of tape.  Please insert new tape\n");
    current_record=-1;
    return(0);
  }

  if (i<0) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Tape error.\n");
    current_record=-1;
    return(0);
  }
 
  current_record++;
  update_checkpoint();
  return(1);
}

int fill_tape_buffer(unsigned char *buffer, int n_records) {
   int i;
   long record;
   char tmpstr[100];

   for (i=0;i<n_records;i++) {
     record=current_record;
     //fprintf( stderr, "Reading record %d\n", i);
     if (!tape_read_record((char *)buffer+i*TAPE_RECORD_SIZE)) {
       
       log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Tape error at record %d\n", current_record);
       return(0);
     }
   }
   return(1);
}

/*
 * $Log: readtape.cpp,v $
 * Revision 1.3.4.4  2007/06/07 20:01:52  mattl
 * *** empty log message ***
 *
 * Revision 1.3.4.3  2006/12/14 22:24:47  korpela
 * *** empty log message ***
 *
 * Revision 1.3.4.2  2006/01/13 00:37:57  korpela
 * Moved splitter to using standard BOINC logging mechanisms.  All stderr now
 * goes to "error.log"
 *
 * Added command line parameters "-iterations=" (number of workunit groups to
 * create before exiting), "-trigger_file_path=" (path to the splitter_stop trigger
 * file.  Default is /disks/setifiler1/wutape/tapedir/splitter_stop).
 *
 * Reduced deadlines by a factor of three.  We now need a 30 MFLOP machine to meet
 * the deadline.
 *
 * Revision 1.3.4.1  2006/01/10 00:39:04  jeffc
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:41  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:49  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.1  2003/06/03 00:16:16  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.3  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.2  2003/04/23 22:10:29  korpela
 * *** empty log message ***
 *
 * Revision 3.1  2002/06/21 00:06:09  eheien
 * *** empty log message ***
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.3  1999/02/22 22:21:09  korpela
 * added -nodb option
 *
 * Revision 2.2  1999/02/11  16:46:28  korpela
 * Added some db access functions.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.3  1998/10/27  00:57:47  korpela
 * Bug fixes.
 *
 * Revision 1.2  1998/10/20  21:33:25  korpela
 * Added fill_tape_buffer()
 * Minor bug fixes.
 *
 * Revision 1.1  1998/10/15  17:08:07  korpela
 * Initial revision
 *
 */

