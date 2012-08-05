/* memset_fast32.h -- (C) Geoffrey Reynolds, September 2006.

   memset_fast32() function.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _MEMSET_FAST32_H
#define _MEMSET_FAST32_H

/* Get an architecture-specific version if one is provided ...
 */
#if USE_ASM
# define NEED_memset_fast32
# if (ARM_CPU)
#  include "asm-arm.h"
# elif (__i386__ && __GNUC__)
#  include "asm-i386-gcc.h"
# elif ((__ppc64__ || __powerpc64__) && __GNUC__)
#  include "asm-ppc64.h"
# elif (__x86_64__ && __GNUC__)
#  include "asm-x86-64-gcc.h"
# endif
#endif

/* ... otherwise use a generic version.
 */
#ifndef HAVE_memset_fast32
#define HAVE_memset_fast32
/* Store count copies of x starting at dst. In critical cases dst will be
   64-aligned and count will be a power of 2, usually greater than 16, but
   other cases must be handled as well.
 */
static inline
void memset_fast32(uint_fast32_t *dst, uint_fast32_t x, uint_fast32_t count)
{
#if 0
  while (count > 0)
    dst[--count] = x;
#else
  uint_fast32_t i;
  for (i = 0; i+8 <= count; i += 8)
  {
    dst[i+0] = x;
    dst[i+1] = x;
    dst[i+2] = x;
    dst[i+3] = x;
    dst[i+4] = x;
    dst[i+5] = x;
    dst[i+6] = x;
    dst[i+7] = x;
  }
  for ( ; i < count; i++)
    dst[i] = x;
#endif
}
#endif

#endif /* _MEMSET_FAST32_H */
