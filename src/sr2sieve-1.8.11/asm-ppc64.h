/* asm-ppc64.h -- (C) 2006 Mark Rodenkirch, 2007 Geoffrey Reynolds.

   PPC assember mulmod, powmod routines.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#if defined(NEED_mulmod64) && !defined(HAVE_mulmod64)
#define HAVE_mulmod64

#define MOD64_MAX_BITS 63

/* These global variables are declared unsigned long long int instead of
   uint64_t so that GCC will be able (when -fstrict-aliasing is active) to
   recognise that they cannot alias elements of arrays of uint64_t. This is
   a nasty kludge, but I cannot see any other way to make GCC aware of this
   fact. Without it, these variables are not recognised as loop invariants
   when the loop modifies an element of a uint64_t array, which most of the
   critical loops do.

   This problem doesn't currently arise for other architectures because
   their corrsponding global variables are float or vector types.
*/
extern unsigned long long int pMagic;
extern unsigned long long int pShift;
extern unsigned long long int bLO;
extern unsigned long long int bHI;

extern void getMagic(uint64_t d);

#define mod64_init(p) { getMagic(p); }
#define mod64_fini() { }

#if USE_INLINE_MULMOD
/* Return a*b (mod p), where a,b < p < 2^63.
 */
static inline uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t p)
{
  register uint64_t ret, t0, t1, t2, t3, t4;

  asm ("li      %0, 64"          "\n\t"
       "sub     %0, %0, %10"     "\n\t"
       "mulld   %2, %6, %7"      "\n\t"
       "mulhdu  %3, %6, %7"      "\n\t"
       "mulhdu  %1, %2, %9"      "\n\t"
       "mulld   %4, %3, %9"      "\n\t"
       "mulhdu  %5, %3, %9"      "\n\t"
       "addc    %4, %1, %4"      "\n\t"
       "addze   %5, %5"          "\n\t"
       "srd     %4, %4, %10"     "\n\t"
       "sld     %5, %5, %0"      "\n\t"
       "or      %4, %4, %5"      "\n\t"
       "mulld   %4, %4, %8"      "\n\t"
       "sub     %0, %2, %4"      "\n\t"
       "cmpdi   cr6, %0, 0"      "\n\t"
       "bge+    cr6, 0f"         "\n\t"
       "add     %0, %0, %8"      "\n"
       "0:"
       : "=&r" (ret), "=&r"(t0), "=&r"(t1), "=&r"(t2), "=&r"(t3), "=&r"(t4)
       : "r" (a), "r" (b), "r" (p), "r" (pMagic), "r" (pShift)
       : "cr0", "cr6" );

  return ret;
}
#else
/* Use the code in mulmod-ppc64.S
 */
extern uint64_t mulmod(uint64_t a, uint64_t b, uint64_t p, uint64_t magic,
                       uint64_t shift) attribute ((const));
#define mulmod64(a, b, p) mulmod(a, b, p, pMagic, pShift)
#endif /* USE_INLINE_MULMOD */


/* Return a*b (mod p), where a,b < p < 2^63 and bHI:bLO = b*pMagic.
 */
#define PRE2_MULMOD64(a,b,p) ({ \
  register uint64_t ret, s, t; \
  asm ("mulhdu  %0, %8, %3"    "\n\t" \
       "mulld   %1, %9, %3"    "\n\t" \
       "mulhdu  %2, %9, %3"    "\n\t" \
       "addc    %1, %0, %1"    "\n\t" \
       "addze   %2, %2"        "\n\t" \
       "srd     %1, %1, %6"    "\n\t" \
       "sld     %2, %2, %7"    "\n\t" \
       "or      %1, %1, %2"    "\n\t" \
       "mulld   %0, %3, %4"    "\n\t" \
       "mulld   %2, %1, %5"    "\n\t" \
       "sub     %0, %0, %2"    "\n\t" \
       "cmpdi   cr6, %0, 0"    "\n\t" \
       "bge+    cr6, 0f"       "\n\t" \
       "add     %0, %0, %5"    "\n"   \
       "0:" \
       : "=&r" (ret), "=&r" (s), "=&r" (t) \
       : "r" (a), "r" (b), "r" (p), "r" (pShift), "r" (64-pShift), \
         "r" (bLO), "r" (bHI) \
       : "cr0", "cr6" ); \
  ret; })

/* Calculate the 128-bit product b * pMagic
 */
#define PRE2_MULMOD64_INIT(b) { \
  asm ("mulld   %0, %2, %3"    "\n\t" \
       "mulhdu  %1, %2, %3" \
       : "=&r" (bLO), "=r" (bHI) : "r" (b), "r" (pMagic) : "cr0" ); \
}

/* Nothing needed.
 */
#define PRE2_MULMOD64_FINI() { }
#endif /* NEED_mulmod64 && !HAVE_mulmod64 */


#if defined(NEED_powmod64) && !defined(HAVE_powmod64)
#define HAVE_powmod64

/* Return a^b (mod p), where a < p < 2^63.
 */
extern uint64_t expmod(uint64_t a, uint64_t b, uint64_t p, uint64_t magic,
                       uint64_t shift) attribute ((const));
#define powmod64(b, e, p) expmod(b, e, p, pMagic, pShift)
#endif /* NEED_powmod64 && !HAVE_powmod64 */


#if defined(NEED_vector) && !defined(HAVE_vector)
#define HAVE_vector

/* in mulmod-ppc64.c */
void vec_mulmod64_initp_ppc64(uint64_t p);
static void vec_mulmod64_finip_ppc64(void) { mod64_fini(); };
#define vec_mulmod64_initp(p) { vec_mulmod64_initp_ppc64(p); }
#define vec_mulmod64_finip() { vec_mulmod64_finip_ppc64(); }

void vec_mulmod64_initb_ppc64(uint64_t b);
static inline void vec_mulmod64_finib_ppc64(void)
{
  PRE2_MULMOD64_FINI();
}
void vec2_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count);
void vec4_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count);
void vec8_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count);

#define vec_mulmod64_initb(b) vec_mulmod64_initb_ppc64(b);
#define vec_mulmod64_finib() vec_mulmod64_finib_ppc64();
#define vec2_mulmod64 vec2_mulmod64_ppc64
#define vec4_mulmod64 vec4_mulmod64_ppc64
#define vec8_mulmod64 vec8_mulmod64_ppc64

#endif /* NEED_vector && !HAVE_vector */
