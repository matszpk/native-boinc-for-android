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
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "sqldefs.h"
#include "sqlrow.h"
#include "sqlblob.h"

#ifndef CLIENT
#include "sqlapi.h"
#endif



std::string escape_char(unsigned char c) {
  std::string rv("");
  if (isprint(c)) {
    switch (c) {
      case '\\':   rv="\\\\";
                  break;
      case '\'':  rv="\\'";
      		  break;
      case '"':   rv="\\\"";
      		  break;
      case ',':   rv="\\,";
      		  break;
      default:    rv+=c;
    }
  } else {
    char buf[8];
    sprintf(buf,"\\%.4o",c);
    rv=buf;
  }
  return rv;
}

unsigned char unescape_char(const unsigned char *&s) {
  unsigned char rv;
  int i;
  if (*s != '\\') return *s;
  switch (*(s+1)) {
      case '0':   sscanf((const char *)(s+1),"%4o",&i);
                  rv=i&0xff;
		  s+=4;
		  break;
      default:    rv=*(++s);
  } 
  return rv;
}
    


       

