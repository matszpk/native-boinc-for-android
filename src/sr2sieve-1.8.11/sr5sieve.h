/* sr5sieve.h -- (C) Geoffrey Reynolds, April 2006.

   Srsieve specialised for the Sierpinski/Riesel Base 5 projects.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _SR5SIEVE_H
#define _SR5SIEVE_H

#include <limits.h>
#include <stdint.h>
#include "config.h"

#define XSTR(ARG) STR(ARG)
#define STR(ARG) #ARG

/* Also check config.h for compiler/OS settings.
 */

/* For sieving sequences k*b^n+c: set BASE to zero to allow b to be
   determined at runtime from the input file; set BASE to a constant integer
   to create a sieve that only sieves sequences with b=BASE. (BASE 5 or 2
   will set options suitable for the S/R Base 5 or SoB/RieselSieve projects).
*/
#ifndef BASE
#define BASE 0
#endif

/* Set DUAL=1 to allow sequences b^n+/-k as well as k*b^n+/-1.
 */
#if (BASE==5)
#define DUAL 0
#endif

#ifndef DUAL
#define DUAL 1
#endif

/* Set DEFAULT_L1_CACHE_SIZE,DEFAULT_L2_CACHE_SIZE to the L1,L2 data cache
   size in Kb to be used if not supplied on the command line or detected
   automatically.
*/
#ifndef DEFAULT_L1_CACHE_SIZE
#define DEFAULT_L1_CACHE_SIZE 16
#endif
#ifndef DEFAULT_L2_CACHE_SIZE
#define DEFAULT_L2_CACHE_SIZE 256
#endif

/* Set BASE_MULTIPLE to the least exponent Q for which sieving in
   subsequence base b^Q will be allowed. Must be a multiple of 2.
*/
#define BASE_MULTIPLE 2

/* Allow sieving in base b^Q for Q chosen from the divisors of LIMIT_BASE.
   Must be a multiple of BASE_MULTIPLE.
*/
#define LIMIT_BASE 720

/* For a prime p that satisfies p=1 (mod r), an "r-th power residue test"
   checks whether a subsequence of k*b^n+c can possibly contain any terms of
   the form x^r (mod p). If there are none then that subsequence can be
   omitted from the BSGS step.

   To conduct r-th power residue tests for each r in a set R of prime
   powers, set POWER_RESIDUE_LCM to lcm(R), and set POWER_RESIDUE_DIVISORS
   to the number of divisors of POWER_RESIDUE_LCM. POWER_RESIDUE_LCM must be
   a multiple of BASE_MULTIPLE, a divisor of LIMIT_BASE, and must be less
   than 2^15. E.g.

   R={2,3,4,5}: POWER_RESIDUE_LCM=60, POWER_RESIDUE_DIVISORS=12
   R={2,3,4,5,8}: POWER_RESIDUE_LCM=120, POWER_RESIDUE_DIVISORS=16
   R={2,3,4,5,8,9}: POWER_RESIDUE_LCM=360, POWER_RESIDUE_DIVISORS=24
   R={2,3,4,5,8,9,16}: POWER_RESIDUE_LCM=720, POWER_RESIDUE_DIVISORS=30
   R={2,3,4,5,7,8,9,16}: POWER_RESIDUE_LCM=5040, POWER_RESIDUE_DIVISORS=60

   Memory/sequence is proportional to POWER_RESIDUE_LCM*POWER_RESIDUE_DIVISORS
*/
#define POWER_RESIDUE_LCM 720
#define POWER_RESIDUE_DIVISORS 30

/* For a hash table expected to hold up to M elements, use a main table of
   at least M/HASH_MAX_DENSITY and at most M/HASH_MIN_DENSITY elements. The
   size will be increased further to minimise density within this range, if
   L1_cache_size is high enough.
*/
#define HASH_MAX_DENSITY 0.60
#define HASH_MIN_DENSITY 0.10

/* Allow sequences k*b^n+c that have both odd and even n remaining (Such as
   151026*5^n-1 in the base 5 case).
*/
#define ALLOW_MIXED_PARITY_SEQUENCES 1

