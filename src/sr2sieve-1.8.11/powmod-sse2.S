/* powmod-sse2.S -- (C) Geoffrey Reynolds, January 2008.

   uint64_t powmod64_sse2(uint64_t b, uint64_t n, uint64_t p);
     Returns b^n (mod p), where 0 <= b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 16-aligned.

   void vec_powmod64_sse2(uint64_t *B, int len, uint64_t n, uint64_t p);
     Assigns B[i] <-- B[i]^n (mod p) for 0 <= i < LIM, where 0 <= b < p < 2^62
     and LIM is the least multiple of 4 satisfying LIM >= len.

     Assumes that len > 0, n > 0, and that B is 16-aligned.
     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 16-aligned.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"

/* For SSE loads/stores that might be affected by a partial read/write, set:
   USE_MOVDQU=0 to use movq/movhps for loads and movlps/movhps for stores.
   USE_MOVDQU=1 to use movdqu for loads and stores.
*/
#ifndef USE_MOVDQU
# define USE_MOVDQU 0
#endif


#if NEED_UNDERSCORE
# define powmod64_sse2 _powmod64_sse2
# define vec_powmod64_sse2 _vec_powmod64_sse2
#endif

	.text
	.globl	powmod64_sse2

	.p2align 4,,15

powmod64_sse2:
	mov	$1, %eax
	lea	4(%esp), %ecx
	movd	%eax, %xmm3
	movdqa	%xmm3, %xmm6		/* {1,0} constant */
	punpcklqdq (%ecx), %xmm3	/* {1,b} */
	movq	8(%ecx), %xmm5		/* {n,0} */
	movq	16(%ecx), %xmm0
	punpcklqdq %xmm0, %xmm0		/* {p,p} constant */
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


	.text
	.globl vec_powmod64_sse2

	.p2align 4,,15

vec_powmod64_sse2:
	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

	sub	$44, %esp
	mov	%esp, %ebp

	movq	80(%ebp), %xmm5
	punpcklqdq %xmm5, %xmm5		/* {p,p} */
	pshufd	$0xF5, %xmm5, %xmm7

	mov	64(%ebp), %esi		/* B */
	mov	68(%ebp), %ecx		/* len */
	lea	(%esi,%ecx,8), %ebx	/* B+len */
	add	$3, %ecx
	and	$-4, %ecx		/* LIM */
	shl	$3, %ecx
	sub	%ecx, %esp		/* A 16-aligned */

	shr	$2, %ecx		/* 2*LIM */
	mov	%esp, %edi		/* A */
	rep
	movsl				/* A[i] <-- B[i] for 0 <= i < LIM */

	mov	76(%ebp), %edx		/* high dword of n */
	bsr	%edx, %ecx		/* ZF predictable */
	setz	32(%ebp)		/* true iff high dword done */
	jnz	0f
	mov	72(%ebp), %edx		/* low dword of n */
	bsr	%edx, %ecx		/* ZF=0 assumed */
0:
	dec	%ecx			/* SF=0 predicted */
	jl	vec_next
	mov	$1, %eax
	shl	%cl, %eax		/* x <-- second highest bit of n */


	.p2align 4,,2

vec_loop:
	mov	64(%ebp), %edi		/* B */

	.p2align 4,,7

vec_sqr:
	/* %edi = B+i, %st(0) = 1.0/p */

	fildll	(%edi)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	(%ebp)
	fildll	8(%edi)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	8(%ebp)
	fildll	16(%edi)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	16(%ebp)
	fildll	24(%edi)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	24(%ebp)

#if USE_MOVDQU
	movdqu	(%edi), %xmm0
	movdqu	16(%edi), %xmm1
#else
	movq	(%edi), %xmm0
	movhps	8(%edi), %xmm0
	movq	16(%edi), %xmm1
	movhps	24(%edi), %xmm1
#endif
	pshufd	$0xF5, %xmm0, %xmm2
	pshufd	$0xF5, %xmm1, %xmm3
	pmuludq %xmm0, %xmm2
	pmuludq %xmm1, %xmm3
	pmuludq %xmm0, %xmm0
	pmuludq %xmm1, %xmm1
	psllq   $33, %xmm2
	psllq   $33, %xmm3
	paddq   %xmm2, %xmm0
	paddq   %xmm3, %xmm1

#if USE_MOVDQU
	movdqu	(%ebp), %xmm2
#else
	movq	(%ebp), %xmm2
	movhps	8(%ebp), %xmm2
#endif
	movdqa  %xmm2, %xmm4
	pshufd	$0xF5, %xmm2, %xmm6
	pmuludq %xmm5, %xmm2
	pmuludq %xmm7, %xmm4
	pmuludq %xmm5, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm2
	paddq   %xmm6, %xmm2
	psubq   %xmm2, %xmm0

#if USE_MOVDQU
	movdqu	16(%ebp), %xmm3
#else
	movq	16(%ebp), %xmm3
	movhps	24(%ebp), %xmm3
