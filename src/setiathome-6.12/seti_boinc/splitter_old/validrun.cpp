/*
 * validrun.c
 *
 * Functions for determining if a section of the tape buffer can be
 * turned into a valid work unit
 *
 * $Id: validrun.cpp,v 1.1.2.1 2006/01/13 00:25:03 jeffc Exp $
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "splitparms.h"
#include "splittypes.h"
#include "splitter.h"
#include "message.h"


int valid_run(tapeheader_t tapeheader[],buffer_pos_t *start_of_wu, 
               buffer_pos_t *end_of_wu) {

  int i=0,valid=1;
  SCOPE_STRING *first_telstr;
  double first_telstr_time=0;
  double first_jd=tapeheader[0].st.jd;
  char tmpstr[256];

  /* find the first telstr that refers to valid data */

  do {
    first_telstr=&(tapeheader[++i].telstr);
    first_telstr_time=first_telstr->st.jd;
  } while ((first_telstr_time<=first_jd) && (i<TAPE_FRAMES_IN_BUFFER) && 
	   ((first_jd-first_telstr_time)<60.0/86400.0));
	   

  if (i==TAPE_FRAMES_IN_BUFFER) {
     sprintf(tmpstr,"No valid telstr with time later than %14.5f\n",first_jd);
     message(tmpstr);
     end_of_wu->frame=1+WU_OVERLAP_FRAMES;
     end_of_wu->byte=0;
     return(0);
  }
    
  /* find the correct byte offset for the start of the work unit */

  start_of_wu->frame=i;
  start_of_wu->byte=(long)((tapeheader[i].telstr.st.jd-tapeheader[i].st.jd)*86400.0*tapeheader[i].samplerate*2/CHAR_BIT);
  start_of_wu->byte &= 0xfffffffe;


  while (start_of_wu->byte<0) {
    start_of_wu->frame--;
    start_of_wu->byte+=TAPE_DATA_SIZE;
  }

  while (start_of_wu->byte>TAPE_DATA_SIZE) {
    start_of_wu->frame++;
    start_of_wu->byte-=TAPE_DATA_SIZE;
  }

  if (start_of_wu->frame<0) {
    sprintf(tmpstr,"Missing telescope strings near %lu\n",
	      tapeheader[0].frameseq);
    message(tmpstr);
    end_of_wu->frame=0;
    end_of_wu->byte=0;
    records_in_buffer=0;
    return(0);
  }   

  if ((start_of_wu->frame+TAPE_FRAMES_PER_WU)>TAPE_FRAMES_IN_BUFFER) {
    sprintf(tmpstr,"Missing telescope strings near %lu\n",
	       tapeheader[0].frameseq);
    message(tmpstr);
    end_of_wu->frame=0;
    end_of_wu->byte=0;
    records_in_buffer=0;
    return(0);
  }   

  /* check for missed frames */
  for (i=0; i<TAPE_FRAMES_PER_WU; i++) {
    int j=start_of_wu->frame+i;
    if (tapeheader[j].missed) {
      sprintf(tmpstr,"Missing frames between %lu and %lu\n",
	      tapeheader[j-1].frameseq,tapeheader[j].frameseq);
      message(tmpstr);
      valid=0;
      end_of_wu->frame=j+WU_OVERLAP_FRAMES+1;
      end_of_wu->byte=0;
      assert((j+WU_OVERLAP_FRAMES)<=TAPE_FRAMES_IN_BUFFER);
    }
  }   

  if (!valid) {
     end_of_wu->frame=0;
     records_in_buffer=0;
     return(valid);
  }

  end_of_wu->frame=start_of_wu->frame+TAPE_FRAMES_PER_WU;
  end_of_wu->byte=start_of_wu->byte;

  return(valid);
}


/*
 * $Log: validrun.cpp,v $
 * Revision 1.1.2.1  2006/01/13 00:25:03  jeffc
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/29 20:35:57  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.1  2003/06/03 00:23:43  korpela
 *
 * Again
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.7  2000/12/01 01:13:29  korpela
 * *** empty log message ***
 *
 * Revision 2.6  1999/06/07 21:00:52  korpela
 * *** empty log message ***
 *
 * Revision 2.5  1999/03/27  16:19:35  korpela
 * *** empty log message ***
 *
 * Revision 2.4  1999/02/22  22:21:09  korpela
 * added -nodb option
 *
 * Revision 2.3  1999/02/11  16:46:28  korpela
 * Added checkpointing.
 *
 * Revision 2.2  1998/11/04  23:08:25  korpela
 * Byte and bit order change.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.3  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.2  1998/10/27  00:59:22  korpela
 * Bug fixes.
 *
 * Revision 1.1  1998/10/22  17:48:20  korpela
 * Initial revision
 *
 *
 */
