/*
 *  Functions for writing tape and work unit headers
 *
 * $Id: writeheader.cpp,v 1.1.2.1 2006/01/13 00:25:04 jeffc Exp $
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include "splitparms.h"
#include "splittypes.h"
#include "timecvt.h"
#include "seti_header.h"

extern int output_xml;

/* Write a tape header into a buffer */
int write_tape_header(char *buffer, tapeheader_t *header) {
/* Unimplemented as yet  */
return(0);
}

char *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep"
                    "Oct","Nov","Dec"};
char *daynames[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

/* Write a work unit header into a FILE */
/*int splitter_write_wu_header(FILE *file, wuheader_t *header) {
    if (!output_xml)
      write_wu_header(file, header->wuhead);
  return (seti_write_wu_header(file,header->wuinfo,output_xml));

}
*/

/*
 * $Log: writeheader.cpp,v $
 * Revision 1.1.2.1  2006/01/13 00:25:04  jeffc
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:44  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:59  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.2  2003/06/03 01:01:18  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:23:43  korpela
 *
 * Again
 *
 * Revision 3.1  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.6  1999/01/04 22:27:55  korpela
 * Updated return codes.
 *
 * Revision 2.5  1998/11/10  00:02:44  korpela
 * Moved remaining wuheader fields into a WU_INFO structure.
 *
 * Revision 2.4  1998/11/09  23:26:27  korpela
 * Added generic version= to default header.
 *
 * Revision 2.3  1998/11/05  21:18:41  korpela
 * Moved name field from header to seti header.
 *
 * Revision 2.2  1998/11/02  18:42:03  korpela
 * Changed write_wu_header to call seti_write_wu_header
 *
 * Revision 2.1  1998/11/02  16:38:29  korpela
 * Will be transfered to client.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.4  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.3  1998/10/27  00:59:43  korpela
 * Bug fixes.
 * /
 *
 * Revision 1.2  1998/10/20  16:32:03  korpela
 * Fixed syntax error.
 *
 * Revision 1.1  1998/10/20  16:27:41  korpela
 * Initial revision
 *
 *
 */
