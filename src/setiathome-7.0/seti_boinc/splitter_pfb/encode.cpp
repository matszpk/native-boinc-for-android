/*
 *
 * binary to ascii encoding for work unit files
 *
 * $Id: encode.cpp,v 1.2.4.2 2007/06/01 03:29:25 korpela Exp $
 *
 */

#include "sah_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

/* Write a range of bytes encoded into printable chars.
 * Encodes 3 bytes into 4 chars.
 * May read up to two bytes past end
 */

void splitter_encode(unsigned char *bin, int nbytes, FILE *f) {
    int count=0, offset=0, nleft, count1=0;
    unsigned char c[4*1024+64];
    unsigned char nl=10;
    assert(nbytes<=(3*1024));
    for (nleft = nbytes; nleft > 0; nleft -= 3) {
        c[0+count1] = bin[offset]&0x3f;                                  // 6
        c[1+count1] = (bin[offset]>>6) | (bin[offset+1]<<2)&0x3f;        // 2+4
        c[2+count1] = ((bin[offset+1]>>4)&0xf) | (bin[offset+2]<<4)&0x3f;// 4+2
        c[3+count1] = bin[offset+2]>>2;                          // 6
        c[0+count1] += 0x20;
        c[1+count1] += 0x20;
        c[2+count1] += 0x20;
        c[3+count1] += 0x20;
        offset += 3;
        count += 4;
	count1 +=4;
        if (count == 64) {
            count = 0;
	    c[count1++]=nl;
        }
    }
    write(fileno(f),c,count1);
}

/*
 *
 * $Log: encode.cpp,v $
 * Revision 1.2.4.2  2007/06/01 03:29:25  korpela
 * Fixes for linux build of Joe's updated code.
 *
 * Probably not working yet.
 *
 * Revision 1.2.4.1  2006/12/14 22:24:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/11 18:53:37  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/29 20:35:37  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.1  2003/06/03 00:16:11  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.3  1998/11/04 23:08:25  korpela
 * Byte and bit order change.
 *
 * Revision 2.2  1998/11/02  21:20:58  korpela
 * changed function name from encode() to splitter_encode() to avoid
 * conflict with encode routine in ../client/util.C.  Will investigate
 * if merging of two functions is possible.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.1  1998/10/27  00:52:56  korpela
 * Initial revision
 *
 *
 */


