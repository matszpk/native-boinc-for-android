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
static int (*real_lstat)(int fd, struct stat* buf) = NULL;

static int (*real_utimes)(const char* filename, const struct timeval times[2]);

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
  }
}

/* check whether is /mnt/sdcard */

static int is_sdcard_dev_determined = 0;
static dev_t sdcard_dev = -1;

int check_sdcard(const char* pathname)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
#ifdef DEBUG
    printf("sdcard dev=%d\n",sdcard_dev);
#endif
    is_sdcard_dev_determined = 1;
  }
  
  if (stat(pathname,&stbuf)==-1)
    return 0;
#ifdef DEBUG
  printf("dev for %s=%llu\n",pathname,stbuf.st_dev);
#endif
  return stbuf.st_dev==sdcard_dev;
}

int check_sdcard_fd(int fd)
{
  struct stat stbuf;
  if (!is_sdcard_dev_determined)
  {
    if (stat("/mnt/sdcard",&stbuf)==-1)
      return 0;
    sdcard_dev = stbuf.st_dev;
#ifdef DEBUG
    printf("sdcard dev=%d\n",sdcard_dev);
#endif
    is_sdcard_dev_determined = 1;
  }
  
  if (fstat(fd,&stbuf)==-1)
    return 0;
#ifdef DEBUG
  printf("dev for %d=%llu\n",fd,stbuf.st_dev);
#endif
  return stbuf.st_dev==sdcard_dev;
}

char* get_realpath_fd(int fd, char* pathbuf)
{
  char procfdname[40];
  struct stat stbuf;
  ssize_t s;
  if (fstat(fd,&stbuf)==-1)
    return NULL;
  
  if (stbuf.st_nlink==0)
    return NULL;
  
  // read from proc/self
  snprintf(procfdname,40,"/proc/self/fd/%d",fd);
  s = readlink(procfdname,pathbuf,PATH_MAX);
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
  if (len < 8+EXECS_NAME_LEN)
    return 0;
  
  if (strcmp(realpathname+len-EXECS_NAME_LEN-1,"/" EXECS_NAME)==0)
    return 1;
  return 0;
}

// must be sdcard file
int check_execmode(const char* realpathname)
{
  char execsname[PATH_MAX];
  char* lastslash = strrchr(realpathname,'/');
  char line[FILENAME_MAX+1];
  char* filename;
  FILE* file;
  int fd;
  size_t dirlen;
  
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  memcpy(execsname,realpathname,dirlen);
  strcpy(execsname+dirlen,"/" EXECS_NAME);
  
  filename = lastslash+1;
  
  file = fopen(execsname,"rb");
  if (file == NULL)
    return 0; // no executables
  fd = fileno(file);
    
  flock(fd, LOCK_SH); // we need read
  
  while (!feof(file) && !ferror(file))
  {
    size_t linelen;
    if (fgets(line, FILENAME_MAX, file)==NULL)
      break;
    linelen = strlen(line)-1;
    line[linelen] = 0; // skip newline
    
    if (strcmp(filename,line)==0)
    { // if found
      flock(fd, LOCK_UN); // unlock it
      fclose(file);
      return 1;
    }
  }
  
  flock(fd, LOCK_UN); // unlock it
  fclose(file);
  return 0; // not found
}

// must be sdcard file
int set_execmode(const char* realpathname, int isexec)
{
  char execsname[PATH_MAX];
  char* lastslash = strrchr(realpathname,'/');
  char line[FILENAME_MAX+1];
  char* filename;
  char filenamelen;
  FILE* file;
  int fd;
  size_t dirlen;
  off_t execpos = -1;
  
  dirlen = (ptrdiff_t)lastslash-(ptrdiff_t)realpathname;
  memcpy(execsname,realpathname,dirlen);
  strcpy(execsname+dirlen,"/" EXECS_NAME);
  
  filename = lastslash+1;
  filenamelen = strlen(filename);
  
  file = fopen(execsname,"rb+");
  if (file==NULL)
    file = fopen(execsname,"wb+");
  fseek(file,0,SEEK_SET);
  if (file == NULL)
    return -1; // no executables
  fd = fileno(file);
    
  flock(fd, LOCK_EX); // we need write
  
  while (!feof(file) && !ferror(file))
  {
    off_t fpos;
    size_t linelen;
    fpos = ftell(file);
    if (fgets(line, FILENAME_MAX, file)==NULL)
      break;
    linelen = strlen(line)-1;
    line[linelen] = 0; // skip newline
    
    if (strcmp(filename,line)==0)
    { // if found
      execpos = fpos;
      break;
    }
  }
  
  if (isexec)
  { // set exec
    if (execpos == -1)
    { // not yet set_exec
      fseek(file,0,SEEK_END);
      fputs(filename,file);
      fputc('\n',file);
      fflush(file);
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
        fseek(file,ipos+filenamelen+1,SEEK_SET);
        readed = fread(mybuf,1,256,file);
        fseek(file,ipos,SEEK_SET);
        fwrite(mybuf,1,readed,file);
        ipos += readed;
      } while(readed == 256 && !ferror(file));
      fflush(file);
      ftruncate(fd,ipos); // delete obsolete
    }
  }
  
  flock(fd, LOCK_UN); // unlock it
  fclose(file);
  return 0; // not found
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
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_access(pathname, mode);

  if (realpath(pathname, realpathname)==NULL)
    return real_access(pathname, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  if (stat(realpathname,&stbuf) == -1)
    return real_access(pathname, mode);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_access(pathname, mode);
  
  if (mode == F_OK && (mode & X_OK) == 0) // if existence or not executable
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
  
  init_handles();
  
  if (!check_sdcard(pathname))
    return real_chmod(pathname, mode);
  
  if (realpath(pathname, realpathname)==NULL)
    return real_chmod(pathname, mode);  // cant determine realpath
  
  if (is_execsfile(realpathname)) // we assume, that doesnt exists
  {
    errno = ENOENT;
    return -1;
  }
  
  if (stat(realpathname,&stbuf) == -1)
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
  
  if (fstat(fd,&stbuf)==-1)
    return real_fchmod(fd, mode);
  
  if (S_ISDIR(stbuf.st_mode)) // is directory, we use execs for files
    return real_fchmod(fd, mode);
  
  // set exec mode
  if(set_execmode(realpathname,(mode&0111))==-1)
    return -1;
  return real_fchmod(fd, mode);
}
     
int getdents(unsigned int fd, struct dirent* dirp, unsigned int count)
{
  return 0;
}

int unlink(const char* filename)
{
  char realpathname[PATH_MAX];
  struct stat stbuf;
  
  init_handles();
  
  if (!check_sdcard(filename))
    return real_unlink(filename);
  
  if (realpath(filename, realpathname)==NULL)
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
  return 0;
}

int __open(const char* file, int flags, mode_t mode)
{
  return 0;
}

int stat(const char* path, struct stat* buf)
{
  return 0;
}

int fstat(int fd, struct stat* buf)
{
  return 0;
}

int lstat(const char* path, struct stat* buf)
{
  return 0;
}
