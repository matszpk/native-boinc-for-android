
// Copyright (c) 1999-2006 Regents of the University of California
//
// FFTW: Copyright (c) 2003,2006 Matteo Frigo
//       Copyright (c) 2003,2006 Massachusets Institute of Technology
//
// fft8g.[cpp,h]: Copyright (c) 1995-2001 Takya Ooura

// This program is free software; you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the 
// Free Software Foundation; either version 2, or (at your option) any later
// version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions
// as an alternative to FFTW and distribute a linked executable and 
// source code.  You must obey the GNU General Public License in all 
// respects for all of the code used other than the FFT library itself.  
// Any modification required to support these libraries must be distributed 
// under the terms of this license.  If you modify this program, you may extend 
// this exception to your version of the program, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.  Please be aware that FFTW is not covered by this exception, 
// therefore you may not use FFTW in any derivative work so modified without 
// permission of the authors of FFTW.

/* Module local signal handlers.  Yes, all defined static, not extern because
 * we don't want to worry about signal handlers in one module clobbering 
 * saved signal handlers in another module, and we don't want to invent a 
 * stack of signal handlers.
 */

#ifndef SIGHANDLER_H
#define SIGHANDLER_H


#include <signal.h>
#include <setjmp.h>

#ifndef MAXSIG

#ifdef NSIG
#define MAXSIG (NSIG+1)

#elif defined(__linux__)
#define MAXSIG 64

#elif defined(SIGRTMAX)
#define MAXSIG SIGRTMAX

#elif defined(_SIGRTMAX)
#define MAXSIG _SIGRTMAX

#else
#define MAXSIG 32
#endif

#endif

#ifdef HAVE_SIGLONGJMP
static sigjmp_buf jb;
#else
static jmp_buf jb;
#define sigsetjmp(x,y) setjmp(x)
#define siglongjmp(x,y) longjmp(x,y)
#endif
static int sc;

typedef void (*signal_handler)(int);

#if defined(HAVE_SYSV_SIGNAL)
#define signal(x,y) sysv_signal(x,y)
#elif defined(HAVE_BSD_SIGNAL)
#define signal(x,y) bsd_signal(x,y)
#endif

/* all our signal handler does is jump to a place we've set */
static void sighandler(int x) {
#if defined(HAVE_SIGACTION)
    sigset_t unblock;
    sigemptyset(&unblock);
    sigaddset(&unblock,x);
    sigprocmask(SIG_UNBLOCK,&unblock,NULL);
#elif defined(HAVE_SYSV_SIGNAL)
    // SYSV resets the handler but keeps signal unblocked
    sysv_signal(x,sighandler);
#elif defined(HAVE_BSD_SIGNAL)
    // BSD keeps the handler, but blocks the signal.
    // Nothing should need to be done here.
#else
    // who knows which version of signal this is.
    //  Do the safe thing.
    signal(x,sighandler);
#endif
    sc++;
    siglongjmp(jb,1);
}

/* flags to mark whether we've been installed already */
static int installed[MAXSIG];

/* a place to save our the existing signal handlers   */
#ifdef HAVE_SIGACTION
static struct sigaction oldhandler[MAXSIG];
#else
static signal_handler oldhandler[MAXSIG];
#endif

/* these are the signals we'll handle */
static const int install_handler[]={
#ifdef SIGILL
    SIGILL,
#endif
#ifdef SIGPRIV
    SIGPRIV,
#endif
#ifdef SIGSEGV
    SIGSEGV,
#endif
#ifdef SIGBUS
    SIGBUS,
#endif
#ifdef SIGIOT
    SIGIOT,
#endif
#ifdef SIGABRT
    SIGABRT,
#endif
#ifdef SIGSTKFLT
    SIGSTKFLT,
#endif
    0
};

/* Keep these as stubs even if signal handling is not enabled */
static inline void install_sighandler() {
#ifdef HAVE_SIGACTION
  struct sigaction new_action;
  new_action.sa_handler=sighandler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags=0;
#endif
  int i=0,sig;
  while ((sig=install_handler[i])!=0) {
    if (!installed[sig]) {
#ifdef HAVE_SIGACTION
      sigaction(sig,&new_action,oldhandler+sig);
#else
      oldhandler[sig]=signal(sig,sighandler);
#endif
      installed[sig]=1;
    }
    i++;
  }
}


static inline void uninstall_sighandler() {
  /* Don't uninstall the windows signal handler! */
  int i=0,sig;
  while ((sig=install_handler[i])!=0) {
    if (installed[sig]) {
#ifdef HAVE_SIGACTION
      sigaction(sig,oldhandler+sig,NULL);
#else
      signal(sig,oldhandler[sig]);
#endif
      installed[sig]=0;
    }
    i++;
  }
}

//static inline void reinstall_sighandler() {
//  uninstall_sighandler();
//  install_sighandler();
//}

#endif /* SIGHANDLER_H */
