/*
 * execperms.c 
 * Author: Mateusz Szpakowski
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dlfcn.h>
#include <limits.h>
#include <sys/limits.h>
#include <sys/param.h>
#include <pthread.h>

#include "config.h"

static int (*real_access)(const char* path, int flags) = NULL;

static int (*real_chmod)(const char* path, mode_t mode) = NULL;
static int (*real_fchmod)(int fd, mode_t mode) = NULL;

static int (*real_chown)(const char* path, uid_t uid, gid_t gid) = NULL;
static int (*real_fchown)(int fd, uid_t uid, gid_t gid) = NULL;
static int (*real_lchown)(const char* path, uid_t uid, gid_t gid) = NULL;

static int (*real_getdents)(unsigned int fd, struct dirent* dirp,
      unsigned int count) = NULL;

static int (*real_mkdir)(const char* dirpath, mode_t mode) = NULL;
static int (*real_unlink)(const char* file) = NULL;
static int (*real_rmdir)(const char* file) = NULL;

static int (*real_readlink)(const char* linkpath, char* buf, size_t bufsize) = NULL;

static int (*real_rename)(const char* oldpath, const char*newpath) = NULL;
static int (*real_link)(const char* oldpath, const char*newpath) = NULL;
static int (*real_symlink)(const char* oldpath, const char*newpath) = NULL;

static int (*real_truncate)(const char* path, off_t length) = NULL;
static int (*real_ftruncate)(int fd, off_t length) = NULL;

static int (*real___open)(const char* file, int flags, mode_t mode) = NULL;

static int (*real_stat)(const char* path, struct stat* buf) = NULL;
static int (*real_fstat)(int fd, struct stat* buf) = NULL;
static int (*real_lstat)(const char* path, struct stat* buf) = NULL;

static int (*real_utimes)(const char* filename, const struct timeval times[2]) = NULL;
static int (*real_futimes)(int fd, const struct timeval times[2]) = NULL;

static void init_handles(void)
{
  if (real_access == NULL)
  {
    real_access = dlsym(RTLD_NEXT, "access");
  
    real_chmod = dlsym(RTLD_NEXT, "chmod");
    real_fchmod = dlsym(RTLD_NEXT, "fchmod");
    
    real_chown = dlsym(RTLD_NEXT, "chown");
    real_fchown = dlsym(RTLD_NEXT, "fchown");
    real_lchown = dlsym(RTLD_NEXT, "lchown");
  
    real_getdents = dlsym(RTLD_NEXT, "getdents");
  
    real_mkdir = dlsym(RTLD_NEXT, "mkdir");
    real_unlink = dlsym(RTLD_NEXT, "unlink");
    real_rmdir = dlsym(RTLD_NEXT, "rmdir");
    
    real_readlink = dlsym(RTLD_NEXT, "readlink");
    
    real_rename = dlsym(RTLD_NEXT, "rename");
    real_link = dlsym(RTLD_NEXT, "link");
    real_symlink = dlsym(RTLD_NEXT, "symlink");
    
    real_truncate = dlsym(RTLD_NEXT, "truncate");
    real_ftruncate = dlsym(RTLD_NEXT, "ftruncate");
    
    real___open = dlsym(RTLD_NEXT, "__open");
    
    real_stat = dlsym(RTLD_NEXT, "stat");
    real_fstat = dlsym(RTLD_NEXT, "fstat");
    real_lstat = dlsym(RTLD_NEXT, "lstat");
    
    real_utimes = dlsym(RTLD_NEXT, "utimes");
    real_futimes = dlsym(RTLD_NEXT, "futimes");
  }
}

/* my buffered IO */

#define MYBUFSIZ (FILENAME_MAX+1)

typedef struct {
  size_t rest_pos;
  size_t rest;
  char buf[MYBUFSIZ];
} mybuf_t;

/* my buffered IO */
void mygetline_init(mybuf_t* buf)
{
  buf->rest_pos = 0;
  buf->rest = 0;
}

