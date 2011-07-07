/* mulmod-i386.S -- (C) Geoffrey Reynolds, November 2006.

   Modular multiplication for x86.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define FPU_MODE_BITS (0x0F00)	/* 64-bit precision, round to zero. */


/* void mod64_init_i386(uint64_t p);

     Sets FPU to double extended precision and round-to-zero.
     Pushes 1.0/p (computed with new settings) onto the FPU stack.
     Saves old FPU precision and rounding status in mod64_rnd.


   void mod64_fini_i386(void);

     Restores saved FPU precision/rounding settings, pops the FPU stack.


   uint64_t mulmod64_i386(uint64_t a, uint64_t b, uint64_t p);

     returns a*b (mod p), where a,b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes stack is 8-aligned.
*/


#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN32 1
#define UNALIGNED_STACK 1
#endif

#if NEED_UNDERSCORE
#define mod64_rnd _mod64_rnd
#define mod64_init_i386 _mod64_init_i386
#define mod64_fini_i386 _mod64_fini_i386
#define mulmod64_i386 _mulmod64_i386
#endif

	.globl	mod64_rnd


	.text

	.globl	mod64_init_i386
	.p2align 4,,7

mod64_init_i386:
	sub	$4, %esp

	fildll	8(%esp)			/* p */
	fld1				/* 1.0, p */

	fnstcw	(%esp)
	mov	(%esp), %ax
	orw	$FPU_MODE_BITS, (%esp)	/* Set new mode bits */
	mov	%ax, mod64_rnd		/* Save old mode bits */
	fldcw   (%esp)

	fdivp   %st(0), %st(1)		/* 1.0/p, using new mode. */

	add	$4, %esp
	ret


	.globl	mod64_fini_i386
	.p2align 4,,7

mod64_fini_i386:
	sub	$4, %esp

	fstp	%st(0)			/* Pop 1.0/p */
	fnstcw	(%esp)
	mov	mod64_rnd, %ax
	and	$FPU_MODE_BITS, %ax
	andw	$~FPU_MODE_BITS, (%esp)
	or	%ax, (%esp)		/* Restore old mode bits */
	fldcw	(%esp)

	add	$4, %esp
	ret


	.globl	mulmod64_i386
	.p2align 4

mulmod64_i386:
	push	%ebx
	sub	$16, %esp

	fildll	32(%esp)
	fildll	24(%esp)
	mov	32(%esp), %eax
	mov	32(%esp), %ecx
	mov	36(%esp), %ebx
	mull	24(%esp)
	fmulp	%st(0), %st(1)
	imul	24(%esp), %ebx
	fmul	%st(1), %st(0)
	imul	28(%esp), %ecx
	mov	%eax, 8(%esp)
	add	%ebx, %ecx
	fistpll	(%esp)
	add	%ecx, %edx
	mov	%edx, 12(%esp)
	mov	(%esp), %eax
	mov	4(%esp), %edx
	mov	44(%esp), %ebx
	mov	40(%esp), %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mull	40(%esp)
	add	%ebx, %ecx
	mov	12(%esp), %ebx
	add	%ecx, %edx
	mov	8(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	40(%esp), %ecx
	sbb	44(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	add	$16, %esp
	pop	%ebx
	ret
