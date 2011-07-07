// expmod-ppc64.s -- (C) 2006 Mark Rodenkirch.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// Calculate base^exponent%prime

#include "config.h"

#if NEED_UNDERSCORE
#define expmod _expmod
#endif

   .text
   .globl expmod

expmod:
    // r3 = base
    // r4 = exponent
    // r5 = prime
    // r6 = magic number
    // r7 = shift

    std   r27, -40(r1)
    std   r28, -48(r1)
    std   r29, -56(r1)
#ifndef __APPLE__
    std   r2, -64(r1)
#endif

    li    r12, 64
    mr    r11, r7
    sub   r12, r12, r7
    
    li    r2, 1
.expmod_loop:
    andi. r0, r4, 1
    beq-  cr0, .square

    mulld  r7, r2, r3    // r8 = lower(a * b)
    mulhdu r8, r2, r3    // r9 = upper(a * b)

    // multiply (a * b) by the magic number
    mulhdu r27, r7, r6
    mulld  r28, r8, r6
    mulhdu r29, r8, r6

    // Get the result which will then be shifted to get the quotient
    addc  r9, r27, r28
    addze r10, r29

    srd   r9, r9, r11
    sld   r10, r10, r12

    // Now get the quotient
    or    r9, r9, r10       // r8 = (a * b) / p

    mulld r9, r9, r5

    // Now get the remainders
    sub   r2, r7, r9        // r2 = (a * b) % p

    cmpdi cr6, r2, 0
    bge+  cr6, .square
    add   r2, r2, r5

.square:
    srdi. r4, r4, 1
    beq-  cr0, .expmod_exit

    mulld  r7, r3, r3    // r8 = lower(b * b)
    mulhdu r8, r3, r3    // r9 = upper(b * b)

    // multiply (a * b) by the magic number
    mulhdu r27, r7, r6
    mulld  r28, r8, r6
    mulhdu r29, r8, r6

    // Get the result which will then be shifted to get the quotient
    addc  r9, r27, r28
    addze r10, r29

    srd   r9, r9, r11
    sld   r10, r10, r12

    // Now get the quotient
    or    r9, r9, r10       // r8 = (b * b) / p

    mulld r9, r9, r5

    // Now get the remainders
    sub   r3, r7, r9        // r3 = (b * b) % p

    cmpdi cr6, r3, 0
    bge+  cr6, .expmod_loop
    add   r3, r3, r5

    b     .expmod_loop

.expmod_exit:
    mr    r3, r2
    ld    r27, -40(r1)
    ld    r28, -48(r1)
    ld    r29, -56(r1)
#ifndef __APPLE__
    ld    r2, -64(r1)
#endif
    blr
