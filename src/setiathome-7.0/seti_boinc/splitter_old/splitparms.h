/*  splitparms.h
 *
 *  Most of the definitions required by the splitter
 *
 *  $Id: splitparms.h,v 1.1.2.1 2006/01/13 00:25:00 jeffc Exp $
 *
 */

#ifndef SPLITPARMS_H
#define SPLITPARMS_H

#include <limits.h>

#define WU_SUBDIR "/download"

#define SPLITTER_VERSION 0x0012
#define MAX_WUS_ONDISK 500000
#define N_SIMULT_SPLITTERS 6

/* Work Unit Parameters */
#define NBYTES 262144L
#define NSAMPLES (NBYTES*CHAR_BIT/2)
#define MAX_POSITION_HISTORY 40
#define WU_FILESIZE (360L*1024)

/* FFT Parameters */
#define FFT_LEN 2048
#define IFFT_LEN 8
#define NSTRIPS (FFT_LEN/IFFT_LEN)

/* Tape format parameters */
#define TAPE_HEADER_SIZE 1024L
#define TAPE_DATA_SIZE 1048576L
#define TAPE_FRAMES_PER_RECORD 8L
#define TAPE_FRAME_SIZE (TAPE_HEADER_SIZE+TAPE_DATA_SIZE)
#define TAPE_RECORD_SIZE (TAPE_FRAMES_PER_RECORD*TAPE_FRAME_SIZE)
#define TAPE_BUFFER_SIZE (((NSTRIPS*NBYTES*7/4)/TAPE_RECORD_SIZE+1)*TAPE_RECORD_SIZE)
#define TAPE_RECORDS_IN_BUFFER (TAPE_BUFFER_SIZE/TAPE_RECORD_SIZE)
#define TAPE_FRAMES_IN_BUFFER (TAPE_RECORDS_IN_BUFFER*8)
#define TAPE_FRAMES_PER_WU (NBYTES*NSTRIPS/TAPE_DATA_SIZE)
#define WU_OVERLAP_RECORDS 2
#define WU_OVERLAP_FRAMES (TAPE_FRAMES_PER_RECORD*WU_OVERLAP_RECORDS)
#define WU_OVERLAP_BYTES (WU_OVERLAP_FRAMES*TAPE_DATA_SIZE)
#define RECORDER_BUFFER_BYTES (1024L*1024L)
#define RECORDER_BUFFER_SAMPLES (RECORDER_BUFFER_BYTES*4)


/* Time Zone Parameters */
#define UTC  0.0
#define AST  (UTC-4.0)

/* Arecibo Observatory Parameters */
#define ARECIBO_LAT  18.3538056
#define ARECIBO_LON  (-66.7552222)

#endif
/*
 * $Log: splitparms.h,v $
 * Revision 1.1.2.1  2006/01/13 00:25:00  jeffc
 * *** empty log message ***
 *
 * Revision 1.10  2004/11/23 21:26:29  jeffc
 * *** empty log message ***
 *
 * Revision 1.9  2004/08/14 04:44:26  jeffc
 * *** empty log message ***
 *
 * Revision 1.8  2004/06/16 20:57:19  jeffc
 * *** empty log message ***
 *
 * Revision 1.7  2004/05/22 18:12:18  korpela
 * *** empty log message ***
 *
 * Revision 1.6  2003/10/27 17:53:21  korpela
 * *** empty log message ***
 *
 * Revision 1.5  2003/09/26 20:48:51  jeffc
 * jeffc - merge in branch setiathome-4_all_platforms_beta.
 *
 * Revision 1.3.2.1  2003/09/22 17:39:34  korpela
 * *** empty log message ***
 *
 * Revision 1.4  2003/09/22 17:05:38  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:42  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:23:40  korpela
 *
 * Again
 *
 * Revision 3.1  2001/11/07 00:51:47  korpela
 * Added splitter version to database.
 * Added max_wus_ondisk option.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.9  2000/12/01 01:13:29  korpela
 * *** empty log message ***
 *
 * Revision 2.8  1999/10/20 19:20:26  korpela
 * *** empty log message ***
 *
 * Revision 2.7  1999/06/07 21:00:52  korpela
 * *** empty log message ***
 *
 * Revision 2.6  1999/03/27  16:19:35  korpela
 * *** empty log message ***
 *
 * Revision 2.5  1999/03/05  01:47:18  korpela
 * Added data_class field.
 *
 * Revision 2.4  1999/02/23  18:57:09  korpela
 * *** empty log message ***
 *
 * Revision 2.3  1999/02/22  22:21:09  korpela
 * Changed version number.
 *
 * Revision 2.2  1999/02/11  16:46:28  korpela
 * Added WU_FILESIZE, RECORDER_BUFFER_BYTES, RECORDER_BUFFER_SAMPLES.`
 *
 * Revision 2.1  1998/11/02  16:38:29  korpela
 * Variable type changes.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.4  1998/10/27  01:10:04  korpela
 * Bug fixes.
 *
 * Revision 1.3  1998/10/19  23:06:40  korpela
 * Added UTC and AST definition.
 * Added ARECIBO_LAT and ARECIBO_LON definitions.
 *
 * Revision 1.2  1998/10/16  19:22:14  korpela
 * Reduced WU file size from 512K to 256K.
 *
 * Revision 1.1  1998/10/15  16:39:51  korpela
 * Initial revision
 *
 */
 
