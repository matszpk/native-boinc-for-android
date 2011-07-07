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

#if USE_INLINE_MULMOD
/* Return a*b (mod p), where a,b < p < 2^63.
  This version should suit the useage in loop-generic.c: allowing GCC to see
  the calculation of 64-pShift should allow it to be moved out of the loop,
  and putting the result in a suits a = mulmod64(a,b,p).
*/
static inline uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t p)
{
  register uint64_t t0, t1, t2, t3, t4;

  asm ("mulld   %1, %5, %6"      "\n\t"
       "mulhdu  %2, %5, %6"      "\n\t"
       "mulhdu  %0, %1, %8"      "\n\t"
       "mulld   %3, %2, %8"      "\n\t"
       "mulhdu  %4, %2, %8"      "\n\t"
       "addc    %3, %0, %3"      "\n\t"
       "addze   %4, %4"          "\n\t"
       "srd     %3, %3, %9"      "\n\t"
       "sld     %4, %4, %10"     "\n\t"
       "or      %3, %3, %4"      "\n\t"
       "mulld   %3, %3, %7"      "\n\t"
       "sub     %5, %1, %3"      "\n\t"
       "cmpdi   cr6, %5, 0"      "\n\t"
       "bge+    cr6, 0f"         "\n\t"
       "add     %5, %5, %7"      "\n"
       "0:"
       : "=&r"(t0), "=&r"(t1), "=&r"(t2), "=&r"(t3), "=&r"(t4), "+r" (a)
       : "r" (b), "r" (p), "r" (pMagic), "r" (pShift), "r" (64-pShift)
       : "cr0", "cr6" );

  return a;
}
#else
/* Use the code in mulmod-ppc64.S
 */
extern uint64_t mulmod(uint64_t a, uint64_t b, uint64_t p, uint64_t magic,
                       uint64_t shift) attribute ((const));
#define mulmod64(a, b, p) mulmod(a, b, p, pMagic, pShift)
#endif /* USE_INLINE_MULMOD */

static inline void mod64_init(uint64_t p)
{
  getMagic(p);
}
static inline void mod64_fini(void) { }

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

/* Assign X[i+2] <-- X[i]*b (mod p), for 0 <= i < 2.
 */
#define VEC2_MULMOD64_NEXT(X,b,p) { \
  register uint64_t s0, s1, t0, t1; \
  asm ("mulhdu  %0, %10, %12"    "\n\t" \
       "mulld   %1, %11, %12"    "\n\t" \
       "mulhdu  %2, %11, %12"    "\n\t" \
       "mulhdu  %3, %10, %13"    "\n\t" \
       "mulld   %4, %11, %13"    "\n\t" \
       "mulhdu  %5, %11, %13"    "\n\t" \
       "addc    %1, %0, %1"      "\n\t" \
       "addze   %2, %2"          "\n\t" \
       "addc    %4, %3, %4"      "\n\t" \
       "addze   %5, %5"          "\n\t" \
       "srd     %1, %1, %8"      "\n\t" \
       "sld     %2, %2, %9"      "\n\t" \
       "srd     %4, %4, %8"      "\n\t" \
       "sld     %5, %5, %9"      "\n\t" \
       "or      %1, %1, %2"      "\n\t" \
       "or      %4, %4, %5"      "\n\t" \
       "mulld   %0, %12, %6"     "\n\t" \
       "mulld   %2, %1, %7"      "\n\t" \
       "mulld   %3, %13, %6"     "\n\t" \
       "mulld   %5, %4, %7"      "\n\t" \
       "sub     %0, %0, %2"      "\n\t" \
       "sub     %3, %3, %5"      "\n\t" \
       "cmpdi   cr6, %0, 0"      "\n\t" \
       "bge+    cr6, 0f"         "\n\t" \
       "add     %0, %0, %7"      "\n"   \
       "0:"                      "\t"   \
       "cmpdi   cr6, %3, 0"      "\n\t" \
       "bge+    cr6, 1f"         "\n\t" \
       "add     %3, %3, %7"      "\n"   \
       "1:" \
       : "=&r" ((X)[2]), "=&r" (s0), "=&r" (t0), \
         "=&r" ((X)[3]), "=&r" (s1), "=&r" (t1)  \
       : "r" (b), "r" (p), "r" (pShift), "r" (64-pShift), \
         "r" (bLO), "r" (bHI), "r" ((X)[0]), "r" ((X)[1]) \
       : "cr0", "cr6" ); \
}

