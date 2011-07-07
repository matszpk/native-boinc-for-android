/* files.c -- (C) Geoffrey Reynolds, May 2006.

   Routines for reading and writing sieve files.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include "gcwsieve.h"

static int line_counter = 0;
static void line_error(const char *msg, const char *file_name)
{
  error("Line %d: %s in input file `%s'.",line_counter,msg,file_name);
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

  while ((ptr = fgets(buf,sizeof(buf),file)) != NULL)
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

static void check_p(uint64_t p, const char *file_name)
{
  if (p_min == 0)
    p_min = p;
  else if (p_min != p)
    warning("--pmin=%"PRIu64" from command line overrides pmin=%"PRIu64
            " from `%s'", p_min, p, file_name);
}

static void check_c(int32_t c, const char *file_name)
{
  if (c != 1 && c != -1)
    line_error("|c| != 1 in n*b^n+c",file_name);
}

void read_abc_file(const char *file_name)
{
  FILE *file;
  const char *line;
  uint64_t p;
  uint32_t b, n;
  int32_t c;
  char ch1, ch2, ch3, ch4;

  if (base_opt)
    b_term = base_opt;

  /* Try to read header */
  file = xfopen(file_name,"r",error);
  line = read_line(file);

  switch (sscanf(line,"ABC $%c*$%c^$%c$%c //%*[^0-9]%"SCNu64,
                 &ch1,&ch2,&ch3,&ch4,&p))
  {
    default:
      line_error("Invalid ABC header",file_name);

    case 5:
      check_p(p,file_name);
      /* fall through */

    case 4: /* MultiSieve format */
      if (ch1 != 'a' || ch2 != 'b' || ch3 != 'a' || ch4 != 'c')
        line_error("Wrong ABC format",file_name);
      if ((line = read_line(file)) != NULL)
      {
        if (sscanf(line, "%"SCNu32" %"SCNu32" %"SCNd32,&n,&b,&c) != 3)
          line_error("Malformed line",file_name);
        if (n < 2)
          line_error("n too small in n*b^n+c",file_name);
        if (b < 2)
          line_error("b too small in n*b^n+c",file_name);
        if (!base_opt)
          b_term = b;
        check_c(c,file_name);
        if (b == b_term)
          add_term(n,c);
        while ((line = read_line(file)) != NULL)
        {
          if (sscanf(line, "%"SCNu32" %"SCNu32" %"SCNd32,&n,&b,&c) != 3)
            line_error("Malformed line",file_name);
          if (b != b_term)
          {
            if (base_opt)
              continue;
            else
              line_error("mismatched b in n*b^n+c",file_name);
          }
          check_c(c,file_name);
          add_term(n,c);
        }
      }
      break;

    case 1:
      switch (sscanf(line,"ABC $%c*%"SCNu32"^$%c%"SCNd32" //%*[^0-9]%"SCNu64,
                 &ch1,&b,&ch3,&c,&p))
      {
        default:
          line_error("Invalid ABC header",file_name);

        case 5:
          check_p(p,file_name);
          /* fall through */

        case 4: /* Fixed c format */
          if (ch1 != 'a' || ch3 != 'a')
            line_error("Wrong ABC format",file_name);
          if (b < 2)
            line_error("b too small in n*b^n+c",file_name);
          if (base_opt && b != b_term)
            break;
          b_term = b;
          check_c(c,file_name);
          while ((line = read_line(file)) != NULL)
          {
            if (sscanf(line, "%"SCNu32,&n) != 1)
              line_error("Malformed line",file_name);
            add_term(n,c);
          }
          break;

        case 3:
          /* The "%*[+$]" allows the "+" to be ignored if present. This only
             works because we have already determined that a number was not
             matched in this position. */
          switch (sscanf(line,"ABC $%c*%"SCNu32"^$%c%*[+$]%c //%*[^0-9]%"
                         SCNu64,&ch1,&b,&ch3,&ch4,&p))
          {
            default:
              line_error("Invalid ABC header",file_name);

            case 5:
              check_p(p,file_name);
              /* fall through */

            case 4: /* Variable c format */
              if (ch1 != 'a' || ch3 != 'a' || ch4 != 'b')
                line_error("Wrong ABC format",file_name);
              if (b < 2)
                line_error("b too small in n*b^n+c",file_name);
              if (base_opt && b != b_term)
                break;
              b_term = b;
              while ((line = read_line(file)) != NULL)
              {
                if (sscanf(line, "%"SCNu32" %"SCNd32,&n,&c) != 2)
                  line_error("Malformed line",file_name);
                check_c(c,file_name);
                add_term(n,c);
              }
              break;
          }
      }
      break;
  }

  if (ferror(file))
    line_error("Read error",file_name);
  fclose(file);

  report(1,"Read %"PRIu32" term%s n*%"PRIu32"^n%s from ABC file `%s'.",
         ncount[0]+ncount[1],plural(ncount[0]+ncount[1]),b_term,
         ncount[0]? (ncount[1]? "+/-1" : "-1") : "+1", file_name);
}

