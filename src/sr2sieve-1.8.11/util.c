/* util.c -- (C) Geoffrey Reynolds, April 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sr5sieve.h"

#define MAX_MSG_LEN 200
#define MAX_LINE_LENGTH 80

static int line_length;

/* Print a message using printf(fmt,...), with enough trailing spaces to
   erase the remains of the previous message. If scroll is nonzero then
   output a '\n' character after the message to advance to a new line,
   otherwise restrict the message to the current line and output a '\r' to
   return to the start of line.
*/
void report(int scroll, const char *fmt, ...)
{
  va_list args;
  int length;

  va_start(args,fmt);
  if (scroll)
  {
    length = vprintf(fmt,args);
  }
  else /* Restrict message to one line */
  {
    char buf[MAX_LINE_LENGTH+1];
    vsnprintf(buf,MAX_LINE_LENGTH,fmt,args);
    buf[MAX_LINE_LENGTH] = '\0';
    length = printf("%s",buf);
  }
  va_end(args);

  while (line_length > length)
  {
    putchar(' ');
    line_length--;
  }

  if (scroll)
  {
    putchar('\n');
    line_length = 0;
  }
  else
  {
    putchar('\r');
    fflush(stdout);
    line_length = length;
  }
}

/* Concatenate msg1 and msg2 and output the resulting message to the log
   file with a timestamp. If print is nonzero then also output the message
   to a new line on the console (scrolling past any existing message).
*/
static void log_msg(int print, const char *msg1, const char *msg2)
{
  FILE *log_file;
  time_t tm;
  char tm_buf[32];
  struct tm *tm_struct;

  if (log_file_name != NULL)
  {
    if ((log_file = fopen(log_file_name,"a")) != NULL)
    {
      time(&tm);
      if ((tm_struct = localtime(&tm)) != NULL)
        if (strftime(tm_buf,sizeof(tm_buf),LOG_STRFTIME_FORMAT,tm_struct) != 0)
          fprintf(log_file,tm_buf);
      fprintf(log_file,"%s%s\n",msg1,msg2);
      fclose(log_file);
    }
    else
    {
      if (line_length > 0)
      {
        putchar('\n');
        line_length = 0;
      }
      printf("WARNING: could not open log file `%s'\n", log_file_name);
    }
  }

  if (print)
  {
    if (line_length > 0)
    {
      putchar('\n');
      line_length = 0;
    }
    printf("%s%s\n",msg1,msg2);
  }
}

/* Log an error message (and echo to console), then exit.
 */
void error(const char *fmt, ...)
{
  va_list args;
  char buf[MAX_MSG_LEN+1];

  va_start(args,fmt);
  vsnprintf(buf,MAX_MSG_LEN,fmt,args);
  buf[MAX_MSG_LEN] = '\0';
  va_end(args);

  log_msg(1,"ERROR: ",buf);

  exit(EXIT_FAILURE);
}

/* Log a warning message (and echo to console).
 */
void warning(const char *fmt, ...)
{
  va_list args;
  char buf[MAX_MSG_LEN+1];

  va_start(args,fmt);
  vsnprintf(buf,MAX_MSG_LEN,fmt,args);
  buf[MAX_MSG_LEN] = '\0';
  va_end(args);

  log_msg(1,"WARNING: ",buf);
}

/* Log a message. If print is nonzero then echo it to the console too.
 */
void logger(int print, const char *fmt, ...)
{
  va_list args;
  char buf[MAX_MSG_LEN+1];

  va_start(args,fmt);
  vsnprintf(buf,MAX_MSG_LEN,fmt,args);
  buf[MAX_MSG_LEN] = '\0';
  va_end(args);

  log_msg(print,"",buf);
}

void *xmalloc(uint32_t sz)
{
  void *ret;

  if ((ret = malloc(sz)) == NULL)
    error("malloc failed.");

  return ret;
}


void *xrealloc(void *d, uint32_t sz)
{
  void *ret;

  if ((ret = realloc(d,sz)) == NULL)
    error("realloc failed.");

  return ret;
}

#if HAVE_MEMALIGN
#include <malloc.h>
void *xmemalign(uint32_t align, uint32_t size)
{
  void *ret;

  if ((ret = memalign(align,size)) == NULL)
    error("memalign failed.");

  return ret;
}

void xfreealign(void *mem)
{
  free(mem);
}
#else /* Don't HAVE_MEMALIGN */
#include "mm_malloc.h"
void *xmemalign(uint32_t align, uint32_t size)
{
  void *ret;

  if ((ret = _mm_malloc(size,align)) == NULL)
    error("memalign failed.");

  return ret;
}

void xfreealign(void *mem)
{
  _mm_free(mem);
}
#endif

uint32_t gcd32(uint32_t a, uint32_t b)
{
  uint32_t c;

  while (b > 0)
  {
    c = a % b;
    a = b;
    b = c;
  }

  return a;
}
