// mulmod-ppc64.s -- (C) 2006,2007 Mark Rodenkirch.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

#include "config.h"

#if NEED_UNDERSCORE
#define mulmod _mulmod
#endif

// Calculate a*b%prime
    .text
    .globl mulmod

mulmod:
     // r3 = a
     // r4 = b
     // r5 = prime
     // r6 = magic number
     // r7 = shift

     li    r12, 64
     mr    r11, r7
     sub   r12, r12, r7

     mulld  r7, r3, r4    // r8 = lower(a * b)
     mulhdu r8, r3, r4    // r9 = upper(a * b)

     // multiply (a * b) by the magic number
     mulhdu r3, r7, r6
     mulld  r9, r8, r6
     mulhdu r10, r8, r6

     // Get the result which will then be shifted to get the quotient
     addc  r9, r3, r9
     addze r10, r10

     srd   r9, r9, r11
     sld   r10, r10, r12

     // Now get the quotient
     or    r9, r9, r10       // r8 = (a * b) / p

     mulld r9, r9, r5

     // Now get the remainders
     sub   r3, r7, r9        // r3 = (a * b) % p

     cmpdi cr6, r3, 0
     bge+  cr6, _mulmod_exit
     add   r3, r3, r5

_mulmod_exit:
     blr
