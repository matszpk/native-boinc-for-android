/* files.c -- (C) Geoffrey Reynolds, May 2006.

   Routines for reading and writing sieve and sequence files.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "sr5sieve.h"

static int line_counter = 0;
void line_error(const char *msg, const char *fmt, const char *filename)
{
  if (fmt != NULL)
    error("Line %d: %s in %s format file `%s'.",line_counter,msg,fmt,filename);
  else
    error("Line %d: %s in file `%s'.",line_counter,msg,filename);
}

/* Reads a line from file, ignoring blank lines and comments. Returns a
   pointer to the first non-whitespace character in the line.
*/
#define MAX_LINE_LENGTH 256
#define COMMENT_CHAR '#'
const char *read_line(FILE *file)
{
  static char buf[MAX_LINE_LENGTH];
  const char *ptr;

  while ((ptr = fgets(buf,MAX_LINE_LENGTH,file)) != NULL)
  {
    line_counter++;
    while (isspace(*ptr))
      ptr++;
    if (*ptr != COMMENT_CHAR && *ptr != '\0')
      break;
  }

  return ptr;
}

FILE *xfopen(const char *fn, const char *mode, void (*fun)(const char *,...))
{
  FILE *ret;

  ret = fopen(fn,mode);
  if (ret == NULL && fun != NULL)
    fun("Failed to open %sput file `%s'.", (*mode == 'r') ? "in" : "out", fn);

  line_counter = 0;
  return ret;
}

void xfclose(FILE *file, const char *fn)
{
  if (file != NULL && fclose(file))
    warning("Problem closing file `%s'.", fn);
}

/* This function cannot parse a general ABCD format file, only the specific
   type (ABCD k*b^n+c with fixed k,b,c and variable n) written by srsieve.
*/
uint32_t read_abcd_file(const char *file_name)
{
  FILE *file;
  const char *line;
  uint64_t p;
  uint32_t seq, k, n, delta, b, n_count, s_count;
  int32_t c;
#if DUAL
  int dual_decided = dual_opt; /* 0 = undecided, 1 = decided */
#endif

  file = xfopen(file_name, "r", error);
  n_count = s_count = seq = n = 0;
  while ((line = read_line(file)) != NULL)
  {
    if (sscanf(line, "%" SCNu32, &delta) == 1)
    {
      if (s_count == 0)
      {
        fclose(file);
        return 0;
      }
      n += delta;
      add_seq_n(seq,n);
      n_count++;
    }
    else
      switch (sscanf(line, "ABCD %" SCNu32 "*%" SCNu32 "^$a%" SCNd32
                     " [%" SCNu32 "] // Sieved to %" SCNu64,
                     &k, &b, &c, &n, &p))
      {
        case 5:
          if (p_min == 0)
            p_min = p;
          else if (p_min != p)
            warning("--pmin=%"PRIu64" from command line overrides pmin=%"
                    PRIu64" from `%s'", p_min, p, file_name);

          /* fall through */

        case 4:
#if (BASE == 0)
          if (b < 2)
            line_error("Invalid base","ABCD",file_name);
          else if (b_term == 0)
            b_term = b;
          else if (b_term != b)
            line_error("Mismatched base","ABCD",file_name);
#else
          if (b != BASE)
            line_error("Invalid base","ABCD",file_name);
#endif
#if DUAL
          if (!dual_decided)
          {
            if (c != 1 && c != -1)
              dual_opt = 1;
            else
              dual_opt = 0;
            dual_decided = 1;
          }
          if (dual_opt)
          {
            if (k != 1)
              line_error("Not dual form b^n+/-k","ABCD",file_name);
            if (c > 0)
              seq = new_seq(c,1);
            else
              seq = new_seq(-c,-1);
          }
          else
#endif
          {
            if (c != 1 && c != -1)
              line_error("Not standard form k*b^n+/-1","ABCD",file_name);
            seq = new_seq(k,c);
          }
          s_count++;
          add_seq_n(seq,n);
          n_count++;
          break;

        default:
          if (s_count == 0)
          {
            fclose(file);
            return 0;
          }
          else
            line_error("Malformed line","ABCD",file_name);
      }
  }

  if (ferror(file))
    line_error("Read error","ABCD",file_name);
  fclose(file);
  report(1,"Read %"PRIu32" term%s for %"PRIu32" sequence%s from ABCD format "
       "file `%s'.",n_count,plural(n_count),s_count,plural(s_count),file_name);

  return n_count;
}

