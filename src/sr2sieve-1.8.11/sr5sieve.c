/* sr5sieve.c -- (C) Geoffrey Reynolds, June 2006.

   Srsieve specialised for the Sierpinski/Riesel Base 5 projects.

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
#include <errno.h>
#include <getopt.h>
#include "sr5sieve.h"
#include "version.h"
#include "arithmetic.h"

#if HAVE_MALLOPT
#include <malloc.h>
#endif

#define NAME BASE_NAME "sieve"
#if DUAL
# if (BASE==0)
#  define DESC "A sieve for multiple sequences k*b^n+/-1 or b^n+/-k"
# else
#  define DESC "A sieve for the (Dual) Sierpinski/Riesel Base " XSTR(BASE) " projects"
# endif
#else
# if (BASE==0)
#  define DESC "A sieve for multiple sequences k*b^n+/-1"
# else
#  define DESC "A sieve for the Sierpinski/Riesel Base " XSTR(BASE) " projects"
# endif
#endif

/* One per process file names. */
char *checkpoint_file_name = NULL;
char *factors_file_name = NULL;
char *log_file_name = NULL;
static char *work_file_name = NULL;
static int factors_opt = 0;

#define DATA_FILE_NAME BASE_NAME "data.txt"
#define DEFAULT_CACHE_FILE_NAME BASE_NAME "cache.bin"

#if (BASE==5)
#define FACTORS_EMAIL "base5.factors@yahoo.com"
#endif

#if (BASE==2 || BASE==0)
#define SOB_DAT "SoB.dat"
#define RIESEL_DAT "riesel.dat"
#endif

int verbose_opt = 0;
int quiet_opt = 0;
uint32_t save_period = DEFAULT_SAVE_PERIOD;
uint32_t L1_cache_size = 0;
uint32_t L2_cache_size = 0;
const char *baby_method_opt = NULL;
const char *giant_method_opt = NULL;
const char *ladder_method_opt = NULL;
const char *duplicates_file_name = NULL;
#if HASHTABLE_SIZE_OPT
uint32_t hashtable_size = 0;
#endif
#if SKIP_CUBIC_OPT
int skip_cubic_opt = 0;
#endif
#if DUAL
int dual_opt = 0;
#endif
#if GIANT_OPT
uint32_t min_giant_opt = MIN_GIANT_DEFAULT;
double scale_giant_opt = SCALE_GIANT_DEFAULT;
#endif
#if LOG_FACTORS_OPT
int log_factors_opt = 0;
#endif


static void banner(void)
{
  printf(NAME " " XSTR(MAJOR_VER) "." XSTR(MINOR_VER) "." XSTR(PATCH_VER)
         " -- " DESC ".\n");
#ifdef __GNUC__
  if (verbose_opt)
    printf("Compiled on " __DATE__ " with GCC " __VERSION__ ".\n");
#endif
}

