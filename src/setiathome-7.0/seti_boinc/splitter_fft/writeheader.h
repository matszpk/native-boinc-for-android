/*
 *  Functions for writing tape and work unit headers
 *
 * $Id: writeheader.h,v 1.2 2003/08/05 17:23:44 korpela Exp $
 *
 */

/* Write a tape header into a buffer */
int write_tape_header(char *buffer, tapeheader_t *header);

/* Write a work unit header into a FILE */
//  int splitter_write_wu_header(FILE *file, wuheader_t *header);

/*
 * $Log: writeheader.h,v $
 * Revision 1.2  2003/08/05 17:23:44  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:23:44  korpela
 *
 * Again
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.2  1999/01/04 22:27:55  korpela
 * Updated return codes.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/15  17:20:01  korpela
 * Initial revision
 *
 */
