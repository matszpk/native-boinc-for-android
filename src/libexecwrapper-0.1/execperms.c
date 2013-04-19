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

#ifdef PROD_DEBUG
#include <stdarg.h>
#include <time.h>
#endif

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
static int (*real_symlink)(const char* oldpath, const char*newpath) = NULL;

static int (*real_truncate)(const char* path, off_t length) = NULL;
static int (*real_ftruncate)(int fd, off_t length) = NULL;

static int (*real___open)(const char* file, int flags, mode_t mode) = NULL;

static int (*real_stat)(const char* path, struct stat* buf) = NULL;
static int (*real_fstat)(int fd, struct stat* buf) = NULL;
static int (*real_lstat)(const char* path, struct stat* buf) = NULL;

static int (*real_utimes)(const char* filename, const struct timeval times[2]) = NULL;
static int (*real_futimes)(int fd, const struct timeval times[2]) = NULL;

#ifdef OLD_HANDLES
int old_access(const char* path, int flags)
{
  return real_access(path, flags);
}

int old_chmod(const char* path, mode_t mode)
{
  return real_chmod(path, mode);
}

int old_fchmod(int fd, mode_t mode)
{
  return real_fchmod(fd, mode);
}

int old_chown(const char* path, uid_t uid, gid_t gid)
{
  return real_chown(path, uid, gid);
}

int old_fchown(int fd, uid_t uid, gid_t gid)
{
  return real_fchown(fd, uid, gid);
}

int old_lchown(const char* path, uid_t uid, gid_t gid)
{
  return real_lchown(path, uid, gid);
}

int old_getdents(unsigned int fd, struct dirent* dirp,
      unsigned int count)
{
  return real_getdents(fd, dirp, count);
}

int old_mkdir(const char* dirpath, mode_t mode)
{
  return real_mkdir(dirpath, mode);
}

int old_unlink(const char* file)
{
  return real_unlink(file);
}

int old_rmdir(const char* file)
{
  return real_rmdir(file);
}

int old_readlink(const char* linkpath, char* buf, size_t bufsize)
{
  return real_readlink(linkpath, buf, bufsize);
}


int old_rename(const char* oldpath, const char*newpath)
{
  return real_rename(oldpath, newpath);
}

int old_symlink(const char* oldpath, const char*newpath)
{
  return real_symlink(oldpath, newpath);
}

int old_truncate(const char* path, off_t length)
{
  return real_truncate(path, length);
}

int old_ftruncate(int fd, off_t length)
{
  return real_ftruncate(fd, length);
}


int old___open(const char* file, int flags, mode_t mode)
{
  return real___open(file, flags, mode);
}

int old_stat(const char* path, struct stat* buf)
{
  return real_stat(path, buf);
}

int old_fstat(int fd, struct stat* buf)
{
  return real_fstat(fd, buf);
}

int old_lstat(const char* path, struct stat* buf)
{
  return real_lstat(path, buf);
}


int old_utimes(const char* filename, const struct timeval times[2])
{
  return real_utimes(filename, times);
}

int old_futimes(int fd, const struct timeval times[2])
{
  return real_futimes(fd, times);
}
#endif

static volatile int handles_inited = 0;

static void init_handles(void)
{
  if (handles_inited == 0)
  {
    handles_inited = 1;
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
    real_symlink = dlsym(RTLD_NEXT, "symlink");
    
    real_truncate = dlsym(RTLD_NEXT, "truncate");
    real_ftruncate = dlsym(RTLD_NEXT, "ftruncate");
    
    real___open = dlsym(RTLD_NEXT, "__open");
    
    real_stat = dlsym(RTLD_NEXT, "stat");
    real_fstat = dlsym(RTLD_NEXT, "fstat");
    real_lstat = dlsym(RTLD_NEXT, "lstat");
    
    real_utimes = dlsym(RTLD_NEXT, "utimes");
    real_futimes = dlsym(RTLD_NEXT, "futimes");
    
    handles_inited = 2; // finish
  }
  else
    while (handles_inited == 1);
}

#ifdef PROD_DEBUG
static void debug_printf(const char* fmt, ...)
{
  int fd;
  int outsize;
  char* out;
  va_list ap;
  out = malloc(4000);
  fd = real___open("/mnt/sdcard/libew_debug.txt",O_CREAT|O_WRONLY|O_APPEND, 0666);
  if (fd == -1)
  {
    free(out);
    return;
  }
  
  outsize = snprintf(out,4000,"%d:%lu: ",getpid(),time(NULL));
  write(fd, out, outsize);
  
  va_start(ap, fmt);
  outsize = vsnprintf(out,4000,fmt,ap);
  write(fd, out, outsize);
  
  va_end(ap);
  close(fd);
  free(out);
}
#endif