/* Assign X[i+4] <-- X[i]*b (mod p), for 0 <= i < 4.
 */
#define VEC4_MULMOD64_NEXT(X,b,p) { \
  register uint64_t s0, s1, s2, s3, t0, t1, t2, t3; \
  asm ("mulhdu  %0, %16, %18"    "\n\t" \
       "mulld   %1, %17, %18"    "\n\t" \
       "mulhdu  %2, %17, %18"    "\n\t" \
       "mulhdu  %3, %16, %19"    "\n\t" \
       "mulld   %4, %17, %19"    "\n\t" \
       "mulhdu  %5, %17, %19"    "\n\t" \
       "mulhdu  %6, %16, %20"    "\n\t" \
       "mulld   %7, %17, %20"    "\n\t" \
       "mulhdu  %8, %17, %20"    "\n\t" \
       "mulhdu  %9, %16, %21"    "\n\t" \
       "mulld   %10, %17, %21"   "\n\t" \
       "mulhdu  %11, %17, %21"   "\n\t" \
       "addc    %1, %0, %1"      "\n\t" \
       "addze   %2, %2"          "\n\t" \
       "addc    %4, %3, %4"      "\n\t" \
       "addze   %5, %5"          "\n\t" \
       "addc    %7, %6, %7"      "\n\t" \
       "addze   %8, %8"          "\n\t" \
       "addc    %10, %9, %10"    "\n\t" \
       "addze   %11, %11"        "\n\t" \
       "srd     %1, %1, %14"     "\n\t" \
       "sld     %2, %2, %15"     "\n\t" \
       "srd     %4, %4, %14"     "\n\t" \
       "sld     %5, %5, %15"     "\n\t" \
       "srd     %7, %7, %14"     "\n\t" \
       "sld     %8, %8, %15"     "\n\t" \
       "srd     %10, %10, %14"   "\n\t" \
       "sld     %11, %11, %15"   "\n\t" \
       "or      %1, %1, %2"      "\n\t" \
       "or      %4, %4, %5"      "\n\t" \
       "or      %7, %7, %8"      "\n\t" \
       "or      %10, %10, %11"   "\n\t" \
       "mulld   %0, %18, %12"    "\n\t" \
       "mulld   %2, %1, %13"     "\n\t" \
       "mulld   %3, %19, %12"    "\n\t" \
       "mulld   %5, %4, %13"     "\n\t" \
       "mulld   %6, %20, %12"    "\n\t" \
       "mulld   %8, %7, %13"     "\n\t" \
       "mulld   %9, %21, %12"    "\n\t" \
       "mulld   %11, %10, %13"   "\n\t" \
       "sub     %0, %0, %2"      "\n\t" \
       "sub     %3, %3, %5"      "\n\t" \
       "sub     %6, %6, %8"      "\n\t" \
       "sub     %9, %9, %11"     "\n\t" \
       "cmpdi   cr6, %0, 0"      "\n\t" \
       "bge+    cr6, 0f"         "\n\t" \
       "add     %0, %0, %13"     "\n"   \
       "0:"                      "\t"   \
       "cmpdi   cr6, %3, 0"      "\n\t" \
       "bge+    cr6, 1f"         "\n\t" \
       "add     %3, %3, %13"     "\n"   \
       "1:"                      "\t"   \
       "cmpdi   cr6, %6, 0"      "\n\t" \
       "bge+    cr6, 2f"         "\n\t" \
       "add     %6, %6, %13"     "\n"   \
       "2:"                      "\t"   \
       "cmpdi   cr6, %9, 0"      "\n\t" \
       "bge+    cr6, 3f"         "\n\t" \
       "add     %9, %9, %13"     "\n"   \
       "3:" \
       : "=&r" ((X)[4]), "=&r" (s0), "=&r" (t0), \
         "=&r" ((X)[5]), "=&r" (s1), "=&r" (t1), \
         "=&r" ((X)[6]), "=&r" (s2), "=&r" (t2), \
         "=&r" ((X)[7]), "=&r" (s3), "=&r" (t3) \
       : "r" (b), "r" (p), "r" (pShift), "r" (64-pShift), "r"(bLO), "r"(bHI), \
         "r" ((X)[0]), "r" ((X)[1]), "r" ((X)[2]), "r" ((X)[3]) \
       : "cr0", "cr6" ); \
}

