/* mulmod-sse2.S -- (C) Geoffrey Reynolds, June 2007.

   Modular multiplication routines for x86/sse2.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define FPU_MODE_BITS (0x0F00)	/* 64-bit precision, round to zero. */


/* uint16_t vec_mulmod64_initp_sse2(const uint64_t *p);

     Setup for multiplying modulo p, where 1 < p < 2^62.
     Sets FPU rounding mode to round-to-zero and pushes 1.0/p onto
     the FPU stack. Returns the old rounding mode.

     Argument passed in %eax [function attribute regparm(1)].


   void vec_mulmod64_initb_sse2(const uint64_t *b);

     Setup for multiplying by b modulo p, where b < p < 2^62.
     Pushes b/p onto the FPU stack.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.

     Argument passed in %eax [function attribute regparm(1)].


   void vec<N>_mulmod64_sse2(const uint64_t *X, uint64_t *Y, int count);

     Assign Y[i] <-- X[i]*b (mod p) for 0 <= i < lim, where X[i] < p < 2^62,
     and where lim is the least positive multiple of N such that count <= lim.
     X and Y must be 16-aligned, and may overlap by multiples of N elements.

     (Currently X and Y need only be 8-aligned for N=2 or N=4).
     Assumes vec_mulmod64_initb_sse2(b) has been called.
     Assumes stack is 16-aligned.
     Arguments passed in %eax,%edx,%ecx [function attribute regparm(3)].


   void vec_mulmod64_finib_sse2(void);

     Release resources ready for another call to vec_mulmod64_initb_sse2().
     Pops b/p off the FPU stack. (Declared inline in asm-i386-gcc.h).

     Assumes vec_mulmod64_initb_sse2(b) has been called.


   void vec_mulmod64_finip_sse2(uint16_t mode);

     Release resources ready for another call to vec_mulmod64_initp_sse2().
     Pops 1.0/p off the FPU stack and restores FPU rounding mode to mode.

     Assumes vec_mulmod64_initp_sse2(p) has been called.
*/

/* NOTES: Pentium 4 suffers a huge penalty for using movdqa to load from
   memory that has been recently written to by fistpll. There is also a
   penalty for using fildll to load from memory written by movdqa. The
   solution seems to be to replace movdqa with movq/movhps for loads and
   with movlps/movhps for stores. This is done for vec2/4_mulmod64_sse2().
   For vec8/16_mulmod64_sse2() the loads and stores seem to be far enough
   apart to avoid the problem.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define mulmod64_init_sse2	_mulmod64_init_sse2
#define mulmod64_fini_sse2	_mulmod64_fini_sse2
#define vec_mulmod64_initp_sse2	_vec_mulmod64_initp_sse2
#define vec_mulmod64_finip_sse2	_vec_mulmod64_finip_sse2
#define vec_mulmod64_initb_sse2	_vec_mulmod64_initb_sse2
#define vec2_mulmod64_sse2	_vec2_mulmod64_sse2
#define vec4_mulmod64_sse2	_vec4_mulmod64_sse2
#define vec8_mulmod64_sse2	_vec8_mulmod64_sse2
#define vec16_mulmod64_sse2	_vec16_mulmod64_sse2
#endif

	.comm	mulmod_data,64

#define mulmod_b	mulmod_data
#define mulmod_bh	16+mulmod_data
#define mulmod_p	32+mulmod_data
#define mulmod_ph	48+mulmod_data

	.text

	.p2align 4
	.globl	vec_mulmod64_initp_sse2
	.globl	mulmod64_init_sse2

vec_mulmod64_initp_sse2:
	movq	(%eax), %xmm0
	punpcklqdq %xmm0, %xmm0
	movdqa	%xmm0, mulmod_p
	psrlq	$32, %xmm0
	movdqa	%xmm0, mulmod_ph

mulmod64_init_sse2:
	sub	$4, %esp

	fildll	(%eax)		/* p */
	fld1			/* 1.0, p */

	fnstcw	(%esp)
	movzwl	(%esp), %eax
	orw	$FPU_MODE_BITS, (%esp)	/* Set new mode bits */
	fldcw   (%esp)

	fdivp   %st(0), %st(1)	/* 1.0/p, using round to zero. */

	add	$4, %esp
	ret


	.p2align 4
	.globl	vec_mulmod64_finip_sse2
	.globl	mulmod64_fini_sse2