/* EPERM tester (for regular account processes)
 * we are checking what operation can be done on the regular account processes */
#ifdef CHECK_SD_PERMS
void check_sd_perms(void)
{
}
#endif

/* my buffered IO */

#define MYBUFSIZ (FILENAME_MAX+1)

typedef struct {
  size_t rest_pos;
  size_t rest;
  char buf[MYBUFSIZ];
} mybuf_t;

/* my buffered IO */
static void mygetline_init(mybuf_t* buf)
{
  buf->rest_pos = 0;
  buf->rest = 0;
}

static char* mygetline(mybuf_t* buf, int fd, char* line, size_t maxlength)
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

static pthread_mutex_t myrealpath_lock = PTHREAD_MUTEX_INITIALIZER;

static char* myrealpath(const char* path, char* resolved)
{
  size_t len;
  int pos = 0;
  const char *s;
  if (path == NULL)
    return NULL;
  
  if (*path=='/') // absolute
  {
    strcpy(resolved,path);
    len = strlen(resolved);
    if (len > 1 && resolved[len-1] == '/')
      resolved[len-1] = 0;
    return resolved;
  }
  
  pthread_mutex_lock(&myrealpath_lock);
  
  if (getcwd(resolved,PATH_MAX)==NULL)
  {
    pthread_mutex_unlock(&myrealpath_lock);
    return NULL;
  }
  pos = strlen(resolved);
  if (resolved[pos-1] != '/')
    resolved[pos++] = '/';
  
  s = path;
  /* scan path */
  while (*s != 0)
  {
    if (*s == '.' && s[1] == '.' && (s[2]==0 || s[2]=='/'))
    {
      if (pos > 1)
      {
        if (resolved[pos-1] == '/') pos -= 2;
        while (pos != 0 && resolved[pos] != '/') pos--;
      }
      s+=2;
    }
    else if (*s == '.' && (s[1]==0 || s[1]=='/'))
    {
      s++;
      if (*s == '/')
        s++;
      continue;
    }
    else
    {
      while (*s != '/' && *s != 0)
        resolved[pos++] = *s++;
    }
        
    if (*s == '/')
    {
      resolved[pos++] = '/';
      s++;
    }
  }
  resolved[pos] = 0;
  
  if (pos > 1 && resolved[pos-1] == '/')
    resolved[pos-1] = 0;
  
  pthread_mutex_unlock(&myrealpath_lock);
  return resolved;
}

/* check whether is /mnt/sdcard */

static int is_sdcard_dev_determined = 0;
static dev_t sdcard_dev = -1;

static char* sdcard_dir = NULL;
static char sdcard_dir_slash[256];
static size_t sdcard_dir_slash_len = 0;

static void init_sdcard_dirs(void)
{
  if (sdcard_dir == NULL)
  {
    char* xsdcard_dir = NULL;
    size_t len = 0;
    size_t lastchar = 0;
    xsdcard_dir = getenv(SDCARDENVNAME);
    if (env_sdcard_dir == NULL)
      xsdcard_dir = "/mnt/sdcard";
    len = strlen(xsdcard_dir);
    lastchar = len>254?254:len;
    
    strncpy(sdcard_dir_slash, xsdcard_dir, lastchar);
    sdcard_dir_slash[lastchar] = '/';
    sdcard_dir_slash[lastchar+1] = 0;
    sdcard_dir = xsdcard_dir; // save after determining
    sdcard_dir_slash_len = lastchar+1;
  }
}

