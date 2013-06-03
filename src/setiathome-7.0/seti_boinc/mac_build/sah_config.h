// Copyright 2006 Regents of the University of California

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

// This file contains compile time configuration parameters
// for the seti@home reference algorithm implementation.

/* Special config.h file for Macintosh */


/************************************** IMPORTANT INSTRUCTIONS: ****************************

  Build the config-i386.h and config-ppc.h in the seti_boinc/mac_build/ directory by
  running the shell script:
      sh [path]makeseticonfigs.sh  path_to_seti_boinc_dir  path_to_boinc_dir

*********************************************************************************************/

/******************* Additional Compiler Flags for Individual Source Files *******************

These are set up in the XCode project itself:

client/vector/analyzeFuncs_altivec.cpp:
 on PowerPC, add the GCC command line options "-DUSE_ALTIVEC -faltivec"

client/vector/analyzeFuncs_vector.cpp
 on PowerPC, add the GCC command line options "-DUSE_ALTIVEC"
 on Intel, add the GCC command line options "-DUSE_SSE -DUSE_SSE2 -DUSE_SSE3"

client/vector/analyzeFuncs_sse.cpp
 on Intel add the GCC command line options "-DUSE_SSE -msse"

client/vector/analyzeFuncs_sse2.cpp
 on Intel add the GCC command line options "-DUSE_SSE -DUSE_SSE2 -msse2"

client/vector/analyzeFuncs_sse3.cpp
 on Intel add the GCC command line options "-DUSE_SSE -DUSE_SSE2
-DUSE_SSE3 -msse3"

client/vector/analyzeFuncs_x86_64.cpp
 on Intel, add the GCC command line options -DUSE_SSE -msse2 -mfpmath=sse"

client/vector/x86_float4.cpp
 on Intel, add the GCC command line options "-DUSE_SSE -msse"
*********************************************************************************************/

#ifndef _SAH_MAC_CONFIG_H_
#define _SAH_MAC_CONFIG_H_

#ifndef __APPLE_CC__
#error Wrong sah_config.h file!
#endif

#include "version.h"    // Get BOINC_VERSION_STRING (BOINC library version)

#undef PACKAGE_STRING
#undef PACKAGE
#undef PACKAGE_NAME
#undef PACKAGE_BUGREPORT
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION


#ifdef __i386__     // Intel

#include "config-i386.h"

#undef COMPILER_STRING
#define COMPILER_STRING "XCode GCC " __VERSION__ " i386"

#else               // PowerPC

// Won't compile unless we define HAVE_STD_MAX, HAVE_STD_MIN and HAVE_STD_TRANSFORM
#define HAVE_STD_MAX 1          // Override the autoconfig setting for Mac PowerPC builds
#define HAVE_STD_MIN 1          // Override the autoconfig setting for Mac PowerPC builds
#define HAVE_STD_TRANSFORM 1    // Override the autoconfig setting for Mac PowerPC builds

#include "config-ppc.h"

#undef COMPILER_STRING
#define COMPILER_STRING "XCode GCC " __VERSION__ " ppc"

// The following three were #undefined in previous versions, though only  
// HAVE_ATANF is actually necessary for OS 10.3.0 compatibility
#undef HAVE_ATANF               // OS 10.3.0 does not have atanf
#undef HAVE_COSF
#undef HAVE_SINF

#endif

// Overrides for both Intel and PowerPC Macs
#ifndef USE_FFTWF
#define USE_FFTWF
#endif

#undef CUSTOM_STRING
//#if !defined(CUSTOM_STRING) && defined(COMPILER_STRING)
#define CUSTOM_STRING PACKAGE_STRING " " COMPILER_STRING "\n\nlibboinc: " BOINC_VERSION_STRING
//#endif

#endif      /*  _SAH_MAC_CONFIG_H  */
