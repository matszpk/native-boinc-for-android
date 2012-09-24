/*
 * execinit.c
 * Author: Mateusz Szpakowski
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/file.h>
#include <dlfcn.h>

#include "config.h"

static void (*real_libc_init)(uintptr_t *elfdata,void (*onexit)(void),
    int (*slingshot)(int, char**, char**),void* structors) = NULL;
static int (*real_unlink)(const char* path) = NULL;
static int (*real_readlink)(const char* linkpath, char* buf, size_t bufsize) = NULL;

void __libc_init(uintptr_t *elfdata, void (*onexit)(void),
    int (*slingshot)(int, char**, char**),void* structors)
{
  int lockfd = -1;
  char* fdstr = NULL;
  char* selfpath = NULL;
  if (real_libc_init == NULL)
  {
    real_libc_init = dlsym(RTLD_NEXT, "__libc_init");
    real_unlink = dlsym(RTLD_NEXT, "unlink");
    real_readlink = dlsym(RTLD_NEXT, "readlink");
  }
  
  fdstr=getenv(FDENVNAME);
  if (fdstr != NULL && sscanf(fdstr,"%d",&lockfd)==1)
  { // if lockfd
#ifdef DEBUG
    printf("lockfd:%d\n",lockfd);
#endif

#ifdef SLEEPING
    sleep(10);
#endif
    selfpath = malloc(PATH_MAX);
    if (selfpath != NULL)
    { // remove self exec
      int size = real_readlink("/proc/self/exe",selfpath, PATH_MAX);
      if (size != -1)
        selfpath[size] = 0;
      else
        selfpath[0] = 0;
      if (strncmp(selfpath,BOINCEXECDIR,BOINCEXECDIR_LEN)==0)
      {
#ifdef DEBUG
        printf("remove self:%s\n",selfpath);
#endif
        real_unlink(selfpath);
      }
      free(selfpath);
    }
  
    flock(lockfd,LOCK_UN);
    close(lockfd);
  }
  // unset obsolete evnvars
  unsetenv(FDENVNAME);
  
  // checking and processing LD_PRELOAD
  unsetenv("LD_PRELOAD");
  
#ifdef DEBUG
  puts("Go to run");
  printf("init:%p\n",real_libc_init);
#endif
  real_libc_init(elfdata,onexit,slingshot,structors);
}
