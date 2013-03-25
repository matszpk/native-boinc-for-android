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

#ifndef _SQLDEFS_H_
#define _SQLDEFS_H_

#ifndef CLIENT
#ifdef USE_INFORMIX
#include "int8.h"
#include "sqlca.h"
#include "sqlda.h"
#include "locator.h"
#include "sqltypes.h"
#include "sqlstype.h"
#ifdef MAX_QUERY_LEN
#undef MAX_QUERY_LEN
#endif
namespace sql {
  static const int MAXBLOB=8192;
  static const int MAXVARCHAR=255;
  static const int MAXCURSORS=20;
  static const int MAX_QUERY_LEN=8192;
}
#elif defined(USE_MYSQL)
#include "mysql.h"
namespace sql {
  static const int MAXBLOB=4096;
  static const int MAXVARCHAR=255;
  static const int MAXCURSORS=20;
  static const int MAX_QUERY_LEN=8192;
}
#endif
#endif
#endif