#if (BASE==5)
void overwrite_abcd(uint64_t p, const char *file_name, uint32_t del_k)
{
  FILE *file;
  uint32_t i, seq, n, s_count, t_count;

  if ((file = xfopen(file_name, "w", warning)) == NULL)
    return;

  s_count = t_count = 0;
  for (seq = 0; seq < seq_count; seq++)
    if (SEQ[seq].k == del_k)
    {
      report(1,"Removed all %"PRIu32" terms for sequence %s from the sieve.",
             SEQ[seq].ncount, seq_str(seq));
    }
    else
    {
      fprintf(file, "ABCD %" PRIu32 "*%u^$a%+" PRId32,
              SEQ[seq].k, (unsigned int)BASE, SEQ[seq].c);
      n = SEQ[seq].N[0];
      if (p > 0)
      {
        fprintf(file," [%" PRIu32 "] // Sieved to %" PRIu64 " with srsieve\n",
                n,p);
        p = 0;
      }
      else
        fprintf(file," [%" PRIu32 "]\n", n);

      for (i = 1; i < SEQ[seq].ncount; i++)
      {
        fprintf(file,"%" PRIu32 "\n",SEQ[seq].N[i]-n);
        n = SEQ[seq].N[i];
      }
      t_count += i;
      s_count ++;
    }

  report(1,"Wrote %"PRIu32" term%s for %"PRIu32" sequence%s to ABCD format "
       "file `%s'.",t_count,plural(t_count),s_count,plural(s_count),file_name);

  xfclose(file,file_name);
}

void write_newpgen_k(uint32_t seq, uint32_t n0, uint32_t n1)
{
  FILE *file;
  char file_name[FILENAME_MAX+1];
  int i, count;

  assert(ABS(SEQ[seq].c) == 1);

  snprintf(file_name, FILENAME_MAX, "%"PRIu32".txt", SEQ[seq].k);
  file_name[FILENAME_MAX] = '\0';
  file = xfopen(file_name, "w", error);
  if (SEQ[seq].c == 1)
    fprintf(file, "%"PRIu64":P:1:%u:257\n", p_min, (unsigned int)BASE);
  else
    fprintf(file, "%"PRIu64":M:1:%u:258\n", p_min, (unsigned int)BASE);

  for (i = 0, count = 0; i < SEQ[seq].ncount && SEQ[seq].N[i] < n1; i++)
    if (SEQ[seq].N[i] >= n0)
    {
      fprintf(file, "%"PRIu32" %"PRIu32"\n", SEQ[seq].k, SEQ[seq].N[i]);
      count++;
    }

  report(1,"Wrote %d term%s for sequence %s to %s format file `%s'.",
         count, plural(count), seq_str(seq), "NewPGen", file_name);
  xfclose(file,file_name);
}

void write_abc_k(uint32_t seq, uint32_t n0, uint32_t n1)
{
  FILE *file;
  char file_name[FILENAME_MAX+1];
  int i, count;

  assert(ABS(SEQ[seq].c) == 1);

  snprintf(file_name, FILENAME_MAX, "%"PRIu32".txt", SEQ[seq].k);
  file_name[FILENAME_MAX] = '\0';
  file = xfopen(file_name, "w", error);
  fprintf(file, "ABC %"PRIu32"*%u^$a%+"PRId32"\n",
          SEQ[seq].k, (unsigned int)BASE, SEQ[seq].c);
  for (i = 0, count = 0; i < SEQ[seq].ncount && SEQ[seq].N[i] < n1; i++)
    if (SEQ[seq].N[i] >= n0)
    {
      fprintf(file, "%"PRIu32"\n", SEQ[seq].N[i]);
      count++;
    }

  report(1,"Wrote %d term%s for sequence %s to %s format file `%s'.",
         count, plural(count), seq_str(seq), "ABC", file_name);
  xfclose(file,file_name);
}
#endif /* BASE==5 */