#if MULTISIEVE_OPT
static void write_term_multisieve(uint32_t n, int32_t c, void *file)
{
  fprintf((FILE *)file,"%"PRIu32" %"PRIu32" %+"PRId32"\n",n,b_term,c);
}
#endif

/* Fixed c format */
static void write_term1(uint32_t n, int32_t c, void *file)
{
  fprintf((FILE *)file,"%"PRIu32"\n",n);
}

/* Variable c format */
static void write_term2(uint32_t n, int32_t c, void *file)
{
  fprintf((FILE *)file,"%"PRIu32" %+"PRId32"\n",n,c);
}

void write_abc_file(int scroll, uint64_t p, const char *file_name)
{
  if (file_name != NULL)
  {
    FILE *file;

    if ((file = xfopen(file_name, "w", warning)) != NULL)
    {
      uint32_t count;

#if MULTISIEVE_OPT
      if (multisieve_opt)
      {
        fprintf(file,"ABC $a*$b^$a$c // CW Sieved to: %"PRIu64"\n",p);
        count = for_each_term(write_term_multisieve,file);
      }
      else
#endif
      {
        if (ncount[0] && ncount[1]) /* use variable c format */
        {
          fprintf(file,"ABC $a*%"PRIu32"^$a$b "
                  "// Sieved to %"PRIu64" with gcwsieve\n",b_term,p);
          count = for_each_term(write_term2,file);
        }
        else /* use fixed c format */
        {
          fprintf(file,"ABC $a*%"PRIu32"^$a%+"PRId32
                  " // Sieved to %"PRIu64" with gcwsieve\n",
                  b_term, ncount[0]? -1 : 1, p);
          count = for_each_term(write_term1,file);
        }
      }
      xfclose(file,file_name);

      report(scroll,"Wrote %"PRIu32" term%s n*%"PRIu32"^n%s to ABC file `%s'.",
             count,plural(count),b_term,
             ncount[0]? (ncount[1]? "+/-1" : "-1") : "+1", file_name);
    }
  }
}

uint64_t read_checkpoint(void)
{
  FILE *file;
  uint64_t p;
  double cpu_secs = 0.0, elapsed_secs = 0.0;
  uint32_t count = 0;

  if ((file = fopen(checkpoint_file_name,"r")) == NULL)
    return p_min;

  if (fscanf(file,"pmin=%"SCNu64",factor_count=%"SCNu32
             ",cpu_secs=%lf,frac_done=%*f,elapsed_secs=%lf",
             &p,&count,&cpu_secs,&elapsed_secs) < 1)
    error("Cannot read checkpoint from `%s'.",checkpoint_file_name);

  xfclose(file,checkpoint_file_name);

  if (p > p_min && p < p_max)
  {
    report(1,"Resuming from checkpoint pmin=%" PRIu64 " in `%s'.",
           p, checkpoint_file_name);
    factor_count = count;
    set_accumulated_cpu(cpu_secs);
    set_accumulated_time(elapsed_secs);
    return p;
  }

  return p_min;
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