static void help(void)
{
  printf("Usage: %s [OPTION ...]\n", NAME);
#if (BASE==5)
  printf(" -d --delete K         "
         "Delete the sequence K*%u^n+/-1 from `%s'.\n",
         (unsigned int)BASE, DATA_FILE_NAME);
  printf(" -g --newpgen K N0 N1  "
         "Generate a NewPGen file for K*%u^n+/-1 with N0 <= n < N1.\n",
         (unsigned int)BASE);
  printf(" -a --abc K N0 N1      "
         "Generate an ABC file for K*%u^n+/-1 with N0 <= n < N1.\n",
         (unsigned int)BASE);
#endif
#if (BASE==2 || BASE==0)
  printf(" -s --sierpinski       "
         "Sieve sequences k*2^n+1 in dat format file '%s'\n",SOB_DAT);
  printf(" -r --riesel           "
         "Sieve sequences k*2^n-1 in dat format file '%s'\n",RIESEL_DAT);
#endif
  printf(" -i --input FILE       "
         "Read sieve from FILE instead of `%s'.\n", DATA_FILE_NAME);
#if (BASE==2 || BASE==0)
  printf("                         Instead of `%s',`%s' when used with"
         " -s,-r.\n", SOB_DAT, RIESEL_DAT);
#endif
  printf(" -p --pmin P0          "
         "Sieve for factors p in the range P0 <= p <= P1 instead\n");
  printf(" -P --pmax P1          "
         " of reading ranges from work file `%s.%s'.\n",
         WORK_FILE_BASE, WORK_FILE_SUFFIX);
  printf(" -f --factors FILE     "
         "Append found factors to FILE instead of %s.%s.\n",
         FACTORS_FILE_BASE,FACTORS_FILE_SUFFIX);
  printf(" -u --uid STRING       "
         "Append -STRING to base of per-process file names.\n");
#if LEGENDRE_CACHE
  printf(" -c --cache            "
         "Save Legendre symbol tables to `%s'.\n",DEFAULT_CACHE_FILE_NAME);
  printf(" -C --cache-file FILE  "
         "Load (or save) Legendre symbol tables from (or to) FILE.\n");
#endif
#if MULTI_PATH
#ifdef __i386__
  printf("    --amd              "
         "Use CMOV optimisations. (Default for AMDs if supported).\n");
  printf("    --intel            "
         "Don't use CMOV optimisations. (Default for Intels).\n");
#endif
#ifdef ARM_CPU
  printf("    --neon             "
         "Use NEON vector optimisations. (Default if supported).\n");
  printf("    --no-neon          "
         "Don't use NEON vector optimisations.\n");
#else
  printf("    --sse2             "
         "Use SSE2 vector optimisations. (Default if supported).\n");
  printf("    --no-sse2          "
         "Don't use SSE2 vector optimisations.\n");
#endif
#endif
#if HAVE_FORK
  printf(" -t --threads NUM      "
         "Start NUM child threads. (Default 0).\n");
#endif
  printf(" -l --L1-cache SIZE    "
         "Assume L1 data cache is SIZE Kb.\n");
  printf(" -L --L2-cache SIZE    "
         "Assume L2 cache is SIZE Kb.\n");
  printf(" -B --baby METHOD      "
         "Use METHOD for baby step mulmods.\n");
  printf(" -G --giant METHOD     "
         "Use METHOD for giant step mulmods.\n");
  printf("    --ladder METHOD    "
         "Use METHOD for ladder mulmods.\n");
#if HASHTABLE_SIZE_OPT
  printf(" -H --hashtable SIZE   "
         "Force use of a SIZE Kb hashtable.\n");
#endif
#if SUBSEQ_Q_OPT
#if (BASE == 0)
  printf(" -Q --subseq Q         "
         "Force sieving k*b^n+c as subsequences (k*b^d)*(b^Q)^m+c.\n");
#else
  printf(" -Q --subseq Q         "
         "Force sieving k*%u^n+c as subsequences (k*%u^d)*(%u^Q)^m+c.\n",
         (unsigned int)BASE,(unsigned int)BASE,(unsigned int)BASE);
#endif
#endif
#if GIANT_OPT
  printf("    --scale-giant X    "
         "Scale the number of giant steps by X (default %.2f).\n",
         SCALE_GIANT_DEFAULT);
  printf("    --min-giant NUM    "
         "Always perform at least NUM giant steps (default %d).\n",
         MIN_GIANT_DEFAULT);
#endif
  printf(" -D --duplicates FILE  "
         "Append duplicate factors to FILE.\n");
#if LOG_FACTORS_OPT
  printf("    --log-factors      "
         "Record each new factor (with date found) in the log file.\n");
#endif
  printf(" -S --save TIME        "
         "Write checkpoint every TIME seconds. (default %"PRIu32").\n",
         DEFAULT_SAVE_PERIOD);
  printf(" -z --lower-priority   "
         "Run at low priority. (-zz lower).\n");
  printf(" -Z --raise-priority   "
         "Run at high priority. (-ZZ higher).\n");
  printf(" -A --affinity N       "
         "Set affinity to CPU number N.\n");
#if SOBISTRATOR_OPT
  printf(" -j --sobistrator      "
         "Run in Sobistrator compatibility mode.\n");
#endif
#if SKIP_CUBIC_OPT
  printf(" -X --skip-cubic       "
         "Skip cubic and higher power residue tests.\n");
#endif
#if NO_LOOKUP_OPT
  printf(" -x --no-lookup        "
         "Don't pre-compute Legendre symbol lookup tables.\n");
#endif
#if DUAL
  printf(" -d --dual             "
         "Sieve b^n+/-k instead of k*b^n+/-1.\n");
#endif
  printf(" -q --quiet            "
         "Don't print found factors.\n");
  printf(" -v --verbose          "
         "Print some extra messages. -vv prints more.\n");
  printf(" -h --help             "
         "Print this help.\n\n");

  printf("Without arguments (normal usage), read sieve from `%s',"
         " read ranges\n", DATA_FILE_NAME);
  printf("A,B from `%s.%s', sieve for factors p in the range"
         " A*10^9 <= p <= B*10^9.\n", WORK_FILE_BASE, WORK_FILE_SUFFIX);

  exit(EXIT_SUCCESS);
}

static const char *short_opts =
#if (BASE==5)
  "d:g:a:"
#endif
#if (BASE==2 || BASE==0)
  "sr"
#endif
#if LEGENDRE_CACHE
  "cC:"
#endif
#if HAVE_FORK
  "t:"
#endif
#if HASHTABLE_SIZE_OPT
  "H:"
