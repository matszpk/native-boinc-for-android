/*
 * execwrapper.c
 * Author: Mateusz Szpakowski
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dlfcn.h>

#include "config.h"

/*
 * main execve wrapper
 */

static int is_sdcard_dev_determined = 0;
static dev_t sdcard_dev = -1;

static int (*real_stat)(const char* path, struct stat* buf) = NULL;

static int (*real_execve)(const char* filename, char* const argv[], char* const envp[]) = NULL;

static int (*real_access)(const char* path, int flags) = NULL;
static int (*real_mkdir)(const char* dirpath, mode_t mode) = NULL;

static int (*real___open)(const char* file, int flags, mode_t mode) = NULL;

static void init_handles(void)
{
  if (real_execve == NULL)
  {
    real_execve = dlsym(RTLD_NEXT, "execve");
    real_stat = dlsym(RTLD_NEXT, "stat");
    
    real_access = dlsym(RTLD_NEXT, "access");
    real_mkdir = dlsym(RTLD_NEXT, "mkdir");
    
    real___open = dlsym(RTLD_NEXT, "__open");
  }
}

#define MYBUF_SIZE (1024)

static char** setup_ldpreenvs(char* const envp[], int lockfd)
{
  uint32_t i;
  size_t newenvp_n;
  char** newenvp = NULL;
  char envfdstr[40]; // environment text for fd
  char* ldpre_envstr = NULL;
  
  // prepare envinonments
  // counting
  for (newenvp_n = 0; envp[newenvp_n] != NULL; newenvp_n++);
  
#ifdef DEBUG
  printf("env_n=%u\n",newenvp_n);
#endif
  newenvp = malloc(sizeof(char*)*(newenvp_n+3));
  if (newenvp == NULL)
    goto error1;
  memcpy(newenvp,envp,sizeof(char*)*(newenvp_n+1));
  
  // pass lockfile fd
  if (lockfd != -1)
  {
    snprintf(envfdstr,32,FDENVNAME "=%d",lockfd);
#ifdef DEBUG
    printf("new fdenv:%s\n",envfdstr);
#endif
    // search in envp FDENVNAME
    for (i = 0; i < newenvp_n; i++)
      if (strncmp(newenvp[i],FDENVNAME "=",FDENVNAME_LEN+1)==0)
      { // if found, we replace it
        newenvp[i] = envfdstr;
#ifdef DEBUG
        puts("fdenv replaced");
#endif
        break;
      }
    
    if (i == newenvp_n)
    { // if not found, we adds
#ifdef DEBUG
      puts("fdenv added");
#endif
      newenvp[newenvp_n++] = envfdstr;
      newenvp[newenvp_n] = NULL;
    }
  }
  
  // check preload
  for (i = 0; i < newenvp_n; i++)
    if (strncmp(newenvp[i],"LD_PRELOAD=",11)==0)
      break;
  
  if (i == newenvp_n)
  { // if not found we add it
#ifdef DEBUG
    puts("ldpreload added");
#endif
    newenvp[newenvp_n++] = "LD_PRELOAD=" EW_PATH;
    newenvp[newenvp_n] = NULL;
  }
  else
  { // replace
#ifdef DEBUG
    puts("ldpreload replaced");
#endif
    newenvp[i] = "LD_PRELOAD=" EW_PATH;
  }
  return newenvp;
  
error1:
  if (newenvp!=NULL)
    free(newenvp);
  if (ldpre_envstr!=NULL)
    free(ldpre_envstr);
  return NULL;
}

static int check_sdcard(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (real_stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
#ifdef DEBUG
    printf("sdcard dev=%d\n",sdcard_dev);
#endif
    is_sdcard_dev_determined = 1;
  }
  
  if (real_stat(pathname,&stbuf)==-1)
    return 0;
#ifdef DEBUG
  printf("dev for %s=%llu\n",pathname,stbuf.st_dev);
#endif
  return stbuf.st_dev==sdcard_dev;
}

int execve(const char* filename, char* const argv[], char* const envp[])
{
  size_t readed,newpathlen;
  int origfd = -1;
  int copyfd = -1;
  int lockfd = -1;
  char newpath[128];
  char newlockpath[128];
  uint32_t newnumber;
  char mybuf[MYBUF_SIZE];
  char** newenvp = NULL;
  
  struct timeval tv1;

  init_handles();
  
  // check whether is execfile in /data directory
  if (!check_sdcard(filename))
  {
#ifdef DEBUG
    printf("exec directly:%s,%s\n",filename,argv[0]);
#endif
    newenvp = setup_ldpreenvs(envp, lockfd);
    if (newenvp==NULL)
      goto error2;
    return real_execve(filename,argv,newenvp); // exec it
  }
  
  ///////////////////////////////
  // real execve wrapper
  ////////////////////////////////
  if (access(filename,X_OK)!=0)
    return -1;
  
  if (real_access(BOINCEXECDIR,R_OK)!=0)
  {
#ifdef DEBUG
    puts("Creating needed directory");
#endif
    real_mkdir(BOINCEXECDIR, 0700);
  }
  
  // determine new paths
#ifdef TESTING
  newnumber = 7;
#else
  gettimeofday(&tv1,NULL);
  srand(tv1.tv_usec);
  newnumber = rand()%10;
#endif
  newpathlen = snprintf(newpath, 128, BOINCEXECDIR "/%u", newnumber);
  strcpy(newlockpath, newpath);
  newlockpath[newpathlen] = 'l';
  newlockpath[newpathlen+1] = 0;
  
#ifdef DEBUG
  printf("newpath:%s,newlock:%s\n",newpath,newlockpath);
#endif
  
  // lock lockfile
  lockfd = real___open(newlockpath,O_CREAT|O_WRONLY,0600);
  if (lockfd == -1)
    goto error2; // fail
  flock(lockfd, LOCK_EX);
  
  // copy file
  origfd = real___open(filename,O_RDONLY,0600);
  copyfd = real___open(newpath,O_WRONLY|O_CREAT|O_TRUNC,0700);
  
  if (origfd == -1)
    goto error2;
  if (copyfd == -1)
    goto error2;
  
  // copy loop
  do { 
    readed = read(origfd,mybuf,MYBUF_SIZE);
    write(copyfd,mybuf,readed);
  } while (readed == MYBUF_SIZE);
  
  close(origfd);
  close(copyfd);
  copyfd = origfd = -1;
  
  newenvp = setup_ldpreenvs(envp, lockfd);
  if (newenvp==NULL)
    goto error2;
    
  // main exec
  return real_execve(newpath, argv, newenvp);

error2:
  if (lockfd != -1)
  { // unlock lockfile
    flock(lockfd, LOCK_UN);
    close(lockfd);
  }
  if (origfd != -1)
    close(origfd);
  if (copyfd != -1)
    close(copyfd);
  return -1;
}
