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

static int (*real_chmod)(const char* path, mode_t mode) = NULL;
static int (*real_fchmod)(int fd, mode_t mode) = NULL;
static int (*real_fchmodat)(int dirfd, const char* pathname, mode_t mode,
        int flags) = NULL;

static int (*real_getdents)(unsigned int fd, struct direct* dirp,
      unsigned int count) = NULL;

static int (*real_unlink)(const char* file) = NULL;
static int (*real_rmdir)(const char* file) = NULL;

static int (*real_open)(const char* file, int flags, mode_t mode) = NULL;
static int (*real_openat)(int dirfd, const char* file, int flags,
          mode_t mode) = NULL;

static int (*real_stat)(const char* path, struct stat* buf) = NULL;
static int (*real_fstat)(int fd, struct stat* buf) = NULL;
static int (*real_fstatat)(int dirfd, const char* pathname,
          struct stat* buf, int flags) = NULL;

int chmod(const char* path, mode_t mode)
{
  return 0;
}

int fchmod(int fd, mode_t mode)
{
  return 0;
}

int fchmodat(int dirfd, const char* pathname, mode_t mode, int flags)
{
  return 0;
}
     
int getdents(unsigned int fd, struct direct* dirp, unsigned int count)
{
  return 0;
}

int unlink(const char* file)
{
  return 0;
}

int rmdir(const char* file)
{
  return 0;
}

int open(const char* file, int flags, mode_t mode)
{
  return 0;
}

int openat(int dirfd, const char* file, int flags, mode_t mode)
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

int fstatat(int dirfd, const char* pathname, struct stat* buf, int flags)
{
  return 0;
}
