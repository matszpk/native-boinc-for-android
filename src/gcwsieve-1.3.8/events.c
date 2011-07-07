/* events.c -- (C) Geoffrey Reynolds, May 2006.

   Interrupts, alarms, and other events that cannot be handled part way
   through a sieve iteration.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "gcwsieve.h"

#if BOINC
#include "boinc_api.h"
#endif

/* Start with an initial event: initialisation.
 */
volatile int event_happened = 1;

/* Setting an entry in events[] has to be atomic, so we cannot use a bitmap.
 */
static uint_fast8_t events[last_event] = {1};

void notify_event(event_t event)
{
  assert(event < last_event);

  /* The order here is important, see note in process_events() 
   */
  events[event] = 1;
  event_happened = 1;
}

static int clear_event(event_t event)
{
  int ret;

  assert(event < last_event);

  ret = events[event];
  events[event] = 0;

  return ret;
}

#if REPORT_PRIMES_OPT
static uint32_t prev_primes_count;
#endif
static uint64_t prev_prime;
static uint32_t last_report_time;  /* in milliseconds */
static uint32_t next_report_due;   /* in seconds */
static uint32_t next_save_due;     /* in seconds */

static void init_progress_report(uint64_t current_prime)
{
#if REPORT_PRIMES_OPT
  prev_primes_count = primes_count;
#endif
  prev_prime = current_prime;
  last_report_time = millisec_elapsed_time();

  next_save_due = time(NULL) + save_period;
  next_report_due = time(NULL) + REPORT_PERIOD;
}

static void progress_report(uint64_t p)
{
  uint32_t current_time = millisec_elapsed_time();
  uint32_t millisec_diff;

  if (current_time == last_report_time)
    return;

  millisec_diff = (current_time - last_report_time);

  print_status(p,(p - prev_prime)*1000/millisec_diff
#if REPORT_PRIMES_OPT
               ,(uint64_t)(primes_count - prev_primes_count)
               *1000/millisec_diff
#endif
               ); /* print_status() */

#if REPORT_PRIMES_OPT
  prev_primes_count = primes_count;
#endif

  prev_prime = p;
  last_report_time = current_time;
}

static void (*old_sigint_handler)(int);
static void (*old_sigterm_handler)(int);
#ifdef SIGHUP
static void (*old_sighup_handler)(int);
#endif

void handle_signal(int signum)
{
  switch (signum)
  {
    case SIGINT:
      notify_event(received_sigint);
      break;
    case SIGTERM:
      notify_event(received_sigterm);
      break;
#ifdef SIGHUP
    case SIGHUP:
      notify_event(received_sighup);
      break;
#endif

#if !BOINC
#ifdef SIGUSR1
    case SIGUSR1:
      notify_event(save_due);
      break;
#endif
#ifdef SIGUSR2
    case SIGUSR2:
      /* Do nothing for now. */
      break;
#endif
#endif /* !BOINC */

#if HAVE_FORK
    case SIGPIPE:
      notify_event(received_sigpipe);
      break;
#endif
  }
}

static void init_signals(void)
{
  /* SIGUSR1 and SIGUSR2 are initialised earlier (if they exist). */

  if ((old_sigint_handler = signal(SIGINT,handle_signal)) == SIG_IGN)
    signal(SIGINT,SIG_IGN);
  if ((old_sigterm_handler = signal(SIGTERM,handle_signal)) == SIG_IGN)
    signal(SIGTERM,SIG_IGN);
#ifdef SIGHUP
  if ((old_sighup_handler = signal(SIGHUP,handle_signal)) == SIG_IGN)
    signal(SIGHUP,SIG_IGN);
#endif
#if HAVE_FORK
  if (num_children > 0)
    if (signal(SIGPIPE,handle_signal) == SIG_IGN)
      signal(SIGPIPE,SIG_IGN);
#endif
}


/* This function is called (via check_events()) from the top level sieve
   loops (prime_sieve() etc.). It can assume that it is safe to tighten any
   sieving parameters other than p_min and p_max.
*/
void process_events(uint64_t current_prime)
{
  /* event_happened was set last in notify_event(), so clear it first which
     ensures that if some signal arrives while we are in process_events()
     it might have to wait until the next sieve iteration to get processed,
     but it won't be lost.
  */
  event_happened = 0;

  if (clear_event(initialise_events))
  {
    init_signals();
    init_progress_report(current_prime);
  }

  if (clear_event(sieve_parameters_changed))
    init_progress_report(current_prime);

  if (clear_event(received_sigterm))
  {
    finish_gcwsieve("SIGTERM was received",current_prime);
    signal(SIGTERM,old_sigterm_handler);
    raise(SIGTERM);
  }

  if (clear_event(received_sigint))
  {
    finish_gcwsieve("SIGINT was received",current_prime);
    signal(SIGINT,old_sigint_handler);
    raise(SIGINT);
  }

#ifdef SIGHUP
  if (clear_event(received_sighup))
  {
    finish_gcwsieve("SIGHUP was received",current_prime);
    signal(SIGHUP,old_sighup_handler);
    raise(SIGHUP);
  }
#endif

#if HAVE_FORK
  if (clear_event(received_sigpipe))
  {
    finish_gcwsieve("SIGPIPE was received",current_prime);
    signal(SIGPIPE,SIG_DFL);
    raise(SIGPIPE);
  }

  if (clear_event(received_sigchld))
  {
    finish_gcwsieve("SIGCHLD was received",current_prime);
    signal(SIGCHLD,SIG_DFL);
    raise(SIGCHLD);
    exit(EXIT_FAILURE);
  }
#endif

  if (clear_event(factor_found))
    next_report_due = time(NULL);

  if (clear_event(report_due))
    progress_report(current_prime);

  if (clear_event(save_due))
#ifdef SMALL_P
    if (smallp_phase > 2)
#endif
    {
#if BOINC
      if (boinc_time_to_checkpoint())
      {
#endif
        write_checkpoint(current_prime);
        write_abc_file(0,current_prime,output_file_name);
#if BOINC
        boinc_checkpoint_completed();
      }
#endif
    }
}

void check_progress(void)
{
  uint32_t current_time = time(NULL);

  if (current_time >= next_save_due)
  {
    next_save_due = current_time + save_period;
    notify_event(save_due);
  }

  if (current_time >= next_report_due)
  {
    next_report_due = current_time + REPORT_PERIOD;
    notify_event(report_due);
  }
}
