/* btop.h -- (C) Geoffrey Reynolds, September 2007.

   Build table of powers.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _BTOP_H
#define _BTOP_H

#include <stdint.h>
#include "config.h"


/* Set USE_LADDER=1 to build the second half of the table of powers using
   products from the first half. This takes advantage of the fact that the
   second half is probably much less dense than the first half.
*/
#define USE_LADDER 1

#if USE_ASM
# define NEED_btop
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

/*
  Use generic code for any functions not defined above in assembler.
*/

#ifndef HAVE_btop
#define VECTOR_LENGTH 4
/* In btop-generic.c */
void btop_generic(const uint32_t *L, uint64_t *T, uint32_t lmin, uint32_t llen, uint64_t p);
#define build_table_of_powers(L,T,lmin,llen,p) btop_generic(L,T,lmin,llen,p)
#endif /* HAVE_btop */

#endif /* _BTOP_H */
