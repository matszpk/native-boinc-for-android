/* btop-x86-64.S -- (C) Geoffrey Reynolds, September 2007.

   Build table of powers for x86-64/SSE2.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN64 1
#endif

#define VECTOR_LENGTH 4

/* void btop_x86_64(const uint32_t *L, uint64_t *T, uint32_t lmin,
                    uint32_t llen, uint64_t p);


   Given T[0], assigns T[j] <-- T[0]^(j+1) (mod p) for 0 < j <= lmin,
   and, if llen > 0, also assigns T[k] <-- T[0]^(k+1) (mod p) for those
   k in {lmin+L[i] : 0 <= i < llen}.

   Assumes T[0] < p < 2^51.
   Assumes that L was composed with VECTOR_LENGTH 4.
   Assumes that one_over_p = 1.0/p computed in round-to-zero mode.
   Assumes that the current SSE rounding mode is round-to-zero.
*/

#if NEED_UNDERSCORE
#define one_over_p _one_over_p
#define btop_x86_64 _btop_x86_64
#endif

	.globl	one_over_p


	.text
	.p2align 4
	.globl	btop_x86_64

btop_x86_64:
	push	%r13
	push	%r12
	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
	mov	%rcx, 56(%rsp)		/* L */
	mov	%rdx, %rsi		/* T */
	mov	%r8d, %edx		/* lmin */
	mov	%r9d, 72(%rsp)		/* llen */
	mov	88(%rsp), %r8		/* p */
#else
	mov	%rdi, -8(%rsp)		/* L */
	mov	%ecx, -16(%rsp)		/* llen */
#endif
	movsd	one_over_p(%rip), %xmm4	/* 1.0/p */

	mov	(%rsi), %rax		/* T[0] */
	mov	%rax, %rbx
	cvtsi2sdq %rax, %xmm5
	mulsd	%xmm4, %xmm5
	xor	%ecx, %ecx		/* fill-loop counter */

	.p2align 4,,7
fill:
	inc	%ecx
	cvtsi2sdq %rbx, %xmm0
	imul	%rax, %rbx
	mulsd	%xmm5, %xmm0
	cvtsd2siq %xmm0, %rbp
	imul	%r8, %rbp
	sub	%rbp, %rbx
	mov	%rbx, %rbp
	sub	%r8, %rbp
	jl	0f
	mov	%rbp, %rbx
0:
	cmp	$(VECTOR_LENGTH-1), %ecx
	mov	%rbx, (%rsi,%rcx,8)
	jb	fill

	cmp	$(VECTOR_LENGTH-1), %edx	/* lmin */
	jbe	ladder

	mov	%rax, %r10			/* T[0] */
	mov	8(%rsi), %r11			/* T[1] */
	mov	16(%rsi), %r12			/* T[2] */
	mov	%rbx, %r13			/* T[3] */

	cvtsi2sdq %rbx, %xmm5
	mulsd	%xmm4, %xmm5

	mov	$4, %ecx

	.p2align 4,,7
next:
	cvtsi2sdq %r10, %xmm0
	cvtsi2sdq %r11, %xmm1
	cvtsi2sdq %r12, %xmm2
	cvtsi2sdq %r13, %xmm3
	imul	%rbx, %r10
	imul	%rbx, %r11
	imul	%rbx, %r12
	imul	%rbx, %r13
	mulsd	%xmm5, %xmm0
	mulsd	%xmm5, %xmm1
	mulsd	%xmm5, %xmm2
	mulsd	%xmm5, %xmm3
	cvtsd2siq %xmm0, %rax
	cvtsd2siq %xmm1, %rbp
	cvtsd2siq %xmm2, %r9
	cvtsd2siq %xmm3, %rdi
	imul	%r8, %rax
	imul	%r8, %rbp
	imul	%r8, %r9
	imul	%r8, %rdi
	sub	%rax, %r10
	sub	%rbp, %r11
	sub	%r9, %r12
	sub	%rdi, %r13

	mov	%r10, %rax
	mov	%r11, %rbp
	mov	%r12, %r9
	mov	%r13, %rdi

	sub	%r8, %rax
	jl	0f
	mov	%rax, %r10
0:	sub	%r8, %rbp
	jl	1f
	mov	%rbp, %r11
1:	sub	%r8, %r9
	jl	2f
	mov	%r9, %r12
2:	sub	%r8, %rdi
	jl	3f
	mov	%rdi, %r13
3:
	mov	%r10, (%rsi,%rcx,8)
	mov	%r11, 8(%rsi,%rcx,8)
	mov	%r12, 16(%rsi,%rcx,8)
	mov	%r13, 24(%rsi,%rcx,8)
	lea	4(%rcx), %ecx
	cmp	%edx, %ecx		/* lmin */
	jbe	next

ladder:
#ifdef _WIN64
	mov	72(%rsp), %ecx		/* llen */
#else
	mov	-16(%rsp), %ecx		/* llen */
#endif
	test	%ecx, %ecx
	jz	out

#ifdef _WIN64
	mov	56(%rsp), %rdi		/* L */
#else
	mov	-8(%rsp), %rdi		/* L */
#endif
	xor	%ebp, %ebp
	mov	(%rsi,%rdx,8), %r9	/* T[lmin] */

	cvtsi2sdq %r9, %xmm5
	mulsd	%xmm4, %xmm5

	.p2align 4,,7
step:
	mov	(%rdi,%rbp,4), %ebx	/* L[i] */
	inc	%ebp
	lea	1(%rdx,%rbx), %r10d	/* lmin+L[i]+1 */
	mov	(%rsi,%rbx,8), %rax	/* T[L[i]] */

	cvtsi2sdq %rax, %xmm0
	imul	%r9, %rax
	mulsd	%xmm5, %xmm0
	cvtsd2siq %xmm0, %rbx
	imul	%r8, %rbx
	sub	%rbx, %rax
	mov	%rax, %rbx
	sub	%r8, %rbx
	jl	0f
	mov	%rbx, %rax
0:
	cmp	%ecx, %ebp
	mov	%rax, (%rsi,%r10,8)	/* T[lmin+L[i]+1] */
	jb	step

out:
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	pop	%r12
	pop	%r13
	ret
