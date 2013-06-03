/*  splittypes.h
 *  
 *  Type definitions specific to the splitter. 
 *
 *
 *  $Id: splittypes.h,v 1.4 2003/08/05 17:23:43 korpela Exp $
 *
 */
#ifndef SPLITTYPES_H
#define SPLITTYPES_H

#include <stdio.h>

#include "splitparms.h"
#include "s_util.h"
#include "seti_header.h"

//typedef struct wuheader {
  //WU_INFO wuhead;
  //SETI_WU_INFO wuinfo;
//} wuheader_t;

typedef struct tapeheader {
  char name[36];
  int rcdtype;
  int numringbufs;
  int numdiskbufs;
  unsigned long frameseq;
  unsigned long dataseq;
  int missed;
  TIME st;
  SCOPE_STRING telstr;
  int source;
  double centerfreq,samplerate;
  char version[16];
} tapeheader_t;

typedef struct buffer_pos {
  int frame;
  long byte;
} buffer_pos_t;

#endif
/*
 * $Log: splittypes.h,v $
 * Revision 1.4  2003/08/05 17:23:43  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.3  2003/07/29 20:35:51  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.2  2003/06/03 01:01:17  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:23:41  korpela
 *
 * Again
 *
 * Revision 3.1  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.5  1999/02/11 16:46:28  korpela
 * Added startblock and norewind support, and wu_database_id.
 *
 * Revision 2.4  1998/11/10 00:02:44  korpela
 * Moved remaining wuheader fields into a WU_INFO structure.
 *
 * Revision 2.3  1998/11/05  21:18:41  korpela
 * Moved name field from header to seti header.
 *
 * Revision 2.2  1998/11/02  21:20:58  korpela
 * Moved tape_version and encoding_type into SETI_WU_INFO.
 *
 * Revision 2.1  1998/11/02  16:38:29  korpela
 * Variable type changes.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.7  1998/10/30  21:01:13  davea
 * *** empty log message ***
 *
 * Revision 1.6  1998/10/27 01:10:58  korpela
 * Modified tapeheader_t to match new tape header fields.
 *
 * Revision 1.5  1998/10/19  23:04:32  korpela
 * Moved times in telstr_t into an st_t.
 * Changed name of ast_t to st_t.
 * Added time zone (tz) field to ast_t.
 *
 * Revision 1.4  1998/10/15  19:13:30  korpela
 * Renamed "unix" fields to "unix_time" because of GCC #define.
 * Added position_history field to wuheader_t.
 *
 * Revision 1.3  1998/10/15  18:58:50  korpela
 * Added time_t unix to ast_t and telstr_t types.
 * Changed times in wuheader_t to time_t types.
 *
 * Revision 1.2  1998/10/15  18:01:19  korpela
 * Removed month field from ast_t and telstr_t.
 *
 * Revision 1.1  1998/10/15  16:20:24  korpela
 * Initial revision
 *
 */