#endif
	movdqa  %xmm3, %xmm4
	pshufd	$0xF5, %xmm3, %xmm6
	pmuludq %xmm5, %xmm3
	pmuludq %xmm7, %xmm4
	pmuludq %xmm5, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm3
	paddq   %xmm6, %xmm3
	psubq   %xmm3, %xmm1

	pxor    %xmm6, %xmm6
	pxor    %xmm4, %xmm4
	psubq   %xmm5, %xmm0
	psubq   %xmm5, %xmm1
	pcmpgtd %xmm0, %xmm6
	pcmpgtd %xmm1, %xmm4
	pshufd  $0xF5, %xmm6, %xmm6
	pshufd  $0xF5, %xmm4, %xmm4
	pand    %xmm5, %xmm6
	pand    %xmm5, %xmm4
	paddq   %xmm6, %xmm0
	paddq   %xmm4, %xmm1
#if USE_MOVDQU
	movdqu	%xmm0, (%edi)
	movdqu	%xmm1, 16(%edi)
#else
	movlps	%xmm0, (%edi)
	movhps	%xmm0, 8(%edi)
	movlps	%xmm1, 16(%edi)
	movhps	%xmm1, 24(%edi)
#endif

	lea	32(%edi), %edi
	cmp	%edi, %ebx
	ja	vec_sqr

	test	%eax, %edx		/* ZF unpredictable */
	jz	vec_shift

	mov	%esp, %esi		/* A */
	mov	64(%ebp), %edi		/* B */

	.p2align 4,,7

vec_mul:
	/* %edi = B+i, %esi = A+i, %st(0) = 1.0/p */

	fildll	(%edi)
	fildll	(%esi)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	(%ebp)
	fildll	8(%edi)
	fildll	8(%esi)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	8(%ebp)
	fildll	16(%edi)
	fildll	16(%esi)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	16(%ebp)
	fildll	24(%edi)
	fildll	24(%esi)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	24(%ebp)

#if USE_MOVDQU
	movdqu	(%edi), %xmm0
#else
	movq	(%edi), %xmm0
	movhps	8(%edi), %xmm0
#endif
	movdqa	(%esi), %xmm2
	pshufd	$0xF5, %xmm0, %xmm4
	pshufd	$0xF5, %xmm2, %xmm1
	pmuludq %xmm2, %xmm4
	pmuludq %xmm0, %xmm1
	pmuludq %xmm0, %xmm2
	psllq   $32, %xmm4
	psllq   $32, %xmm1
	paddq   %xmm4, %xmm2
	paddq   %xmm1, %xmm2

#if USE_MOVDQU
	movdqu	16(%edi), %xmm6
#else
	movq	16(%edi), %xmm6
	movhps	24(%edi), %xmm6
#endif
	movdqa	16(%esi), %xmm3
	pshufd	$0xF5, %xmm6, %xmm4
	pshufd	$0xF5, %xmm3, %xmm1
	pmuludq %xmm3, %xmm4
	pmuludq %xmm6, %xmm1
	pmuludq %xmm6, %xmm3
	psllq   $32, %xmm4
	psllq   $32, %xmm1
	paddq   %xmm4, %xmm3
	paddq   %xmm1, %xmm3

#if USE_MOVDQU
	movdqu	(%ebp), %xmm0
#else
	movq	(%ebp), %xmm0
	movhps	8(%ebp), %xmm0
#endif
	movdqa  %xmm0, %xmm4
	pshufd	$0xF5, %xmm0, %xmm6
	pmuludq %xmm5, %xmm0
	pmuludq %xmm7, %xmm4
	pmuludq %xmm5, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm0
	paddq   %xmm6, %xmm0
	psubq   %xmm0, %xmm2

#if USE_MOVDQU
	movdqu	16(%ebp), %xmm1
#else
	movq	16(%ebp), %xmm1
	movhps	24(%ebp), %xmm1
#endif
	movdqa  %xmm1, %xmm4
	pshufd	$0xF5, %xmm1, %xmm6
	pmuludq %xmm5, %xmm1
	pmuludq %xmm7, %xmm4
	pmuludq %xmm5, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm6
	paddq   %xmm4, %xmm1
	paddq   %xmm6, %xmm1
	psubq   %xmm1, %xmm3

	pxor    %xmm4, %xmm4
	pxor    %xmm6, %xmm6
	psubq   %xmm5, %xmm2
	psubq   %xmm5, %xmm3
	pcmpgtd %xmm2, %xmm4
	pcmpgtd %xmm3, %xmm6
	pshufd  $0xF5, %xmm4, %xmm4
	pshufd  $0xF5, %xmm6, %xmm6
	pand    %xmm5, %xmm4
	pand    %xmm5, %xmm6
	paddq   %xmm4, %xmm2
	paddq   %xmm6, %xmm3
#if USE_MOVDQU
	movdqu	%xmm2, (%edi)
	movdqu	%xmm3, 16(%edi)
#else
	movlps	%xmm2, (%edi)
	movhps	%xmm2, 8(%edi)
	movlps	%xmm3, 16(%edi)
	movhps	%xmm3, 24(%edi)
#endif

	lea	32(%edi), %edi
	lea	32(%esi), %esi
	cmp	%edi, %ebx
	ja	vec_mul

	.p2align 4,,1

vec_shift:
	shr	%eax			/* x >>= 1 */
	jnz	vec_loop

vec_next:
	testb	$1, 32(%ebp)		/* 1 iff high dword done */
	mov	72(%ebp), %edx		/* low dword */
	mov	$0x80000000, %eax	/* x <-- high bit of next dword */
	movb	$1, 32(%ebp)		/* high dword done */
	jz	vec_loop

	.p2align 4,,2

vec_done:
	mov	%ebp, %esp
	add	$44, %esp
	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret
