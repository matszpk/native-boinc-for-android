/* threads.c -- (C) Geoffrey Reynolds, January 2008.

   Simple multithreading using fork() and pipe().

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"
#if HAVE_FORK

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sr5sieve.h"


/* Number of children to start. Set from command-line switch `-t --threads'
   in sr5sieve.c
*/
int num_children = 0;

/* Child number 0 <= child_num < num_children of current thread,
   or -1 if parent.
 */
int child_num = -1;
 
/* For i < affinity_opt, child_affinity[i] is set from the i'th use of
   the command line switch `-A --affinity' in sr5sieve.c
*/
int affinity_opt = 0;
int child_affinity[MAX_CHILDREN];

/* In parent thread, child_pid[i] is the pid of child i.
 */
static int child_pid[MAX_CHILDREN];

/* Return the index of pid in child_pid[], or -1 if not found.
 */
static int child_pid_index(int pid)
{
  int i;

  for (i = 0; i < num_children; i++)
    if (child_pid[i] == pid)
      return i;

  return -1;
}

/* hightide[i] is the greatest prime reported complete by child i.
 */
static uint64_t hightide[MAX_CHILDREN];

/* Return a lower bound on untested primes. All p in p_min <= p <= lowtide()
   are known to be tested and any factors reported.
 */
uint64_t get_lowtide(void)
{
  int i;
  uint64_t lowtide;

  for (lowtide = hightide[0], i = 1; i < num_children; i++)
    if (hightide[i] < lowtide)
      lowtide = hightide[i];

  return lowtide;
}

/* Pipes to communicate between parent and child processes.
   Parent writes to primes_pipe and reads from factors_pipe.
   Children read from primes_pipe and write to factors_pipe.
*/
static int primes_pipe[2];
static int factors_pipe[2];

/* Parcel up this many primes before sending to children, to reduce pipe
   cost.

   TODO: Check that the total pipe buffer size is not exceeded, as that
   could result in deadlock. In the worst case we might need
   PRIMES_PARCELSIZE * num_children * sizeof(uint64_t) bytes.
*/
#define PRIMES_PARCELSIZE 64
static uint64_t primes_parcel[PRIMES_PARCELSIZE];

/* Read primes_parcel from the primes pipe.
   Return 1 on success, 0 on failure/EOF.
*/
static int read_primes_pipe(void)
{
  return (read(primes_pipe[0],&primes_parcel,sizeof(primes_parcel))
          == sizeof(primes_parcel));
}

/* Write primes_parcel to the primes pipe. Return 1 on success, 0 on failure.
 */
static int write_primes_pipe(void)
{
  return (write(primes_pipe[1],&primes_parcel,sizeof(primes_parcel))
          == sizeof(primes_parcel));
}

#define MSG_COMPLETE UINT32_MAX
static struct factor_msg_t
{
  uint32_t s;      /* subsequence or MSG_COMPLETE */
  uint32_t m;      /* m in n=mQ+d or child number */
  uint64_t p;      /* prime factor or hightide */
} factor_msg;

/* Read factor_msg from the factors pipe. Return 1 on success, 0 on failure/EOF.
 */
static int read_factors_pipe(void)
{
  return
    (read(factors_pipe[0],&factor_msg,sizeof(factor_msg))==sizeof(factor_msg));
}

/* Write factor_msg to the factors pipe. Return 1 on success, 0 on failure.
 */
static int write_factors_pipe(void)
{
  return
   (write(factors_pipe[1],&factor_msg,sizeof(factor_msg))==sizeof(factor_msg));
}

/* Called by child threads from bsgs.c to report a possible factor.
 */
void child_eliminate_term(uint32_t subseq, uint32_t m, uint64_t p)
{
  factor_msg.s = subseq;
  factor_msg.m = m;
  factor_msg.p = p;
  if (!write_factors_pipe())
    _exit(EXIT_FAILURE);
}

/* Loop for child child_num: Read a batch of primes from primes_pipe, call
   fun(p) for each prime p, then report the batch complete. Quit when a
   zero is received.
 */
