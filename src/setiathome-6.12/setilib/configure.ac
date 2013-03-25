#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(setilib, 0.1)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR([include/seti_cfg.h])
AC_CONFIG_HEADERS([seti_config.h])
AC_COPYRIGHT([
// Copyright (c) 1999-2006 Regents of the University of California

// This library is free software; you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published by the 
// Free Software Foundation; either version 2.1, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU Lesser General Public License 
// along with this library; see the file COPYING.  If not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
// MA 02111-1307, USA. or visit http://www.gnu.org/copyleft/lesser.html
])

AC_REVISION([$Revision: 1.10 $])

AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_MAKE_SET

SAH_OPTION_BITNESS 

# Checks for libraries.
AC_CHECK_LIB([m], [sin])
AC_CHECK_LIB([fftw3f], [fftwf_plan_dft_1d], [
    AC_DEFINE([HAVE_LIBFFTW3F],[1],[Define to 1 if you have the fftw3f library])
    APP_LIBS="-lfftw3f ${APP_LIBS}"
    ])

AC_SUBST(APP_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h strings.h unistd.h fftw3.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_C_BIGENDIAN
AX_C_FLOAT_WORDS_BIGENDIAN
ACX_PTHREAD([AC_DEFINE(HAVE_PTHREAD,1, [Have pthread])])

# Checks for library functions.
AC_CHECK_FUNCS([pow sqrt strstr strlcat strlcpy atoll])

# Find BOINCDIR
SAH_CHECK_BOINC
# Find SETI_BOINC_DIR
SAH_CHECK_SAH
# Find INFORMIXDIR
SAH_CHECK_INFORMIX
# Find MYSQL
SAH_CHECK_MYSQL
# Find HEALPIX
SAH_CHECK_HEALPIX

SAH_LARGEFILE_BREAKS_CXX
SAH_HEADER_STDCXX
SAH_CHECK_NAMESPACES
AH_TEMPLATE([HAVE_STD_MIN],[Define to 1 if min is in namespace std::])
AH_TEMPLATE([HAVE_STD_MAX],[Define to 1 if max is in namespace std::])
AH_TEMPLATE([HAVE_STD_TRANSFORM],[Define to 1 if transform is in namespace std::
])
SAH_FUNCS_IN_NAMESPACE([['min(0,0)'] ['max(0,0)'] ['transform((char *)0,(char *)0,(char *)0,(int(*)(int))malloc)']],std)

AH_TOP([
#ifndef _SETI_CONFIG_H_
#define _SETI_CONFIG_H_

#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef VERSION
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT

])

AH_BOTTOM([ 

/* std_fixes.h is located in boinc/lib */
#ifndef ANDROID
#include "std_fixes.h"
#endif

#endif
])


AC_CONFIG_FILES([
  lib/Makefile 
  Makefile 
  include/Makefile
  demo/Makefile
  ])

AC_OUTPUT
