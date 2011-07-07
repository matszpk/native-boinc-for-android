/* gcwsieve.c -- (C) Geoffrey Reynolds, March 2007.

   A sieve for Generalised Cullen and Woodall numbers.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include "gcwsieve.h"
#include "version.h"
#include "arithmetic.h"
#ifndef NDEBUG
#include "netnotify.h"
#endif

#if BOINC
#include "boinc_api.h"
#endif

#ifdef _MSC_VER
/* strtoull is a C99 function declared in stdlib.h */
#define strtoull   _strtoui64
#endif

#define NAME "gcwsieve"
#define DESC "A sieve for Generalised Cullen/Woodall numbers n*b^n+/-1"

const char *output_file_name = NULL;
const char *factors_file_name = NULL;
const char *checkpoint_file_name = NULL;
const char *log_file_name = NULL;
uint32_t save_period = DEFAULT_SAVE_PERIOD;
int quiet_opt = 0;
int verbose_opt = 0;
#if PREFETCH_OPT
int prefetch_opt = 0;
#endif
#if MULTISIEVE_OPT
int multisieve_opt = 0;
#endif
#if REPORT_PRIMES_OPT
int report_primes_opt = 0;
#endif
uint32_t L1_cache_size = 0;
uint32_t L2_cache_size = 0;
#ifdef SMALL_P
int smallp_phase = 0;
#endif

uint32_t base_opt = 0;
int cw_opt[2] = {0,0};

static int read_work_file(const char *file_name);
static void update_work_file(const char *file_name);

static void banner(void)
{
  printf(NAME " " XSTR(MAJOR_VER) "." XSTR(MINOR_VER) "." XSTR(PATCH_VER)
         " -- " DESC ".\n");
#ifdef SMALL_P
  printf("Compiled with SMALL_P option for sieving 2 < p < n_max.\n");
#endif
#ifdef __GNUC__
  if (verbose_opt)
    printf("Compiled on " __DATE__ " with GCC " __VERSION__ ".\n");
#endif
}

static void help(void)
{
  printf("Usage: %s [OPTION ...]\n", NAME);
  printf(" -p --pmin P0\n");
  printf(" -P --pmax P1          "
         "Sieve for factors p in the range P0 <= p <= P1\n");
  printf(" -i --input FILE       "
         "Read sieve from ABC format FILE (default `%s').\n",
         DEFAULT_INPUT_FILE_NAME);
  printf(" -o --output FILE      "
         "Write sieve to ABC format FILE.\n");
  printf(" -n --nmin N0\n");
  printf(" -N --nmax N1          "
         "Restrict sieve to terms n*b^n+/-1 where N0 <= n <= N1.\n");
  printf(" -b --base B           "
         "Restrict sieve to base B terms.\n");
  printf(" -C --cullen           "
         "Restrict sieve to Cullen terms n*b^n+1.\n");
  printf(" -W --woodall          "
         "Restrict sieve to Woodall terms n*b^n-1.\n");
#ifdef SMALL_P
  printf(" -B --begin            "
         "Begin a new sieve. Use -b -n -N -C -W to select.\n");
#endif
  printf(" -f --factors FILE     "
         "Append new factors to FILE (default `%s.%s').\n",
         DEFAULT_FACTORS_FILE_BASE,DEFAULT_FACTORS_FILE_EXT);
  printf(" -w --work FILE        "
         "Read sieve ranges from FILE (default `%s.%s').\n",
         DEFAULT_WORK_FILE_BASE,DEFAULT_WORK_FILE_EXT);
  printf(" -c --checkpoint FILE  "
         "Write checkpoints to FILE (default `%s.%s').\n",
         DEFAULT_CHECKPOINT_FILE_BASE,DEFAULT_CHECKPOINT_FILE_EXT);
  printf(" -u --uid STR          "
         "Append -STR to base of default per-process file names.\n");
  printf(" -s --save TIME        "
         "Save output/checkpoint every TIME minutes (default %u).\n",
         (unsigned int)DEFAULT_SAVE_PERIOD/60);
#if KNOWN_FACTORS_OPT
  printf(" -k --known FILE       "
         "Remove known factors in FILE before starting sieve.\n");
#endif
  printf(" -d --dummy X          "
         "Add dummy terms to fill gaps > X between exponents.\n");
#if MULTISIEVE_OPT
  printf(" -m --multisieve       "
         "Write factors and ABC files in MultiSieve format.\n");
#endif
#if REPORT_PRIMES_OPT
  printf(" -R --report-primes    "
         "Report primes/sec instead of p/sec.\n");
#endif
  printf(" -l --L1-cache SIZE    "
         "Assume L1 data cache is SIZE Kb.\n");
  printf(" -L --L2-cache SIZE    "
         "Assume L2 cache is SIZE Kb.\n");
#if defined(__i386__) && MULTI_PATH
  printf("    --sse2             "
         "Use SSE2 vector optimisations. (Default if supported)\n");
  printf("    --no-sse2          "
         "Don't use SSE2 vector optimisations.\n");
#elif defined(__x86_64__) && MULTI_PATH
  printf("    --intel            "
         "Process 4 terms in parallel. (Default for Intel CPUs).\n");
  printf("    --amd              "
         "Process 6 terms in parallel. (Default for AMD CPUs).\n");
#endif
#if HAVE_FORK
  printf(" -t --threads NUM      "
         "Start NUM child threads. (Default 0).\n");
#endif
#if PREFETCH_OPT
  printf("    --prefetch         "
         "Force use of software prefetch.\n");
  printf("    --no-prefetch      "
         "Prevent use of software prefetch.\n");
#endif
  printf(" -z --lower-priority   "
         "Run at low priority. (-zz lower).\n");
  printf(" -Z --raise-priority   "
         "Run at high priority. (-ZZ higher).\n");
  printf(" -A --affinity N       "
         "Set affinity to CPU number N.\n");
  printf(" -q --quiet            "
         "Don't print found factors.\n");
  printf(" -v --verbose          "
         "Print some extra messages. -vv prints more.\n");
  printf(" -h --help             "
         "Print this help.\n\n");

  exit(EXIT_SUCCESS);
}

