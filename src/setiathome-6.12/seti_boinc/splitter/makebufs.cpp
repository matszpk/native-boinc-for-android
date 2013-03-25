/*
 * makebuf.c
 *
 * Creates temporary files and buffers for use in processing....
 *
 * $Id: makebufs.cpp,v 1.3.2.2 2006/12/14 22:24:39 korpela Exp $
 *
 */

#include "sah_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "splitparms.h"
#include "splittypes.h"
#include "splitter.h"
#include "message.h"
 
int tapebuffd;
char tapebufname[30];

void delbuffer(void) {
  munmap((char *)tapebuffer,TAPE_BUFFER_SIZE);
  close(tapebuffd);
  unlink(tapebufname);
}

void delbuffer_sig(int i) {
    munmap((char *)tapebuffer,TAPE_BUFFER_SIZE); 
    close(tapebuffd);
    unlink(tapebufname);
    exit(1);
}



void makebuffers(unsigned char **tapebuffer) {
  char buf[1024];
/* 
 * Allocate temp files for tape and wu buffers.  Memory mapping these
 * files will prevent us from running out of virtual memory
 */
/*  sprintf(tapebufname,"tape%d",getpid());
  
  if ((tapebuffd=open(tapebufname,O_RDWR|O_CREAT,0777))==-1) {
    fprintf(stderr,"Unable to open temp file!\n");
    fprintf(errorlog,"Unable to open temp file!\n");
    exit(EXIT_FAILURE);
  }
  
  if (ftruncate(tapebuffd,TAPE_BUFFER_SIZE) == -1) {
    fprintf(stderr,"ftruncate failure\n");
    fprintf(stderr," errno=%d\n",errno);
    fprintf(errorlog,"ftruncate failure\n");
    fprintf(errorlog," errno=%d\n",errno);
    exit(EXIT_FAILURE);
  }

  if (((*tapebuffer=
     mmap(0,TAPE_BUFFER_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,tapebuffd,0))==(char *)-1))
  { 
    fprintf(stderr,"mmap failure\n");
    fprintf(errorlog,"mmap failure\n");
    exit(EXIT_FAILURE);
  }

  atexit(delbuffer);
  signal(SIGINT,delbuffer_sig);
*/
  if (!(*tapebuffer=(unsigned char *)malloc(TAPE_BUFFER_SIZE))) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"unable to allocate memory for tape buffer\n");
    exit(0);
  }
}

/*
 * $Log: makebufs.cpp,v $
 * Revision 1.3.2.2  2006/12/14 22:24:39  korpela
 * *** empty log message ***
 *
 * Revision 1.3.2.1  2006/01/13 00:37:57  korpela
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
 * Revision 1.3  2004/06/16 20:57:18  jeffc
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/29 20:35:42  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.1  2003/06/03 00:16:13  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.3  1998/12/14 23:41:44  korpela
 * *** empty log message ***
 *
 * Revision 2.2  1998/11/02 21:20:58  korpela
 * Added signal handler for removal of temporary files on SIGINT.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.4  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.3  1998/10/27  00:55:43  korpela
 * Bug Fixes
 *
 * Revision 1.2  1998/10/20  20:40:54  korpela
 * Removed wu buffer.
 *
 * Revision 1.1  1998/10/19  19:01:56  korpela
 * Initial revision
 *
 */
