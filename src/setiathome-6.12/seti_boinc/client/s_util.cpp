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

// util.C
//
// $Id: s_util.cpp,v 1.10.2.4 2006/12/14 22:21:47 korpela Exp $
//

#include "sah_config.h"

#include <cstdio>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

#include "timecvt.h"
#include "util.h"
#include "s_util.h"

const char * const seti_error::message[]={
                                       "Success",
                                       "Can't create file -- disk full?",
				       "Can't read from file",
				       "Can't write to file -- disk full?",
				       "Can't allocate memory",
				       "Can't open file",
				       "Bad workunit header",
				       "Garbled encoded workunit",
				       "Garbled binary workunit",
				       "result_overflow",
				       "Unhandled signal",
				       "atexit() failure",
				       "Vectorized functions unsupported",
				       "Floating point failure"
};

void seti_error::print() const {
  std::cerr << "SETI@home error " << -value << " " ;
  if ((value <= atexit_failure) && (value >=0)) {
    std::cerr << message[value] ;
  } else {
    std::cerr << "Unknown error" ;
  }

  std::cerr << std::endl << data << std::endl;
  std::cerr << "File: "  << file << std::endl;
  std::cerr << "Line: "  << line << std::endl;
  std::cerr << std::endl;
}  

void strip_cr(char*p ) {
  char* q = strchr(p, '\n');
  if (q) *q = 0;
}

// Encode a range of bytes into printable chars, and write to file.
// Encodes 3 bytes into 4 chars.
// May read up to two bytes past end.

void encode(unsigned char* bin, int nbytes, FILE* f) {
  int count=0, offset=0, nleft;
  unsigned char c0, c1, c2, c3;
  for (nleft = nbytes; nleft > 0; nleft -= 3) {
    c0 = bin[offset]&0x3f;     // 6
    c1 = (bin[offset]>>6) | (bin[offset+1]<<2)&0x3f; // 2+4
    c2 = ((bin[offset+1]>>4)&0xf) | (bin[offset+2]<<4)&0x3f;// 4+2
    c3 = bin[offset+2]>>2;    // 6
    c0 += 0x20;
    c1 += 0x20;
    c2 += 0x20;
    c3 += 0x20;
    fprintf(f, "%c%c%c%c", c0, c1, c2, c3);
    offset += 3;
    count += 4;
    if (count == 64) {
      count = 0;
      fprintf(f, "\n");
    }
  }
  fprintf(f, "\n");
}

// Read from file, decode into bytes, put in array.
// May write up to two bytes past end of array.
int decode(unsigned char* bin, int nbytes, FILE* f) {
  unsigned char buf[256], *p, c0, c1, c2;
  int i, n, m, nleft = nbytes, offset=0;
  int nbadlines = 0;
  while (1) {
    if (nleft <= 0) break;
    p = (unsigned char*)fgets((char*)buf, 256, f);
    if (!p) {
      SETIERROR(BAD_DECODE, "file ended too soon");
    }
    n = (int)strlen((char*)buf)-1;
    if (n%4) {
      nbadlines++;
      if (nbadlines == 100) {
        SETIERROR(BAD_DECODE, "too many bad lines - rejecting file");
      }
      n = 64;
      fprintf(stderr, "encoded line has bad length: %s\n", buf);
    }
    m = (n/4)*3;
    if (m > nleft+2) {
      //fprintf(stderr, "encoded line too long\n");
      //return BAD_DECODE;
      n = ((nleft+2)/3)*4;
    }
    p = buf;
    for (i=0; i<n/4; i++) {
      p[0] -= 0x20;
      p[1] -= 0x20;
      p[2] -= 0x20;
      p[3] -= 0x20;
      c0 = p[0]&0x3f | p[1]<<6; // 6 + 2
      c1 = p[1]>>2 | p[2]<<4; // 4 + 4
      c2 = p[2]>>4 | p[3]<<2; // 2 + 6
      p += 4;
      bin[offset++] = c0;
      bin[offset++] = c1;
      bin[offset++] = c2;
      nleft -= 3;
    }
  }
  return 0;
}


// Read from file, put in array.
int read_bin_data(unsigned char* bin, int nbytes, FILE* f) {
    int i;
    size_t n;

    for(i=0; i < nbytes; i += 256) {
        n = fread((void *)(bin+i), 256, 1, f);
        if (!n) {
            SETIERROR(BAD_BIN_READ,"file ended too soon");
        }
    }
    return 0;
}


// see the doc on binary data representation

void bits_to_floats(unsigned char* raw, sah_complex* data, int nsamples) {
  int i, j, k=0;
  unsigned char c;

  for (i=0; i<nsamples/4; i++) {
    j = (i&1) ? i-1 : i+1;
    c = raw[j];
    for (j=0; j<4; j++) {
      data[k][0] = (float)((c&2)?1:-1);
      data[k][1] = (float)((c&1)?1:-1);
      k++;
      c >>= 2;
    }
  }
}

int float_to_uchar(float float_element[], unsigned char char_element[],
                   long num_elements, float scale_factor) {
  long i, test_int;

  for(i = 0; i < num_elements; i++) {
    test_int = ROUND(float_element[i]/scale_factor);
    if(test_int >= 0 && test_int <= 255)
      char_element[i] = (unsigned)test_int;
    else
      char_element[i] = test_int < 0 ? 0 : 255;
  }
  return 0;
}

char* error_string(int e) {
  char* p;
  static char buf[256];

  switch(e) {
    case CANT_CREATE_FILE:
      sprintf(buf, "Can't create file - disk full?");
      p = buf;
      break;
    case WRITE_FAILED: p = "Can't write to file - disk full?"; break;
    case MALLOC_FAILED: p = "Can't allocate memory"; break;
    case FOPEN_FAILED:
      sprintf(buf, "Can't open file");
      p = buf;
      break;
    case BAD_HEADER: p = "Bad file header"; break;
    case BAD_DECODE: p = "Can't decode data"; break;

    default: sprintf(buf, "Unknown error %d", e) ; return buf;
  }
  return p;
}

