/* btop-x86-sse2.S -- (C) Geoffrey Reynolds, September 2007.

   Build table of powers for x86 machines with SSE2.

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

#define VECTOR_LENGTH 4

/* void btop_x86_sse2(const uint32_t *L, uint64_t *T, uint32_t lmin,
                      uint32_t llen, uint64_t p);


   Given T[0], assigns T[j] <-- T[0]^(j+1) (mod p) for 0 < j <= lmin,
   and, if llen > 0, also assigns T[k] <-- T[0]^(k+1) (mod p) for those
   k in {lmin+L[i] : 0 <= i < llen}.

   Assumes T[0] < p < 2^62.
   Assumes that L was composed with VECTOR_LENGTH 4.
   Assumes FPU is set to double extended precision and round to zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
   Assumes that the stack is 16-aligned, unless UNALIGNED_STACK=1.
*/


#if NEED_UNDERSCORE
#define btop_x86_sse2 _btop_x86_sse2
#endif

	.text
	.p2align 4
	.globl	btop_x86_sse2

btop_x86_sse2:
#if UNALIGNED_STACK
	mov	%esp, %eax
	and	$~15, %esp	/* 0 mod 16 */
	sub	$8, %esp	/* 8 mod 16 */
	pushl	24(%eax)
	pushl	20(%eax)	/* 0 mod 16 */
	pushl	16(%eax)
	pushl	12(%eax)
	pushl	8(%eax)
	pushl	4(%eax)		/* 0 mod 16 */
	push	%eax		/* 12 mod 16 */
#endif
	push	%ebp
	push	%edi
	push	%esi
	push	%ebx
	sub	$76, %esp

	mov	100(%esp), %ebp				/* T */
	fildll	(%ebp)					/* T[0] */
	fmul	%st(1),%st(0)

	lea	8*(VECTOR_LENGTH-1)(%ebp), %edi		/* T+VECTOR_LENGTH-1 */
	mov	%ebp, %esi
	mov	(%ebp), %eax
	mov	4(%ebp), %edx

fill:
	mov	(%ebp), %ebx
	mov	4(%ebp), %ecx
	fildll  (%esi)
	imul    %edx, %ebx
	fmul    %st(1), %st(0)
	imul    %eax, %ecx
	fistpll (%esp)
	mull    (%ebp)
	mov     %eax, 12(%esp)
	add     %ecx, %ebx
	add     %ebx, %edx
	mov     (%esp), %eax
	mov     %edx, 8(%esp)
	mov     4(%esp), %edx
	mov     116(%esp), %ebx
	mov     112(%esp), %ecx
	imul    %eax, %ebx
	imul    %edx, %ecx
	mull    112(%esp)
	add     %ebx, %ecx
	mov     8(%esp), %ebx
	add     %ecx,%edx
	mov     12(%esp), %ecx
	sub     %eax, %ecx
	sbb     %edx, %ebx
	mov     %ecx, %eax
	mov     %ebx, %edx
	sub     112(%esp), %ecx
	sbb     116(%esp), %ebx
	jl      0f
	mov     %ecx, %eax
	mov     %ebx, %edx
0:
	mov	%eax, 8(%esi)
	mov	%edx, 12(%esi)
	add	$8, %esi
	cmp	%edi, %esi
	jb	fill

	fstp    %st(0)
	cmpl	$(VECTOR_LENGTH), 104(%esp)
	jb	ladder

	fildll  8*(VECTOR_LENGTH-1)(%ebp)
	fmul    %st(1),%st(0)

	movq	8*(VECTOR_LENGTH-1)(%ebp), %xmm1
	punpcklqdq %xmm1, %xmm1
	movq	112(%esp), %xmm0
	punpcklqdq %xmm0, %xmm0
	movdqa	%xmm0, %xmm4
	movdqa	%xmm1, %xmm5
	psrlq	$32, %xmm4
	psrlq	$32, %xmm5
	movdqa	%xmm4, 32(%esp)				/* {ph,ph} */
	movdqa	%xmm5, 48(%esp)				/* {bh,bh} */

	mov	104(%esp), %edi				/* lmin */
	mov	%ebp, %esi
	lea	-8*VECTOR_LENGTH(%ebp,%edi,8), %edi

	movq	(%ebp), %xmm2				/* T[0] */
	movhps	8(%ebp), %xmm2				/* T[1] */
	movq	16(%ebp), %xmm3				/* T[2] */
	movhps	24(%ebp), %xmm3				/* T[3] */

	.p2align 4,,7