char* mygetline(mybuf_t* buf, int fd, char* line, size_t maxlength)
{
  size_t line_pos = 0;
  ssize_t readed;
  char* ptr2;
  size_t next_pos2;
  size_t next_linesize;
  // fetch from rest
  if (buf->rest_pos < buf->rest)
  {
    size_t next_pos;
    size_t linesize;
    char* ptr = memchr(buf->buf+buf->rest_pos,'\n',buf->rest-buf->rest_pos);
    
    if (ptr != NULL) // not found
    {
      next_pos = (ptrdiff_t)ptr-(ptrdiff_t)(buf->buf);
      linesize = next_pos-buf->rest_pos;
      if (linesize>maxlength)
        return NULL; // line too long
      memcpy(line,buf->buf+buf->rest_pos,linesize);
      line[linesize] = 0;
      buf->rest_pos = next_pos+1;
      return line;
    }
    else
    {
      line_pos = buf->rest-buf->rest_pos;
      if (line_pos>maxlength)
        return NULL; // line too long
      memcpy(line,buf->buf+buf->rest_pos,line_pos);
    }
  }
  
  // read next
  readed = read(fd, buf->buf, MYBUFSIZ);
  if (readed < 0 || (readed == 0 && line_pos == 0))
  {
    buf->rest = 0;
    buf->rest_pos = 0;
    return NULL; // error
  }
  if (readed == 0) // line is not empty
  {
    buf->rest = 0;
    buf->rest_pos = 0;
    line[line_pos] = 0;
    return line;
  }
  
  ptr2 = memchr(buf->buf,'\n',readed);
  if (ptr2 == NULL) // line too long
    return NULL;
  
  next_pos2 = (ptrdiff_t)ptr2-(ptrdiff_t)(buf->buf);
  next_linesize = line_pos + next_pos2;
  if (next_linesize>maxlength) // line too long
    return NULL;
  
  memcpy(line+line_pos, buf->buf, next_linesize-line_pos);
  line[next_linesize] = 0;
  
  buf->rest = readed - next_pos2-1;
  memmove(buf->buf, buf->buf+next_pos2+1, buf->rest);
  buf->rest_pos = 0;
  
  return line;
}

/*
 * myrealpath
 */

static pthread_mutex_t myrealpath_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *
myrealpath_int(path, resolved)
        const char *path;
        char *resolved;
{
        struct stat sb;
        int fd, n, rootd, serrno;
        char *p, *q, wbuf[MAXPATHLEN];
        int symlinks = 0;

        /* Save the starting point. */
        if ((fd = real___open(".", O_RDONLY, 0600)) < 0) {
                (void)strcpy(resolved, ".");
                return (NULL);
        }

        /*
         * Find the dirname and basename from the path to be resolved.
         * Change directory to the dirname component.
         * lstat the basename part.
         *     if it is a symlink, read in the value and loop.
         *     if it is a directory, then change to that directory.
         * get the current directory name and append the basename.
         */
        (void)strncpy(resolved, path, MAXPATHLEN - 1);
        resolved[MAXPATHLEN - 1] = '\0';
loop:
        q = strrchr(resolved, '/');
        if (q != NULL) {
                p = q + 1;
                if (q == resolved)
                        q = "/";
                else {
                        do {
                                --q;
                        } while (q > resolved && *q == '/');
                        q[1] = '\0';
                        q = resolved;
                }
                if (chdir(q) < 0)
                        goto err1;
        } else
                p = resolved;

        /* Deal with the last component. */
        if (*p != '\0' && real_lstat(p, &sb) == 0) {
                if (S_ISLNK(sb.st_mode)) {
                      if (++symlinks > MAXSYMLINKS) {
                              errno = ELOOP;
                              goto err1;
                      }
                        n = real_readlink(p, resolved, MAXPATHLEN - 1);
                        if (n < 0)
                                goto err1;
                        resolved[n] = '\0';
                        goto loop;
                }
                if (S_ISDIR(sb.st_mode)) {
                        if (chdir(p) < 0)
                                goto err1;
                        p = "";
                }
        }

        /*
         * Save the last component name and get the full pathname of
         * the current directory.
         */
        (void)strcpy(wbuf, p);
        if (getcwd(resolved, MAXPATHLEN) == 0)
                goto err1;

        /*
         * Join the two strings together, ensuring that the right thing
         * happens if the last component is empty, or the dirname is root.
         */
        if (resolved[0] == '/' && resolved[1] == '\0')
                rootd = 1;
        else
                rootd = 0;

        if (*wbuf) {
                if (strlen(resolved) + strlen(wbuf) + (1-rootd) + 1 >
                    MAXPATHLEN) {
                        errno = ENAMETOOLONG;
                        goto err1;
                }
                if (rootd == 0)
                        (void)strcat(resolved, "/");
                (void)strcat(resolved, wbuf);
        }

        /* Go back to where we came from. */
        if (fchdir(fd) < 0) {
                serrno = errno;
                goto err2;
        }

        /* It's okay if the close fails, what's an fd more or less? */
        (void)close(fd);
        return (resolved);

err1:   serrno = errno;
        (void)fchdir(fd);
err2:   (void)close(fd);
        errno = serrno;
        return (NULL);
}

