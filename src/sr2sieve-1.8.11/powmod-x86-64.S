/* powmod-x86-64.S -- (C) Geoffrey Reynolds, January 2008.

   uint64_t powmod64_x86_64(uint64_t b, uint64_t n, uint64_t p, double invp);
     Returns b^n (mod p), where 0 <= b < p < 2^51.

     Assumes current SSE rounding mode is round-to-zero.
     Assumes invp = 1.0/p computed in round-to-zero mode.

   void vec_powmod64_x86_64(uint64_t *B, int len, uint64_t n, uint64_t p,
                            double invp);
     Assigns B[i] <-- B[i]^n (mod p) for 0 <= i < LIM, where 0 <= b < p < 2^51
     and LIM is the least multiple of 4 satisfying LIM >= len.

     Assumes that len > 0, n > 0, and that B is 16-aligned.
     Assumes current SSE rounding mode is round-to-zero.
     Assumes invp = 1.0/p computed in round-to-zero mode.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"

/* Set USE_CMOV=1 to use a conditional move instead of a branch, even when
   the branch is predictable. Slower on Intel, but maybe faster on AMD.
*/
#ifndef USE_CMOV
# define USE_CMOV 0
#endif

#if NEED_UNDERSCORE
#define powmod64_x86_64 _powmod64_x86_64
#define vec_powmod64_x86_64 _vec_powmod64_x86_64
#endif

#ifdef _WIN64
#define ARG1 %rcx
#define ARG2 %rdx
#define ARG3 %r8
#define ARG4 %xmm3
#define REG5 %r9
#define REG5l %r9d
#define REG6 %r10
#define REG7 %r11
#else
#define ARG1 %rdi
#define ARG2 %rsi
#define ARG3 %rdx
#define ARG4 %xmm0
#define REG5 %rcx
#define REG5l %ecx
#define REG6 %r8
#define REG7 %r9
#endif


	.text
	.globl powmod64_x86_64

	.p2align 4,,15

powmod64_x86_64:
	/* ARG1 = b, ARG2 = n, ARG3 = p, ARG4 = 1.0/p */

	mov	$1, %eax
	mov	$1, REG5l

	.p2align 4,,15

mulsqr_loop:
	/* %rax = REG5 = a, ARG1 = b, ARG2 = n, ARG3 = p, ARG4 = 1.0/p */

	cvtsi2sdq ARG1, %xmm1
	cvtsi2sdq %rax, %xmm2
	imul	ARG1, %rax
	imul	ARG1, ARG1
	mulsd	%xmm1, %xmm2
	mulsd	ARG4, %xmm2
	mulsd	%xmm1, %xmm1
	mulsd	ARG4, %xmm1
	cvtsd2siq %xmm2, REG6
	cvtsd2siq %xmm1, REG7
	imul	ARG3, REG6
	imul	ARG3, REG7
	sub	REG6, %rax
	sub	REG7, ARG1
	mov	%rax, REG6
	mov	ARG1, REG7

correct_mul:
	sub     ARG3, REG6	/* CF=0 predicted */
#if USE_CMOV
	cmovnc	REG6, %rax
#else
	jc      correct_sqr
	mov     REG6, %rax
#endif

correct_sqr:
	sub     ARG3, REG7	/* CF=0 predicted */
#if USE_CMOV
	cmovnc	REG7, ARG1
#else
	jc	shift
	mov     REG7, ARG1
#endif

shift:
	/* REG5 = a, %rax = a*b, ARG1 = b^2, ARG2 = n */

	shr	ARG2		/* CF unpredictable */
	cmovnc	REG5, %rax	/* Discard multiply if CF=0 */
	mov	%rax, REG5
	jnz	mulsqr_loop

	ret


#undef ARG1
#undef ARG2
#undef ARG3
#undef ARG4
#undef REG5
#undef REG5l
#undef REG6
#undef REG7

/* left-right-powmod(b,n,p)
     a <-- b
     x <-- most significant bit of n
     while x > 0
       b <-- b^2 (mod p)
       x <-- x-1
       if bit x of n is set
         b <-- a*b (mod p)
     return b
*/

	.text
	.globl vec_powmod64_x86_64

	.p2align 4,,15

vec_powmod64_x86_64:
	/* %rdi = B, %esi = len, %rdx = n, %rcx = p, %xmm0 = 1.0/p */

	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rsi
	push	%rdi
#endif
	push	%r12
	push	%r13
	push	%r14
	push	%r15
#ifdef _WIN64
	movsd	104(%rsp), %xmm0	/* 1.0/p */
	sub	$56, %rsp		/* 16-aligned */
	movdqa	%xmm6, (%rsp)
	movdqa	%xmm7, 16(%rsp)
	movdqa	%xmm8, 32(%rsp)
	mov	%rcx, %rdi
	mov	%rdx, %rsi
	mov	%r8, %rdx
	mov	%r9, %rcx
#else
	sub	$8, %rsp		/* 16-aligned */
#endif
	mov	%rsp, %rbp

	mov	%rcx, %rbx
	bsr	%rdx, %rcx
	dec	%ecx			/* SF=0 predicted */
	jl	vec_done
	mov	$1, %eax
	shl	%cl, %rax		/* second highest bit of n */

	mov	%esi, %r10d
	mov	%rdi, %r11

	add	$3, %esi
	and	$-4, %esi
	mov	%esi, %ecx		/* LIM */
	shl	$3, %esi
	sub	%rsi, %rsp		/* A[LIM] 16-aligned */

	mov	%rdi, %rsi
	mov	%rsp, %rdi
	rep
	movsq				/* A[i] <-- B[i], 0 <= i < LIM */

	mov	%r10d, %esi
	mov	%r11, %rdi

	.p2align 4,,1

