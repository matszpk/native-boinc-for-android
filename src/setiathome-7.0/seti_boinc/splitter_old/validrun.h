/*
 * validrun.h
 *
 * Functions for determining if a section of the tape buffer can be
 * turned into a valid work unit
 *
 * $Id: validrun.h,v 1.1.2.1 2006/01/13 00:25:04 jeffc Exp $
 *
 */

int valid_run(tapeheader_t tapeheader[],buffer_pos_t *start_of_wu, 
               buffer_pos_t *end_of_wu);

/*
 * $Log: validrun.h,v $
 * Revision 1.1.2.1  2006/01/13 00:25:04  jeffc
 * *** empty log message ***
 *
 * Revision 1.1  2003/06/03 00:23:43  korpela
 *
 * Again
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
 * Revision 1.1  1998/10/22  17:49:15  korpela
 * Initial revision
 *
 *
 */
