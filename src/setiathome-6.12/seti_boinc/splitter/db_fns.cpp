/* 
 * $Id: db_fns.cpp,v 1.3.4.2 2006/12/14 22:24:37 korpela Exp $
 *
 */
  

#include "sah_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "splitparms.h"
#include "splittypes.h"
#include "splitter.h"
#include "message.h"
#include "readtape.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "db_table.h"
#include "schema_master.h"


static tape tape_struct;

int update_tape_entry( tapeheader_t *tapeheader) ;

int get_last_block(tapeheader_t *tapeheader) {
    int err;
  strncat(tape_struct.tapename,tapeheader->name,20);
  if (!db_tape_lookup_name(&tape_struct)) {
    return(tape_struct.last_block_done/TAPE_FRAMES_PER_RECORD);
  } else {
      if ((err=db_tape_new(&tape_struct))) {
        log_messages.printf( SCHED_MSG_LOG::MSG_CRITICAL,"Unable to create db entry for tape %s error %d.\n",tapeheader->name,err);      
        exit(1);
      }
    return 0;
  }
}
  

/* returns id number of tape db entry or zero if failure */
/*int update_tape_entry( tapeheader_t *tapeheader) {
  if (strncmp(tapeheader->name,tape_struct.tapename,20)) {
    strncat(tape_struct.tapename,tapeheader->name,20);
    if (db_tape_lookup_name(&tape_struct)) {
      tape_struct.id=0;
      strncpy(tape_struct.tapename,tapeheader->name,20);
      tape_struct.start_time=tapeheader->st.jd;
      tape_struct.last_block_time=tapeheader->st.jd;
      tape_struct.last_block_done=tapeheader->frameseq;
      if (db_tape_new(&tape_struct)) {
	char tmpstr[256];
        fprintf( stderr, "Point 2\n" );
	sprintf(tmpstr,"Unable to create db entry for tape %s.",tapeheader->name);
	message(tmpstr);
        return(0);
      } 
    }
  }
  tape_struct.last_block_time=tapeheader->st.jd;
  tape_struct.last_block_done=tapeheader->frameseq;
  if (db_tape_update(&tape_struct)) {
    char tmpstr[256];
    sprintf(tmpstr,"Unable to update db entry for tape %s.",tapeheader->name);
    message(tmpstr);
    return(0);
  }
  return(tape_struct.id);
}
*/

/* returns workunit group id number on success, 0 on failure */
/*
int create_wugrp_entry(workunit_grp &wugrp, int tapenum, tapeheader_t *tapeheader) {
   int i,n;

   wugrp.tapenum=tapenum;
   strncpy(wugrp.wugrpname,wugrpname,64);
   wugrp.splitter_version=wuheader->wuinfo.splitter_version;
   wugrp.start_ra=wuheader->wuinfo.start_ra;
   wugrp.start_dec=wuheader->wuinfo.start_dec;
   wugrp.end_ra=wuheader->wuinfo.end_ra;
   wugrp.end_dec=wuheader->wuinfo.end_dec;
   wugrp.angle_range=wuheader->wuinfo.angle_range;
   wugrp.true_angle_range=wuheader->wuinfo.true_angle_range;
   wugrp.beam_width=wuheader->wuinfo.beam_width;
   wugrp.time_recorded=wuheader->wuinfo.time_recorded;
   wugrp.fft_len=wuheader->wuinfo.fft_len;
   wugrp.ifft_len=wuheader->wuinfo.ifft_len;
   wugrp.receiver=wuheader->wuinfo.source;
   wugrp.nsamples=wuheader->wuinfo.nsamples;
   wugrp.sample_rate=tapeheader->samplerate;
   wugrp.data_class=wuheader->wuinfo.data_class;
   strncpy(wugrp.tape_version,wuheader->wuinfo.tape_version,13);
   wugrp.num_positions=wuheader->wuinfo.num_positions;
   if (db_workunit_grp_new(&wugrp)) {
     char tmpstr[256];
     sprintf(tmpstr,"Unable to create db entry for workunit_grp %s\n",wugrpname);
     message(tmpstr);
     return(0);
   }
   for (i=0;i<wugrp.num_positions;i++) {
     char tmpstr[34];
     sprintf(tmpstr,"%14.5f %6.3f %6.2f",
       wuheader->wuinfo.position_history[i].st.jd,
       wuheader->wuinfo.position_history[i].ra,
       wuheader->wuinfo.position_history[i].dec
     );
     if (n=db_workunit_grp_update_position(wugrp.id,i,tmpstr)) {
         fprintf( stderr, "%d\n", n );
       sprintf(tmpstr,"Unable to add position %d to workunit_grp %s\n",i,wugrpname);
       message(tmpstr);
     }
   }
   return (wugrp.id);
}

int create_workunit_entry(wuheader_t *wuheader, int wugrpid, int subband_number) {
  WORKUNIT wu;

  memset(&wu,0,sizeof(wu));
  strncpy(wu.name,wuheader->wuhead.name,63);
  wu.grpnum=wugrpid;
  wu.subb_center=wuheader->wuinfo.subband_center;
  wu.subb_base=wuheader->wuinfo.subband_base;
  wu.subb_sample_rate=wuheader->wuinfo.subband_sample_rate;
  wu.subband_number=subband_number;
  wu.data_class=wuheader->wuinfo.data_class;
  if (db_workunit_new(&wu)) {
    char tmpstr[256];
    sprintf(tmpstr,"Unable to create workunit entry %s\n",wu.name);
    message(tmpstr);
    return(0);
  }
  return(wu.id);
}
*/

/*
 * $Log: db_fns.cpp,v $
 * Revision 1.3.4.2  2006/12/14 22:24:37  korpela
 * *** empty log message ***
 *
 * Revision 1.3.4.1  2006/01/13 00:37:56  korpela
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
 * Revision 1.3  2003/09/11 18:53:37  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:40  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:35  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.2  2003/06/03 01:01:16  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:16:10  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.2  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.1  2001/11/07 00:51:47  korpela
 * Added splitter version to database.
 * Added max_wus_ondisk option.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 1.4  2000/12/01 01:13:29  korpela
 * *** empty log message ***
 *
// Revision 1.3  1999/03/05  01:47:18  korpela
// Added data_class field.
//
// Revision 1.2  1999/02/22  22:21:09  korpela
// Fixed half-day error.
//
// Revision 1.1  1999/02/11  16:46:28  korpela
// Initial revision
//
 * 
 */
