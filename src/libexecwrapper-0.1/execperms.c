/*
 * execperms.c 
 * Author: Mateusz Szpakowski
 */

static int (*real_chmod)(const char* path, mode_t mode) = NULL;

int chmod(const char* path, mode_t mode)
{
  return 0;
}

static int (*real_fchmod)(int fd, mode_t mode) = NULL;

int fchmod(int fd, mode_t mode)
{
  return 0;
}
