/* swizzle.h -- (C) Geoffrey Reynolds, August 2007.

   Swizzle loop declarations.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _SWIZZLE_H
#define _SWIZZLE_H

#include <stdint.h>
#include "config.h"

#ifndef SWIZZLE
#define SWIZZLE 2
#endif

/* Number of bytes needed for loop_data_t when R[] is length _LEN. */
#define sizeof_loop_data(_LEN) \
 (sizeof(struct loop_data_t) + ((_LEN)-1)*sizeof(struct loop_rec_t))

#if USE_ASM
# define NEED_swizzle_loop
# define NEED_loop_data
# if defined(ARM_CPU)
#  include "asm-arm.h"
# elif defined(__i386__) && defined(__GNUC__)
#  include "asm-i386-gcc.h"
# elif defined(__i386__) && defined(_MSC_VER)
#  include "asm-i386-msc.h"
# elif (defined(__ppc64__) || defined(__powerpc64__)) && defined(__GNUC__)
#  include "asm-ppc64.h"
# elif defined(__x86_64__) && defined(__GNUC__)
#  include "asm-x86-64-gcc.h"
# elif defined(__x86_64__) && defined(_MSC_VER)
#  include "asm-x86-64-msc.h"
# endif
#endif

#ifndef HAVE_loop_data
struct loop_rec_t { uint32_t N[SWIZZLE]; uint32_t G[SWIZZLE]; };
struct loop_data_t { uint64_t X[SWIZZLE]; struct loop_rec_t R[1]; };
#endif

/*
  Use generic code for any functions not defined above in assembler.
*/

#ifndef HAVE_swizzle_loop
/* In loop-generic.c */
int swizzle_loop_generic(const uint64_t *T, struct loop_data_t *D,
                         int i, uint64_t p);
int swizzle_loop_generic_prefetch(const uint64_t *T, struct loop_data_t *D,
                                  int i, uint64_t p);
#define swizzle_loop(a,b,c,d) swizzle_loop_generic(a,b,c,d)
#define swizzle_loop_prefetch(a,b,c,d) swizzle_loop_generic_prefetch(a,b,c,d)
#endif /* HAVE_swizzle_loop */

#endif /* _SWIZZLE_H */