next:
	fildll	(%esi)					/* T[0] */
	fmul	%st(1), %st(0)
	fistpll	(%esp)
	fildll	8(%esi)					/* T[1] */
	fmul	%st(1), %st(0)
	fistpll	8(%esp)
	fildll	16(%esi)				/* T[2] */
	fmul	%st(1), %st(0)
	fistpll	16(%esp)
	fildll	24(%esi)				/* T[3] */
	fmul	%st(1), %st(0)
	fistpll	24(%esp)

	movdqa	%xmm2, %xmm4
	movdqa	%xmm3, %xmm5
	pshufd	$0xF5, %xmm2, %xmm6
	pshufd	$0xF5, %xmm3, %xmm7
	pmuludq	%xmm1, %xmm2
	pmuludq	%xmm1, %xmm3
	pmuludq	48(%esp), %xmm4
	pmuludq	48(%esp), %xmm5
	pmuludq	%xmm1, %xmm6
	pmuludq	%xmm1, %xmm7
	psllq	$32, %xmm4
	psllq	$32, %xmm5
	psllq	$32, %xmm6
	psllq	$32, %xmm7
	paddq	%xmm4, %xmm2
	paddq	%xmm5, %xmm3
	paddq	%xmm6, %xmm2
	paddq	%xmm7, %xmm3

	movq	(%esp), %xmm4
	movhps	8(%esp), %xmm4
	movdqa  %xmm4, %xmm7
	pshufd	$0xF5, %xmm4, %xmm6
	pmuludq	%xmm0, %xmm6
	pmuludq	32(%esp), %xmm7
	pmuludq	%xmm0, %xmm4
	psllq	$32, %xmm6
	psllq	$32, %xmm7
	paddq	%xmm6, %xmm4
	paddq	%xmm7, %xmm4

	pxor	%xmm6, %xmm6
	psubq	%xmm4, %xmm2
	psubq	%xmm0, %xmm2
	pcmpgtd	%xmm2, %xmm6
	pshufd	$0xF5, %xmm6, %xmm6
	pand	%xmm0, %xmm6
	paddq	%xmm6, %xmm2
	movlps	%xmm2, 32(%esi)
	movhps	%xmm2, 40(%esi)

	movq	16(%esp), %xmm5
	movhps	24(%esp), %xmm5
	movdqa	%xmm5, %xmm7
	pshufd	$0xF5,%xmm5,%xmm6
	pmuludq	%xmm0, %xmm6
	pmuludq	32(%esp), %xmm7
	pmuludq	%xmm0, %xmm5
	psllq	$32, %xmm6
	psllq	$32, %xmm7
	paddq	%xmm6, %xmm5
	paddq	%xmm7, %xmm5

	pxor	%xmm7, %xmm7
	psubq	%xmm5, %xmm3
	psubq	%xmm0, %xmm3
	pcmpgtd	%xmm3, %xmm7
	pshufd	$0xF5, %xmm7, %xmm7
	pand	%xmm0, %xmm7
	paddq	%xmm7, %xmm3
	movlps	%xmm3, 48(%esi)
	movhps	%xmm3, 56(%esi)

	add	$(8*VECTOR_LENGTH), %esi
	cmp	%edi, %esi
	jbe	next

	fstp	%st(0)

ladder:
	mov	108(%esp), %eax
	test	%eax, %eax
	jz	out

	mov	104(%esp), %ecx
	fildll  (%ebp,%ecx,8)
	fmul    %st(1),%st(0)
	movl	$0, 64(%esp)

	.p2align 4,,7
step:
	mov	64(%esp), %eax
	mov	96(%esp), %edx
	mov	104(%esp), %ecx
	incl	64(%esp)
	mov	(%edx,%eax,4), %esi
	mov	(%ebp,%ecx,8), %eax
	mov	4(%ebp,%ecx,8), %edx
	lea	(%esi,%ecx), %edi

	fildll	(%ebp,%esi,8)
	mov	(%ebp,%esi,8), %ebx
	mov	4(%ebp,%esi,8), %ecx
	imul	%edx, %ebx
	fmul	%st(1), %st(0)
	imul	%eax, %ecx
	fistpll	(%esp)
	mull	(%ebp,%esi,8)
	mov	%eax, 12(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	(%esp), %eax
	mov	%edx, 8(%esp)
	mov	4(%esp), %edx
	mov	116(%esp), %ebx
	mov	112(%esp), %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mull	112(%esp)
	add	%ebx, %ecx
	mov	8(%esp), %ebx
	add	%ecx,%edx
	mov	12(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	112(%esp), %ecx
	sbb	116(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	mov	108(%esp), %ebx
	cmp	%ebx, 64(%esp)
	mov	%eax, 8(%ebp,%edi,8)
	mov	%edx, 12(%ebp,%edi,8)
	jb	step

	fstp    %st(0)
out:
	add	$76, %esp
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%ebp
#if UNALIGNED_STACK
	pop	%esp
#endif
	ret
