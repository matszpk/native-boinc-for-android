/*
 *
 * binary to ascii encoding for work unit files
 *
 * $Id: encode.h,v 1.1 2003/06/03 00:16:11 korpela Exp $
 *
 */

/* Write a range of bytes encoded into printable chars.
 * Encodes 3 bytes into 4 chars.
 * May read up to two bytes past end
 */

void splitter_encode(unsigned char *bin, int nbytes, FILE *f);

/*
 * $Log: encode.h,v $
 * Revision 1.1  2003/06/03 00:16:11  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.2  1998/11/02 21:20:58  korpela
 * Changed routine name from encode() to splitter_encode().  See encode.C
 * for details.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/27  01:06:46  korpela
 * Initial revision
 *
 *
 */