static char* myrealpath(const char* path, char* resolved)
{
  char* ret=NULL;
  pthread_mutex_lock(&myrealpath_mutex);
  ret = myrealpath_int(path, resolved);
  pthread_mutex_unlock(&myrealpath_mutex);
  return ret;
}

/* check whether is /mnt/sdcard */

static int is_sdcard_dev_determined = 0;
static dev_t sdcard_dev = -1;

int check_sdcard(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (real_stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (real_stat(pathname,&stbuf)==-1)
    return 0;
  return stbuf.st_dev==sdcard_dev;
}

int check_sdcard_link(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (real_stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (real_lstat(pathname,&stbuf)==-1)
    return 0;
  return stbuf.st_dev==sdcard_dev;
}

int check_sdcard_fd(int fd)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (real_stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (real_fstat(fd,&stbuf)==-1)
    return 0;
  return stbuf.st_dev==sdcard_dev;
}

char* get_realpath_fd(int fd, char* pathbuf)
{
  char procfdname[40];
  struct stat stbuf;
  ssize_t s;
  if (real_fstat(fd,&stbuf)==-1)
    return NULL;
  
  if (stbuf.st_nlink==0)
    return NULL;
  
  // read from proc/self
  snprintf(procfdname,40,"/proc/self/fd/%d",fd);
  s = real_readlink(procfdname,pathbuf,PATH_MAX);
  if (s==-1)
    return NULL;
  pathbuf[s] = 0;
  
  if (pathbuf[0]!='/')
    return NULL;
  
  return pathbuf;
}

int is_execsfile(const char* realpathname)
{
  size_t len = strlen(realpathname);
  if (len < EXECS_NAME_LEN)
    return 0;
  
  if (len == EXECS_NAME_LEN && strcmp(realpathname,EXECS_NAME)==0)
    return 1;
  if (strcmp(realpathname+len-EXECS_NAME_LEN-1,"/" EXECS_NAME)==0)
    return 1;
  return 0;
}

// must be sdcard file
int check_execmode(const char* realpathname)
{
  char execsname[PATH_MAX];
  const char* lastslash = strrchr(realpathname,'/');
  char line[FILENAME_MAX+1];
  const char* filename;
  mybuf_t buf;
  int fd;
  size_t dirlen;

  if (lastslash == NULL)
    lastslash = realpathname;
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  memcpy(execsname,realpathname,dirlen);
  strcpy(execsname+dirlen,"/" EXECS_NAME);
  
  filename = lastslash+1;
  
  fd = real___open(execsname,O_RDONLY,0000);
  if (fd == -1)
    return 0; // no executables
  
  flock(fd, LOCK_SH); // we need read
  mygetline_init(&buf);
  
  while (mygetline(&buf,fd,line,FILENAME_MAX)!=NULL)
  {
    if (strcmp(filename,line)==0)
    { // if found
      flock(fd, LOCK_UN); // unlock it
      close(fd);
      return 1;
    }
  }
  
  flock(fd, LOCK_UN); // unlock it
  close(fd);
  return 0; // not found
}

// must be sdcard file
int set_execmode(const char* realpathname, int isexec)
{
  char execsname[PATH_MAX];
  const char* lastslash = strrchr(realpathname,'/');
  char line[FILENAME_MAX+1];
  const char* filename;
  char filenamelen;
  mybuf_t buf;
  int fd;
  size_t dirlen;
  off_t tmppos,execpos = -1;
  
  if (lastslash == NULL)
    lastslash = realpathname;
  
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  memcpy(execsname,realpathname,dirlen);
  strcpy(execsname+dirlen,"/" EXECS_NAME);
  
  filename = lastslash+1;
  filenamelen = strlen(filename);
  
  fd = real___open(execsname,O_CREAT|O_RDWR,0600);
  if (fd == -1)
    return -1; // no executables
    
  flock(fd, LOCK_EX); // we need write
  mygetline_init(&buf);
  
  tmppos = 0;
  while (mygetline(&buf,fd,line,FILENAME_MAX)!=NULL)
  {
    if (strcmp(filename,line)==0)
    { // if found
      execpos = tmppos;
      break;
    }
    tmppos += strlen(line)+1;
  }
  
  if (isexec)
  { // set exec
    if (execpos == -1)
    { // not yet set_exec
      lseek(fd,0,SEEK_END);
      write(fd,filename,filenamelen);
      write(fd,"\n",1);
    }
  }
  else
  { // unset exec
    if (execpos != -1)
    {
      int readed;
      off_t ipos = execpos;
      char mybuf[256];
      
      // move to back
      do {
        lseek(fd,ipos+filenamelen+1,SEEK_SET);
        readed = read(fd,mybuf,256);
        if (readed == -1)
          break;
        lseek(fd,ipos,SEEK_SET);
        write(fd,mybuf,readed);
        ipos += readed;
      } while(readed == 256);
      real_ftruncate(fd,ipos); // delete obsolete
    }
  }
  
  flock(fd, LOCK_UN); // unlock it
  close(fd);
  return 0;
}

/***
 * WRAPPERS 
 ***/

int access(const char* pathname, int mode)
{
  char realpathname[PATH_MAX];
  int hasexecmode = 0;
  int ret;
  struct stat stbuf;
#ifdef DEBUG
  printf("call access(%s,%d)\n",pathname,mode);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_access(pathname, mode);

  if (myrealpath(pathname, realpathname)==NULL)
    return real_access(pathname, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  if (real_stat(realpathname,&stbuf) == -1)
    return real_access(pathname, mode);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_access(pathname, mode);
  
  if (mode == F_OK || (mode & X_OK) == 0) // if existence or not executable
    return real_access(pathname, mode);
  
  hasexecmode = check_execmode(realpathname);
  ret = real_access(pathname, mode^X_OK); // if no flags, we check existence
  if (ret == 0) // other is ok
  {
    if (!hasexecmode)
    { // we dont have exec perms
      errno = EACCES;
      return -1;
    }
    return 0;
  }
  return ret;
}

int chmod(const char* pathname, mode_t mode)
{
  char realpathname[PATH_MAX];
  struct stat stbuf;
#ifdef DEBUG
  printf("call chmod(%s,%d)\n",pathname,mode);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_chmod(pathname, mode);
  
  if (myrealpath(pathname, realpathname)==NULL)
    return real_chmod(pathname, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  if (real_stat(realpathname,&stbuf) == -1)
    return real_chmod(pathname, mode);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_chmod(pathname, mode);
  
  // set exec mode
  if(set_execmode(realpathname,(mode&0111))==-1)
    return -1;
  return real_chmod(pathname,mode);
}

int fchmod(int fd, mode_t mode)
{
  char realpathname[PATH_MAX];
  struct stat stbuf;
#ifdef DEBUG
  printf("call fchmod(%d,%d)\n",fd,mode);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_fchmod(fd, mode);
  
  if (get_realpath_fd(fd,realpathname)==NULL)
    return real_fchmod(fd, mode);
  
  if (is_execsfile(realpathname))
  {
    errno = ENOENT;
    return -1;
  }
  
  if (real_fstat(fd,&stbuf)==-1)
    return real_fchmod(fd, mode);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_fchmod(fd, mode);
  
  // set exec mode
  if(set_execmode(realpathname,(mode&0111))==-1)
    return -1;
  return real_fchmod(fd, mode);
}

int chown(const char* pathname, uid_t uid, gid_t gid)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call chown(%s,%u,%u)\n",pathname,uid,gid);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_chown(pathname, uid, gid);
  
  if (myrealpath(pathname, realpathname)==NULL)
    return real_chown(pathname, uid, gid);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_chown(pathname, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call fchown(%d,%u,%u)\n",fd,uid,gid);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_fchown(fd, uid, gid);
  
  if (get_realpath_fd(fd, realpathname)==NULL)
    return real_fchown(fd, uid, gid);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_fchown(fd, uid, gid);
}

int lchown(const char* pathname, uid_t uid, gid_t gid)
{
  init_handles();
#ifdef DEBUG
  printf("call lchown(%s,%u,%u\n",pathname,uid,gid);
#endif
  
  if (!check_sdcard_link(pathname))
    return real_lchown(pathname, uid, gid);
  
  if (is_execsfile(pathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_lchown(pathname, uid, gid);
}

int getdents(unsigned int fd, struct dirent* dirp, unsigned int count)
{
  int rc, pos, entry;
  int execsfound = 0;
  char realpathname[PATH_MAX];
  struct dirent* dentry;
  struct dirent* dend;
#ifdef DEBUG
  printf("call getdents(%d,%p,%u)\n",fd,dirp,count);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_getdents(fd, dirp, count);
  
  if (get_realpath_fd(fd, realpathname)==NULL)
    return real_getdents(fd, dirp, count);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  rc = real_getdents(fd, dirp, count);
  if (rc <= 0)
    return rc;
  // scan for EXECS
  
  entry = 0;
  dend = (struct dirent*)(((char*)dirp)+rc);
  for (dentry = dirp, pos = 0; dentry < dend;)
  {
    //printf("getdents:d_name:%s\n",dentry->d_name);
    if (strcmp(dentry->d_name,EXECS_NAME)==0)
    {
      execsfound = 1;
      break;
    }
    dentry = (struct dirent*)(((char*)dentry)+dentry->d_reclen);
    entry++;
  }
  
  if (execsfound)
  {
    //printf("execsfound\n");
    memmove(dentry,((char*)dentry)+dentry->d_reclen,
            (ptrdiff_t)dend-(ptrdiff_t)dentry-dentry->d_reclen);
    
    rc -= dentry->d_reclen;
  }
  return rc;
}


int mkdir(const char* dirpath, mode_t mode)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call mkdir(%s,%d)\n",dirpath,mode);
#endif
  
  if (!check_sdcard(dirpath))
    return real_mkdir(dirpath, mode);
  
  if (myrealpath(dirpath, realpathname)==NULL)
    return real_mkdir(dirpath, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = EINVAL;
    return -1;
  }
  
  return real_mkdir(dirpath, mode);
}


int unlink(const char* filename)
{
  char realpathname[PATH_MAX];
  struct stat stbuf;
#ifdef DEBUG
  printf("call unlink(%s)\n",filename);
#endif
  
  init_handles();
  
  if (!check_sdcard(filename))
    return real_unlink(filename);
  
  if (myrealpath(filename, realpathname)==NULL)
    return real_unlink(filename);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  if (stat(realpathname,&stbuf) == -1)
    return real_unlink(filename);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_unlink(filename);
  
  if (set_execmode(realpathname,0) == -1) // unset exec
    return -1;
  return real_unlink(filename);
}

int rmdir(const char* dirpath)
{
  int fd, ret;
  char realpathname[PATH_MAX];
  struct stat stbuf;
  char execspath[PATH_MAX];
  size_t dirpathlen;
  char* execscopy = NULL;
#ifdef DEBUG
  printf("call rmdir(%s)\n",dirpath);
#endif
  
  init_handles();
  
  if (!check_sdcard(dirpath))
    return real_rmdir(dirpath);
  
  if (myrealpath(dirpath, realpathname)==NULL)
    return real_rmdir(dirpath);  // cant determine realpath
  
  dirpathlen = strlen(dirpath);
  strcpy(execspath, dirpath);
  if (dirpathlen > 0  && dirpath[dirpathlen-1] != '/')
  {
    execspath[dirpathlen] = '/';
    dirpathlen++;
  }
  strcpy(execspath+dirpathlen,EXECS_NAME);
  
  // buffering .__execs__ before rmdir
  if ((fd = real___open(execspath,O_RDONLY,0644))==0)
  {
    flock(fd,LOCK_SH);
    if (stat(execspath,&stbuf)==0)
    {
      execscopy = malloc(stbuf.st_size);
      read(fd,execscopy,stbuf.st_size);
    }
    flock(fd,LOCK_UN);
    close(fd);
  }
  
  real_unlink(execspath);
  ret = real_rmdir(dirpath);
  
  if (ret != 0 && execscopy!=NULL)
  { // if fail, we recreate .__execs__
    if ((fd = real___open(execspath,O_CREAT|O_WRONLY,0600))==0)
    {
      flock(fd,LOCK_EX);
      write(fd,execscopy,stbuf.st_size);
      flock(fd,LOCK_UN);
      close(fd);
    }
  }
  if (execscopy!=NULL)
    free(execscopy);
  
  return ret;
}

int readlink(const char* linkpath, char* buf, size_t bufsize)
{
  char refpath[PATH_MAX];
  int size;
#ifdef DEBUG
  printf("call readlink(%s,%p,%u)\n",linkpath,buf,bufsize);
#endif
  size = real_readlink(linkpath, refpath, bufsize);
  
  if (size==-1)  // cant determine realpath
    return -1;
  
  refpath[size] = 0;
  
  if (!check_sdcard_link(refpath))
  {
    memcpy(buf,refpath,size);
    return size;
  }
  
  if (is_execsfile(refpath)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  memcpy(buf,refpath,size);
  return size;
}

int __open(const char* filename, int flags, mode_t mode)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call __open(%s,%d,%d)\n",filename,flags,mode);
#endif
  
  init_handles();
  
  if (!check_sdcard(filename))
    return real___open(filename, flags, mode);
  
  if (myrealpath(filename, realpathname)==NULL)
    return real___open(filename, flags, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real___open(filename, flags, mode);
}

int rename(const char* oldpath, const char*newpath)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call rename(%s,%s)\n",oldpath,newpath);
#endif
  
  init_handles();
  
  if (!check_sdcard(oldpath) && !check_sdcard(newpath))
    return real_rename(oldpath, newpath);
  
  if (myrealpath(oldpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = ENOENT;
    return -1;
  }
  
  if (myrealpath(newpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = EINVAL;
    return -1;
  }
  
  return real_rename(oldpath, newpath);
}

int link(const char* oldpath, const char*newpath)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call link(%s,%s)\n",oldpath,newpath);
#endif
  
  init_handles();
  
  if (!check_sdcard(oldpath) && !check_sdcard(newpath))
    return real_link(oldpath, newpath);
  
  if (myrealpath(oldpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = ENOENT;
    return -1;
  }
  
  if (myrealpath(newpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = EINVAL;
    return -1;
  }
  
  return real_link(oldpath, newpath);
}

int symlink(const char* oldpath, const char*newpath)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call symlink(%s,%s)\n",oldpath,newpath);
#endif
  
  init_handles();
  
  if (!check_sdcard(oldpath) && !check_sdcard(newpath))
    return real_symlink(oldpath, newpath);
  
  if (myrealpath(oldpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = ENOENT;
    return -1;
  }
  
  if (myrealpath(newpath, realpathname)!=NULL &&
      is_execsfile(realpathname))
  {
    errno = EINVAL;
    return -1;
  }
  
  return real_symlink(oldpath, newpath);
}

int truncate(const char* pathname, off_t length)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call truncate(%s,%lu)\n",pathname,length);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_truncate(pathname, length);
  
  if (myrealpath(pathname, realpathname)==NULL)
    return real_truncate(pathname, length);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_truncate(pathname, length);
}

int ftruncate(int fd, off_t length)
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call ftruncate(%d,%lu)\n",fd,length);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_ftruncate(fd, length);
  
  if (get_realpath_fd(fd, realpathname)==NULL)
    return real_ftruncate(fd, length);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_ftruncate(fd, length);
}

int stat(const char* pathname, struct stat* buf)
{
  char realpathname[PATH_MAX];
  int ret;
#ifdef DEBUG
  printf("call stat(%s,%p)\n",pathname,buf);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_stat(pathname, buf);
  
  if (myrealpath(pathname, realpathname)==NULL)
    return real_stat(pathname, buf);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  ret = real_stat(pathname,buf);
  if (ret == -1)
    return -1;
  
  if (!S_ISDIR(buf->st_mode)) // is directory, we use execs for files
    buf->st_mode &= (check_execmode(realpathname) ? 0111 : 0);
  return ret;
}

int fstat(int fd, struct stat* buf)
{
  char realpathname[PATH_MAX];
  int ret;
#ifdef DEBUG
  write(1,"call fstat\n",11);
  //printf("call fstat(%d,%p)\n",fd,buf);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_fstat(fd, buf);
  
  if (get_realpath_fd(fd, realpathname)==NULL)
    return real_fstat(fd, buf);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  ret = real_fstat(fd, buf);
  if (ret == -1)
    return -1;
  
  if (!S_ISDIR(buf->st_mode)) // is directory, we use execs for files
  {
    buf->st_mode &= ~0111;
    buf->st_mode |= (check_execmode(realpathname) ? 0111 : 0);
  }
  return ret;
}

int lstat(const char* pathname, struct stat* buf)
{
  int ret;
#ifdef DEBUG
  printf("call lstat(%s,%p)\n",pathname,buf);
#endif
  
  init_handles();
  
  if (!check_sdcard_link(pathname))
    return real_stat(pathname, buf);
  
  if (is_execsfile(pathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  ret = real_lstat(pathname,buf);
  if (ret == -1)
    return -1;
  
  if (!S_ISDIR(buf->st_mode) && !S_ISLNK(buf->st_mode))
  {
    buf->st_mode &= ~0111;
    buf->st_mode |= (check_execmode(pathname) ? 0111 : 0);
  }
  return ret;
}

int utimes(const char* pathname, const struct timeval times[2])
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call utimes(%s,%p)\n",pathname,times);
#endif
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_utimes(pathname, times);
  
  if (myrealpath(pathname, realpathname)==NULL)
    return real_utimes(pathname, times);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_utimes(pathname, times);
}

int futimes(int fd, const struct timeval times[2])
{
  char realpathname[PATH_MAX];
#ifdef DEBUG
  printf("call futimes(%d,%p)\n",fd,times);
#endif
  
  init_handles();
  
  if (!check_sdcard_fd(fd))
    return real_futimes(fd, times);
  
  if (get_realpath_fd(fd, realpathname)==NULL)
    return real_futimes(fd, times);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  return real_futimes(fd, times);
}