static void child_thread(void (*fun)(uint64_t))
{
  int i, status;

  close(primes_pipe[1]);
  close(factors_pipe[0]);

  /* Set thread affinity.
   */
  if (child_num < affinity_opt)
    set_cpu_affinity(child_affinity[child_num]);

  /* Request initial parcel of primes.
   */
  factor_msg.s = MSG_COMPLETE;
  factor_msg.m = child_num;
  factor_msg.p = 0;
  if ((status = write_factors_pipe()) != 0)
  {
    /* Main loop.
     */
    while ((status = read_primes_pipe()) != 0)
    {
      for (i = 0; i < PRIMES_PARCELSIZE && primes_parcel[i] > 0; i++)
        fun(primes_parcel[i]);

      if (i == 0) /* Empty parcel, quit. */
        break;

      factor_msg.s = MSG_COMPLETE;
      factor_msg.m = child_num;
      factor_msg.p = primes_parcel[i-1];
      if ((status = write_factors_pipe()) == 0)
        break;
    }
  }

  close(primes_pipe[0]);
  close(factors_pipe[1]);

  _exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
}

static int parcel_len;
void parent_thread(uint64_t p)
{
  /* If there is room in the parcel, add this prime and return for another.
   */
  primes_parcel[parcel_len++] = p;
  if (parcel_len < PRIMES_PARCELSIZE && p > 0)
    return;

  /* Now that the parcel is ready, keep processing messages until we get a
     request for more work (MSG_COMPLETE), and then send the parcel.
  */
  while (read_factors_pipe())
    if (factor_msg.s == MSG_COMPLETE)
    {
      if (factor_msg.p > 0)
        hightide[factor_msg.m] = factor_msg.p;
      write_primes_pipe();
      parcel_len = 0;
      return;
    }
    else
      eliminate_term(factor_msg.s,factor_msg.m,factor_msg.p);
}

/* Only react to SIGCHLD if the child has terminated, not if it has been
   stopped or continued.
*/
static void handle_sigchld(int signum)
{
  int tmp;

  tmp = errno;
  if (waitpid(-1,NULL,WNOHANG) > 0)
    notify_event(received_sigchld);
  errno = tmp;
}

/* Start children. They will initialize and then block until work is ready.
 */
void init_threads(uint64_t pmin, void (*fun)(uint64_t))
{
  int i, pid;

  if (pipe(primes_pipe) < 0 || pipe(factors_pipe) < 0)
    error("pipe() failed.");

  parcel_len = 0;
  for (i = 0; i < num_children; i++)
    hightide[i] = pmin;

  signal(SIGCHLD,handle_sigchld); /* SIGCHLD is ignored by default. */

  for (i = 0; i < num_children; i++)
  {
    if ((pid = fork()) < 0) /* Error */
    {
      error("fork() failed.");
    }
    else if (pid == 0) /* Child process */
    {
      child_num = i;
      child_thread(fun);
    }
    else /* Parent process */
    {
      child_pid[i] = pid;
      if (verbose_opt)
        report(1,"Child thread %d started, pid=%d.",i,pid);
    }
  }

  close(primes_pipe[0]);
  close(factors_pipe[1]);
}

/* Wait for children to stop, check termination status.
   Return number of children that failed to finish.
*/
int fini_threads(uint64_t pmax)
{
  int i, pid, status, incomplete;

  /* Ignore SIGCHLD ... */
  signal(SIGCHLD,SIG_DFL);

  /* ... unblock children ... */
  for (i = 0; i <= num_children; i++)
    parent_thread(0);

  /* ... wait for children to finish. */
  incomplete = 0;
  while ((pid = waitpid(-1,&status,0)) > 0)
  {
    i = child_pid_index(pid);
    assert(i >= 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
    {
      /* Child terminated normally, via _exit(EXIT_SUCCESS). */
      hightide[i] = pmax;
      if (verbose_opt)
        report(1,"Child thread %d finished.",i);
    }
    else
    {
      /* Child terminated abnormally, so the packet of primes it was working
         on at the time are presumed lost. */
      incomplete++;
      warning("Child thread %d failed to finish, status=%d, last p=%"PRIu64,
              i,status,hightide[i]);
    }
  }

  close(primes_pipe[1]);
  close(factors_pipe[0]);

  return incomplete;
}

#endif /* HAVE_FORK */