uint64_t read_checkpoint(uint64_t pmin, uint64_t pmax)
{
  FILE *file;
  uint64_t p;
  double cpu_secs = 0.0, elapsed_secs = 0.0;
  uint32_t count = 0;

  if ((file = fopen(checkpoint_file_name,"r")) == NULL)
    return pmin;

  if (fscanf(file,"pmin=%"SCNu64",factor_count=%"SCNu32
             ",cpu_secs=%lf,frac_done=%*f,elapsed_secs=%lf",
             &p,&count,&cpu_secs,&elapsed_secs) < 1)
    error("Cannot read checkpoint from `%s'.",checkpoint_file_name);

  xfclose(file,checkpoint_file_name);

  if (p > pmin && p < pmax)
  {
#if SOBISTRATOR_OPT
    if (!sobistrator_opt)
#endif
      report(1,"Resuming from checkpoint pmin=%" PRIu64 " in `%s'.",
             p, checkpoint_file_name);

    factor_count = count;
    set_accumulated_cpu(cpu_secs);
    set_accumulated_time(elapsed_secs);
    return p;
  }

  return pmin;
}

void write_checkpoint(uint64_t p)
{
  FILE *file;

  if ((file = xfopen(checkpoint_file_name,"w",warning)) != NULL)
  {
    fprintf(file,"pmin=%"PRIu64",factor_count=%"PRIu32
            ",cpu_secs=%.3f,frac_done=%f,elapsed_secs=%.3f\n",
            p,factor_count,get_accumulated_cpu(),frac_done(p),
            get_accumulated_time());
    xfclose(file,checkpoint_file_name);
  }
}

void remove_checkpoint(void)
{
  remove(checkpoint_file_name);
}

#if (BASE==2 || BASE==0)
uint32_t read_dat_file(const char *file_name, int32_t c)
{
  FILE *file;
  const char *line;
  uint32_t seq, k, n, delta, n_count, s_count;

#if (BASE==0)
  assert(b_term == 0 || b_term == 2);
  b_term = 2;
#endif

  file = xfopen(file_name,"r",error);
  n_count = s_count = seq = n = 0;
  while ((line = read_line(file)) != NULL)
  {
    if (sscanf(line,"%"SCNu32,&delta) == 1)
    {
      if (s_count > 0)
      {
        if (line[0] == '+')
          n += delta;
        else
          n = delta;
        add_seq_n(seq,n);
        n_count++;
      }
      /* else continue; skip first few unused lines. */
    }
    else if (sscanf(line,"k=%"SCNu32,&k) == 1)
    {
      seq = new_seq(k,c);
      s_count++;
      n = 0;
    }
    else
      line_error("Malformed line","dat",file_name);
  }

  if (ferror(file))
    line_error("Read error","dat",file_name);
  fclose(file);
  printf("Read %"PRIu32" term%s for %"PRIu32" sequence%s from dat format file"
         " `%s'.\n",n_count,plural(n_count),s_count,plural(s_count),file_name);

  return n_count;
}
#endif

#if USE_COMMAND_LINE_FILE
#include <string.h>
#include <stdlib.h>
void read_argc_argv(int *argc, char ***argv, const char *file_name)
{
  FILE *file;
  char *line;
  char **strv;
  int i, len;

  if ((file = xfopen(file_name,"r",NULL)) == NULL)
    return;

  line = xmalloc(1024);
  if (fgets(line,1023,file) == NULL)
  {
    free(line);
    fclose(file);
    return;
  }

  len = 16;
  strv = xmalloc(len*sizeof(char *));
  if ((strv[0] = strtok(line," \t\r\n")) == NULL)
  {
    free(strv);
    free(line);
    fclose(file);
    return;
  }

  for (i = 1; ((strv[i] = strtok(NULL," \t\r\n")) != NULL); i++)
    if (len <= i+1)
    {
      len += 16;
      strv = xrealloc(strv,len*sizeof(char *));
    }

  report(1,"(Read %d command line arguments from file `%s').",i-1,file_name);

  *argc = i;
  *argv = strv;
  fclose(file);
}
#endif
