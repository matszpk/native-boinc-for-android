/* powmod-sse2.S -- (C) Geoffrey Reynolds, January 2008.

   uint64_t powmod64_sse2(uint64_t b, uint64_t n, uint64_t p);

     Returns b^n (mod p), where 0 <= b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 16-aligned, unless UNALIGNED_STACK=1.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN32 1
#define UNALIGNED_STACK 1
#endif

/* For SSE loads/stores that might be affected by a partial read/write, set:
   USE_MOVDQU=0 to use movq/movhps for loads and movlps/movhps for stores.
   USE_MOVDQU=1 to use movdqu for loads and stores.
*/
#ifndef USE_MOVDQU
# define USE_MOVDQU 0
#endif


#if NEED_UNDERSCORE
# define powmod64_sse2 _powmod64_sse2
#endif

	.text
	.globl	powmod64_sse2

	.p2align 4,,15

powmod64_sse2:
	mov	$1, %eax
	lea	4(%esp), %ecx
	movd	%eax, %xmm3
	movdqa	%xmm3, %xmm6		/* {1,0} constant */
#if UNALIGNED_STACK
	movq	(%ecx), %xmm4
	punpcklqdq %xmm4, %xmm3		/* {1,b} */
#else
	punpcklqdq (%ecx), %xmm3	/* {1,b} */
#endif
	movq	8(%ecx), %xmm5		/* {n,0} */
	movq	16(%ecx), %xmm0
	punpcklqdq %xmm0, %xmm0		/* {p,p} constant */

#if UNALIGNED_STACK
	add	$4, %ecx
	and	$-8, %ecx		/* 8-align */
#endif

#if USE_MOVDQU
	movdqu	%xmm3, (%ecx)
#else
	movlps	%xmm3, (%ecx)
	movhps	%xmm3, 8(%ecx)
#endif

	.p2align 4,,15

mulsqr_loop:
	/* %xmm3 = (%ecx) = {a,b} */

	fildll	8(%ecx)
	fildll	(%ecx)
	nop
	fmul	%st(2), %st(0)
	fmul	%st(1), %st(0)
	fistpll	(%ecx)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	8(%ecx)

	pshufd	$0xEE, %xmm3, %xmm7
	pshufd	$0xF5, %xmm3, %xmm4
	pshufd	$0xFF, %xmm3, %xmm2
	pmuludq	%xmm7, %xmm4
	pmuludq	%xmm3, %xmm2
	pmuludq	%xmm3, %xmm7
	psllq	$32, %xmm4
	psllq	$32, %xmm2
	paddq	%xmm4, %xmm7
	paddq	%xmm2, %xmm7

#if USE_MOVDQU
	movdqu	(%ecx), %xmm1
#else
	movq	(%ecx), %xmm1
	movhps	8(%ecx), %xmm1
#endif

	/* %xmm0 = {p,p} */

	pshufd	$0xF5, %xmm0, %xmm4
	pshufd	$0xF5, %xmm1, %xmm2
	pmuludq	%xmm1, %xmm4
	pmuludq	%xmm0, %xmm2
	pmuludq	%xmm0, %xmm1
	psllq	$32, %xmm4
	psllq	$32, %xmm2
	paddq	%xmm4, %xmm1
	paddq	%xmm2, %xmm1

	pxor	%xmm4, %xmm4
	psubq	%xmm1, %xmm7
	psubq	%xmm0, %xmm7
	pcmpgtd	%xmm7, %xmm4
	pshufd	$0xF5, %xmm4, %xmm4
	pand	%xmm0, %xmm4
	paddq	%xmm4, %xmm7

	/* %xmm7 = {a*b,b^2}, %xmm3 = {a,b}, %xmm5 = {n,0}, %xmm6 = {1,0} */

	movdqa	%xmm5, %xmm2
	pxor	%xmm7, %xmm3
	pxor	%xmm1, %xmm1
	pandn	%xmm6, %xmm2		/* {1,0} or {0,0}  unpredictable */
	psubq	%xmm2, %xmm1		/* {0,0} or {-1,0} */
	pand	%xmm3, %xmm1
	pxor	%xmm1, %xmm7		/* {a*b,b^2} or {a,b^2} */
	movdqa	%xmm7, %xmm3
#if USE_MOVDQU
	movdqu	%xmm7, (%ecx)
#else
	movlps	%xmm7, (%ecx)
	movhps	%xmm7, 8(%ecx)
#endif

	/* %xmm5 = {n,0}, %xmm6 = {1,0} */

	psrlq	$1, %xmm5		/* n >>= 1 */
	pxor	%xmm1, %xmm1
	pxor	%xmm2, %xmm2
	psubq	%xmm6, %xmm1		/* {-1,0} */
	pcmpeqd %xmm5, %xmm2
	pandn	%xmm1, %xmm2
	pmovmskb %xmm2, %eax
	test	%eax, %eax
	jnz	mulsqr_loop

	mov	(%ecx), %eax
	mov	4(%ecx), %edx
	ret