#endif
#if SUBSEQ_Q_OPT
  "Q:"
#endif
#if SOBISTRATOR_OPT
  "j"
#endif
#if SKIP_CUBIC_OPT
  "X"
#endif
#if NO_LOOKUP_OPT
  "x"
#endif
#if DUAL
  "d"
#endif
  "i:p:P:f:u:l:L:B:G:D:S:zZA:qvh";

static const struct option long_opts[] =
  {
#if (BASE==5)
    {"delete",     required_argument, 0, 'd'},
    {"newpgen",    required_argument, 0, 'g'},
    {"abc",        required_argument, 0, 'a'},
#endif
#if (BASE==2 || BASE==0)
    {"sierpinski", required_argument, 0, 's'},
    {"riesel",     required_argument, 0, 'r'},
#endif
    {"input",      required_argument, 0, 'i'},
    {"pmin",       required_argument, 0, 'p'},
    {"pmax",       required_argument, 0, 'P'},
    {"factors",    required_argument, 0, 'f'},
    {"uid",        required_argument, 0, 'u'},
#if LEGENDRE_CACHE
    {"cache",      no_argument,       0, 'c'},
    {"cache-file", required_argument, 0, 'C'},
#endif
#if MULTI_PATH
#ifdef ARM_CPU
    {"no-neon",    no_argument,       0, '1'},
    {"neon",       no_argument,       0, '2'},
#else
    {"no-sse2",    no_argument,       0, '1'},
    {"sse2",       no_argument,       0, '2'},
#endif
#ifdef __i386__
    {"amd",        no_argument,       0, '3'},
    {"intel",      no_argument,       0, '4'},
#endif
#endif /* MULTI_PATH */
#if HAVE_FORK
    {"threads",    required_argument, 0, 't'},
#endif
    {"L1-cache",   required_argument, 0, 'l'},
    {"L2-cache",   required_argument, 0, 'L'},
    {"baby",       required_argument, 0, 'B'},
    {"giant",      required_argument, 0, 'G'},
    {"ladder",     required_argument, 0, '5'},
#if HASHTABLE_SIZE_OPT
    {"hashtable",  required_argument, 0, 'H'},
#endif
#if SUBSEQ_Q_OPT
    {"subseq",     required_argument, 0, 'Q'},
#endif
#if GIANT_OPT
    {"scale-giant", required_argument, 0, '6'},
    {"min-giant",  required_argument, 0, '7'},
#endif
    {"duplicates", required_argument, 0, 'D'},
#if LOG_FACTORS_OPT
    {"log-factors", no_argument,      0, '8'},
#endif
    {"save",       required_argument, 0, 'S'},
    {"lower-priority", no_argument,   0, 'z'},
    {"raise-priority", no_argument,   0, 'Z'},
    {"affinity",   required_argument, 0, 'A'},
#if SOBISTRATOR_OPT
    {"sobistrator", no_argument,      0, 'j'},
#endif
#if SKIP_CUBIC_OPT
    {"skip-cubic", no_argument,       0, 'X'},
#endif
#if NO_LOOKUP_OPT
    {"no-lookup",  no_argument,       0, 'x'},
#endif
#if DUAL
    {"dual",       no_argument,       0, 'd'},
#endif
    {"quiet",      no_argument,       0, 'q'},
    {"verbose",    no_argument,       0, 'v'},
    {"help",       no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };

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

static time_t start_date;
static uint64_t work_pmin;
static uint32_t terms_read = 0;
static int read_work_file(const char *uid);
static void update_work_file(void);

static int opt_ind, opt_c;

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

static void check_p_range(uint64_t pmin, uint64_t pmax, const char *filename)
{
  uint32_t low_lim;

#ifdef HAVE_vec_powmod64
  low_lim = MAX(k_max,POWER_RESIDUE_LCM);
#else
  low_lim = k_max;
#endif

  if (pmin <= low_lim || pmin >= pmax || pmax >= (UINT64_C(1)<<MOD64_MAX_BITS))
  {
    if (filename == NULL)
      error("`-p --pmin P0' and `-P --pmax P1' must satisfy %"
            PRIu32" < P0 < P1 < 2^"XSTR(MOD64_MAX_BITS)".", low_lim);
    else
      error("pmin=%"PRIu64",pmax=%"PRIu64" from `%s' does not satisfy %"
            PRIu32" < pmin < pmax < 2^"XSTR(MOD64_MAX_BITS)".",
            pmin, pmax, filename, low_lim);
  }
}

#if MULTI_PATH
static int code_path = 0;
#ifdef ARM_CPU
static int neon_opt = 0;
#else
static int sse2_opt = 0;
#endif
#ifdef __i386__
static int cmov_opt = 0;
#endif
#endif /* MULTI_PATH */

#if (__x86_64__ && MULTI_PATH)
static void x86_64_select_code_path(uint64_t pmax)
{
  int new_path = code_path;

  if (code_path == 2 && pmax >= (UINT64_C(1) << 51))
  {
    if (verbose_opt)
      report(1,"Range end is too high for SSE2 code path, switching to x87 FPU.");
    fini_sieve_sse2();
    new_path = 1;
  }
  else if (code_path == 1 && pmax < (UINT64_C(1) << 51))
  {
    if (verbose_opt)
      report(1,"Range end is low enough, switching back to SSE2 code path.");
    fini_sieve();
    new_path = 2;
  }

  code_path = new_path;
}
#endif

static void do_sieve(void)
{
#if MULTI_PATH
#if __x86_64__
  if (sse2_opt >= 0) /* User hasn't forced use of x87 FPU */
    x86_64_select_code_path(p_max);
#endif
  switch (code_path)
  {
    default:
    case 1:
      sieve();
      break;
#ifdef ARM_CPU
    case 2:
      sieve_neon();
      break;
#else
    case 2:
      sieve_sse2();
      break;
#endif
#ifdef __i386__
    case 3:
      sieve_cmov();
      break;
#endif
  }
#else
  sieve();
#endif
}

int main(int argc, char **argv)
{
  int want_help = 0;
  uint32_t i;
#if (BASE==5)
  uint32_t k_to_write = 0, k_to_delete = 0;
  uint32_t newpgen_opt = 0, abc_opt = 0, n0 = 0, n1 = UINT32_MAX;
#endif
#if (BASE==2 || BASE==0)
  int riesel_dat_opt = 0, sob_dat_opt = 0;
#endif
#if LEGENDRE_CACHE
  int legendre_cache_opt = 0, legendre_cache_file_opt = 0;
  const char *cache_file_name = DEFAULT_CACHE_FILE_NAME;
#endif
  int priority_opt = 0;
  const char *data_file_name = NULL, *uid = NULL;
  uint32_t L1_opt = 0, L2_opt = 0;

  set_accumulated_time(0.0);
  set_accumulated_cpu(0.0);

#if HAVE_MALLOPT
  /* All memory allocation takes place during initialization, so it doesn't
     have to be fast. Reducing this threshold allows more memory to be
     returned to the system when free() is called.
  */
  mallopt(M_MMAP_THRESHOLD,16000);
#endif

#if USE_COMMAND_LINE_FILE
  /* If no comand line arguments are given, attempt to read them from file.
   */
  if (argc == 1)
    read_argc_argv(&argc,&argv,NAME COMMAND_LINE_FILE_NAME_SUFFIX);
#endif

  while ((opt_c = getopt_long(argc,argv,short_opts,long_opts,&opt_ind)) != -1)
    switch (opt_c)
    {
#if (BASE==5)
      case 'd':
        k_to_delete = strtoul(optarg,NULL,0);
        break;
      case 'g':
        newpgen_opt = 1;
        k_to_write = strtoul(optarg,NULL,0);
        break;
      case 'a':
        abc_opt = 1;
        k_to_write = strtoul(optarg,NULL,0);
        break;
#endif
#if (BASE==2 || BASE==0)
      case 'r':
        riesel_dat_opt = 1;
        break;
      case 's':
        sob_dat_opt = 1;
        break;
#endif
      case 'i':
        data_file_name = optarg;
        break;
      case 'p':
        p_min = parse_uint(UINT64_MAX);
        break;
      case 'P':
        p_max = parse_uint(UINT64_MAX);
        break;
      case 'f':
        factors_file_name = optarg;
        factors_opt = 1;
        break;
      case 'u':
        uid = optarg;
        break;
#if LEGENDRE_CACHE
      case 'c':
        legendre_cache_opt = 1;
        break;
      case 'C':
        legendre_cache_file_opt = 1;
        cache_file_name = optarg;
        break;
#endif
#if MULTI_PATH
#ifdef ARM_CPU
      case '1':
        neon_opt = -1; /* --no-sse2 */
        break;
      case '2':
        neon_opt = 1; /* --sse2 */
        break;
#else
      case '1':
        sse2_opt = -1; /* --no-sse2 */
        break;
      case '2':
        sse2_opt = 1; /* --sse2 */
        break;
#endif
#ifdef __i386__
      case '3':
        cmov_opt = 1; /* --amd */
        break;
      case '4':
        cmov_opt = -1; /* --intel */
        break;
#endif
#endif /* MULTI_PATH */
#if HAVE_FORK
      case 't':
        num_children = strtol(optarg,NULL,0);
        if (num_children < 0 || num_children > MAX_CHILDREN)
          argerror("is out of range");
        break;
#endif
      case 'l':
        L1_opt = strtoul(optarg,NULL,0);
        break;
      case 'L':
        L2_opt = strtoul(optarg,NULL,0);
        break;
      case 'B':
        baby_method_opt = optarg;
        break;
      case 'G':
        giant_method_opt = optarg;
        break;
      case '5':
        ladder_method_opt = optarg;
        break;
#if HASHTABLE_SIZE_OPT
      case 'H':
        i = strtoul(optarg,NULL,0) * 1024;
        for (hashtable_size = 1024; hashtable_size < i; )
          hashtable_size <<= 1;
        break;
#endif
#if SUBSEQ_Q_OPT
      case 'Q':
        subseq_Q = strtoul(optarg,NULL,0);
        break;
#endif
#if GIANT_OPT
      case '6':
        scale_giant_opt = strtod(optarg,NULL);
        if (scale_giant_opt < 0.1)
          scale_giant_opt = 0.1;
        else if (scale_giant_opt > 10.0)
          scale_giant_opt = 10.0;
        break;
      case '7':
        min_giant_opt = strtoul(optarg,NULL,0);
        if (min_giant_opt < 1)
          min_giant_opt = 1;
        break;
#endif
      case 'D':
        duplicates_file_name = optarg;
        break;
#if LOG_FACTORS_OPT
      case '8':
        log_factors_opt = 1;
        break;
#endif
      case 'S':
        save_period = strtoul(optarg,NULL,0);
        break;
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
#if SOBISTRATOR_OPT
      case 'j':
        sobistrator_opt = 1;
        break;
#endif
#if SKIP_CUBIC_OPT
      case 'X':
        skip_cubic_opt = 1;
        break;
#endif
#if NO_LOOKUP_OPT
      case 'x':
        no_lookup_opt = 1;
        break;
#endif
#if DUAL
      case 'd':
        dual_opt = 1;
        break;
#endif
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

  log_file_name = per_process_file_name(LOG_FILE_BASE,LOG_FILE_SUFFIX,uid);

#if SUBSEQ_Q_OPT
  if (subseq_Q % BASE_MULTIPLE != 0)
    error("Subsequence base exponent Q=%"PRIu32" is not a multiple of %u.",
          subseq_Q, (unsigned int)BASE_MULTIPLE);
  if (subseq_Q != 0 && LIMIT_BASE % subseq_Q != 0)
    error("Subsequence base exponent Q=%"PRIu32" is not a divisor of %u.",
          subseq_Q, (unsigned int)LIMIT_BASE);
#endif

#if LEGENDRE_CACHE
  if (legendre_cache_opt && legendre_cache_file_opt)
    error("-c --cache and -C --cache-file switches are mutually exclusive.");
# if NO_LOOKUP_OPT
  if (no_lookup_opt && (legendre_cache_opt || legendre_cache_file_opt))
    error("-x --no-lookup and %s switches are mutually exclusive.",
          legendre_cache_opt? "-c --cache" : "-C --cache-file");
# endif
#endif


#if MULTI_PATH
  /* Choose code path based on switches --amd, --intel, --sse2, --no-sse2.
   */
#if __i386__
  if (sse2_opt < 0)
  {
    if (cmov_opt > 0)
      code_path = 3;
    else if (cmov_opt < 0)
      code_path = 1;
  }
  else if (sse2_opt > 0)
    code_path = 2;
#elif __x86_64__
  if (sse2_opt < 0)
    code_path = 1;
  else if (sse2_opt > 0)
    code_path = 2;
#elif ARM_CPU
  if (neon_opt < 0)
    code_path = 1;
  else if (neon_opt > 0)
    code_path = 2;
#endif
  /* If code path was not fully determined above then test hardware features.
   */
  if (code_path == 0)
  {
#if __x86_64__
    /* x86-64 always has SSE2. Choose default based on p_max instead. */
    code_path = (p_max < (UINT64_C(1) << 51)) ? 2 : 1;
#elif __i386__
    if (sse2_opt >= 0 && have_sse2()) /* Prefer SSE2 if available. */
      code_path = 2;
    else if (have_cmov() && is_amd()) /* Prefer CMOV on AMD CPUs */
      code_path = 3;
    else
      code_path = 1;
#elif ARM_CPU
    if (neon_opt >= 0 && is_neon())
      code_path = 2;
    else
      code_path = 1;
#endif
  }
  /* Report code path selection.
   */
  if (verbose_opt)
  {
#if ARM_CPU
    if(code_path == 2)
      printf("NEON code path, ");
#else
    if(code_path == 2)
      printf("SSE2 code path, ");
#endif
#if __x86_64__
    else
      printf("x87 code path, ");
#elif __i386__
    else if (code_path == 3)
      printf("AMD code path, ");
    else
      printf("Intel code path, ");
#endif
  }
#endif /* MULTI_PATH */


  set_cache_sizes(L1_opt,L2_opt);

  if (priority_opt)
    set_process_priority(priority_opt);

#if (BASE==5)
  if (newpgen_opt && abc_opt)
    error("Switches -g and -a are mutually exclusive.");
  if (k_to_write)
  {
    if (optind < argc)
      n0 = strtoul(argv[optind++],NULL,0);
    if (optind < argc)
      n1 = strtoul(argv[optind++],NULL,0);
  }
#endif

  if (optind < argc)
    error("Unhandled argument %s.", argv[optind]);

  work_pmin = p_min;

#if (BASE==2 || BASE==0)
  if (sob_dat_opt && riesel_dat_opt && data_file_name != NULL)
    error("Not all three of -i -s -r can be used together.");
  if (sob_dat_opt)
  {
    uint32_t t;
    const char *file_name = (data_file_name) ? : SOB_DAT;
    report(0,"Reading `%s' ...",file_name);
    if ((t = read_dat_file(file_name,1)) == 0)
      error("Invalid or unrecognised file `%s'.",file_name);
    terms_read += t;
  }
  if (riesel_dat_opt)
  {
    uint32_t t;
    const char *file_name = (data_file_name) ? : RIESEL_DAT;
    report(0,"Reading `%s' ...",file_name);
    if ((t = read_dat_file(file_name,-1)) == 0)
      error("Invalid or unrecognised file `%s'.",file_name);
    terms_read += t;
  }
  if (!sob_dat_opt && !riesel_dat_opt)
  {
    const char *file_name = (data_file_name) ? : DATA_FILE_NAME;
    report(0,"Reading `%s' ...",file_name);
    if ((terms_read = read_abcd_file(file_name)) == 0)
      error("Invalid or unrecognised file `%s'.",file_name);
  }
#else
  data_file_name = (data_file_name) ? : DATA_FILE_NAME;
  report(0,"Reading `%s' ...", data_file_name);
  if ((terms_read = read_abcd_file(data_file_name)) == 0)
    error("Invalid or unrecognised file `%s'.", data_file_name);
#endif

#if DUAL
  if (verbose_opt)
  {
# if (BASE==0)
    if (dual_opt)
      report(1,"Sieving dual form sequences %"PRIu32"^n+/-k.",b_term);
    else
      report(1,"Sieving standard form sequences k*%"PRIu32"^n+/-1.",b_term);
# else
    if (dual_opt)
      report(1,"Sieving dual form sequences %u^n+/-k.",(unsigned)BASE);
    else
      report(1,"Sieving standard form sequences k*%u^n+/-1.",(unsigned)BASE);
# endif
  }
#endif

#if (BASE==5)
  if (k_to_write)
  {
    for (i = 0; i < seq_count; i++)
      if (SEQ[i].k == k_to_write)
        break;
    if (i == seq_count)
      error("%"PRIu32"*%u^n+/-1 not found in %s.",
            k_to_write, (unsigned int)BASE, data_file_name);
    if (newpgen_opt)
      write_newpgen_k(i,n0,n1);
    else
      write_abc_k(i,n0,n1);
    exit(EXIT_SUCCESS);
  }

  if (k_to_delete)
  {
    overwrite_abcd(p_min,data_file_name,k_to_delete);
    exit(EXIT_SUCCESS);
  }
#endif

  finish_candidate_seqs();

#if LEGENDRE_CACHE
  if (legendre_cache_opt)
  {
    /* Force writing of a new cache file, even if it already exists.
     */
    generate_legendre_tables(NULL);
    write_legendre_cache(cache_file_name);
  }
  else
  {
    generate_legendre_tables(cache_file_name);
    if (legendre_cache_file_opt)
    {
      FILE *tmp;

      /* Write a new cache file only if one doesn't already exist.
       */
      if ((tmp = fopen(cache_file_name,"r")) == NULL)
        write_legendre_cache(cache_file_name);
      else
        fclose(tmp);
    }
  }
#else
  generate_legendre_tables(NULL);
#endif

#if SOBISTRATOR_OPT
  if (sobistrator_opt)
  {
    sob_status_file_name = (riesel_dat_opt && !sob_dat_opt) ?
      per_process_file_name(RIESEL_STATUS_FILE_BASE,SOB_STATUS_FILE_SUFFIX,uid)
      : per_process_file_name(SOB_STATUS_FILE_BASE,SOB_STATUS_FILE_SUFFIX,uid);
    sob_range_file_name =
      per_process_file_name(SOB_RANGE_FILE_BASE,SOB_RANGE_FILE_SUFFIX,uid);
    if (!factors_opt)
      factors_file_name = per_process_file_name("fact","txt",uid);
    if (duplicates_file_name == NULL)
      duplicates_file_name = per_process_file_name("factexcl","txt",uid);
  }
#endif

  checkpoint_file_name =
    per_process_file_name(CHECKPOINT_FILE_BASE,CHECKPOINT_FILE_SUFFIX,uid);

#if SOBISTRATOR_OPT
  /************************************************************************
   *
   * proth_sieve/JJsieve compatibility mode, for use with Sobistrator.
   * First process range given on command line or in SoBStatus.dat, then
   * process any ranges in nextrange.txt.
   *
   ************************************************************************/
  if (sobistrator_opt)
  {
    if (work_pmin || p_max) /* --pmin or --pmax given on the command line */
    {
      uint64_t p0,p1;

      work_pmin = p_min; /* in case it was set from the input sieve file. */
      check_p_range(work_pmin,p_max,NULL);

      if (sob_continue_range(&p0,&p1))
      {
        if (p0 != work_pmin || p1 != p_max)
        {
          warning("Discarding existing range in `%s'.",sob_status_file_name);
          sob_start_range(work_pmin,p_max);
        }
      }
      else
      {
        sob_start_range(work_pmin,p_max);
      }
    }
    else
    {
      if (!sob_continue_range(&work_pmin,&p_max))
        if (!sob_next_range(&work_pmin,&p_max))
          goto sob_done;
      check_p_range(work_pmin,p_max,sob_status_file_name);
    }

    do
    {
      check_p_range(work_pmin,p_max,sob_range_file_name);
      factor_count = 0;
      read_checkpoint(work_pmin,p_max); /* For factor_count, etc. */
      p_min = sob_read_checkpoint(work_pmin,p_max);
      if (p_min < p_max)
      {
        notify_event(sieve_parameters_changed);
        do_sieve();
      }
    } while (sob_next_range(&work_pmin,&p_max));

  sob_done:
    report(1,"No more work in `%s'.",sob_range_file_name);
  }
  else
#endif
  /************************************************************************
   *
   * Original sr5sieve mode, process one range given by -p and -P switches.
   *
   ************************************************************************/
  if (work_pmin || p_max) /* --pmin or --pmax given on the command line */
  {
    work_pmin = p_min; /* in case it was set from the input sieve file. */
    check_p_range(p_min,p_max,NULL);
    if (!factors_opt)
      factors_file_name =
        per_process_file_name(FACTORS_FILE_BASE,FACTORS_FILE_SUFFIX,uid);
    p_min = read_checkpoint(p_min,p_max);
    do_sieve();
  }
  else
  /************************************************************************
   *
   * Original sr5sieve mode, process multiple ranges from the work file.
   *
   ************************************************************************/
  {
    work_file_name =
      per_process_file_name(WORK_FILE_BASE,WORK_FILE_SUFFIX,uid);
    while (read_work_file(uid))
    {
      work_pmin = p_min;
      factor_count = 0;
      p_min = read_checkpoint(p_min,p_max);
      notify_event(sieve_parameters_changed);
      do_sieve();
#ifdef FACTORS_EMAIL
      if (factor_count > 0)
        report(1,"Please send `%s' to %s.", factors_file_name, FACTORS_EMAIL);
#endif
      update_work_file();
    }
  }

  /************************************************************************
   *
   * Processing of all ranges complete.
   *
   ************************************************************************/
#if MULTI_PATH
  switch (code_path)
  {
    default:
    case 1:
      fini_sieve();
      break;
#ifdef ARM_CPU
    case 2:
      fini_sieve_neon();
      break;
#else
    case 2:
      fini_sieve_sse2();
      break;
#endif
#ifdef __i386__
    case 3:
      fini_sieve_cmov();
      break;
#endif
  }
#else
  fini_sieve();
#endif

  return EXIT_SUCCESS;
}

#define RANGE_BLOCK 1000000000
static int read_work_file(const char *uid)
{
  uint64_t p0, p1;
  char buf[100];
  FILE *work_file;
  int ret;

  report(0,"Reading project work file `%s' ...",work_file_name);
  work_file = xfopen(work_file_name,"r",error);

  ret = fscanf(work_file, "%"SCNu64"%*[-,\t ]%"SCNu64,&p0,&p1);
  xfclose(work_file,work_file_name);
  switch (ret)
  {
    case EOF:
      report(1,"No more work in `%s'.",work_file_name);
      return 0;

    case 2:
      if (p0 >= p1)
        error("Bad range %"PRIu64",%"PRIu64" in `%s'.",p0,p1,work_file_name);
      if (p1 > ((uint64_t)(((uint64_t)1 << MOD64_MAX_BITS)-1))/RANGE_BLOCK)
        error("Range end %"PRIu64" too high in `%s'.",p1,work_file_name);

      if (!factors_opt)
      {
        if (factors_file_name != NULL)
          free(factors_file_name);
        sprintf(buf,"%s%"PRIu64,FACTORS_FILE_BASE,p0);
        factors_file_name = per_process_file_name(buf,FACTORS_FILE_SUFFIX,uid);
      }
      p_min = p0*RANGE_BLOCK;
      p_max = p1*RANGE_BLOCK;
      if (p_min <= k_max)
      {
        p_min = k_max+1;
        warning("Range start %"PRIu64" too low in `%s'.",p0,work_file_name);
        warning("Starting sieve at p=%"PRIu64".",p_min);
      }
      if (p_max <= p_min)
        error("Range end %"PRIu64" too low in `%s'.",p1,work_file_name);
      return 1;

    case 0:
    case 1:
      error("Malformed first line in `%s'.",work_file_name);

    default:
      error("Error while reading `%s'.",work_file_name);
  }
}

#define MAX_WORK_LINES 1000
#define MAX_LINE_LENGTH 80
static void update_work_file(void)
{
  FILE *work_file;
  char *lines[MAX_WORK_LINES];
  int i, j;

  work_file = xfopen(work_file_name,"r",error);

  for (i = 0; i < MAX_WORK_LINES; i++)
  {
    lines[i] = xmalloc(MAX_LINE_LENGTH);
    if (fgets(lines[i],MAX_LINE_LENGTH,work_file) == NULL)
    {
      free(lines[i]);
      break;
    }
  }

  xfclose(work_file,work_file_name);
  work_file = xfopen(work_file_name,"w",error);

  for (j = 1; j < i; j++)
    fputs(lines[j],work_file);
  xfclose(work_file,work_file_name);

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
void print_status(uint64_t p, uint32_t p_per_sec)
{
  static int toggle = 0;
  char buf[32];

  toggle = !toggle; /* toggle between reporting ETA and factor rate. */

  if (toggle && factor_count && p_per_sec)
  {
    uint64_t p_per_factor = (p-work_pmin)/factor_count;
    snprintf(buf,31,"%"PRIu64" sec/factor",p_per_factor/p_per_sec);
  }
  else
  {
    double done = (double)(p-p_min)/(p_max-p_min);
    buf[0] = '\0';
    if (done > 0.0) /* Avoid division by zero */
    {
      time_t finish_date = start_date+(time(NULL)-start_date)/done;
      struct tm *finish_tm = localtime(&finish_date);
      if (!finish_tm || !strftime(buf,sizeof(buf),STRFTIME_FORMAT,finish_tm))
        buf[0] = '\0';
    }
  }
  buf[31] = '\0';

  report(0,"p=%"PRIu64", %"PRIu32" p/sec, %"PRIu32" factor%s, %.1f%% done, %s",
         p,p_per_sec,factor_count,plural(factor_count),100.0*frac_done(p),buf);
}

static double expected_factors(uint32_t n, uint64_t p0, uint64_t p1)
{
  /* TODO: Use a more accurate formula. This one is only reasonable when
     p0/p1 is close to 1. */

  return n*(1-log(p0)/log(p1));
}

void start_srsieve(void)
{
  report(1,"Expecting to find factors for about %.2f terms in this range.",
           expected_factors(terms_read,work_pmin,p_max));

 logger(1,"%s started: %"PRIu32" <= n <= %"PRIu32", %"PRIu64" <= p <= %"PRIu64,
         NAME " " XSTR(MAJOR_VER) "." XSTR(MINOR_VER) "." XSTR(PATCH_VER),
         n_min, n_max, p_min, p_max);

  start_date = time(NULL);
}

void finish_srsieve(const char *reason, uint64_t p)
{
  logger(1,"%s stopped: at p=%"PRIu64" because %s.",
         NAME " " XSTR(MAJOR_VER) "." XSTR(MINOR_VER) "." XSTR(PATCH_VER),
         p, reason);
  logger(1,"Found factors for %"PRIu32" term%s in %.3f sec."
         " (expected about %.2f)",factor_count,plural(factor_count),
         get_accumulated_time(),expected_factors(terms_read,work_pmin,p));

  if (p >= p_max) /* Sieving job completed */
  {
#if SOBISTRATOR_OPT
    if (sobistrator_opt)
      sob_finish_range();
#endif
#if REMOVE_CHECKPOINT
    remove_checkpoint();
#endif
  }
  else
  {
#if SOBISTRATOR_OPT
    if (sobistrator_opt)
      sob_write_checkpoint(p);
#endif
    write_checkpoint(p);
  }

  set_accumulated_time(0.0);
  set_accumulated_cpu(0.0);
}
