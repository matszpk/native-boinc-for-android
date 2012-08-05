#include <stdint.h>
#include "arithmetic.h"

static uint64_t mulmod_b, mulmod_p;

void vec_mulmod64_initp_ppc64(uint64_t p)
{
  mulmod_p = p;
  mod64_init(p);
}

void vec_mulmod64_initb_ppc64(uint64_t b)
{
  mulmod_b = b;
  PRE2_MULMOD64_INIT(b);
}

void vec2_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count)
{
  register uint64_t s0, s1, t0, t1;
  register uint64_t b, p, b_lo, b_hi, p_lshift, p_rshift;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  b_lo = bLO;
  b_hi = bHI;
  p_lshift = pShift;
  p_rshift = 64-pShift;

  for (i = 0; i < count; i += 2)
    asm ("mulhdu  %0, %10, %12"    "\n\t"
         "mulld   %1, %11, %12"    "\n\t"
         "mulhdu  %2, %11, %12"    "\n\t"
         "mulhdu  %3, %10, %13"    "\n\t"
         "mulld   %4, %11, %13"    "\n\t"
         "mulhdu  %5, %11, %13"    "\n\t"
         "addc    %1, %0, %1"      "\n\t"
         "addze   %2, %2"          "\n\t"
         "addc    %4, %3, %4"      "\n\t"
         "addze   %5, %5"          "\n\t"
         "srd     %1, %1, %8"      "\n\t"
         "sld     %2, %2, %9"      "\n\t"
         "srd     %4, %4, %8"      "\n\t"
         "sld     %5, %5, %9"      "\n\t"
         "or      %1, %1, %2"      "\n\t"
         "or      %4, %4, %5"      "\n\t"
         "mulld   %0, %12, %6"     "\n\t"
         "mulld   %2, %1, %7"      "\n\t"
         "mulld   %3, %13, %6"     "\n\t"
         "mulld   %5, %4, %7"      "\n\t"
         "sub     %0, %0, %2"      "\n\t"
         "sub     %3, %3, %5"      "\n\t"
         "cmpdi   cr6, %0, 0"      "\n\t"
         "bge+    cr6, 0f"         "\n\t"
         "add     %0, %0, %7"      "\n"
         "0:"                      "\t"
         "cmpdi   cr6, %3, 0"      "\n\t"
         "bge+    cr6, 1f"         "\n\t"
         "add     %3, %3, %7"      "\n"
         "1:"
         : "=&r" (Y[i+0]), "=&r" (s0), "=&r" (t0),
           "=&r" (Y[i+1]), "=&r" (s1), "=&r" (t1)
         : "r" (b), "r" (p), "r" (p_lshift), "r" (p_rshift),
           "r" (b_lo), "r" (b_hi), "r" (X[i+0]), "r" (X[i+1])
         : "cr0", "cr6" );
}

void vec4_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count)
{
  register uint64_t s0, s1, s2, s3, t0, t1, t2, t3;
  register uint64_t b, p, b_lo, b_hi, p_lshift, p_rshift;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  b_lo = bLO;
  b_hi = bHI;
  p_lshift = pShift;
  p_rshift = 64-pShift;

  for (i = 0; i < count; i += 4)
    asm ("mulhdu  %0, %16, %18"    "\n\t"
         "mulld   %1, %17, %18"    "\n\t"
         "mulhdu  %2, %17, %18"    "\n\t"
         "mulhdu  %3, %16, %19"    "\n\t"
         "mulld   %4, %17, %19"    "\n\t"
         "mulhdu  %5, %17, %19"    "\n\t"
         "mulhdu  %6, %16, %20"    "\n\t"
         "mulld   %7, %17, %20"    "\n\t"
         "mulhdu  %8, %17, %20"    "\n\t"
         "mulhdu  %9, %16, %21"    "\n\t"
         "mulld   %10, %17, %21"   "\n\t"
         "mulhdu  %11, %17, %21"   "\n\t"
         "addc    %1, %0, %1"      "\n\t"
         "addze   %2, %2"          "\n\t"
         "addc    %4, %3, %4"      "\n\t"
         "addze   %5, %5"          "\n\t"
         "addc    %7, %6, %7"      "\n\t"
         "addze   %8, %8"          "\n\t"
         "addc    %10, %9, %10"    "\n\t"
         "addze   %11, %11"        "\n\t"
         "srd     %1, %1, %14"     "\n\t"
         "sld     %2, %2, %15"     "\n\t"
         "srd     %4, %4, %14"     "\n\t"
         "sld     %5, %5, %15"     "\n\t"
         "srd     %7, %7, %14"     "\n\t"
         "sld     %8, %8, %15"     "\n\t"
         "srd     %10, %10, %14"   "\n\t"
         "sld     %11, %11, %15"   "\n\t"
         "or      %1, %1, %2"      "\n\t"
         "or      %4, %4, %5"      "\n\t"
         "or      %7, %7, %8"      "\n\t"
         "or      %10, %10, %11"   "\n\t"
         "mulld   %0, %18, %12"    "\n\t"
         "mulld   %2, %1, %13"     "\n\t"
         "mulld   %3, %19, %12"    "\n\t"
         "mulld   %5, %4, %13"     "\n\t"
         "mulld   %6, %20, %12"    "\n\t"
         "mulld   %8, %7, %13"     "\n\t"
         "mulld   %9, %21, %12"    "\n\t"
         "mulld   %11, %10, %13"   "\n\t"
         "sub     %0, %0, %2"      "\n\t"
         "sub     %3, %3, %5"      "\n\t"
         "sub     %6, %6, %8"      "\n\t"
         "sub     %9, %9, %11"     "\n\t"
         "cmpdi   cr6, %0, 0"      "\n\t"
         "bge+    cr6, 0f"         "\n\t"
         "add     %0, %0, %13"     "\n"
         "0:"                      "\t"
         "cmpdi   cr6, %3, 0"      "\n\t"
         "bge+    cr6, 1f"         "\n\t"
         "add     %3, %3, %13"     "\n"
         "1:"                      "\t"
         "cmpdi   cr6, %6, 0"      "\n\t"
         "bge+    cr6, 2f"         "\n\t"
         "add     %6, %6, %13"     "\n"
         "2:"                      "\t"
         "cmpdi   cr6, %9, 0"      "\n\t"
         "bge+    cr6, 3f"         "\n\t"
         "add     %9, %9, %13"     "\n"
         "3:"
         : "=&r" (Y[i+0]), "=&r" (s0), "=&r" (t0),
           "=&r" (Y[i+1]), "=&r" (s1), "=&r" (t1),
           "=&r" (Y[i+2]), "=&r" (s2), "=&r" (t2),
           "=&r" (Y[i+3]), "=&r" (s3), "=&r" (t3)
         : "r" (b), "r" (p),
           "r" (p_lshift), "r" (p_rshift), "r"(b_lo), "r"(b_hi),
           "r" (X[i+0]), "r" (X[i+1]), "r" (X[i+2]), "r" (X[i+3])
         : "cr0", "cr6" );
}