vec_loop:
	/* %rbx = p, %rdi = B, %esi = len, %rdx = n, %rax = x, %xmm0 = 1.0/p */
	/* %rsp = A, %rbp = frame */

	xor	%ecx, %ecx		/* i <-- 0 */

	.p2align 4,,7

vec_sqr:
	mov	(%rdi,%rcx,8), %r8
	mov	8(%rdi,%rcx,8), %r9
	mov	16(%rdi,%rcx,8), %r10
	mov	24(%rdi,%rcx,8), %r11
	cvtsi2sdq %r8, %xmm1
	cvtsi2sdq %r9, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	imul	%r8, %r8
	imul	%r9, %r9
	imul	%r10, %r10
	imul	%r11, %r11
	mulsd	%xmm1, %xmm1
	mulsd	%xmm2, %xmm2
	mulsd	%xmm3, %xmm3
	mulsd	%xmm4, %xmm4
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm4
	cvtsd2siq %xmm1, %r12
	cvtsd2siq %xmm2, %r13
	cvtsd2siq %xmm3, %r14
	cvtsd2siq %xmm4, %r15

	imul	%rbx, %r12
	imul	%rbx, %r13
	imul	%rbx, %r14
	imul	%rbx, %r15
	sub	%r12, %r8
	sub	%r13, %r9
	sub	%r14, %r10
	sub	%r15, %r11
	mov	%r8, (%rdi,%rcx,8)
	mov	%r9, 8(%rdi,%rcx,8)
	mov	%r10, 16(%rdi,%rcx,8)
	mov	%r11, 24(%rdi,%rcx,8)

	sub	%rbx, %r8
	jc	0f
	mov	%r8, (%rdi,%rcx,8)
0:	sub	%rbx, %r9
	jc	1f
	mov	%r9, 8(%rdi,%rcx,8)
1:	sub	%rbx, %r10
	jc	2f
	mov	%r10, 16(%rdi,%rcx,8)
2:	sub	%rbx, %r11
	jc	3f
	mov	%r11, 24(%rdi,%rcx,8)
3:
	add	$4, %ecx
	cmp	%ecx, %esi
	ja	vec_sqr

	test	%rax, %rdx		/* ZF unpredictable */
	jz	vec_shift

	xor	%ecx, %ecx		/* i <-- 0 */

	.p2align 4,,7

vec_mul:
	/* %rbx = p, %rdi = B, %esi = len, %rdx = n, %rax = x, %xmm0 = 1.0/p */
	/* %rsp = A, %rbp = frame */

	mov	(%rdi,%rcx,8), %r8
	mov	8(%rdi,%rcx,8), %r9
	mov	16(%rdi,%rcx,8), %r10
	mov	24(%rdi,%rcx,8), %r11
	mov	(%rsp,%rcx,8), %r12
	mov	8(%rsp,%rcx,8), %r13
	mov	16(%rsp,%rcx,8), %r14
	mov	24(%rsp,%rcx,8), %r15
	cvtsi2sdq %r8, %xmm1
	cvtsi2sdq %r9, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	cvtsi2sdq %r12, %xmm5
	cvtsi2sdq %r13, %xmm6
	cvtsi2sdq %r14, %xmm7
	cvtsi2sdq %r15, %xmm8
	imul	%r12, %r8
	imul	%r13, %r9
	imul	%r14, %r10
	imul	%r15, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	mulsd	%xmm5, %xmm1
	mulsd	%xmm6, %xmm2
	mulsd	%xmm7, %xmm3
	mulsd	%xmm8, %xmm4
	cvtsd2siq %xmm1, %r12
	cvtsd2siq %xmm2, %r13
	cvtsd2siq %xmm3, %r14
	cvtsd2siq %xmm4, %r15

	imul	%rbx, %r12
	imul	%rbx, %r13
	imul	%rbx, %r14
	imul	%rbx, %r15
	sub	%r12, %r8
	sub	%r13, %r9
	sub	%r14, %r10
	sub	%r15, %r11
	mov	%r8, (%rdi,%rcx,8)
	mov	%r9, 8(%rdi,%rcx,8)
	mov	%r10, 16(%rdi,%rcx,8)
	mov	%r11, 24(%rdi,%rcx,8)

	sub	%rbx, %r8
	jc	0f
	mov	%r8, (%rdi,%rcx,8)
0:	sub	%rbx, %r9
	jc	1f
	mov	%r9, 8(%rdi,%rcx,8)
1:	sub	%rbx, %r10
	jc	2f
	mov	%r10, 16(%rdi,%rcx,8)
2:	sub	%rbx, %r11
	jc	3f
	mov	%r11, 24(%rdi,%rcx,8)
3:
	add	$4, %ecx
	cmp	%ecx, %esi
	ja	vec_mul

	.p2align 4,,2

vec_shift:
	shr	%rax
	jnz	vec_loop

	.p2align 4,,2

vec_done:
	mov	%rbp, %rsp
#ifdef _WIN64
	movdqa	(%rsp), %xmm6
	movdqa	16(%rsp), %xmm7
	movdqa	32(%rsp), %xmm8
	add	$56, %rsp
#else
	add	$8, %rsp
#endif
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
#ifdef _WIN64
	pop	%rdi
	pop	%rsi
#endif
	pop	%rbx
	pop	%rbp
	ret
