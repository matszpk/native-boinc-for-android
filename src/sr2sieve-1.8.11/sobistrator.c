/* sobistrator.c -- (C) Geoffrey Reynolds, October 2007.

   Routines for Sobistrator compatibility mode.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "sr5sieve.h"

#if SOBISTRATOR_OPT

int sobistrator_opt = 0;                  /* Set if -j switch given */
const char *sob_status_file_name = NULL;  /* e.g. SoBStatus.dat */
const char *sob_range_file_name = NULL;   /* e.g. nextrange.txt */

static uint32_t last_checkpoint_time;     /* Calendar time */
static uint64_t last_checkpoint_prime;    /* p */

/* Read a range from file f.
   Returns 1 if successful, with pmin and pmax modified.
   Throws an error if the range is invalid.
   Returns 0 if otherwise (EOF), with pmin and pmax unmodified.
 */
static int read_range(uint64_t *pmin, uint64_t *pmax, FILE *f, const char *fn)
{
  uint64_t p[2];
  const char *fmt_p[2] = { "pmin=%"SCNu64, "pmax=%"SCNu64 };
  const char *line;
  int next; /* 0 = min, 1 = max */

  if ((line = read_line(f)) == NULL)
    return 0;
  if (sscanf(line,fmt_p[0],&p[0]) == 1)
    next = 1;
  else if (sscanf(line,fmt_p[1],&p[1]) == 1)
    next = 0;
  else
    line_error("Unparsed range",NULL,fn);

  if ((line = read_line(f)) == NULL)
    line_error("Incomplete range",NULL,fn);
  if (sscanf(line,fmt_p[next],&p[next]) != 1)
    line_error("Unparsed range",NULL,fn);

  if (p[0] >= p[1])
    line_error("Invalid range",NULL,fn);

  *pmin = p[0];
  *pmax = p[1];

  return 1;
}

/* Write range to file f.
   Throw an error if unsuccessful.
 */
static void write_range(uint64_t pmin, uint64_t pmax, FILE *f, const char *fn)
{
  if (fprintf(f,"pmax=%"PRIu64"\n" "pmin=%"PRIu64"\n",pmax,pmin) < 0)
    error("Writing to file `%s' failed.",fn);
}

/* Start a new range, initialize SoBStatus.dat
 */
void sob_start_range(uint64_t pmin, uint64_t pmax)
{
  FILE *file;

  file = xfopen(sob_status_file_name,"w",error);
  write_range(pmin,pmax,file,sob_status_file_name);
  xfclose(file,sob_status_file_name);
}

/* Continue a range in SoBStatus.dat
   Return 1 if successful, 0 otherwise.
 */
int sob_continue_range(uint64_t *pmin, uint64_t *pmax)
{
  FILE *file;
  int ret = 0;

  if ((file = xfopen(sob_status_file_name,"r",NULL)) != NULL)
  {
    if ((ret = read_range(pmin,pmax,file,sob_status_file_name)) != 0)
      report(1,"Continuing with range pmin=%"PRIu64",pmax=%"PRIu64" in `%s'.",
             *pmin,*pmax,sob_status_file_name);
    xfclose(file,sob_status_file_name);
  }

  return ret;
}

/* Finish a range. Append `Done' to SoBStatus.dat
 */ 
void sob_finish_range(void)
{
  FILE *file;

  if ((file = xfopen(sob_status_file_name,"a",warning)) != NULL)
  {
    if (fprintf(file,"Done\n") < 0)
      warning("Writing to file `%s' failed.",sob_status_file_name);
    xfclose(file,sob_status_file_name);
  }
}

/* Checkpoint, append current p and speed in kp/s to SoBStatus.dat
 */
void sob_write_checkpoint(uint64_t p)
{
  FILE *file;
  uint32_t this_time, kp_per_sec;

  if ((this_time = millisec_elapsed_time()) != last_checkpoint_time)
    if ((file = xfopen(sob_status_file_name,"a",warning)) != NULL)
    {
      kp_per_sec = /* kp/s == p/ms */
        (p-last_checkpoint_prime)/(this_time-last_checkpoint_time);

      if (fprintf(file,"pmin=%"PRIu64" @ %"PRIu32" kp/s\n",p,kp_per_sec) < 0)
        warning("Writing to file `%s' failed.",sob_status_file_name);
      xfclose(file,sob_status_file_name);

      last_checkpoint_time = this_time;
      last_checkpoint_prime = p;
    }
}

/* Read the range p0,p1 and checkpoint p from SoBStatus.dat.
   If no file exists or p0 != pmin or p1 != pmax, return pmin.
   If no checkpoint line exists, return pmax.
   If the range is complete, return pmax.
   Otherwise return p.
*/
uint64_t sob_read_checkpoint(uint64_t pmin, uint64_t pmax)
{
  FILE *file;
  const char *line;
  uint64_t p0, p1, p;

  p = pmin;
  if ((file = xfopen(sob_status_file_name,"r",NULL)) != NULL)
  {
    if (read_range(&p0,&p1,file,sob_status_file_name))
      if (p0 == pmin && p1 == pmax)
      {
        while ((line = read_line(file)) != NULL)
          if (sscanf(line,"pmin=%"SCNu64,&p) != 1)
            break;

        if (p < pmin || p > pmax)
          line_error("Invalid checkpoint",NULL,sob_status_file_name);

        if (line != NULL && !strncmp("Done",line,4))
          p = pmax;

        if (p > pmin && p < pmax)
          report(1,"Resuming from checkpoint pmin=%"PRIu64" in `%s'.",
                 p,sob_status_file_name);
      }
    xfclose(file,sob_status_file_name);
  }

  last_checkpoint_time = millisec_elapsed_time();
  last_checkpoint_prime = p;

  return p;
}

/* Set pmin,pmax to the next range in nextrange.txt if one exists.
   The range is deleted from nextrange.txt and a new SoBStatus.dat created.
   MAX_RANGES is the maximum number of ranges in nextrange.txt
   Return 1 if successful, 0 otherwise.
*/
#define MAX_RANGES 128
int sob_next_range(uint64_t *pmin, uint64_t *pmax)
{
  FILE *file;
  uint64_t p0, p1;
  int done = 0;

  if ((file = xfopen(sob_range_file_name,"r",NULL)) != NULL)
  {
    if (read_range(&p0,&p1,file,sob_range_file_name))
    {
      uint64_t R0[MAX_RANGES], R1[MAX_RANGES];
      uint32_t i, j;

      for (j = 0; j < MAX_RANGES; j++)
        if (!read_range(R0+j,R1+j,file,sob_range_file_name))
          break;

      xfclose(file,sob_range_file_name);

      if (j >= MAX_RANGES)
        error("Too many ranges (max %"PRIu32") in `%s'",j,sob_range_file_name);

      file = xfopen(sob_range_file_name,"w",error);
      for (i = 0; i < j; i++)
        write_range(R0[i],R1[i],file,sob_range_file_name);

      done = 1;
    }
    xfclose(file,sob_range_file_name);
  }

  if (done)
  {
    sob_start_range(p0,p1);
    *pmin = p0;
    *pmax = p1;

    report(1,"Starting new range pmin=%"PRIu64",pmax=%"PRIu64" from `%s'.",
           p0,p1,sob_range_file_name);
  }

  return done;
}

#endif /* SOBISTRATOR_OPT */