/* Set USE_SETUP_HASHTABLE=1 to use a hashtable to store power residue
   partial products in setup64(). Otherwise a linear array is used. The
   hashtable may be faster when POWER_RESIDUE_LCM is large or when there
   are many sequences in the sieve.
*/
#if (BASE==5)
#define USE_SETUP_HASHTABLE 1
#else
#define USE_SETUP_HASHTABLE 0
#endif

/* Set LEGENDRE_CACHE=1 to read Legendre lookup tables from a cache file if
   one is found, and to enable the -c --cache and -C --cache-file command
   line switches.
*/
#define LEGENDRE_CACHE 1

/* Set CHECK_FOR_GFN=1 to check whether a sequence consists of Generalised
   Fermat numbers when generating the Legendre symbol tables. A better
   table can be used for these sequences.
*/
#if (BASE==5)
#define CHECK_FOR_GFN 0
#else
#define CHECK_FOR_GFN 1
#endif

/* Set REMOVE_CHECKPOINT=1 to remove the checkpoint file when the current
   range is complete.
*/
#define REMOVE_CHECKPOINT 1

/* The name that the executable refers to itself by is sr2sieve if BASE==2
   or if BASE==0. Otherwise it refers to itself as sr<BASE>sieve. Filenames
   follow this pattern: sr2work.txt, sr2data.txt etc.
 */
#if (BASE==0)
#define BASE_NAME "sr2"
#else
#define BASE_NAME "sr" XSTR(BASE)
#endif

/* Checkpoint file name template.
 */
#define CHECKPOINT_FILE_BASE "checkpoint"
#define CHECKPOINT_FILE_SUFFIX "txt"

/* Factors file name template.
 */
#define FACTORS_FILE_BASE "factors"
#define FACTORS_FILE_SUFFIX "txt"

/* Log file name template.
 */
#define LOG_FILE_BASE BASE_NAME "sieve"
#define LOG_FILE_SUFFIX "log"

/* Work file name template.
 */
#define WORK_FILE_BASE BASE_NAME "work"
#define WORK_FILE_SUFFIX "txt"

/* Date format used for log file entries (see date --help)
 */
#define LOG_STRFTIME_FORMAT "%c "

/* Report period in seconds.
 */
#define REPORT_PERIOD 60

/* Default save period in seconds, if not supplied by the -S --save switch.
 */
#define DEFAULT_SAVE_PERIOD 300

/* Set CHECK_FACTORS=1 to double check found factors.
 */
#define CHECK_FACTORS 1

/* Set HASHTABLE_SIZE_OPT=1 to provide the -H command-line switch, which
   forces use of a specified size hashtable.
*/
#define HASHTABLE_SIZE_OPT 1

/* Set SUBSEQ_Q_OPT=1 to provide the -Q command-line switch, which
   forces use of a specific subsequence base b^Q.
*/
#define SUBSEQ_Q_OPT 1

/* When USE_COMMAND_LINE_FILE=1 and no command line arguments are given,
   read the command line from a file called NAME-command-line.txt, where
   NAME is the name under which the program was invoked. This can be useful
   where the use of the shell and batch files has been disabled.
*/
#define USE_COMMAND_LINE_FILE 1
#define COMMAND_LINE_FILE_NAME_SUFFIX "-command-line.txt"

/* Set UPDATE_BITMAPS=1 to remove terms as factors are found. This is
   different to the way proth_sieve does it, so for compatibility with
   proth_sieve set UPDATE_BITMAPS=0.
*/
#define UPDATE_BITMAPS 1

/* Set SOBISTRATOR_OPT=1 to enable the `-j --sobistrator' switch.
 */
#if (BASE==2 || BASE==0)
#define SOBISTRATOR_OPT 1
#else
#define SOBISTRATOR_OPT 0
#endif