static int check_sdcard(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    init_sdcard_dirs();  
    if (real_stat(sdcard_dir,&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (strcmp(pathname,sdcard_dir)==0 || strcmp(pathname,sdcard_dir_slash)==0)
    return 0;
  
  if (real_stat(pathname,&stbuf)==-1)
  {
    if (real_access(pathname,F_OK)==-1)  // dir path is not available
    {
      if (strncmp(pathname,sdcard_dir_slash,sdcard_dir_slash_len)==0)
        return 1;
    }
    return 0;
  }
  return stbuf.st_dev==sdcard_dev;
}

static int check_sdcard_link(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    init_sdcard_dirs();  
    if (real_stat(sdcard_dir,&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (strcmp(pathname,sdcard_dir)==0 || strcmp(pathname,sdcard_dir_slash)==0)
    return 0;
  
  if (real_lstat(pathname,&stbuf)==-1)
  {
    if (real_access(pathname,F_OK)==-1) // dir path is not available
    {
      if (strncmp(pathname,sdcard_dir_slash,sdcard_dir_slash_len)==0)
        return 1;
    }
    return 0;
  }
  return stbuf.st_dev==sdcard_dev;
}

static int check_sdcard_fd(int fd)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    init_sdcard_dirs();
    if (real_stat(sdcard_dir,&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
    is_sdcard_dev_determined = 1;
  }
  
  if (real_fstat(fd,&stbuf)==-1)
    return 0;
  return stbuf.st_dev==sdcard_dev;
}

static char* get_realpath_fd(int fd, char* pathbuf)
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

static int is_execsfile(const char* realpathname)
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

static int open_execs_lock(const char* realpathname, int iswrite)
{
  char* execsname;
  const char* lastslash = strrchr(realpathname,'/');
  const char* filename;
  int fd;
  size_t dirlen;
  
  if ((execsname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }

  if (lastslash == NULL)
    lastslash = realpathname;
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  memcpy(execsname,realpathname,dirlen);
  strcpy(execsname+dirlen,"/" EXECS_NAME);
  
  filename = lastslash+1;
  
  if (!iswrite)
  {
    fd = real___open(execsname,O_RDONLY,0000);
    if (fd == -1)
    {
      free(execsname);
      return -1; // no executables
    }
    
    flock(fd, LOCK_SH); // we need readed
  }
  else // writing exec
  {
    fd = real___open(execsname,O_CREAT|O_RDWR,0600);
    if (fd == -1)
    {
      free(execsname);
      return -1; // no executables
    }
    
    flock(fd, LOCK_EX); // we need write
  }
  free(execsname);
  return fd;
}

static void close_execs_lock(int fd)
{
  if (fd != -1)
  {
    flock(fd, LOCK_UN);
    close(fd);
  }
}

// must be sdcard file
static int check_execmode(int fd, const char* realpathname)
{
  char* line;
  const char* lastslash = strrchr(realpathname,'/');
  const char* filename;
  mybuf_t buf;
  size_t dirlen;
  
  if (fd == -1)
    return 0;

  if ((line = malloc(FILENAME_MAX+1))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (lastslash == NULL)
    lastslash = realpathname;
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  
  filename = lastslash+1;
  
  mygetline_init(&buf);
  
  if (lseek(fd, 0, SEEK_SET)==-1) //rewind execs file
  {
    free(line);
    return -1;
  }
  errno = 0;
  while (mygetline(&buf,fd,line,FILENAME_MAX)!=NULL)
  {
    if (strcmp(filename,line)==0) // if found
    {
      free(line);
      return 1;
    }
  }
  if (errno == EIO)
  {
    free(line);
    return -1;
  }
  
  free(line);
  return 0; // not found
}

// must be sdcard file
static int set_execmode(int fd, const char* realpathname, int isexec)
{
  const char* lastslash = strrchr(realpathname,'/');
  char* line;
  const char* filename;
  char filenamelen;
  mybuf_t buf;
  size_t dirlen;
  off_t tmppos,execpos = -1;
  
#ifdef PROD_DEBUG
  debug_printf("set_execmode (%s,%d)\n",realpathname,isexec);
#endif
  
  if (fd == -1)
    // returns 0 if unsetting isexec (no error)
    return (isexec)?-1:0;
  
  if ((line = malloc(FILENAME_MAX+1))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (lastslash == NULL)
    lastslash = realpathname;
  
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  
  filename = lastslash+1;
  filenamelen = strlen(filename);
  
  mygetline_init(&buf);
  
  tmppos = 0;
  if (lseek(fd, 0, SEEK_SET) == -1) //rewind execs file
  {
    free(line);
    return -1;
  }
  errno = 0;
  while (mygetline(&buf,fd,line,FILENAME_MAX)!=NULL)
  {
    if (strcmp(filename,line)==0)
    { // if found
      execpos = tmppos;
      break;
    }
    tmppos += strlen(line)+1;
  }
  if (errno == EIO)
  {
    free(line);
    return -1;
  }
  
  if (isexec)
  { // set exec
    if (execpos == -1)
    { // not yet set_exec
      int ret;
      ret = lseek(fd,0,SEEK_END);
      if (ret != -1)
        ret = write(fd,filename,filenamelen);
      if (ret != -1)
        ret = write(fd,"\n",1);
      if (ret == -1)
      {
        free(line);
        return -1;
      }
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
        int ret;
        ret = lseek(fd,ipos+filenamelen+1,SEEK_SET);
        if (ret != -1)
          ret = readed = read(fd,mybuf,256);
        if (ret != -1)
          ret = lseek(fd,ipos,SEEK_SET);
        if (ret != -1)
          ret = write(fd,mybuf,readed);
        if (ret == -1)
        {
          free(line);
          return -1;
        }
        ipos += readed;
      } while(readed == 256);
      if (real_ftruncate(fd,ipos)==-1) // delete obsolete
      {
        free(line);
        return -1;
      }
    }
  }
  free(line);
  return 0;
}

/***
 * WRAPPERS 
 ***/

int access(const char* pathname, int mode)
{
  int tmperrno;
  char* realpathname;
  int hasexecmode = 0;
  int ret;
  int execsfd;
  struct stat stbuf;
#ifdef DEBUG
  printf("call access(%s,%d)\n",pathname,mode);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_access(pathname, mode);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_access(pathname, mode);
  }

  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  if (real_stat(realpathname,&stbuf) == -1)
  {
    free(realpathname);
    return real_access(pathname, mode);
  }
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
  {
    free(realpathname);
    return real_access(pathname, mode);
  }
  
  if (mode == F_OK || (mode & X_OK) == 0) // if existence or not executable
  {
    free(realpathname);
    return real_access(pathname, mode);
  }
  
  execsfd = open_execs_lock(realpathname,0);
  if (execsfd == -1 && errno != ENOENT)
  {
    free(realpathname);
    return -1;
  }
    
  hasexecmode = check_execmode(execsfd, realpathname);
  if (hasexecmode==-1)
  {
    close_execs_lock(execsfd);
    free(realpathname);
    return -1;
  }
  
  ret = real_access(pathname, mode^X_OK); // if no flags, we check existence
  tmperrno = errno;
  if (ret == 0) // other is ok
  {
    if (!hasexecmode)
    { // we dont have exec perms
      errno = EACCES;
      close_execs_lock(execsfd);
      free(realpathname);
      return -1;
    }
    close_execs_lock(execsfd);
    errno = tmperrno;
    free(realpathname);
    return 0;
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int chmod(const char* pathname, mode_t mode)
{
  int tmperrno;
  char* realpathname;
  struct stat stbuf;
  int execsfd;
  int ret;
#ifdef DEBUG
  printf("call chmod(%s,%d)\n",pathname,mode);
#endif
  
  init_handles();
#ifdef PROD_DEBUG
  debug_printf("chmod (%s,%o)\n",pathname,mode);
#endif
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_chmod(pathname, mode);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_chmod(pathname, mode);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  if (real_stat(realpathname,&stbuf) == -1)
  {
    free(realpathname);
    return real_chmod(pathname, mode);
  }
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
  {
    free(realpathname);
    return real_chmod(pathname, mode);
  }
  
  execsfd = open_execs_lock(realpathname,1);
  if (execsfd == -1)
  {
    free(realpathname);
    return -1;
  }
  ret = real_chmod(pathname,mode);
  tmperrno = errno;
#ifdef PROD_DEBUG
  debug_printf("step3 chmod (%s,%o)=%d,%d\n",realpathname,mode,ret,errno);
#endif
  if (ret == 0 || (errno == EPERM))
  {
    // set exec mode
    if(set_execmode(execsfd,realpathname,(mode&0111))==-1)
    {
      close_execs_lock(execsfd);
      errno = tmperrno;
      free(realpathname);
      return -1;
    }
    ret = 0;
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int fchmod(int fd, mode_t mode)
{
  int tmperrno;
  char* realpathname;
  struct stat stbuf;
  int execsfd;
  int ret;
#ifdef DEBUG
  printf("call fchmod(%d,%d)\n",fd,mode);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_fchmod(fd, mode);
  }
  
  if (get_realpath_fd(fd,realpathname)==NULL)
  {
    free(realpathname);
    return real_fchmod(fd, mode);
  }
  // sdcard_dir initialized in check_sdcard_fd
  if (strcmp(realpathname,sdcard_dir)==0)
  {
    free(realpathname);
    return real_fchmod(fd, mode);
  }
  
  if (is_execsfile(realpathname))
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  if (real_fstat(fd,&stbuf)==-1)
  {
    free(realpathname);
    return real_fchmod(fd, mode);
  }
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
  {
    free(realpathname);
    return real_fchmod(fd, mode);
  }
  
  execsfd = open_execs_lock(realpathname,1);
  if (execsfd == -1)
  {
    free(realpathname);
    return -1;
  }
  ret = real_fchmod(fd, mode);
  tmperrno = errno;
  if (ret == 0)
    // set exec mode
    if(set_execmode(execsfd,realpathname,(mode&0111))==-1)
    {
      close_execs_lock(execsfd);
      errno = tmperrno;
      free(realpathname);
      return -1;
    }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int chown(const char* pathname, uid_t uid, gid_t gid)
{
  char* realpathname;
#ifdef DEBUG
  printf("call chown(%s,%u,%u)\n",pathname,uid,gid);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_chown(pathname, uid, gid);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_chown(pathname, uid, gid);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_chown(pathname, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid)
{
  char* realpathname;
#ifdef DEBUG
  printf("call fchown(%d,%u,%u)\n",fd,uid,gid);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_fchown(fd, uid, gid);
  }
  
  if (get_realpath_fd(fd, realpathname)==NULL)
  {
    free(realpathname);
    return real_fchown(fd, uid, gid);  // cant determine realpath
  }
  
  if (strcmp(realpathname,sdcard_dir)==0)
  {
    free(realpathname);
    return real_fchown(fd, uid, gid);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_fchown(fd, uid, gid);
}

int lchown(const char* pathname, uid_t uid, gid_t gid)
{
  char* realpathname;
  init_handles();
  
#ifdef DEBUG
  printf("call lchown(%s,%u,%u\n",pathname,uid,gid);
#endif
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_lchown(pathname, uid, gid);  // cant determine realpath
  }
  
  if (!check_sdcard_link(realpathname))
  {
    free(realpathname);
    return real_lchown(pathname, uid, gid);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_lchown(pathname, uid, gid);
}

int getdents(unsigned int fd, struct dirent* dirp, unsigned int count)
{
  int rc, pos, entry;
  int execsfound = 0;
  char* realpathname;
  struct dirent* dentry;
  struct dirent* dend;
#ifdef DEBUG
  printf("call getdents(%d,%p,%u)\n",fd,dirp,count);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_getdents(fd, dirp, count);
  }
  
  if (get_realpath_fd(fd, realpathname)==NULL)
  {
    free(realpathname);
    return real_getdents(fd, dirp, count);  // cant determine realpath
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  rc = real_getdents(fd, dirp, count);
  if (rc <= 0)
  {
    free(realpathname);
    return rc;
  }
  // scan for EXECS
  entry = 0;
  dend = (struct dirent*)(((char*)dirp)+rc);
  for (dentry = dirp, pos = 0; dentry < dend;)
  {
    //printf("getdents:d_ino:%llu,%s\n",dentry->d_ino,dentry->d_name);
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
    memmove(dentry,((char*)dentry)+dentry->d_reclen,
            ((ptrdiff_t)dend-(ptrdiff_t)dentry)-dentry->d_reclen);
    
    rc -= dentry->d_reclen;
  }
  free(realpathname);
  return rc;
}


int mkdir(const char* dirpath, mode_t mode)
{
  char* realpathname;
#ifdef DEBUG
  printf("call mkdir(%s,%d)\n",dirpath,mode);
#endif
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(dirpath, realpathname)==NULL)
  {
    free(realpathname);
    return real_mkdir(dirpath, mode);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_mkdir(dirpath, mode);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = EINVAL;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_mkdir(dirpath, mode);
}


int unlink(const char* filename)
{
  int tmperrno;
  char* realpathname;
  struct stat stbuf;
  int execsfd;
  int ret;
#ifdef DEBUG
  printf("call unlink(%s)\n",filename);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(filename, realpathname)==NULL)
  {
    free(realpathname);
    return real_unlink(filename);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_unlink(filename);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  if (real_stat(realpathname,&stbuf) == -1)
  {
    free(realpathname);
    return real_unlink(filename);
  }
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
  {
    free(realpathname);
    return real_unlink(filename);
  }
  
  execsfd = open_execs_lock(realpathname,1);
  if (execsfd == -1)
  {
    free(realpathname);
    return -1;
  }
  ret = real_unlink(filename);
  tmperrno = errno;
  if (ret == 0)
    if (set_execmode(execsfd,realpathname,0) == -1) // unset exec
    {
      close_execs_lock(execsfd);
      errno = tmperrno;
      free(realpathname);
      return -1;
    }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int rmdir(const char* dirpath)
{
  int tmperrno;
  int fd, ret;
  char* realpathname;
  struct stat stbuf;
  char* execspath;
  size_t dirpathlen;
  char* execscopy = NULL;
#ifdef DEBUG
  printf("call rmdir(%s)\n",dirpath);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX*2))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  execspath = realpathname+PATH_MAX;
  
  if (myrealpath(dirpath, realpathname)==NULL)
  {
    free(realpathname);
    return real_rmdir(dirpath);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_rmdir(dirpath);
  }
  
  dirpathlen = strlen(dirpath);
  strcpy(execspath, dirpath);
  if (dirpathlen > 0  && dirpath[dirpathlen-1] != '/')
  {
    execspath[dirpathlen] = '/';
    dirpathlen++;
  }
  strcpy(execspath+dirpathlen,EXECS_NAME);
  
  // buffering .__execs__ before rmdir
  if ((fd = real___open(execspath,O_RDONLY,0644))!=-1)
  {
    flock(fd,LOCK_SH);
    if (real_stat(execspath,&stbuf)==0)
    {
      execscopy = malloc(stbuf.st_size);
      if (execscopy == NULL)
      {
        flock(fd,LOCK_UN);
        close(fd);
        free(realpathname);
        errno = ENOMEM;
        return -1;
      }
      read(fd,execscopy,stbuf.st_size);
    }
    flock(fd,LOCK_UN);
    close(fd);
  }
  
  real_unlink(execspath);
  ret = real_rmdir(dirpath);
  tmperrno = errno;
  
  if (ret != 0 && execscopy!=NULL)
  { // if fail, we recreate .__execs__
    if ((fd = real___open(execspath,O_CREAT|O_WRONLY,0600))!=-1)
    {
      flock(fd,LOCK_EX);
      write(fd,execscopy,stbuf.st_size);
      flock(fd,LOCK_UN);
      close(fd);
    }
  }
  if (execscopy!=NULL)
    free(execscopy);
  
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int readlink(const char* linkpath, char* buf, size_t bufsize)
{
  char* refpath;
  int size;
#ifdef DEBUG
  printf("call readlink(%s,%p,%u)\n",linkpath,buf,bufsize);
#endif
  init_handles();
  
  if ((refpath = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  size = real_readlink(linkpath, refpath, bufsize);
  
  if (size==-1)  // cant determine realpath
  {
    free(refpath);
    return -1;
  }
  
  refpath[size] = 0;
  
  if (!check_sdcard_link(refpath))
  {
    memcpy(buf,refpath,size);
    free(refpath);
    return size;
  }
  
  if (is_execsfile(refpath)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(refpath);
    return -1;
  }
  
  memcpy(buf,refpath,size);
  free(refpath);
  return size;
}

int __open(const char* filename, int flags, mode_t mode)
{
  int tmperrno;
  char* realpathname;
  int file_exists;
  int ret;
  int execsfd;
#ifdef DEBUG
  printf("call __open(%s,%d,%d)\n",filename,flags,mode);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(filename, realpathname)==NULL)
  {
    free(realpathname);
    return real___open(filename, flags, mode);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real___open(filename, flags, mode);
  }
  
  file_exists = real_access(filename, F_OK);
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  execsfd = open_execs_lock(realpathname,1);
  if (execsfd == -1)
  {
    free(realpathname);
    return -1;
  }
  
  ret = real___open(filename, flags, mode);
  tmperrno = errno;
  if (ret != -1 && (flags & O_CREAT) != 0 &&
      file_exists == -1 && (mode & 0111) != 0)
  {
    if (set_execmode(execsfd,realpathname,1)==-1)
    {
      close_execs_lock(execsfd);
      free(realpathname);
      return -1;
    }
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int rename(const char* oldpath, const char*newpath)
{
  int tmperrno;
  int ret;
  char* oldrealpathname;
  char* newrealpathname;
  char* oldrpp;
  char* newrpp;
  int isexecs = 0;
  int oldexecsfd, newexecsfd;
  int sdcard_old, sdcard_new;
  char* oldslash;
  char* newslash;
  size_t len;
  struct stat stbuf;
#ifdef DEBUG
  printf("call rename(%s,%s)\n",oldpath,newpath);
#endif
  
  init_handles();
  
  if ((oldrealpathname = malloc(PATH_MAX*2))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  newrealpathname = oldrealpathname+PATH_MAX;
  
  oldrpp = myrealpath(oldpath, oldrealpathname);
  newrpp = myrealpath(newpath, newrealpathname);
  
  if (oldrpp==NULL)
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  if (newrpp==NULL)
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  
  sdcard_old = check_sdcard(oldrealpathname);
  sdcard_new = check_sdcard(newrealpathname);
  
  if (!sdcard_old || !sdcard_new)  // EXDEV not supported by Linux
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  
  if (is_execsfile(oldrpp))
  {
    errno = ENOENT;
    free(oldrealpathname);
    return -1;
  }
  if (is_execsfile(newrpp))
  {
    errno = EINVAL;
    free(oldrealpathname);
    return -1;
  } 
  
  if (real_stat(oldrealpathname,&stbuf)==-1) // probably doesnt exists
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  if (S_ISDIR(stbuf.st_mode)) // if directory
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  
  len = strlen(oldrpp);
  if (oldrpp[len-1]=='/' && len>1)
    oldrpp[len-1] = 0;
  len = strlen(newrpp);
  if (newrpp[len-1]=='/' && len>1)
    newrpp[len-1] = 0;
  
  oldslash = strrchr(oldrpp,'/');
  newslash = strrchr(newrpp,'/');
  if (strncmp(oldrpp,newrpp,(ptrdiff_t)oldslash-(ptrdiff_t)oldrpp+1)==0 &&
      (ptrdiff_t)oldslash-(ptrdiff_t)oldrpp==(ptrdiff_t)newslash-(ptrdiff_t)newrpp)
  { // same dir
    oldexecsfd = open_execs_lock(oldrpp,1);
    if (oldexecsfd == -1)
    {
      free(oldrealpathname);
      return -1;
    }
    
    ret = real_rename(oldpath, newpath);
    tmperrno = errno;
    if (ret == 0)
    {
      isexecs = check_execmode(oldexecsfd, oldrpp);
      if (isexecs != -1)
      {
        if (set_execmode(oldexecsfd,oldrpp,0) == -1)
        {
          tmperrno = errno;
          ret = -1;
        }
        else if (isexecs)
          if (set_execmode(oldexecsfd, newrpp, 1) == -1)
          {
            tmperrno = errno;
            ret = -1;
          }
      }
      else // use current errno
      {
        tmperrno = errno;
        ret = -1;
      }
    }
    close_execs_lock(oldexecsfd);
    errno = tmperrno;
  }
  else
  {
    oldexecsfd = open_execs_lock(oldrpp,1);
    if (oldexecsfd == -1)
    {
      free(oldrealpathname);
      return -1;
    }
    newexecsfd = open_execs_lock(newrpp,1);
    if (newexecsfd == -1)
    {
      close_execs_lock(oldexecsfd);
      free(oldrealpathname);
      return -1;
    }
    
    ret = real_rename(oldpath, newpath);
    tmperrno = errno;
    if (ret == 0)
    {
      isexecs = check_execmode(oldexecsfd, oldrpp);
      if (isexecs != -1)
      {
        if (set_execmode(oldexecsfd,oldrpp,0) == -1)
        {
          tmperrno = errno;
          ret = -1;
        }
        else if (isexecs)
          if (set_execmode(newexecsfd, newrpp, 1) == -1)
          {
            tmperrno = errno;
            ret = -1;
          }
      }
      else // use current errno
      {
        tmperrno = errno;
        ret = -1;
      }
    }
    close_execs_lock(oldexecsfd);
    close_execs_lock(newexecsfd);
    errno = tmperrno;
  }
  free(oldrealpathname);
  return ret;
}

int symlink(const char* oldpath, const char*newpath)
{
  int sdcard_old,sdcard_new;
  char* oldrpp;
  char* newrpp;
  char* oldrealpathname;
  char* newrealpathname;
#ifdef DEBUG
  printf("call symlink(%s,%s)\n",oldpath,newpath);
#endif
  
  init_handles();
  
  if ((oldrealpathname = malloc(PATH_MAX*2))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  newrealpathname = oldrealpathname+PATH_MAX;
  
  oldrpp = myrealpath(oldpath, oldrealpathname);
  newrpp = myrealpath(newpath, newrealpathname);
  
  if (oldrpp==NULL)
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  if (newrpp==NULL)
  {
    free(oldrealpathname);
    return real_rename(oldpath, newpath);
  }
  
  sdcard_old = check_sdcard(oldrealpathname);
  sdcard_new = check_sdcard(newrealpathname);
  
  if (!sdcard_old && !sdcard_new)
  {
    free(oldrealpathname);
    return real_symlink(oldpath, newpath);
  }
  
  if (sdcard_old && is_execsfile(oldrpp))
  {
    errno = ENOENT;
    free(oldrealpathname);
    return -1;
  }
  if (sdcard_new && is_execsfile(newrpp))
  {
    errno = EINVAL;
    free(oldrealpathname);
    return -1;
  } 
  
  free(oldrealpathname);
  return real_symlink(oldpath, newpath);
}

int truncate(const char* pathname, off_t length)
{
  char* realpathname;
#ifdef DEBUG
  printf("call truncate(%s,%lu)\n",pathname,length);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_truncate(pathname, length);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_truncate(pathname, length);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_truncate(pathname, length);
}

int ftruncate(int fd, off_t length)
{
  char* realpathname;
#ifdef DEBUG
  printf("call ftruncate(%d,%lu)\n",fd,length);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_ftruncate(fd, length);
  }
  
  if (get_realpath_fd(fd, realpathname)==NULL)
  {
    free(realpathname);
    return real_ftruncate(fd, length);  // cant determine realpath
  }
  if (strcmp(realpathname,sdcard_dir)==0)
  {
    free(realpathname);
    return real_ftruncate(fd, length);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_ftruncate(fd, length);
}

int stat(const char* pathname, struct stat* buf)
{
  int tmperrno;
  char* realpathname;
  int execsfd;
  int ret;
#ifdef DEBUG
  printf("call stat(%s,%p)\n",pathname,buf);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_stat(pathname, buf);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_stat(pathname, buf);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  execsfd = open_execs_lock(realpathname,0);
  if (execsfd == -1 && errno != ENOENT)
  {
    free(realpathname);
    return -1;
  }
  
  ret = real_stat(pathname,buf);
  tmperrno = errno;
  if (ret == -1)
  {
    close_execs_lock(execsfd);
    errno = tmperrno;
    free(realpathname);
    return -1;
  }
  
  if (!S_ISDIR(buf->st_mode)) // is not directory, we use execs for files
  {
    int isexecs;
    if ((isexecs = check_execmode(execsfd,realpathname))==-1)
    {
      close_execs_lock(execsfd);
      free(realpathname);
      return -1;
    }
    
    buf->st_mode &= ~0111;
    buf->st_mode |= (isexecs ? 0111 : 0);
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int fstat(int fd, struct stat* buf)
{
  int tmperrno;
  char* realpathname;
  int execsfd;
  int ret;
#ifdef DEBUG
  write(1,"call fstat\n",11);
  //printf("call fstat(%d,%p)\n",fd,buf);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_fstat(fd, buf);
  }
  
  if (get_realpath_fd(fd, realpathname)==NULL)
  {
    free(realpathname);
    return real_fstat(fd, buf);  // cant determine realpath
  }
  if (strcmp(realpathname,sdcard_dir)==0)
  {
    free(realpathname);
    return real_fstat(fd, buf);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  execsfd = open_execs_lock(realpathname,0);
  if (execsfd == -1 && errno != ENOENT)
  {
    free(realpathname);
    return -1;
  }
  
  ret = real_fstat(fd, buf);
  tmperrno = errno;
  if (ret == -1)
  {
    close_execs_lock(execsfd);
    errno = tmperrno;
    free(realpathname);
    return -1;
  }
  
  if (!S_ISDIR(buf->st_mode)) // is not directory, we use execs for files
  {
    int isexecs;
    if ((isexecs = check_execmode(execsfd,realpathname))==-1)
    {
      close_execs_lock(execsfd);
      free(realpathname);
      return -1;
    }
    
    buf->st_mode &= ~0111;
    buf->st_mode |= (isexecs ? 0111 : 0);
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int lstat(const char* pathname, struct stat* buf)
{
  int tmperrno;
  char* realpathname;
  int execsfd;
  int ret;
#ifdef DEBUG
  printf("call lstat(%s,%p)\n",pathname,buf);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_stat(pathname, buf);  // cant determine realpath
  }
  
  if (!check_sdcard_link(realpathname))
  {
    free(realpathname);
    return real_lstat(pathname, buf);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  execsfd = open_execs_lock(realpathname,0);
  if (execsfd == -1 && errno != ENOENT)
  {
    free(realpathname);
    return -1;
  }
  
  ret = real_lstat(pathname,buf);
  tmperrno = errno;
  if (ret == -1)
  {
    close_execs_lock(execsfd);
    errno = tmperrno;
    free(realpathname);
    return -1;
  }
  
  if (!S_ISDIR(buf->st_mode) && !S_ISLNK(buf->st_mode))
  { // is not directory, we use execs for files
    int isexecs;
    if ((isexecs = check_execmode(execsfd,realpathname))==-1)
    {
      close_execs_lock(execsfd);
      free(realpathname);
      return -1;
    }
    
    buf->st_mode &= ~0111;
    buf->st_mode |= (isexecs ? 0111 : 0);
  }
  close_execs_lock(execsfd);
  errno = tmperrno;
  free(realpathname);
  return ret;
}

int utimes(const char* pathname, const struct timeval times[2])
{
  char* realpathname;
#ifdef DEBUG
  printf("call utimes(%s,%p)\n",pathname,times);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (myrealpath(pathname, realpathname)==NULL)
  {
    free(realpathname);
    return real_utimes(pathname, times);  // cant determine realpath
  }
  
  if (!check_sdcard(realpathname))
  {
    free(realpathname);
    return real_utimes(pathname, times);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_utimes(pathname, times);
}

int futimes(int fd, const struct timeval times[2])
{
  char* realpathname;
#ifdef DEBUG
  printf("call futimes(%d,%p)\n",fd,times);
#endif
  
  init_handles();
  
  if ((realpathname = malloc(PATH_MAX))==NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  
  if (!check_sdcard_fd(fd))
  {
    free(realpathname);
    return real_futimes(fd, times);
  }
  
  if (get_realpath_fd(fd, realpathname)==NULL)
  {
    free(realpathname);
    return real_futimes(fd, times);  // cant determine realpath
  }
  if (strcmp(realpathname,sdcard_dir)==0)
  {
    free(realpathname);
    return real_futimes(fd, times);
  }
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    free(realpathname);
    return -1;
  }
  
  free(realpathname);
  return real_futimes(fd, times);
}