#if BOINC
static char *get_boinc_file_name(const char *file_name)
{
  char *resolved_name = NULL;
  char tmp_name[512];

  if (file_name != NULL)
  {
    boinc_resolve_filename(file_name,tmp_name,sizeof(tmp_name));
    resolved_name = xmalloc(strlen(tmp_name)+1);
    strcpy(resolved_name,tmp_name);
  }

  return resolved_name;
}
#endif


static char *per_process_file_name(const char *base_name, const char *suffix,
                                   const char *uid)
{
  size_t len;
  char *name;

  len = strlen(base_name) + 1 + strlen(suffix) + 1;
  if (uid != NULL)
    len = len + strlen(uid) + 1;
  name = xmalloc(len);
  if (uid != NULL)
    sprintf(name,"%s-%s.%s",base_name,uid,suffix);
  else
    sprintf(name,"%s.%s",base_name,suffix);

  return name;
}

static const char *short_opts =
#ifdef SMALL_P
  "B"
#endif
#if HAVE_FORK
  "t:"
#endif
#if KNOWN_FACTORS_OPT
  "k:"
#endif
#if MULTISIEVE_OPT
  "m"
#endif
#if REPORT_PRIMES_OPT
  "R"
#endif
  "p:P:i:o:n:N:b:CWf:w:c:u:s:d:l:L:zZA:qvh";