#if SOBISTRATOR_OPT
#define SOB_STATUS_FILE_BASE "SoBStatus"
#define RIESEL_STATUS_FILE_BASE "RieselStatus"
#define SOB_STATUS_FILE_SUFFIX "dat"
#define SOB_RANGE_FILE_BASE "nextrange"
#define SOB_RANGE_FILE_SUFFIX "txt"
#endif

/* Set SKIP_CUBIC_OPT=1 to enable the `-X --skip-cubic' switch, which causes
   cubic and higher power residue tests to be skipped. This might result in
   better peformance when there are a very large number of sequences in the
   sieve but only a short range of n.
*/
#if (BASE==0)
#define SKIP_CUBIC_OPT 1
#endif

/* Set NO_LOOKUP_OPT=1 to provide the -x --no-lookup switch, which causes
   Legendre symbols to be computed as needed instead of using precomputed
   lookup tables.
*/
#if (BASE==0)
#define NO_LOOKUP_OPT 1
#endif

/* Set GIANT_OPT=1 to provide the --min-giant and --scale-giant switches.
 */
#define GIANT_OPT 1

#if GIANT_OPT
#define MIN_GIANT_DEFAULT 1
#define SCALE_GIANT_DEFAULT 1.1
#endif

/* Set LOG_FACTORS_OPT=1 to provie the --log-factors switch.
 */
#define LOG_FACTORS_OPT 1


/* Nothing below here should normally need adjustment. */

#ifdef __GNUC__
/* macros that evaluate their arguments only once. From the GCC manual.
 */
#define MIN(a,b) \
   ({ typeof (a) _a = (a); \
      typeof (b) _b = (b); \
      _a < _b ? _a : _b; })
#define MAX(a,b) \
   ({ typeof (a) _a = (a); \
      typeof (b) _b = (b); \
      _a > _b ? _a : _b; })
#define ABS(a) \
   ({ typeof (a) _a = (a); \
      _a < 0 ? -_a : _a; })
#else
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#if (MULTI_PATH && CODE_PATH==2)
#ifdef ARM_CPU
# define CODE_PATH_NAME(name) name##_neon
#else
# define CODE_PATH_NAME(name) name##_sse2
#endif
#elif (MULTI_PATH && CODE_PATH==3)
# define CODE_PATH_NAME(name) name##_cmov
#else
# define CODE_PATH_NAME(name) name
#endif


/* bsgs.c */

void sieve(void);
void fini_sieve(void);

#if MULTI_PATH
void sieve_neon(void);
void fini_sieve_neon(void);
void sieve_sse2(void);
void fini_sieve_sse2(void);
void sieve_cmov(void);
void fini_sieve_cmov(void);
#endif


/* choose.c */

typedef struct {uint32_t m; uint32_t M;} steps_t;

extern steps_t *steps;

uint32_t make_table_of_steps(void);
void free_table_of_steps(void);
void trim_steps(int vec_len, uint32_t max_baby_steps);
uint32_t find_best_Q(uint32_t *subseqs);


/* clock.c */

double get_accumulated_cpu(void);
void set_accumulated_cpu(double seconds);
uint32_t millisec_elapsed_time(void);
double get_accumulated_time(void);
void set_accumulated_time(double seconds);
uint64_t timestamp(void);


/* threads.c */

#if HAVE_FORK
#define MAX_CHILDREN 8
extern int num_children;
extern int child_num;
extern int affinity_opt;
extern int child_affinity[MAX_CHILDREN];

uint64_t get_lowtide(void);
void child_eliminate_term(uint32_t subseq, uint32_t m, uint64_t p);
void parent_thread(uint64_t p);
void init_threads(uint64_t pmin, void (*fun)(uint64_t));
int fini_threads(uint64_t pmax);
#endif


/* cpu.c */

void set_cache_sizes(uint32_t L1_opt, uint32_t L2_opt);
#if defined(__i386__) || defined(__x86_64__)
int have_sse2(void);
int have_cmov(void);
int is_amd(void);
#endif
#ifdef ARM_CPU
int is_neon(void);
#endif


/* events.c */

