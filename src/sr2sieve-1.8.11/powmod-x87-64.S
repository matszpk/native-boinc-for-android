/* powmod-x87-64.S -- (C) Geoffrey Reynolds, January 2008.

   uint64_t powmod64_x87_64(uint64_t b, uint64_t n, uint64_t p);
     Returns b^n (mod p), where 0 <= b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.

   void vec_powmod64_x87_64(uint64_t *B, int len, uint64_t n, uint64_t p);
     Assigns B[i] <-- B[i]^n (mod p) for 0 <= i < LIM, where 0 <= b < p < 2^62
     and LIM is the least multiple of 4 satisfying LIM >= len.

     Assumes that len > 0, n > 0, and that B is 16-aligned.
     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"

#if NEED_UNDERSCORE
#define powmod64_x87_64 _powmod64_x87_64
#define vec_powmod64_x87_64 _vec_powmod64_x87_64
#endif

/* Set USE_CMOV=1 to use a conditional move instead of a branch, even when
   the branch is predictable. Slower on Intel, but maybe faster on AMD.
*/
#ifndef USE_CMOV
# define USE_CMOV 0
#endif

#ifdef _WIN64
#define ARG1 %rcx
#define ARG2 %rdx
#define ARG3 %r8
#define REG4 %r9
#define REG4l %r9d
#define REG5 %r10
#define REG6 %r11
#define TMP 16(%rsp) /* Shadow space */
#else
#define ARG1 %rdi
#define ARG2 %rsi
#define ARG3 %rdx
#define REG4 %rcx
#define REG4l %ecx
#define REG5 %r8
#define REG6 %r9
#define TMP -32(%rsp) /* Red zone */
#endif


	.text
	.globl powmod64_x87_64

	.p2align 4,,15

powmod64_x87_64:
	push	%rbp
	lea	TMP, %rbp
	mov	$1, %eax
	mov	$1, REG4l

	.p2align 4,,15

mulsqr_loop:
	/* %rax = REG4 = a, ARG1 = b, ARG2 = n, ARG3 = p, %st(0) = 1.0/p */

	mov	ARG1, (%rbp)
	mov	%rax, 8(%rbp)
	fildll	(%rbp)
	fildll	8(%rbp)
	fmul	%st(2), %st(0)
	fmul	%st(1), %st(0)
	fistpll	16(%rbp)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	24(%rbp)
	imul	ARG1, %rax
	imul	ARG1, ARG1
	mov	16(%rbp), REG5
	mov	24(%rbp), REG6
	imul	ARG3, REG5
	imul	ARG3, REG6
	sub	REG5, %rax
	sub	REG6, ARG1
	mov	%rax, REG5
	mov	ARG1, REG6

correct_mul:
	sub     ARG3, REG5	/* CF=0 predicted */
#if USE_CMOV
	cmovnc	REG5, %rax
#else
	jc      correct_sqr
	mov     REG5, %rax
#endif

correct_sqr:
	sub     ARG3, REG6	/* CF=0 predicted */
#if USE_CMOV
	cmovnc	REG6, ARG1
#else
	jc	shift
	mov     REG6, ARG1
#endif

shift:
	/* REG4 = a, %rax = a*b, ARG1 = b^2, ARG2 = n */

	shr	ARG2		/* CF unpredictable */
	cmovnc	REG4, %rax	/* Discard multiply if CF=0 */
	mov	%rax, REG4
	jnz	mulsqr_loop

	pop	%rbp
	ret


#undef ARG1
#undef ARG2
#undef ARG3
#undef REG4
#undef REG4l
#undef REG5
#undef REG6
#undef TMP

#ifdef _WIN64 /* Shadow space */
#define VAR1 80(%rbp)
#define VAR2 88(%rbp)
#define VAR3 96(%rbp)
#define VAR4 104(%rbp)
#else /* Stack */
#define VAR1 (%rbp)
#define VAR2 8(%rbp)
#define VAR3 16(%rbp)
#define VAR4 24(%rbp)
#endif

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
	.globl vec_powmod64_x87_64

	.p2align 4,,15

vec_powmod64_x87_64:
	/* %rdi = B, %esi = len, %rdx = n, %rcx = p, %st(0) = 1.0/p */

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
	sub	$8, %rsp		/* 16-aligned */
	mov	%rcx, %rdi
	mov	%rdx, %rsi
	mov	%r8, %rdx
	mov	%r9, %rcx
#else
	sub	$40, %rsp		/* 16-aligned */
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
	/* %rbx = p, %rdi = B, %esi = len, %rdx = n, %rax = x, */
	/* %rsp = A, %rbp = frame, %st(0) = 1.0/p */

	xor	%ecx, %ecx		/* i <-- 0 */

	.p2align 4,,7

vec_sqr:
	fildll	(%rdi,%rcx,8)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	VAR1
	fildll	8(%rdi,%rcx,8)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	VAR2
	fildll	16(%rdi,%rcx,8)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	VAR3
	fildll	24(%rdi,%rcx,8)
	fmul	%st(0), %st(0)
	fmul	%st(1), %st(0)
	fistpll	VAR4

	mov	(%rdi,%rcx,8), %r8
	mov	8(%rdi,%rcx,8), %r9
	mov	16(%rdi,%rcx,8), %r10
	mov	24(%rdi,%rcx,8), %r11
	imul	%r8, %r8
	imul	%r9, %r9
	imul	%r10, %r10
	imul	%r11, %r11
	mov	VAR1, %r12
	mov	VAR2, %r13
	mov	VAR3, %r14
	mov	VAR4, %r15
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
	/* %rbx = p, %rdi = B, %esi = len, %rdx = n, %rax = x, */
	/* %rsp = A, %rbp = frame, %st(0) = 1.0/p */

	fildll	(%rdi,%rcx,8)
	fildll	(%rsp,%rcx,8)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	VAR1
	fildll	8(%rdi,%rcx,8)
	fildll	8(%rsp,%rcx,8)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	VAR2
	fildll	16(%rdi,%rcx,8)
	fildll	16(%rsp,%rcx,8)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	VAR3
	fildll	24(%rdi,%rcx,8)
	fildll	24(%rsp,%rcx,8)
	fmulp	%st(0), %st(1)
	fmul	%st(1), %st(0)
	fistpll	VAR4

	mov	(%rdi,%rcx,8), %r8
	mov	8(%rdi,%rcx,8), %r9
	mov	16(%rdi,%rcx,8), %r10
	mov	24(%rdi,%rcx,8), %r11
	mov	(%rsp,%rcx,8), %r12
	mov	8(%rsp,%rcx,8), %r13
	mov	16(%rsp,%rcx,8), %r14
	mov	24(%rsp,%rcx,8), %r15
	imul	%r12, %r8
	imul	%r13, %r9
	imul	%r14, %r10
	imul	%r15, %r11
	mov	VAR1, %r12
	mov	VAR2, %r13
	mov	VAR3, %r14
	mov	VAR4, %r15
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
	add	$8, %rsp
#else
	add	$40, %rsp
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
