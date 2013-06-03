// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public 
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend 
// this exception to your version of the file, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.

// unix_main.C: The SETI@home client program.
// $Id: main.cpp,v 1.43.2.19 2007/07/16 15:36:50 korpela Exp $
// Main program for command-line application.
//

// Usage: client [options]
// -version  show version info
// -verbose  print running status
// -standalone


#include "sah_config.h"

#ifdef _WIN32
#include "boinc_win.h"
#endif

#ifndef _WIN32
#include <cstdio>
#include <cstdlib>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cstring>
#include <sys/types.h>
#include <errno.h>
#endif


#include "diagnostics.h"
#include "util.h"
#include "s_util.h"
#include "boinc_api.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#include "graphics2.h"
#endif

#ifdef BOINC_APP_GRAPHICS

#endif

#include "util.h"
#include "s_util.h"
#include "str_util.h"
#include "str_replace.h"
#include "analyze.h"
#include "analyzeFuncs.h"
#include "analyzePoT.h"
#include "worker.h"
#include "sah_version.h"
#include "chirpfft.h"
#include "gaussfit.h"

#ifdef __APPLE_CC__
#include "app_icon.h"
#include <sys/resource.h>
#endif


static int g_argc;
static char **g_argv;

void usage() {
  printf(
    "options:\n"
#ifdef BOINC_APP_GRAPHICS
    " -nographics run without graphics\n"
    " -notranspose do not transpose data for pulse and gaussian searches\n"
    " -default_functions  use the safe unoptimized default functions\n"
    " -standalone (implies -nographics)\n"
#else
    " -standalone\n"
#endif
    " -version  show version info\n"
    " -verbose  print running status\n"
  );
}

void print_error(int e) {
  char* p;

  p = error_string(e);
  fprintf(stderr,"%s\n",p);
}

void print_version() {
  printf(
    "SETI@home client.\n"
    "Version: %d.%02d\n",
    gmajor_version, gminor_version
  );
  printf(
    "\nSETI@home is sponsored by individual donors around the world.\n"
    "If you'd like to contribute to the project,\n"
    "please visit the SETI@home web site at\n"
    "http://setiathome.ssl.berkeley.edu.\n"
    "The project is also sponsored by the Planetary Society,\n"
    "the University of California, Sun Microsystems, Paramount Pictures,\n"
    "Fujifilm Computer Products, Informix, Engineering Design Team Inc,\n"
    "The Santa Cruz Operation (SCO), Intel, Quantum Corporation,\n"
    "and the SETI Institute.\n\n"
    "SETI@home was developed by David Gedye (Founder),\n"
    "David Anderson (Director), Dan Werthimer (Chief Scientist),\n"
    "Hiram Clawson, Jeff Cobb, Charlie Fenton,\n"
    "Eric Heien, Eric Korpela, Matt Lebofsky,\n"
    "Tetsuji 'Maverick' Rai and Rom Walton\n"
  );
}

extern double sigma_thresh;
extern double f_PowerThresh;
extern double f_PeakPowerThresh;
extern double chi_sq_thresh;
bool notranspose_flag=false;
bool default_functions_flag=false;

int run_stage;


typedef seti_error boinc_error;

//void atexit_handler(void) {
//  if (run_stage == GRXINIT) {
//      fprintf(stderr,"Staring in non-graphical mode");
//      fflush(stderr);
//      fclose(stderr);
//      g_argv[g_argc]="-nographics";
//      execv(g_argv[0],g_argv);
//      SETIERROR(UNHANDLED_SIGNAL, "from atexit_handler()");
//  }
//}

extern APP_INIT_DATA app_init_data;

#ifdef HAVE_ARMOPT
extern "C" {
extern void setupARMFPU(void);
}
#endif

int main(int argc, char** argv) {
  int retval = 0, i;
#ifdef HAVE_ARMOPT
  setupARMFPU();
#endif
  
  FORCE_FRAME_POINTER;
  run_stage=PREGRX;
  g_argc=argc;

  if (!(g_argv=(char **)calloc(argc+2,sizeof(char *)))) {
    exit(MALLOC_FAILED);
  }


  setbuf(stdout, 0);

  bool standalone = false;
  g_argv[0]=argv[0];

  for (i=1; i<argc; i++) {
    g_argv[i]=argv[i];
    char *p=argv[i];
    while (*p=='-') p++;
    if (!strncmp(p,"vers",4)) {
      print_version();
      exit(0);
    } else if (!strncmp(p,"verb",4)) {
      verbose = true;
    } else if (!strncmp(p, "st", 2)) {
        standalone = true;
#ifdef BOINC_APP_GRAPHICS
        nographics_flag = true;
#endif
    } else if (!strncmp(p, "nog", 3)) {
#ifdef BOINC_APP_GRAPHICS
        nographics_flag = true;
#endif
    } else if (!strncmp(p, "not", 3)) {
        notranspose_flag = true;
    } else if (!strncmp(p, "def", 3)) {
        default_functions_flag = true;
    } else {
      fprintf(stderr, "bad arg: %s\n", argv[i]);
      usage();
      exit(1);
    }
  }

  try {

    // Initialize Diagnostics
    //
    unsigned long dwDiagnosticsFlags = 0;

    dwDiagnosticsFlags = 
        BOINC_DIAG_DUMPCALLSTACKENABLED | 
        BOINC_DIAG_HEAPCHECKENABLED |
        BOINC_DIAG_TRACETOSTDERR |
        BOINC_DIAG_REDIRECTSTDERR;

    retval = boinc_init_diagnostics(dwDiagnosticsFlags);
    if (retval) {
      SETIERROR(retval, "from boinc_init_diagnostics()");
    }

#ifdef __APPLE__
    setMacIcon(argv[0], MacAppIconData, sizeof(MacAppIconData));
#ifdef __i386__
    rlimit rlp;
    
    getrlimit(RLIMIT_STACK, &rlp);
    rlp.rlim_cur = rlp.rlim_max;
    setrlimit(RLIMIT_STACK, &rlp);
#endif
#endif

    // Initialize BOINC
    //

    boinc_parse_init_data_file();
    boinc_get_init_data(app_init_data);
#ifdef BOINC_APP_GRAPHICS
    sah_graphics_init(app_init_data);
#endif
    retval = boinc_init();
    if (retval) {
      SETIERROR(retval, "from boinc_init()");
    }
    worker();
  }
  catch (seti_error e) {
    e.print();
    exit(static_cast<int>(e));
  }
  return 0;
}

#ifdef _WIN32


//
// Function: WinMain
//
// Purpose:  Application entry point, used instead of main so that on Win9x platforms
//             windows do not appear for each instance executing on the system.
//
// Date:     01/29/04
//
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR Args, int WinMode) {	
    LPSTR command_line;
    char* argv[100];
    int argc, retval;

    command_line = GetCommandLine();
    argc = parse_command_line(command_line,argv);
    retval = main(argc, argv);	

	return retval;
}


#endif
