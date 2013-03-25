/*
 *  Functions for sending messages to stderr and log files.
 *
 *  $Id: message.h,v 1.1.4.1 2006/01/13 00:37:57 korpela Exp $
 */

#include "sched_msgs.h"

void message(char *msg);

/*
 * $Log: message.h,v $
 * Revision 1.1.4.1  2006/01/13 00:37:57  korpela
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
 * Revision 1.1  2003/06/03 00:16:14  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.1  1998/11/02 16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/27  01:09:00  korpela
 * Initial revision
 *
 */
