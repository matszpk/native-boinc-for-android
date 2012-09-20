/*
 * execwrapper.c
 * Author: Mateusz Szpakowski
 */

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

static int (*real_execve)(const char* filename, char* const argv[], char* const envp[]) = NULL;

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

int execve(const char* filename, char* const argv[], char* const envp[])
{
  size_t readed,newpathlen;
  int origfd = -1;
  int copyfd = -1;
  int lockfd = -1;
  char newpath[128];
  char newlockpath[128];
  uint32_t newnumber;
  char* mybuf = NULL;
  char* realfilename = NULL;
  char** newenvp = NULL;
  
  char* extstorage = NULL;
  size_t extstorage_len = 0;
  
  struct timeval tv1;

  if (real_execve == NULL)
    real_execve = dlsym(RTLD_NEXT, "execve");

  realfilename = malloc(PATH_MAX);
  if (realfilename == NULL)
    return -1;
  if (realpath(filename, realfilename) == NULL)
    return -1;
  
  extstorage = getenv("EXTERNAL_STORAGE");
  if (extstorage==NULL)
    extstorage = "/mnt/sdcard";
  extstorage_len = strlen(extstorage);
  if (extstorage[extstorage_len-1]=='/')
    extstorage_len--; // remove slash
  
#ifdef DEBUG
  printf("extstorage:%s\n",extstorage);
#endif
  
  // check whether is execfile in /data directory
  if (strncmp(realfilename,"/sdcard/",8)!=0 &&
      (strncmp(realfilename,extstorage,extstorage_len)!=0 ||
       realfilename[extstorage_len]!='/'))
  {
#ifdef DEBUG
    printf("exec directly:%s,%s,%s\n",realfilename,filename,argv[0]);
#endif
    free(realfilename);
    
    newenvp = setup_ldpreenvs(envp, lockfd);
    if (newenvp==NULL)
      goto error2;
    return real_execve(filename,argv,newenvp); // exec it
  }
      
  free(realfilename); // freeing it

  ///////////////////////////////
  // real execve wrapper
  ////////////////////////////////   
  if (access(BOINCEXECDIR,R_OK)!=0)
  {
#ifdef DEBUG
    puts("Creating needed directory");
#endif
    mkdir(BOINCEXECDIR, 0700);
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
  lockfd = open(newlockpath,O_CREAT|O_WRONLY,0600);
  if (lockfd == -1)
    goto error2; // fail
  flock(lockfd, LOCK_EX);
  
  mybuf = malloc(MYBUF_SIZE);
  if (mybuf == NULL)
    goto error2;
  
  // copy file
  origfd = open(filename,O_RDONLY);
  copyfd = open(newpath,O_WRONLY|O_CREAT,0700);
  
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
  
  free(mybuf);
  mybuf = NULL;
  
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
  
  if (mybuf!=NULL)
    free(mybuf);
  return -1;
}
