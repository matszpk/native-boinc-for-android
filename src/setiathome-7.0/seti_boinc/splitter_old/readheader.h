/*
 *  Functions for reading tape and work unit headers
 *
 * $Id: readheader.h,v 1.1.2.1 2006/01/13 00:24:58 jeffc Exp $
 *
 */

/* Read a tape header at buffer into a tapeheader_t */
int read_tape_header(char *buffer, tapeheader_t *header);

/* Read a work unit header from a FILE into a wuheader_t */
//int read_wu_header(FILE *file, workunit *header);
// no longer needed

/* Parse tape headers from binary tape header into a tapeheader_t */
int parse_tape_headers(unsigned char *tapebuffer, tapeheader_t *tapeheaders);
/*
 * $Log: readheader.h,v $
 * Revision 1.1.2.1  2006/01/13 00:24:58  jeffc
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:41  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:16:16  korpela
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
 * Revision 1.2  1998/10/27  01:09:22  korpela
 * Added parse_tape_headers()
 *
 * Revision 1.1  1998/10/15  17:19:21  korpela
 * Initial revision
 *
 */