typedef enum
{
  initialise_events,
  received_sigterm,
  received_sigint,
  received_sighup,
#if HAVE_FORK
  received_sigpipe,
  received_sigchld,
#endif
  report_due,
  save_due,

  sieve_parameters_changed,
  factor_found,

  /* add more events here */

  last_event
} event_t;

static inline void check_events(uint64_t current_prime)
{
  extern volatile int event_happened;
  extern void process_events(uint64_t);

  if (event_happened)
  {
#if HAVE_FORK
    /* When multithreading, the current prime is just the one most recently
       dispatched to the children threads, there is no guarantee that it has
       actually been tested yet. get_lowtide() is a lower bound on all
       untested primes.
    */
    if (num_children > 0)
      current_prime = get_lowtide();
#endif
    process_events(current_prime);
  }
}
void handle_signal(int signum);
void notify_event(event_t event);
void check_progress(void);


/* factors.c */

void save_factor(const char *file_name, uint32_t k, int32_t c, uint32_t n,
                 uint64_t p);
#if CHECK_FACTORS
int is_factor(uint32_t k, int32_t c, uint32_t n, uint64_t p);
#endif


/* files.c */

#ifdef EOF
void line_error(const char *msg, const char *fmt, const char *filename) attribute((noreturn));
const char *read_line(FILE *file);
FILE *xfopen(const char *fn, const char *mode, void (*fun)(const char *,...));
void xfclose(FILE *f, const char *fn);
#endif
uint32_t read_abcd_file(const char *file_name);
#if (BASE==5)
void overwrite_abcd(uint64_t p, const char *file_name, uint32_t del_k);
void write_newpgen_k(uint32_t seq, uint32_t n0, uint32_t n1);
void write_abc_k(uint32_t seq, uint32_t n0, uint32_t n1);
#endif
uint64_t read_checkpoint(uint64_t pmin, uint64_t pmax);
void write_checkpoint(uint64_t p);
void remove_checkpoint(void);

#if (BASE == 2 || BASE == 0)
uint32_t read_dat_file(const char *file_name, int32_t c);
#endif
#if USE_COMMAND_LINE_FILE
void read_argc_argv(int *argc, char ***argv, const char *file_name);
#endif


/* legendre.c */

#if NO_LOOKUP_OPT
extern int no_lookup_opt;
int legendre32_64(int32_t a, uint64_t p) attribute ((const));
#endif

#if LEGENDRE_CACHE
extern int legendre_cache_opt;

void write_legendre_cache(const char *file_name);
#endif

void generate_legendre_tables(const char *file_name);


/* primes.c */

void init_prime_sieve(uint64_t pmax);
void prime_sieve(uint64_t low_prime, uint64_t high_prime,
                 void (*bsgs_fun)(uint64_t));
void fini_prime_sieve(void);


/* priority.c */

void set_process_priority(int level);
void set_cpu_affinity(int cpu_number);


/* sequences.c */

typedef struct
{
  uint32_t k;       /* k in k*b^n+c */
  int32_t c;        /* c in k*b^n+c */
  union
  {
    uint32_t *N;        /* list of remaining n */
    uint_fast32_t *map; /* bitmap of positive results for (-ckb^n/p) */
#if NO_LOOKUP_OPT
    int32_t kc_core;
#endif
  };
  union
  {
    uint32_t ncount;  /* number of remaining n */
    uint32_t mod;     /* (p/2)%mod gives index into map[] for (-ckb^n/p) */
#if NO_LOOKUP_OPT
    uint32_t parity;
#endif
  };
  union
  {
    uint32_t nsize;   /* N has room for nsize n */
    uint32_t first;   /* first subsequence */
  };
  uint32_t last;      /* last subsequence */
} seq_t;

typedef struct
{
  const uint32_t **sc_lists[POWER_RESIDUE_DIVISORS];
} scl_t;

#define SUBSEQ_MAX UINT32_MAX

extern seq_t      *SEQ;
extern scl_t      *SCL;
extern uint32_t    seq_count;

extern uint64_t    p_min;
extern uint64_t    p_max;
extern uint32_t    n_min;
extern uint32_t    n_max;
extern uint32_t    k_max;