static const struct option long_opts[] =
  {
    {"pmin",       required_argument, 0, 'p'},
    {"pmax",       required_argument, 0, 'P'},
    {"input",      required_argument, 0, 'i'},
    {"output",     required_argument, 0, 'o'},
    {"nmin",       required_argument, 0, 'n'},
    {"nmax",       required_argument, 0, 'N'},
    {"base",       required_argument, 0, 'b'},
    {"cullen",     no_argument,       0, 'C'},
    {"woodall",    no_argument,       0, 'W'},
#ifdef SMALL_P
    {"begin",      no_argument,       0, 'B'},
#endif
    {"factors",    required_argument, 0, 'f'},
    {"work",       required_argument, 0, 'w'},
    {"checkpoint", required_argument, 0, 'c'},
    {"uid",        required_argument, 0, 'u'},
    {"save",       required_argument, 0, 's'},
#if KNOWN_FACTORS_OPT
    {"known",      required_argument, 0, 'k'},
#endif
    {"dummy",      required_argument, 0, 'd'},
#if MULTISIEVE_OPT
    {"multisieve", no_argument,       0, 'm'},
#endif
#if REPORT_PRIMES_OPT
    {"report-primes", no_argument,    0, 'R'},
#endif
    {"L1-cache",   required_argument, 0, 'l'},
    {"L2-cache",   required_argument, 0, 'L'},
#if defined(__i386__) && MULTI_PATH
    {"no-sse2",    no_argument,       0, '1'},
    {"sse2",       no_argument,       0, '2'},
#elif defined(__x86_64__) && MULTI_PATH
    {"intel",      no_argument,       0, '1'},
    {"amd",        no_argument,       0, '2'},
#endif
#if HAVE_FORK
    {"threads",    required_argument, 0, 't'},
#endif
#if PREFETCH_OPT
    {"prefetch",   no_argument,       0, '3'},
    {"no-prefetch", no_argument,      0, '4'},
#endif
    {"lower-priority", no_argument,   0, 'z'},
    {"raise-priority", no_argument,   0, 'Z'},
    {"affinity",   required_argument, 0, 'A'},
    {"quiet",      no_argument,       0, 'q'},
    {"verbose",    no_argument,       0, 'v'},
    {"help",       no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };


static time_t start_date;
static int opt_ind, opt_c;
static uint64_t work_pmin;

static void attribute ((noreturn)) argerror(const char *str)
{
  if (long_opts[opt_ind].val == opt_c)
    error("--%s %s: argument %s.", long_opts[opt_ind].name, optarg, str);
  else
    error("-%c %s: argument %s.", opt_c, optarg, str);
}

static uint64_t parse_uint(uint64_t limit)
{
  uint64_t num;
  uint32_t expt;
  char *tail;

  errno = 0;
  num = strtoull(optarg,&tail,0);

  if (errno == 0 && num <= limit)
  {
    switch (*tail)
    {
      case 'e':
      case 'E':
        expt = strtoul(tail+1,&tail,0);
        if (errno != 0)
          goto range_error;
        if (*tail != '\0')
          break;
        while (expt-- > 0)
          if (num > limit/10)
            goto range_error;
          else
            num *= 10;
      case '\0':
        return num;
    }

    argerror("is malformed");
  }

 range_error:
  argerror("is out of range");
}

static void check_p_range(void)
{
#ifdef SMALL_P
  if (p_min <= 2 || p_min>=p_max || p_max >= UINT64_C(1)<<31)
    error("Sieve range P0 <= p <= P1 must be in 2 < P0 < P1 < 2^31.\n");
#else
  uint32_t lowlim = MAX(n_max,b_term);

  if (p_min <= lowlim || p_min>=p_max || p_max >= UINT64_C(1)<<MOD64_MAX_BITS)
    error("Sieve range P0 <= p <= P1 must be in %"PRIu32" < P0 < P1 < 2^%u.\n",
          lowlim, (unsigned int)MOD64_MAX_BITS);
#endif
}

int main(int argc, char **argv)
{
  const char *input_file_name = DEFAULT_INPUT_FILE_NAME;
  const char *work_file_name = NULL;
#if KNOWN_FACTORS_OPT
  const char *known_file_name = NULL;
#endif
  const char *uid = NULL;
  int priority_opt = 0;
#if MULTI_PATH
  int code_path = 0;
#endif
  uint32_t L1_opt = 0, L2_opt = 0;
#ifdef SMALL_P
  int begin_opt = 0;
#endif
  int want_help = 0;
  
#ifndef NDEBUG
  netnotify_wait();
#endif

#if BOINC
  boinc_init();
#endif

  set_accumulated_time(0.0);
  set_accumulated_cpu(0.0);

#if USE_COMMAND_LINE_FILE
  /* If no comand line arguments are given, attempt to read them from file.
   */
  if (argc == 1)
  {
#if BOINC
    char resolved_name[512];
    boinc_resolve_filename(NAME COMMAND_LINE_FILE_NAME_SUFFIX,
                           resolved_name, sizeof(resolved_name));
    read_argc_argv(&argc,&argv,resolved_name);
#else
    read_argc_argv(&argc,&argv,NAME COMMAND_LINE_FILE_NAME_SUFFIX);
#endif
  }
#endif

  n_min = 0, n_max = UINT32_MAX;

  while ((opt_c = getopt_long(argc,argv,short_opts,long_opts,&opt_ind)) != -1)
    switch (opt_c)
    {
      case 'p':
        p_min = parse_uint(UINT64_MAX);
        break;
      case 'P':
        p_max = parse_uint(UINT64_MAX);
        break;
      case 'i':
        input_file_name = optarg;
        break;
      case 'o':
        output_file_name = optarg;
        break;
      case 'n':
        n_min = parse_uint(UINT32_MAX);
        break;
      case 'N':
        n_max = parse_uint(UINT32_MAX);
        break;
      case 'b':
        base_opt = strtoul(optarg,NULL,0);
        if (base_opt < 2)
          argerror("is out of range");
        break;
      case 'C':
        cw_opt[1] = 1;
        break;
      case 'W':
        cw_opt[0] = 1;
        break;
#ifdef SMALL_P
      case 'B':
        begin_opt = 1;
        break;
#endif
      case 'f':
        factors_file_name = optarg;
        break;
      case 'w':
        work_file_name = optarg;
        break;
      case 'c':
        checkpoint_file_name = optarg;
        break;
      case 'u':
        uid = optarg;
        break;
      case 's':
        save_period = strtoul(optarg,NULL,0) * 60;
        break;
#if KNOWN_FACTORS_OPT
      case 'k':
        known_file_name = optarg;
        break;
#endif
      case 'd':
        dummy_opt = strtoul(optarg,NULL,0);
        break;
#if MULTISIEVE_OPT
      case 'm':
        multisieve_opt = 1;
        break;
#endif
#if REPORT_PRIMES_OPT
      case 'R':
        report_primes_opt = 1;
        break;
#endif
      case 'l':
        L1_opt = strtoul(optarg,NULL,0);
        break;
      case 'L':
        L2_opt = strtoul(optarg,NULL,0);
        break;
#if MULTI_PATH
      case '1':
        code_path = 1;
        break;
      case '2':
        code_path = 2;
        break;
#endif
#if HAVE_FORK
      case 't':
        num_children = strtol(optarg,NULL,0);
        if (num_children < 0 || num_children > MAX_CHILDREN)
          argerror("out of range");
        break;
#endif
#if PREFETCH_OPT
      case '3':
        prefetch_opt = 1;
        break;
      case '4':
        prefetch_opt = -1;
        break;
#endif
      case 'z':
        priority_opt--;
        break;
      case 'Z':
        priority_opt++;
        break;
      case 'A':
#if HAVE_FORK
        if (affinity_opt < MAX_CHILDREN)
          child_affinity[affinity_opt++] = strtol(optarg,NULL,0);
#else
        set_cpu_affinity(strtol(optarg,NULL,0));
#endif
        break;
      case 'q':
        quiet_opt++;
        break;
      case 'v':
        verbose_opt++;
        break;
      case 'h':
        want_help = 1;
        break;
      default:
        return 1;
    }

  banner();

  if (want_help)
    help();

#if HAVE_FORK
  if (affinity_opt && num_children == 0)
    set_cpu_affinity(child_affinity[0]);
#endif

  /* If neither `-C --cullen' nor `-W --woodall' is given then allow either.
   */
  if (cw_opt[0] == 0 && cw_opt[1] == 0)
    cw_opt[0] = cw_opt[1] = 1;

#if !BOINC
  /* We install these signal handlers early because the default handler
     terminates the program. SIGTERM and SIGINT handlers are not installed
     until sieving begins, but termination is the right default for them.
  */
#ifdef SIGUSR1
  signal(SIGUSR1,handle_signal);
#endif
#ifdef SIGUSR2
  signal(SIGUSR2,handle_signal);
#endif
#endif /* !BOINC */

  log_file_name = per_process_file_name(LOG_FILE_BASE,LOG_FILE_EXT,uid);
  if (checkpoint_file_name == NULL)
    checkpoint_file_name = per_process_file_name(DEFAULT_CHECKPOINT_FILE_BASE,
                                              DEFAULT_CHECKPOINT_FILE_EXT,uid);
  if (factors_file_name == NULL)
    factors_file_name = per_process_file_name(DEFAULT_FACTORS_FILE_BASE,
                                              DEFAULT_FACTORS_FILE_EXT,uid);
  if (work_file_name == NULL)
    work_file_name = per_process_file_name(DEFAULT_WORK_FILE_BASE,
                                         DEFAULT_WORK_FILE_EXT,uid);

#if BOINC
  log_file_name = get_boinc_file_name(log_file_name);
  checkpoint_file_name = get_boinc_file_name(checkpoint_file_name);
  factors_file_name = get_boinc_file_name(factors_file_name);
  work_file_name = get_boinc_file_name(work_file_name);
  input_file_name = get_boinc_file_name(input_file_name);
  output_file_name = get_boinc_file_name(output_file_name);
#if KNOWN_FACTORS_OPT
  known_file_name = get_boinc_file_name(known_file_name);
#endif
#endif

#if defined(__i386__) && MULTI_PATH
  if (code_path == 0)
    code_path = (have_sse2()) ? 2 : 1;
  if (verbose_opt && code_path == 2)
    printf("SSE2 code path, ");
#elif defined(__x86_64__) && MULTI_PATH
  if (code_path == 0)
    code_path = (is_amd()) ? 2 : 1;
  if (verbose_opt)
  {
    if (code_path == 1)
      printf("Intel code path, ");
    else  /* code_path == 2 */
      printf("AMD code path, ");
  }
#endif

  set_cache_sizes(L1_opt,L2_opt);

#if PREFETCH_OPT
  /* If support for the prefetch instruction is not detected and the user
     has not forced its use with the --prefetch switch, then don't use it.
  */
  if (prefetch_opt == 0 && !have_prefetch())
    prefetch_opt = -1;
#endif

  if (priority_opt)
    set_process_priority(priority_opt);

  if (optind < argc)
    error("Unhandled argument %s.", argv[optind]);

  if (n_min >= n_max)
    error("-n --nmin N0 and -N --nmax N1 must satisfy N0 < N1.");

#ifdef SMALL_P
  if (begin_opt)
  {
    if (!base_opt)
      error("-b is a required argument when beginning a new sieve.");
    if (n_max == UINT32_MAX)
      error("-N is a required argument when beginning a new sieve.");

    p_min = 3;
    if (p_max == 0)
      p_max = n_max;
    begin_sieve();
  }
  else
#endif
  read_abc_file(input_file_name);

#if KNOWN_FACTORS_OPT
  remove_known_factors(known_file_name);
#endif

  finish_candidate_terms();

  if (p_max > 0)
  {
#ifdef SMALL_P
    if (begin_opt)
      work_pmin = 2;
    else
#endif
    {
      check_p_range();
      work_pmin = p_min;
      p_min = read_checkpoint();
    }
#if MULTI_PATH
    switch (code_path)
    {
      default:
      case 1:
        sieve();
        break;
      case 2:
#if defined(__i386__)
        sieve_sse2();
#elif defined(__x86_64__)
        sieve_amd();
#endif
        break;
    }
#else
    sieve();
#endif
  }
  else while (read_work_file(work_file_name))
  {
    check_p_range();
    factor_count = 0;
    work_pmin = p_min;
    p_min = read_checkpoint();
    notify_event(sieve_parameters_changed);
#if MULTI_PATH
    switch (code_path)
    {
      default:
      case 1:
        sieve();
        break;
      case 2:
#if defined(__i386__)
        sieve_sse2();
#elif defined(__x86_64__)
        sieve_amd();
#endif
        break;
    }
#else
    sieve();
#endif
    update_work_file(work_file_name);
#if BOINC
    break;
#endif
  }

#if BOINC
  if (factor_count == 0) /* Ensure a non-empty factors file exists */
  {
    FILE *file = xfopen(factors_file_name,"a",error);
    fprintf(file,"no factors\n");
    fclose(file);
  }
  boinc_finish(0);
#endif

  return EXIT_SUCCESS;
}

#define RANGE_BLOCK 1000000000
static int read_work_file(const char *file_name)
{
  uint64_t p0, p1;
  FILE *file;
  int ret;

  report(0,"Reading work file `%s' ...",file_name);
  file = xfopen(file_name,"r",error);

  ret = fscanf(file,"%"SCNu64"%*[-,\t ]%"SCNu64,&p0,&p1);
  xfclose(file,file_name);
  switch (ret)
  {
    case EOF:
      report(1,"No more work in `%s'.",file_name);
      return 0;

    case 2:
      if (p0 >= p1)
        error("Bad range %"PRIu64",%"PRIu64" in `%s'.",p0,p1,file_name);
      if (p1 > UINT64_MAX/RANGE_BLOCK)
        error("Range end %"PRIu64" too high in `%s'.",p1,file_name);
      report(1,"Using range %"PRIu64"*10^9 <= p <= %"PRIu64"*10^9 from `%s'.",
             p0,p1,file_name);
      p_min = p0*RANGE_BLOCK;
      p_max = p1*RANGE_BLOCK;
      return 1;

    case 0:
    case 1:
      error("Malformed first line in `%s'.",file_name);

    default:
      error("Error while reading `%s'.",file_name);
  }
}

#define MAX_WORK_LINES 100
#define MAX_LINE_LENGTH 60
static void update_work_file(const char *file_name)
{
  FILE *file;
  char *lines[MAX_WORK_LINES];
  int i, j;

  file = xfopen(file_name,"r",error);

  for (i = 0; i < MAX_WORK_LINES; i++)
  {
    lines[i] = xmalloc(MAX_LINE_LENGTH);
    if (fgets(lines[i],MAX_LINE_LENGTH,file) == NULL)
    {
      free(lines[i]);
      break;
    }
  }

  xfclose(file,file_name);
  file = xfopen(file_name,"w",error);

  for (j = 1; j < i; j++)
    fputs(lines[j],file);
  xfclose(file,file_name);

  for (j = 0; j < i; j++)
    free(lines[j]);
}

/* Return the fraction of the current range that has been done.
 */
double frac_done(uint64_t p)
{
  return (double)(p-work_pmin)/(p_max-work_pmin);
}

#define STRFTIME_FORMAT "ETA %d %b %H:%M"

void print_status(uint64_t p, uint32_t p_per_sec
#if REPORT_PRIMES_OPT
                  ,uint32_t primes_per_sec
#endif
                  )
{
  static int toggle = 0;
  char buf1[32], buf2[32];

  toggle = !toggle; /* toggle between reporting ETA and factor rate. */

  if (toggle && factor_count && p_per_sec)
  {
    uint64_t p_per_factor = (p-work_pmin)/factor_count;
    snprintf(buf1,sizeof(buf1),"%"PRIu64" sec/factor",p_per_factor/p_per_sec);
  }
  else
  {
    double done = (double)(p-p_min)/(p_max-p_min);
    buf1[0] = '\0';
    if (done > 0.0) /* Avoid division by zero */
    {
      time_t finish_date = start_date+(time(NULL)-start_date)/done;
      struct tm *finish_tm = localtime(&finish_date);
      if (!finish_tm || !strftime(buf1,sizeof(buf1),STRFTIME_FORMAT,finish_tm))
        buf1[0] = '\0';
    }
  }

#if REPORT_PRIMES_OPT
  if (report_primes_opt)
    snprintf(buf2,sizeof(buf2),"%"PRIu32" primes/sec",primes_per_sec);
  else
#endif
    snprintf(buf2,sizeof(buf2),"%"PRIu32" p/sec",p_per_sec);


  report(0,"p=%"PRIu64", %s, %"PRIu32" factor%s, %.1f%% done, %s",
         p,buf2,factor_count,plural(factor_count),100.0*frac_done(p),buf1);

#if BOINC
  boinc_fraction_done(frac_done(p));
#endif
}

static double expected_factors(uint32_t n, uint64_t p0, uint64_t p1)
{
  /* TODO: Use a more accurate formula. This one is only reasonable when
     pmin/pmax is close to 1.
   */

  return n*(1-log(p0)/log(p1));
}

void start_gcwsieve(void)
{
  if (verbose_opt)
    report(1,"Expecting to find factors for about %.2f terms.",
           expected_factors(ncount[0]+ncount[1],work_pmin,p_max));

  logger(1,"%s started: %"PRIu32" <= n <= %"PRIu32", %"PRIu64" <= p <= %"
         PRIu64, NAME " "XSTR(MAJOR_VER)"."XSTR(MINOR_VER)"."XSTR(PATCH_VER),
         n_min, n_max, p_min, p_max);

  start_date = time(NULL);
}

void finish_gcwsieve(const char *reason, uint64_t p)
{
  logger(1,"%s stopped: at p=%"PRIu64" because %s.",
         NAME " "XSTR(MAJOR_VER)"."XSTR(MINOR_VER)"."XSTR(PATCH_VER),p,reason);
  write_abc_file(1,p,output_file_name);

  logger(verbose_opt,"Found factors for %"PRIu32" term%s in %.3f sec."
         " (expected about %.2f)",factor_count,plural(factor_count),
         get_accumulated_time(),expected_factors(ncount[0]+ncount[1],work_pmin,p));

#if REMOVE_CHECKPOINT
  if (p >= p_max) /* Sieving job completed */
    remove_checkpoint();
  else
#endif
    write_checkpoint(p);

  set_accumulated_time(0.0);
  set_accumulated_cpu(0.0);
}