vec_mulmod64_finip_sse2:
mulmod64_fini_sse2:
	sub	$4, %esp

	fstp	%st(0)
	fnstcw	(%esp)
	and	$FPU_MODE_BITS, %ax
	andw	$~FPU_MODE_BITS, (%esp)	/* Clear bits */
	or	%ax, (%esp)	/* Restore old bits */
	fldcw	(%esp)

	add	$4, %esp
	ret


	.p2align 4
	.globl	vec_mulmod64_initb_sse2

vec_mulmod64_initb_sse2:
	movq	(%eax), %xmm0
	punpcklqdq %xmm0, %xmm0
	movdqa	%xmm0, mulmod_b
	psrlq	$32, %xmm0
	movdqa	%xmm0, mulmod_bh

	fildll	(%eax)
	fmul	%st(1), %st(0)

	ret


	.p2align 4
	.globl	vec2_mulmod64_sse2

vec2_mulmod64_sse2:
	sub	$12+16, %esp

	movdqa  mulmod_b, %xmm0
	movdqa  mulmod_bh, %xmm1
	movdqa	mulmod_p, %xmm5
	movdqa	mulmod_ph, %xmm7

	.p2align 4,,7
loop2:
	fildll	(%eax)
	fmul	%st(1), %st(0)
	fistpll (%esp)
	fildll	8(%eax)
	fmul	%st(1), %st(0)
	fistpll 8(%esp)

	movq	(%eax), %xmm2
	movhps	8(%eax), %xmm2
	movdqa  %xmm2, %xmm4
	pshufd	$0xF5, %xmm2, %xmm6
	pmuludq %xmm0, %xmm2
	pmuludq %xmm1, %xmm4
	pmuludq %xmm0, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm2
	paddq   %xmm6, %xmm2

	movq	(%esp), %xmm3
	movhps	8(%esp), %xmm3
	movdqa  %xmm3, %xmm4
	pshufd	$0xF5, %xmm3, %xmm6
	pmuludq %xmm5, %xmm3
	pmuludq %xmm7, %xmm4
	pmuludq %xmm5, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm3
	paddq   %xmm6, %xmm3

	pxor    %xmm6, %xmm6
	psubq   %xmm3, %xmm2
	psubq   %xmm5, %xmm2
	pcmpgtd %xmm2, %xmm6
	pshufd  $0xF5, %xmm6, %xmm6
	pand    %xmm5, %xmm6
	paddq   %xmm6, %xmm2
	movlps	%xmm2, (%edx)
	movhps	%xmm2, 8(%edx)

	add	$16, %eax
	add	$16, %edx
	sub	$2, %ecx
	jg	loop2

	add	$12+16, %esp
	ret


	.p2align 4
	.globl	vec4_mulmod64_sse2

vec4_mulmod64_sse2:
	sub	$12+64, %esp

	movdqa  mulmod_b, %xmm0
	movdqa	mulmod_p, %xmm1

	.p2align 4,,7