extern int16_t div_shift[POWER_RESIDUE_LCM/2];
extern uint8_t divisor_index[POWER_RESIDUE_LCM+1];

#if (BASE == 0)
extern uint32_t b_term;
#endif

const char *kbnc_str(uint32_t k, uint32_t n, int32_t c);
const char *seq_str(uint32_t seq);
uint32_t new_seq(uint32_t k, int32_t c);
void add_seq_n(uint32_t seq, uint32_t n);
void finish_candidate_seqs(void);
uint32_t *make_setup_ladder(double max_density);


/* sobistrator.c */

#if SOBISTRATOR_OPT
extern int sobistrator_opt;
extern const char *sob_status_file_name;
extern const char *sob_range_file_name;

void sob_start_range(uint64_t pmin, uint64_t pmax);
int sob_continue_range(uint64_t *pmin, uint64_t *pmax);
void sob_finish_range(void);
void sob_write_checkpoint(uint64_t p);
uint64_t sob_read_checkpoint(uint64_t pmin, uint64_t pmax);
int sob_next_range(uint64_t *pmin, uint64_t *pmax);
#endif


/* sr5sieve.c */

extern char *checkpoint_file_name;
extern char *factors_file_name;
extern char *log_file_name;
extern int verbose_opt;
extern int quiet_opt;
extern uint32_t save_period;
extern uint32_t L1_cache_size;
extern uint32_t L2_cache_size;
extern const char *baby_method_opt;
extern const char *giant_method_opt;
extern const char *ladder_method_opt;
extern const char *duplicates_file_name;
#if HASHTABLE_SIZE_OPT
extern uint32_t hashtable_size;
#endif
#if SKIP_CUBIC_OPT
extern int skip_cubic_opt;
#endif
#if DUAL
extern int dual_opt;
#endif
#if GIANT_OPT
extern uint32_t min_giant_opt;
extern double scale_giant_opt;
#endif
#if LOG_FACTORS_OPT
extern int log_factors_opt;
#endif

double frac_done(uint64_t p);
void print_status(uint64_t p, uint32_t p_per_sec);
void start_srsieve(void);
void finish_srsieve(const char *reason, uint64_t p);


/* subseq.c */

typedef struct
{
  uint_fast32_t *M;     /* Bitmap of terms, bit i corresponds to n=iQ+d */
  uint32_t seq;         /* This is a subsequence of SEQ[seq] */
#ifndef NDEBUG
  uint32_t mcount;      /* Number of remaining terms (popcount of M) */
#endif
#if CHECK_FOR_GFN
  uint32_t a, b;
#endif
} subseq_t;

extern subseq_t *SUBSEQ;
#if LIMIT_BASE < UINT16_MAX
extern uint16_t *subseq_d; /* Each term k*b^n+c satisfies n=d (mod Q) */
#else
extern uint32_t *subseq_d; /* Each term k*b^n+c satisfies n=d (mod Q) */
#endif
extern uint32_t subseq_count;
extern uint32_t subseq_Q;
extern uint32_t factor_count;
extern int benchmarking;

void make_subseqs(void);
uint32_t seq_parity(uint32_t seq);
void eliminate_term(uint32_t subseq, uint32_t m, uint64_t p);


/* util.c */

void error(const char *fmt, ...) attribute ((noreturn,format(printf,1,2)));
void warning(const char *fmt, ...) attribute ((format(printf,1,2)));
void logger(int print, const char *fmt, ...) attribute ((format(printf,2,3)));
void report(int scroll, const char *fmt, ...) attribute ((format(printf,2,3)));
void *xmalloc(uint32_t sz) attribute ((malloc));
void *xrealloc(void *d, uint32_t sz);
void *xmemalign(uint32_t align, uint32_t size) attribute ((malloc));
void xfreealign(void *mem);
uint32_t gcd32(uint32_t a, uint32_t b) attribute ((const));
static inline const char *plural(int n)
{
  return (n == 1) ? "" : "s";
}

#endif /* _SR5SIEVE_H */
