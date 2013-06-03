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

// $Id: malloc_a.cpp,v 1.6.2.5 2007/05/31 22:03:10 korpela Exp $
#include "sah_config.h"

#include <cstdlib>
#include <cstring>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

//JWS: fftwf_malloc() is inadequate for AVX, XOP, and future SIMD > 128 bit
//#ifdef USE_FFTWF
//#include <fftw3.h>
//#endif

#include "malloc_a.h"

// for processor portability (e.g. 32/64/etc, bit processors)
typedef size_t PointerArith;

// malloc_a and free_a are used to allocate one memory block

// Allocates memory on N-byte boundary
//
// When fftwf is used, we use fftwf_malloc and ignore the alignment setting,
// since this guarantees proper alignment for SIMD types.
//
// On WIN32, we use _aligned_malloc()
//
// Where memalign() is available, we use it.
//
// If none of the above, we use our own algorithm.
//
// Layout in memory of the pointers and buffer:
//
//  +-- malloc'd memory pointer
//  |
//  |          +-- save malloc'd memory pointer here for use with free_a()
//  V          V
//  +--------+---+------------------- ...
//  | unused |ptr|  aligned buffer
//  +--------+---+------------------- ...
//               ^
//               |
//               +-- aligned memory pointer returned from malloc_a()
//
//
//
void *malloc_a(size_t size, size_t alignment) {
//#if defined(USE_FFTWF)
//  return fftwf_malloc(size);
//#elif defined(HAVE__ALIGNED_MALLOC)
#if defined(HAVE__ALIGNED_MALLOC)
  return _aligned_malloc(size,alignment);
#elif defined(HAVE_MEMALIGN)
  return memalign(alignment,size);
#else
  PointerArith byteAlignment;
  void *pmem;
  void *palignedMem;
  void **pp;

  // ensure byteAlignment is positive (if alignment is 0, make it 1)
  if (alignment < 1) {
    alignment = 1;
  }
  byteAlignment = alignment - 1;

  pmem = (void *) malloc(size + byteAlignment + sizeof(void *));    // allocate memory
  if (pmem == NULL) return NULL;

  // align memory on N byte boundary
  palignedMem = (void *) (((PointerArith) pmem + byteAlignment + sizeof(void *)) & ~(byteAlignment));

  pp = (void **) palignedMem;
  pp[-1] = pmem;    // store original address

  return(palignedMem);            // return aligned memory
#endif
}

//Frees memory that was allocated with malloc_a
void free_a(void *palignedMem) {
//#if defined(USE_FFTWF)
//  fftwf_free(palignedMem);
//#elif defined(HAVE__ALIGNED_FREE)
#if defined(HAVE__ALIGNED_FREE)
  _aligned_free(palignedMem);
#elif defined(HAVE_MEMALIGN)
  free(palignedMem);
#else
  void **pp;

  if (palignedMem == NULL) return;

  pp = (void **) palignedMem;

  //pp[-1] pts to original address of malloc'd memory
  free(pp[-1]);
#endif
}

void *calloc_a(size_t size, size_t nitems, size_t alignment) {
  void* p = malloc_a(size*nitems, alignment);
  if (p)
    memset(p, 0, size*nitems);
  return p;
}

