/*
 * $Id: db_fns.h,v 1.1.2.1 2006/01/13 00:24:50 jeffc Exp $
 *
 */


#ifndef DB_FNS_H
#define DB_FNS_H

#include "schema_master.h"

int get_last_block(tapeheader_t *tapeheader) ;

//int update_tape_entry( tapeheader_t *tapeheader) ;

/* returns workunit group id number on success, 0 on failure */
//int create_wugrp_entry(char *wugrpname,wuheader_t *wuheader, int tapenum, tapeheader_t *tapeheader) ;

//int create_workunit_entry(wuheader_t *wuheader, int wugrpid, int subband_number) ;

#endif

/*
 * 
 * $Log: db_fns.h,v $
 * Revision 1.1.2.1  2006/01/13 00:24:50  jeffc
 * *** empty log message ***
 *
 * Revision 1.3  2003/08/05 17:23:40  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.2  2003/06/03 01:01:16  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:16:10  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 1.2  1999/02/22 22:21:09  korpela
 * added -nodb option
 *
 * Revision 1.1  1999/02/11  16:46:28  korpela
 * Initial revision
 *
 *
 */
