/* gcwsieve.h -- (C) Geoffrey Reynolds, March 2007.

   A sieve for Generalised Cullen and Woodall numbers.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _GCWSIEVE_H
#define _GCWSIEVE_H

#include <limits.h>
#include <stdint.h>
#include "config.h"

#define XSTR(ARG) STR(ARG)
#define STR(ARG) #ARG

/* Also check config.h for compiler settings.
 */

/* Set DEFAULT_L1_CACHE_SIZE,DEFAULT_L2_CACHE_SIZE to the L1,L2 data cache
   size, in Kb. (If not set in the Makefile).
*/
#ifndef DEFAULT_L1_CACHE_SIZE
#define DEFAULT_L1_CACHE_SIZE 16
#endif
#ifndef DEFAULT_L2_CACHE_SIZE
#define DEFAULT_L2_CACHE_SIZE 128
#endif

/* File to read input sieve from if not specified by `-i --input FILE'.
 */
#define DEFAULT_INPUT_FILE_NAME "sieve.txt"

/* File to write factors to if not specified by `-f --factors FILE'.
 */
#define DEFAULT_FACTORS_FILE_BASE "factors"
#define DEFAULT_FACTORS_FILE_EXT "txt"

/* File to write checkpoint to if not specified by `-c --checkpoint FILE'.
 */
#define DEFAULT_CHECKPOINT_FILE_BASE "checkpoint"
#define DEFAULT_CHECKPOINT_FILE_EXT "txt"

/* File to read ranges from if not specified by `-w --work FILE'.
 */
#define DEFAULT_WORK_FILE_BASE "work"
#define DEFAULT_WORK_FILE_EXT "txt"

/* File to write log reports and report factors to.
 */
#define LOG_FILE_BASE "gcwsieve"
#define LOG_FILE_EXT "log"

/* Date format used for log file entries (see date --help)
 */
#define LOG_STRFTIME_FORMAT "%c "

/* Report period in seconds.
 */
#define REPORT_PERIOD 60

/* Save period in seconds.
 */
#if BOINC
#define DEFAULT_SAVE_PERIOD 5
#else
#define DEFAULT_SAVE_PERIOD 3600
#endif

/* Set CHECK_FACTORS=1 to double check found factors.
 */
#define CHECK_FACTORS 1

/* When USE_COMMAND_LINE_FILE=1 and no command line arguments are given,
   read the command line from a file called NAME-command-line.txt, where
   NAME is the name under which the program was invoked. This can be useful
   where the use of the shell and batch files has been disabled.
*/
#define USE_COMMAND_LINE_FILE 1
#define COMMAND_LINE_FILE_NAME_SUFFIX "-command-line.txt"

/* Set MULTISIEVE_OPT to allow the -m switch, which causes the -f and -o
   switches to write factors and ABC files in the same format as MultiSieve.
*/
#define MULTISIEVE_OPT 1

/* Set KNOWN_FACTORS_OPT to allow the -k switch, which removes known factors
   from the sieve before starting.
*/
#define KNOWN_FACTORS_OPT 1

/* Set REPORT_PRIMES_OPT=1 to allow the `-R --report-primes' switch, which
   reports primes/sec (primes tested per second) instead of p/sec (increase
   in p per second).
*/
#define REPORT_PRIMES_OPT 1

/* Set REMOVE_CHECKPOINT=1 to remove the checkpoint file when the current
   range is complete.
*/
#define REMOVE_CHECKPOINT 1


/* Nothing below here should normally need adjustment. */

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#if defined(__i386__) && MULTI_PATH && CODE_PATH==2
# define CODE_PATH_NAME(name) name##_sse2
#elif defined(__x86_64__) && MULTI_PATH && CODE_PATH==2
# define CODE_PATH_NAME(name) name##_amd
#else
# define CODE_PATH_NAME(name) name
#endif


/* sieve.c */

void sieve(void);

#if defined(__i386__) && MULTI_PATH
void sieve_sse2(void);
#elif defined(__x86_64__) && MULTI_PATH
void sieve_amd(void);
#endif


/* begin.c */

void begin_sieve(void);


/* cpu.c */

void set_cache_sizes(uint32_t L1_opt, uint32_t L2_opt);
#if defined(__i386__) && MULTI_PATH
int have_sse2(void);
#elif defined(__x86_64__) && MULTI_PATH
int is_amd(void);
#endif
#if PREFETCH_OPT
int have_prefetch(void);
#endif


/* clock.c */

double get_accumulated_cpu(void);
void set_accumulated_cpu(double seconds);
uint32_t millisec_elapsed_time(void);
double get_accumulated_time(void);
void set_accumulated_time(double seconds);
uint64_t timestamp(void);


