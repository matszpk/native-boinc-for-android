/* readtape.h
 *
 * Functions for reading tapes.
 *
 * $Id: readtape.h,v 1.3.2.1 2006/12/14 22:24:47 korpela Exp $
 *
 */

#ifndef READTAPE_H
#define READTAPE_H

#include "sah_config.h"

/* Status flag to indicate whether open "tape" device is a tape drive */
extern int is_tape;

/* Check if tape is busy.  If so return 1 else return 0 */
int tape_busy();

/* Rewind and eject the tape.  Return 1 if sucessful else return 0 */
int tape_eject();

/* Rewind to beginning of tape. Return 1 if sucessful, 0 if failure */
int tape_rewind();

/* Open a device, check if it is a tape device.  If so, set is_tape
 * flag. If not, it resets the flag.  All tape routines check this flag
 * so routines will work for both tapes and files
 */
int open_tape_device(char *device);

/* Seek to a specific record number returns 1 if successful */
int select_record(int record_number);

/* Read a record of length TAPE_RECORD_SIZE into a buffer 
 * Returns 1 if successful
 */
int tape_read_record(char *buffer);

/* Fill a buffer of length n_records*TAPE_RECORD_SIZE
 * with data from tape.  Return 1 if sucessful.
 */
int fill_tape_buffer(unsigned char *buffer, int n_records);

extern int current_record;

#endif
/*
 * $Log: readtape.h,v $
 * Revision 1.3.2.1  2006/12/14 22:24:47  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/26 20:48:51  jeffc
 * jeffc - merge in branch setiathome-4_all_platforms_beta.
 *
 * Revision 1.2.2.1  2003/09/23 16:01:45  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:41  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:23:39  korpela
 *
 * Again
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.2  1999/02/11 16:46:28  korpela
 * Added db access functions.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/20  21:35:51  korpela
 * Added fill_tape_buffer()
 *
 * Revision 1.1  1998/10/15  16:49:59  korpela
 * Initial revision
 *
 */
