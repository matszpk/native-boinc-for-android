/* lookup-ind.h -- (C) Geoffrey Reynolds, January 2009.

   Functions to compute indices into the Legendre symbol lookup tables using
   precomputed inverses.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _LOOKUP_IND_H
#define _LOOKUP_IND_H

#if USE_ASM && __GNUC__ && (__i386__ || __x86_64__)
#define HAVE_lookup_ind 1
#endif

#ifdef HAVE_lookup_ind

#if __i386__ && __SSE2__
/* Functions in lookup-sse2.S */
void init_lookup_ind_sse2(void *IND, const uint32_t *X, uint32_t len)
     __attribute__((regparm(3)));
void gen_lookup_ind_sse2(uint32_t *dst, const void *IND,
                         uint32_t len, uint64_t n) __attribute__((regparm(3)));
# define LI_STRUCT_SIZE 16
# define LI_VECTOR_LEN 4
# define init_lookup_ind init_lookup_ind_sse2
# define gen_lookup_ind gen_lookup_ind_sse2

#elif __i386__
/* Functions in lookup-i386.S */
void init_lookup_ind_i386(void *IND, const uint32_t *X, uint32_t len)
     __attribute__((regparm(3)));
void gen_lookup_ind_i386(uint32_t *dst, const void *IND,
                         uint32_t len, uint64_t n) __attribute__((regparm(3)));
# define LI_STRUCT_SIZE 16
# define LI_VECTOR_LEN 2
# define init_lookup_ind init_lookup_ind_i386
# define gen_lookup_ind gen_lookup_ind_i386

#elif __x86_64__ && USE_FPU_MULMOD
/* Functions in lookup-x87_64.S */
void init_lookup_ind_x87_64(void *IND, const uint32_t *X, uint32_t len);
void gen_lookup_ind_x87_64(uint32_t *dst, const void *IND,
                           uint32_t len, uint64_t n);
# define LI_STRUCT_SIZE 16
# define LI_VECTOR_LEN 4
# define init_lookup_ind init_lookup_ind_x87_64
# define gen_lookup_ind gen_lookup_ind_x87_64

#elif __x86_64__
/* Functions in lookup-x86_64.S */
void init_lookup_ind_x86_64(void *IND, const uint32_t *X, uint32_t len);
void gen_lookup_ind_x86_64(uint32_t *dst, const void *IND,
                           uint32_t len, uint64_t n);
# define LI_STRUCT_SIZE 12
# define LI_VECTOR_LEN 4
# define init_lookup_ind init_lookup_ind_x86_64
# define gen_lookup_ind gen_lookup_ind_x86_64
#endif

#endif /* HAVE_lookup_ind */

#endif /* _LOOKUP_IND_H */