/* fork.c */
#if HAVE_FORK
#define MAX_CHILDREN 8
extern int num_children;
extern int child_num;
extern int affinity_opt;
extern int child_affinity[MAX_CHILDREN];

uint64_t get_lowtide(void);
void child_eliminate_term(uint32_t i, int cw, uint64_t p);
void parent_thread(uint64_t p);
void init_threads(uint64_t pmin, void (*fun)(uint64_t));
int fini_threads(uint64_t pmax);
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
void notify_event(event_t event);
void check_progress(void);
void handle_signal(int signum);


/* factors.c */

extern uint32_t factor_count;

void report_factor(uint32_t n, int cw, uint64_t p);
#if KNOWN_FACTORS_OPT
void remove_known_factors(const char *file_name);
#endif


/* files.c */

#ifdef EOF
FILE *xfopen(const char *fn, const char *mode, void (*fun)(const char *,...));
void xfclose(FILE *f, const char *fn);
const char *read_line(FILE *file);
#endif
void read_abc_file(const char *file_name);
void write_abc_file(int scroll, uint64_t p, const char *file_name);
#if USE_COMMAND_LINE_FILE
void read_argc_argv(int *argc, char ***argv, const char *file_name);
#endif
uint64_t read_checkpoint(void);
void write_checkpoint(uint64_t p);
void remove_checkpoint(void);

/* invmod.c */
#ifdef ARM_CPU
extern uint64_t invmod(uint64_t a, uint64_t b);
#endif

/* primes.c */

#if REPORT_PRIMES_OPT
extern uint32_t primes_count;
#endif

void init_prime_sieve(uint64_t pmax);
void prime_sieve(uint64_t pmin, uint64_t pmax, void (*fun)(uint64_t));
void fini_prime_sieve(void);


/* priority.c */

void set_process_priority(int level);
void set_cpu_affinity(int cpu_number);


/* gcwsieve.c */

extern const char *factors_file_name;
extern const char *output_file_name;
extern const char *checkpoint_file_name;
extern const char *log_file_name;
extern uint32_t save_period;
extern int quiet_opt;
extern int verbose_opt;
#if PREFETCH_OPT
extern int prefetch_opt;
#endif
#if MULTISIEVE_OPT
extern int multisieve_opt;
#endif
#if REPORT_PRIMES_OPT
extern int report_primes_opt;
#endif
extern uint32_t L1_cache_size;
extern uint32_t L2_cache_size;
#ifdef SMALL_P
extern int smallp_phase;
#endif

extern uint32_t base_opt;
extern int cw_opt[2];

double frac_done(uint64_t p);
void print_status(uint64_t p, uint32_t p_per_sec
#if REPORT_PRIMES_OPT
                  ,uint32_t primes_per_sec
#endif
                  ); /* print_status() */
void start_gcwsieve(void);
void finish_gcwsieve(const char *reason, uint64_t p);


/* terms.c */

extern uint32_t b_term;

extern uint32_t *N[2];      /* list of remaining n */
extern uint32_t ncount[2];  /* number of remaining n */
extern uint32_t tcount[2];  /* total number of terms */
extern uint64_t p_min;
extern uint64_t p_max;
extern uint32_t n_min;
extern uint32_t n_max;
extern uint32_t gmul;
extern uint32_t *G[2];
extern uint32_t gmax;
extern uint32_t dummy_opt;
extern int benchmarking;   /* Don't report factors if set. */

const char *term_str(uint32_t n, int cw);
void add_term(uint32_t n, int32_t c);
void finish_candidate_terms(void);
void eliminate_term(uint32_t n, int cw, uint64_t p);
uint32_t for_each_term(void (*fun)(uint32_t,int32_t c,void *), void *arg);
int is_equal(uint32_t n, int cw, uint64_t p);


/* util.c */

void error(const char *fmt, ...) attribute ((noreturn,format(printf,1,2)));
void warning(const char *fmt, ...) attribute ((format(printf,1,2)));
void logger(int print, const char *fmt, ...) attribute ((format(printf,2,3)));
void *xmalloc(uint32_t sz) attribute ((malloc));
void *xrealloc(void *d, uint32_t sz);
void *xmemalign(uint32_t align, uint32_t size) attribute ((malloc));
void xfreealign(void *mem);
void report(int scroll, const char *fmt, ...) attribute ((format(printf,2,3)));
uint32_t gcd32(uint32_t a, uint32_t b) attribute ((const));
static inline const char *plural(int n)
{
  return (n == 1) ? "" : "s";
}

#endif /* _GCWSIEVE_H */