void vec8_mulmod64_ppc64(const uint64_t *X, uint64_t *Y, int count)
{
  register uint64_t s0, s1, s2, s3, t0, t1, t2, t3;
  register uint64_t b, p, b_lo, b_hi, p_lshift, p_rshift;
  int i;

  b = mulmod_b;
  p = mulmod_p;
  b_lo = bLO;
  b_hi = bHI;
  p_lshift = pShift;
  p_rshift = 64-pShift;

  for (i = 0; i < count; i += 8)
  {
    asm ("mulhdu  %0, %16, %18"    "\n\t"
         "mulld   %1, %17, %18"    "\n\t"
         "mulhdu  %2, %17, %18"    "\n\t"
         "mulhdu  %3, %16, %19"    "\n\t"
         "mulld   %4, %17, %19"    "\n\t"
         "mulhdu  %5, %17, %19"    "\n\t"
         "mulhdu  %6, %16, %20"    "\n\t"
         "mulld   %7, %17, %20"    "\n\t"
         "mulhdu  %8, %17, %20"    "\n\t"
         "mulhdu  %9, %16, %21"    "\n\t"
         "mulld   %10, %17, %21"   "\n\t"
         "mulhdu  %11, %17, %21"   "\n\t"
         "addc    %1, %0, %1"      "\n\t"
         "addze   %2, %2"          "\n\t"
         "addc    %4, %3, %4"      "\n\t"
         "addze   %5, %5"          "\n\t"
         "addc    %7, %6, %7"      "\n\t"
         "addze   %8, %8"          "\n\t"
         "addc    %10, %9, %10"    "\n\t"
         "addze   %11, %11"        "\n\t"
         "srd     %1, %1, %14"     "\n\t"
         "sld     %2, %2, %15"     "\n\t"
         "srd     %4, %4, %14"     "\n\t"
         "sld     %5, %5, %15"     "\n\t"
         "srd     %7, %7, %14"     "\n\t"
         "sld     %8, %8, %15"     "\n\t"
         "srd     %10, %10, %14"   "\n\t"
         "sld     %11, %11, %15"   "\n\t"
         "or      %1, %1, %2"      "\n\t"
         "or      %4, %4, %5"      "\n\t"
         "or      %7, %7, %8"      "\n\t"
         "or      %10, %10, %11"   "\n\t"
         "mulld   %0, %18, %12"    "\n\t"
         "mulld   %2, %1, %13"     "\n\t"
         "mulld   %3, %19, %12"    "\n\t"
         "mulld   %5, %4, %13"     "\n\t"
         "mulld   %6, %20, %12"    "\n\t"
         "mulld   %8, %7, %13"     "\n\t"
         "mulld   %9, %21, %12"    "\n\t"
         "mulld   %11, %10, %13"   "\n\t"
         "sub     %0, %0, %2"      "\n\t"
         "sub     %3, %3, %5"      "\n\t"
         "sub     %6, %6, %8"      "\n\t"
         "sub     %9, %9, %11"     "\n\t"
         "cmpdi   cr6, %0, 0"      "\n\t"
         "bge+    cr6, 0f"         "\n\t"
         "add     %0, %0, %13"     "\n"
         "0:"                      "\t"
         "cmpdi   cr6, %3, 0"      "\n\t"
         "bge+    cr6, 1f"         "\n\t"
         "add     %3, %3, %13"     "\n"
         "1:"                      "\t"
         "cmpdi   cr6, %6, 0"      "\n\t"
         "bge+    cr6, 2f"         "\n\t"
         "add     %6, %6, %13"     "\n"
         "2:"                      "\t"
         "cmpdi   cr6, %9, 0"      "\n\t"
         "bge+    cr6, 3f"         "\n\t"
         "add     %9, %9, %13"     "\n"
         "3:"
         : "=&r" (Y[i+0]), "=&r" (s0), "=&r" (t0),
           "=&r" (Y[i+1]), "=&r" (s1), "=&r" (t1),
           "=&r" (Y[i+2]), "=&r" (s2), "=&r" (t2),
           "=&r" (Y[i+3]), "=&r" (s3), "=&r" (t3)
         : "r" (b), "r" (p),
           "r" (p_lshift), "r" (p_rshift), "r"(b_lo), "r"(b_hi),
           "r" (X[i+0]), "r" (X[i+1]), "r" (X[i+2]), "r" (X[i+3])
         : "cr0", "cr6" );

    asm ("mulhdu  %0, %16, %18"    "\n\t"
         "mulld   %1, %17, %18"    "\n\t"
         "mulhdu  %2, %17, %18"    "\n\t"
         "mulhdu  %3, %16, %19"    "\n\t"
         "mulld   %4, %17, %19"    "\n\t"
         "mulhdu  %5, %17, %19"    "\n\t"
         "mulhdu  %6, %16, %20"    "\n\t"
         "mulld   %7, %17, %20"    "\n\t"
         "mulhdu  %8, %17, %20"    "\n\t"
         "mulhdu  %9, %16, %21"    "\n\t"
         "mulld   %10, %17, %21"   "\n\t"
         "mulhdu  %11, %17, %21"   "\n\t"
         "addc    %1, %0, %1"      "\n\t"
         "addze   %2, %2"          "\n\t"
         "addc    %4, %3, %4"      "\n\t"
         "addze   %5, %5"          "\n\t"
         "addc    %7, %6, %7"      "\n\t"
         "addze   %8, %8"          "\n\t"
         "addc    %10, %9, %10"    "\n\t"
         "addze   %11, %11"        "\n\t"
         "srd     %1, %1, %14"     "\n\t"
         "sld     %2, %2, %15"     "\n\t"
         "srd     %4, %4, %14"     "\n\t"
         "sld     %5, %5, %15"     "\n\t"
         "srd     %7, %7, %14"     "\n\t"
         "sld     %8, %8, %15"     "\n\t"
         "srd     %10, %10, %14"   "\n\t"
         "sld     %11, %11, %15"   "\n\t"
         "or      %1, %1, %2"      "\n\t"
         "or      %4, %4, %5"      "\n\t"
         "or      %7, %7, %8"      "\n\t"
         "or      %10, %10, %11"   "\n\t"
         "mulld   %0, %18, %12"    "\n\t"
         "mulld   %2, %1, %13"     "\n\t"
         "mulld   %3, %19, %12"    "\n\t"
         "mulld   %5, %4, %13"     "\n\t"
         "mulld   %6, %20, %12"    "\n\t"
         "mulld   %8, %7, %13"     "\n\t"
         "mulld   %9, %21, %12"    "\n\t"
         "mulld   %11, %10, %13"   "\n\t"
         "sub     %0, %0, %2"      "\n\t"
         "sub     %3, %3, %5"      "\n\t"
         "sub     %6, %6, %8"      "\n\t"
         "sub     %9, %9, %11"     "\n\t"
         "cmpdi   cr6, %0, 0"      "\n\t"
         "bge+    cr6, 0f"         "\n\t"
         "add     %0, %0, %13"     "\n"
         "0:"                      "\t"
         "cmpdi   cr6, %3, 0"      "\n\t"
         "bge+    cr6, 1f"         "\n\t"
         "add     %3, %3, %13"     "\n"
         "1:"                      "\t"
         "cmpdi   cr6, %6, 0"      "\n\t"
         "bge+    cr6, 2f"         "\n\t"
         "add     %6, %6, %13"     "\n"
         "2:"                      "\t"
         "cmpdi   cr6, %9, 0"      "\n\t"
         "bge+    cr6, 3f"         "\n\t"
         "add     %9, %9, %13"     "\n"
         "3:"
         : "=&r" (Y[i+4]), "=&r" (s0), "=&r" (t0),
           "=&r" (Y[i+5]), "=&r" (s1), "=&r" (t1),
           "=&r" (Y[i+6]), "=&r" (s2), "=&r" (t2),
           "=&r" (Y[i+7]), "=&r" (s3), "=&r" (t3)
         : "r" (b), "r" (p),
           "r" (p_lshift), "r" (p_rshift), "r"(b_lo), "r"(b_hi),
           "r" (X[i+4]), "r" (X[i+5]), "r" (X[i+6]), "r" (X[i+7])
         : "cr0", "cr6" );
  }
}
