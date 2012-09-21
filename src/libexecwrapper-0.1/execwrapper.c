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
#include <paths.h>
#include <stdarg.h>
#include <alloca.h>
#include <limits.h>

#include "config.h"

/*
 * wrapping other execs - ensure that execve will be executes always
 * from bionic
 */

extern char **environ;

int
execl(const char *name, const char *arg, ...)
{
        va_list ap;
        char **argv;
        int n;

        va_start(ap, arg);
        n = 1;
        while (va_arg(ap, char *) != NULL)
                n++;
        va_end(ap);
        argv = alloca((n + 1) * sizeof(*argv));
        if (argv == NULL) {
                errno = ENOMEM;
                return (-1);
        }
        va_start(ap, arg);
        n = 1;
        argv[0] = (char *)arg;
        while ((argv[n] = va_arg(ap, char *)) != NULL)
                n++;
        va_end(ap);
        return (execve(name, argv, environ));
}

int
execle(const char *name, const char *arg, ...)
{
        va_list ap;
        char **argv, **envp;
        int n;

        va_start(ap, arg);
        n = 1;
        while (va_arg(ap, char *) != NULL)
                n++;
        va_end(ap);
        argv = alloca((n + 1) * sizeof(*argv));
        if (argv == NULL) {
                errno = ENOMEM;
                return (-1);
        }
        va_start(ap, arg);
        n = 1;
        argv[0] = (char *)arg;
        while ((argv[n] = va_arg(ap, char *)) != NULL)
                n++;
        envp = va_arg(ap, char **);
        va_end(ap);
        return (execve(name, argv, envp));
}

int
execlp(const char *name, const char *arg, ...)
{
        va_list ap;
        char **argv;
        int n;

        va_start(ap, arg);
        n = 1;
        while (va_arg(ap, char *) != NULL)
                n++;
        va_end(ap);
        argv = alloca((n + 1) * sizeof(*argv));
        if (argv == NULL) {
                errno = ENOMEM;
                return (-1);
        }
        va_start(ap, arg);
        n = 1;
        argv[0] = (char *)arg;
        while ((argv[n] = va_arg(ap, char *)) != NULL)
                n++;
        va_end(ap);
        return (execvp(name, argv));
}

int
execv(const char *name, char * const *argv)
{
        (void)execve(name, argv, environ);
        return (-1);
}

int
execvp(const char *name, char * const *argv)
{
        char **memp;
        int cnt, lp, ln, len;
        char *p;
        int eacces = 0;
        char *bp, *cur, *path, buf[MAXPATHLEN];

        /*
         * Do not allow null name
         */
        if (name == NULL || *name == '\0') {
                errno = ENOENT;
                return (-1);
        }

        /* If it's an absolute or relative path name, it's easy. */
        if (strchr(name, '/')) {
                bp = (char *)name;
                cur = path = NULL;
                goto retry;
        }
        bp = buf;

        /* Get the path we're searching. */
        if (!(path = getenv("PATH")))
                path = _PATH_DEFPATH;
        len = strlen(path) + 1;
        cur = alloca(len);
        if (cur == NULL) {
                errno = ENOMEM;
                return (-1);
        }
        strlcpy(cur, path, len);
        path = cur;
        while ((p = strsep(&cur, ":"))) {
                /*
                 * It's a SHELL path -- double, leading and trailing colons
                 * mean the current directory.
                 */
                if (!*p) {
                        p = ".";
                        lp = 1;
                } else
                        lp = strlen(p);
                ln = strlen(name);

                /*
                 * If the path is too long complain.  This is a possible
                 * security issue; given a way to make the path too long
                 * the user may execute the wrong program.
                 */
                if (lp + ln + 2 > (int)sizeof(buf)) {
                        struct iovec iov[3];

                        iov[0].iov_base = "execvp: ";
                        iov[0].iov_len = 8;
                        iov[1].iov_base = p;
                        iov[1].iov_len = lp;
                        iov[2].iov_base = ": path too long\n";
                        iov[2].iov_len = 16;
                        (void)writev(STDERR_FILENO, iov, 3);
                        continue;
                }
                memcpy(buf, p, lp);
                buf[lp] = '/';
                memcpy(buf + lp + 1, name, ln);
                buf[lp + ln + 1] = '\0';

retry:          (void)execve(bp, argv, environ);
                switch(errno) {
                case E2BIG:
                        goto done;
                case EISDIR:
                case ELOOP:
                case ENAMETOOLONG:
                case ENOENT:
                        break;
                case ENOEXEC:
                        for (cnt = 0; argv[cnt]; ++cnt)
                                ;
                        memp = alloca((cnt + 2) * sizeof(char *));
                        if (memp == NULL)
                                goto done;
                        memp[0] = "sh";
                        memp[1] = bp;
                        memcpy(memp + 2, argv + 1, cnt * sizeof(char *));
                        (void)execve(_PATH_BSHELL, memp, environ);
                        goto done;
                case ENOMEM:
                        goto done;
                case ENOTDIR:
                        break;
                case ETXTBSY:
                        /*
                         * We used to retry here, but sh(1) doesn't.
                         */
                        goto done;
                case EACCES:
                        eacces = 1;
                        break;
                default:
                        goto done;
                }
        }
        if (eacces)
                errno = EACCES;
        else if (!errno)
                errno = ENOENT;
done:
        return (-1);
}


/*
 * main execve wrapper
 */

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

#ifdef DEBUG
  puts("realpathing");
#endif

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