loop4:
	fildll	(%eax)
	fmul	%st(1), %st(0)
	fistpll (%esp)
	fildll	8(%eax)
	fmul	%st(1), %st(0)
	fistpll 8(%esp)
	fildll	16(%eax)
	fmul	%st(1), %st(0)
	fistpll 16(%esp)
	fildll	24(%eax)
	fmul	%st(1), %st(0)
	fistpll 24(%esp)

	movdqa	(%eax), %xmm2
	movdqa	16(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq mulmod_bh, %xmm4
	pmuludq mulmod_bh, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, 32(%esp)
	movdqa	%xmm3, 48(%esp)

	movq	(%esp), %xmm2
	movhps	8(%esp), %xmm2
	movq	16(%esp), %xmm3
	movhps	24(%esp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm1, %xmm2
	pmuludq %xmm1, %xmm3
	pmuludq mulmod_ph, %xmm4
	pmuludq mulmod_ph, %xmm5
	pmuludq %xmm1, %xmm6
	pmuludq %xmm1, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	32(%esp), %xmm4
	movdqa	48(%esp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm1, %xmm4
	psubq   %xmm1, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm1, %xmm6
	pand    %xmm1, %xmm7
	paddq	%xmm6, %xmm4
	paddq	%xmm7, %xmm5
	movlps	%xmm4, (%edx)
	movhps	%xmm4, 8(%edx)
	movlps	%xmm5, 16(%edx)
	movhps	%xmm5, 24(%edx)

	add	$32, %eax
	add	$32, %edx
	sub	$4, %ecx
	jg	loop4

	add	$12+64, %esp
	ret


	.p2align 4
	.globl	vec8_mulmod64_sse2

vec8_mulmod64_sse2:
	sub	$4, %ecx
	jle	vec4_mulmod64_sse2

	sub	$12+128, %esp

	.p2align 4,,7
loop8:
	fildll	(%eax)
	fmul	%st(1), %st(0)
	fistpll (%esp)
	fildll	8(%eax)
	fmul	%st(1), %st(0)
	fistpll 8(%esp)
	fildll	16(%eax)
	fmul	%st(1), %st(0)
	fistpll 16(%esp)
	fildll	24(%eax)
	fmul	%st(1), %st(0)
	fistpll 24(%esp)
	fildll	32(%eax)
	fmul	%st(1), %st(0)
	fistpll 32(%esp)
	fildll	40(%eax)
	fmul	%st(1), %st(0)
	fistpll 40(%esp)
	fildll	48(%eax)
	fmul	%st(1), %st(0)
	fistpll 48(%esp)
	fildll	56(%eax)
	fmul	%st(1), %st(0)
	fistpll 56(%esp)

	movdqa  mulmod_b, %xmm0
	movdqa  mulmod_bh, %xmm1

	movdqa	(%eax), %xmm2
	movdqa	16(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, 64(%esp)
	movdqa	%xmm3, 80(%esp)

	movdqa	32(%eax), %xmm2
	movdqa	48(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, 96(%esp)
	movdqa	%xmm3, 112(%esp)

	movdqa	mulmod_p, %xmm0
	movdqa	mulmod_ph, %xmm1
#if 0
	movdqa  (%esp), %xmm2
	movdqa  16(%esp), %xmm3
#else
	movq	(%esp), %xmm2
	movhps	8(%esp), %xmm2
	movq	16(%esp), %xmm3
	movhps	24(%esp), %xmm3
#endif
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	64(%esp), %xmm4
	movdqa	80(%esp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, (%edx)
	movdqa  %xmm5, 16(%edx)

	movdqa  32(%esp), %xmm2
	movdqa  48(%esp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	96(%esp), %xmm4
	movdqa	112(%esp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, 32(%edx)
	movdqa  %xmm5, 48(%edx)

	add	$64, %eax
	add	$64, %edx
	sub	$8, %ecx
	jg	loop8

	add	$12+128, %esp
	add	$4, %ecx
	jg	vec4_mulmod64_sse2

	ret


	.p2align 4
	.globl	vec16_mulmod64_sse2

vec16_mulmod64_sse2:
	cmp	$12, %ecx
	jle	vec8_mulmod64_sse2

	push	%ebp
	sub	$8+256, %esp
	lea	128(%esp), %ebp
	sub	$12, %ecx

	.p2align 4,,7
loop16:
	fildll	(%eax)
	fmul	%st(1), %st(0)
	fistpll (%ebp)
	fildll	8(%eax)
	fmul	%st(1), %st(0)
	fistpll 8(%ebp)
	fildll	16(%eax)
	fmul	%st(1), %st(0)
	fistpll 16(%ebp)
	fildll	24(%eax)
	fmul	%st(1), %st(0)
	fistpll 24(%ebp)
	fildll	32(%eax)
	fmul	%st(1), %st(0)
	fistpll 32(%ebp)
	fildll	40(%eax)
	fmul	%st(1), %st(0)
	fistpll 40(%ebp)
	fildll	48(%eax)
	fmul	%st(1), %st(0)
	fistpll 48(%ebp)
	fildll	56(%eax)
	fmul	%st(1), %st(0)
	fistpll 56(%ebp)
	fildll	64(%eax)
	fmul	%st(1), %st(0)
	fistpll 64(%ebp)
	fildll	72(%eax)
	fmul	%st(1), %st(0)
	fistpll 72(%ebp)
	fildll	80(%eax)
	fmul	%st(1), %st(0)
	fistpll 80(%ebp)
	fildll	88(%eax)
	fmul	%st(1), %st(0)
	fistpll 88(%ebp)
	fildll	96(%eax)
	fmul	%st(1), %st(0)
	fistpll 96(%ebp)
	fildll	104(%eax)
	fmul	%st(1), %st(0)
	fistpll 104(%ebp)
	fildll	112(%eax)
	fmul	%st(1), %st(0)
	fistpll 112(%ebp)
	fildll	120(%eax)
	fmul	%st(1), %st(0)
	fistpll 120(%ebp)

	movdqa  mulmod_b, %xmm0
	movdqa  mulmod_bh, %xmm1

	movdqa	(%eax), %xmm2
	movdqa	16(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, -16(%ebp)
	movdqa	%xmm3, -32(%ebp)

	movdqa	32(%eax), %xmm2
	movdqa	48(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, -48(%ebp)
	movdqa	%xmm3, -64(%ebp)

	movdqa	64(%eax), %xmm2
	movdqa	80(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, -80(%ebp)
	movdqa	%xmm3, -96(%ebp)

	movdqa	96(%eax), %xmm2
	movdqa	112(%eax), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3
	movdqa	%xmm2, -112(%ebp)
	movdqa	%xmm3, -128(%ebp)

	movdqa	mulmod_p, %xmm0
	movdqa	mulmod_ph, %xmm1

	movdqa  (%ebp), %xmm2
	movdqa  16(%ebp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	-16(%ebp), %xmm4
	movdqa	-32(%ebp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, (%edx)
	movdqa  %xmm5, 16(%edx)

	movdqa  32(%ebp), %xmm2
	movdqa  48(%ebp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	-48(%ebp), %xmm4
	movdqa	-64(%ebp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, 32(%edx)
	movdqa  %xmm5, 48(%edx)

	movdqa  64(%ebp), %xmm2
	movdqa  80(%ebp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	-80(%ebp), %xmm4
	movdqa	-96(%ebp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, 64(%edx)
	movdqa  %xmm5, 80(%edx)

	movdqa  96(%ebp), %xmm2
	movdqa  112(%ebp), %xmm3
	movdqa  %xmm2, %xmm4
	movdqa  %xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm0, %xmm3
	pmuludq %xmm1, %xmm4
	pmuludq %xmm1, %xmm5
	pmuludq %xmm0, %xmm6
	pmuludq %xmm0, %xmm7
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm6
	psllq   $32, %xmm7
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm6, %xmm2
	paddq   %xmm7, %xmm3

	movdqa	-112(%ebp), %xmm4
	movdqa	-128(%ebp), %xmm5
	pxor    %xmm6, %xmm6
	pxor    %xmm7, %xmm7
	psubq   %xmm2, %xmm4
	psubq   %xmm3, %xmm5
	psubq   %xmm0, %xmm4
	psubq   %xmm0, %xmm5
	pcmpgtd %xmm4, %xmm6
	pcmpgtd %xmm5, %xmm7
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm7, %xmm7
	pand    %xmm0, %xmm6
	pand    %xmm0, %xmm7
	paddq   %xmm6, %xmm4
	paddq   %xmm7, %xmm5
	movdqa  %xmm4, 96(%edx)
	movdqa  %xmm5, 112(%edx)

	sub	$-128, %eax
	sub	$-128, %edx
	sub	$16, %ecx
	jg	loop16

	add	$8+256, %esp
	pop	%ebp
	add	$12, %ecx
	jg	vec8_mulmod64_sse2

	ret