/* Assign X[i+8] <-- X[i]*b (mod p), for 0 <= i < 8.
 */
#define VEC8_MULMOD64_NEXT(X,b,p) { \
  register uint64_t s0, s1, s2, s3, t0, t1, t2, t3; \
  asm ("mulhdu  %0, %16, %18"    "\n\t" \
       "mulld   %1, %17, %18"    "\n\t" \
       "mulhdu  %2, %17, %18"    "\n\t" \
       "mulhdu  %3, %16, %19"    "\n\t" \
       "mulld   %4, %17, %19"    "\n\t" \
       "mulhdu  %5, %17, %19"    "\n\t" \
       "mulhdu  %6, %16, %20"    "\n\t" \
       "mulld   %7, %17, %20"    "\n\t" \
       "mulhdu  %8, %17, %20"    "\n\t" \
       "mulhdu  %9, %16, %21"    "\n\t" \
       "mulld   %10, %17, %21"   "\n\t" \
       "mulhdu  %11, %17, %21"   "\n\t" \
       "addc    %1, %0, %1"      "\n\t" \
       "addze   %2, %2"          "\n\t" \
       "addc    %4, %3, %4"      "\n\t" \
       "addze   %5, %5"          "\n\t" \
       "addc    %7, %6, %7"      "\n\t" \
       "addze   %8, %8"          "\n\t" \
       "addc    %10, %9, %10"    "\n\t" \
       "addze   %11, %11"        "\n\t" \
       "srd     %1, %1, %14"     "\n\t" \
       "sld     %2, %2, %15"     "\n\t" \
       "srd     %4, %4, %14"     "\n\t" \
       "sld     %5, %5, %15"     "\n\t" \
       "srd     %7, %7, %14"     "\n\t" \
       "sld     %8, %8, %15"     "\n\t" \
       "srd     %10, %10, %14"   "\n\t" \
       "sld     %11, %11, %15"   "\n\t" \
       "or      %1, %1, %2"      "\n\t" \
       "or      %4, %4, %5"      "\n\t" \
       "or      %7, %7, %8"      "\n\t" \
       "or      %10, %10, %11"   "\n\t" \
       "mulld   %0, %18, %12"    "\n\t" \
       "mulld   %2, %1, %13"     "\n\t" \
       "mulld   %3, %19, %12"    "\n\t" \
       "mulld   %5, %4, %13"     "\n\t" \
       "mulld   %6, %20, %12"    "\n\t" \
       "mulld   %8, %7, %13"     "\n\t" \
       "mulld   %9, %21, %12"    "\n\t" \
       "mulld   %11, %10, %13"   "\n\t" \
       "sub     %0, %0, %2"      "\n\t" \
       "sub     %3, %3, %5"      "\n\t" \
       "sub     %6, %6, %8"      "\n\t" \
       "sub     %9, %9, %11"     "\n\t" \
       "cmpdi   cr6, %0, 0"      "\n\t" \
       "bge+    cr6, 0f"         "\n\t" \
       "add     %0, %0, %13"     "\n"   \
       "0:"                      "\t"   \
       "cmpdi   cr6, %3, 0"      "\n\t" \
       "bge+    cr6, 1f"         "\n\t" \
       "add     %3, %3, %13"     "\n"   \
       "1:"                      "\t"   \
       "cmpdi   cr6, %6, 0"      "\n\t" \
       "bge+    cr6, 2f"         "\n\t" \
       "add     %6, %6, %13"     "\n"   \
       "2:"                      "\t"   \
       "cmpdi   cr6, %9, 0"      "\n\t" \
       "bge+    cr6, 3f"         "\n\t" \
       "add     %9, %9, %13"     "\n"   \
       "3:" \
       : "=&r" ((X)[8]), "=&r" (s0), "=&r" (t0), \
         "=&r" ((X)[9]), "=&r" (s1), "=&r" (t1), \
         "=&r" ((X)[10]), "=&r" (s2), "=&r" (t2), \
         "=&r" ((X)[11]), "=&r" (s3), "=&r" (t3) \
       : "r" (b), "r" (p), "r" (pShift), "r" (64-pShift), "r"(bLO), "r"(bHI), \
         "r" ((X)[0]), "r" ((X)[1]), "r" ((X)[2]), "r" ((X)[3]) \
       : "cr0", "cr6" ); \
 \
  asm ("mulhdu  %0, %16, %18"    "\n\t" \
       "mulld   %1, %17, %18"    "\n\t" \
       "mulhdu  %2, %17, %18"    "\n\t" \
       "mulhdu  %3, %16, %19"    "\n\t" \
       "mulld   %4, %17, %19"    "\n\t" \
       "mulhdu  %5, %17, %19"    "\n\t" \
       "mulhdu  %6, %16, %20"    "\n\t" \
       "mulld   %7, %17, %20"    "\n\t" \
       "mulhdu  %8, %17, %20"    "\n\t" \
       "mulhdu  %9, %16, %21"    "\n\t" \
       "mulld   %10, %17, %21"   "\n\t" \
       "mulhdu  %11, %17, %21"   "\n\t" \
       "addc    %1, %0, %1"      "\n\t" \
       "addze   %2, %2"          "\n\t" \
       "addc    %4, %3, %4"      "\n\t" \
       "addze   %5, %5"          "\n\t" \
       "addc    %7, %6, %7"      "\n\t" \
       "addze   %8, %8"          "\n\t" \
       "addc    %10, %9, %10"    "\n\t" \
       "addze   %11, %11"        "\n\t" \
       "srd     %1, %1, %14"     "\n\t" \
       "sld     %2, %2, %15"     "\n\t" \
       "srd     %4, %4, %14"     "\n\t" \
       "sld     %5, %5, %15"     "\n\t" \
       "srd     %7, %7, %14"     "\n\t" \
       "sld     %8, %8, %15"     "\n\t" \
       "srd     %10, %10, %14"   "\n\t" \
       "sld     %11, %11, %15"   "\n\t" \
       "or      %1, %1, %2"      "\n\t" \
       "or      %4, %4, %5"      "\n\t" \
       "or      %7, %7, %8"      "\n\t" \
       "or      %10, %10, %11"   "\n\t" \
       "mulld   %0, %18, %12"    "\n\t" \
       "mulld   %2, %1, %13"     "\n\t" \
       "mulld   %3, %19, %12"    "\n\t" \
       "mulld   %5, %4, %13"     "\n\t" \
       "mulld   %6, %20, %12"    "\n\t" \
       "mulld   %8, %7, %13"     "\n\t" \
       "mulld   %9, %21, %12"    "\n\t" \
       "mulld   %11, %10, %13"   "\n\t" \
       "sub     %0, %0, %2"      "\n\t" \
       "sub     %3, %3, %5"      "\n\t" \
       "sub     %6, %6, %8"      "\n\t" \
       "sub     %9, %9, %11"     "\n\t" \
       "cmpdi   cr6, %0, 0"      "\n\t" \
       "bge+    cr6, 0f"         "\n\t" \
       "add     %0, %0, %13"     "\n"   \
       "0:"                      "\t"   \
       "cmpdi   cr6, %3, 0"      "\n\t" \
       "bge+    cr6, 1f"         "\n\t" \
       "add     %3, %3, %13"     "\n"   \
       "1:"                      "\t"   \
       "cmpdi   cr6, %6, 0"      "\n\t" \
       "bge+    cr6, 2f"         "\n\t" \
       "add     %6, %6, %13"     "\n"   \
       "2:"                      "\t"   \
       "cmpdi   cr6, %9, 0"      "\n\t" \
       "bge+    cr6, 3f"         "\n\t" \
       "add     %9, %9, %13"     "\n"   \
       "3:" \
       : "=&r" ((X)[12]), "=&r" (s0), "=&r" (t0), \
         "=&r" ((X)[13]), "=&r" (s1), "=&r" (t1), \
         "=&r" ((X)[14]), "=&r" (s2), "=&r" (t2), \
         "=&r" ((X)[15]), "=&r" (s3), "=&r" (t3) \
       : "r" (b), "r" (p), "r" (pShift), "r" (64-pShift), "r"(bLO), "r"(bHI), \
         "r" ((X)[4]), "r" ((X)[5]), "r" ((X)[6]), "r" ((X)[7]) \
       : "cr0", "cr6" ); \
}
#endif /* NEED_vector && !HAVE_vector */
