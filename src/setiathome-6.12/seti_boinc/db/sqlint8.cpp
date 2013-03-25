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

#include "sah_config.h"
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "sqlint8.h"

#ifdef HAVE_INTTYPES_H

#elif defined(LLONG_MAX) || defined(HAVE_LONG_LONG)

#elif defined(USE_MYSQL) && defined(longlong_defined)

#elif defined(USE_INFORMIX)

char _ifx_int8::pval[20];

#elif defined(_INTEGRAL_MAX_BITS) && (_INTEGRAL_MAX_BITS >= 64)

#elif defined(HAVE_LONG_DOUBLE)

#else

#endif
