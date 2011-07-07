/* loop-a64.S -- (C) Geoffrey Reynolds, August 2007.

   Main loop for x86-64 machines, optimised for AMD Athlon 64.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN64 1
#endif

/* Prefetch data for this number of loop iterations in advance.
*/
#define PREFETCH_ITER 6


/* The following C function was used as a template:

   struct loop_rec_t { uint32_t N[SWIZZLE]; uint32_t G[SWIZZLE]; };
   struct loop_data_t { uint64_t X[SWIZZLE]; struct loop_rec_t R[0]; };

   int swizzle_loop(const uint64_t *T, loop_data_t *D, int i, uint64_t p)
   {
     while (--i >= 0)
     {
       int j;

       for (j = 0; j < SWIZZLE; j++)
         D->X[j] = mulmod64(D->X[j],T[D->R[i].G[j]],p);

       for (j = 0; j < SWIZZLE; j++)
         if (D->X[j] == D->R[i].N[j])
           return i;
     }

     return i;
   }
*/


/* This loop is optimised for the Athlon 64 architecture. The important
   difference between Athlon 64 and Core 2 is the higher latency of the
   cvtsi2sdq/cvtsd2siq instructions on the Athlon 64. Using SWIZZLE=6
   hides the 12 clocks latency of the cvtsi2sdq instructions, but there
   are not enough general registers to fully hide the 10 clocks latency
   of the cvtsd2siq instructions. Conditional moves are used because,
   unlike on the Core 2, `cmovl' results in about the same throughput as
   a correctly predicted `jl'.

   int swizzle_loop6_a64(const uint64_t *T, loop_data_t *D, int i, uint64_t p);

   Assumes that D was composed with SWIZZLE=6.
   Assumes that one_over_p = 1.0/p computed in round-to-zero mode.
   Assumes that the current SSE rounding mode is round-to-zero.
*/


#if NEED_UNDERSCORE
#define one_over_p _one_over_p
#define swizzle_loop6_a64 _swizzle_loop6_a64
#define swizzle_loop6_a64_prefetch _swizzle_loop6_a64_prefetch
#endif

	.globl	one_over_p


	.text
#if USE_PREFETCH
	.globl	swizzle_loop6_a64_prefetch
#else
	.globl	swizzle_loop6_a64
#endif
	.p2align 4

#if USE_PREFETCH
swizzle_loop6_a64_prefetch:
#else
swizzle_loop6_a64:
#endif

	push	%r15
	push	%r14
	push	%r13
	push	%r12
	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
	sub	$120, %rsp

	mov	%rcx, %rdi
	mov	%rdx, %rsi
	mov	%r8, %rdx
	mov	%r9, %rcx
	movdqa	%xmm6, (%rsp)
	movdqa	%xmm7, 16(%rsp)
	movdqa	%xmm8, 32(%rsp)
	movdqa	%xmm9, 48(%rsp)
	movdqa	%xmm10, 64(%rsp)
	movdqa	%xmm11, 80(%rsp)
	movdqa	%xmm12, 96(%rsp)
#endif
	movsd	one_over_p(%rip), %xmm0	/* 1.0/p */
#ifdef _WIN64
	mov	%rdi, 192(%rsp)
	mov	%rsi, 200(%rsp)
	mov	%edx, %eax
	mov	%rcx, 216(%rsp)
#else
	mov	%rdi, -8(%rsp)
	mov	%rsi, -16(%rsp)
	mov	%edx, %eax
	mov	%rcx, -24(%rsp)
#endif
	mov	40(%rsi), %rdi
	mov	32(%rsi), %r8
	mov	24(%rsi), %r9
	mov	16(%rsi), %rbx
	mov	8(%rsi), %rcx
	mov	(%rsi), %rdx
	imul	$48, %rax, %rbp
	lea	48(%rsi,%rbp), %rsi
	jmp	test

	.p2align 4
loop:
#ifdef _WIN64
	mov	192(%rsp), %rbp		/* T */
#else
	mov	-8(%rsp), %rbp		/* T */
#endif
#if USE_PREFETCH
	prefetchnta -48*PREFETCH_ITER(%rsi)
#endif
	mov	44(%rsi), %r15d
	mov	40(%rsi), %r14d
	mov	36(%rsi), %r13d
	mov	32(%rsi), %r12d
	mov	28(%rsi), %r11d
	mov	24(%rsi), %r10d

	mov	(%rbp,%r15,8), %r15
	mov	(%rbp,%r14,8), %r14
	mov	(%rbp,%r13,8), %r13
	mov	(%rbp,%r12,8), %r12
	mov	(%rbp,%r11,8), %r11
	mov	(%rbp,%r10,8), %r10

	cvtsi2sdq %rdi, %xmm1
	cvtsi2sdq %r8, %xmm2
	cvtsi2sdq %r9, %xmm3
	cvtsi2sdq %rbx, %xmm4
	cvtsi2sdq %rcx, %xmm5
	cvtsi2sdq %rdx, %xmm6
	cvtsi2sdq %r15, %xmm7
	cvtsi2sdq %r14, %xmm8
	cvtsi2sdq %r13, %xmm9
	cvtsi2sdq %r12, %xmm10
	cvtsi2sdq %r11, %xmm11
	cvtsi2sdq %r10, %xmm12
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	mulsd	%xmm0, %xmm5
	mulsd	%xmm0, %xmm6
	mulsd	%xmm7, %xmm1
	mulsd	%xmm8, %xmm2
	mulsd	%xmm9, %xmm3
	mulsd	%xmm10, %xmm4
	mulsd	%xmm11, %xmm5
	mulsd	%xmm12, %xmm6

	/* Idealy these imuls should be started once all the cvtsd2siqs
	   are in progress, to hide the 10 cycle latency of cvtsd2siq.
	   However there are no spare registers.
	 */
	imul	%r15, %rdi
	cvtsd2siq %xmm1, %r15
	imul	%r14, %r8
	cvtsd2siq %xmm2, %r14
	imul	%r13, %r9
	cvtsd2siq %xmm3, %r13
	imul	%r12, %rbx
	cvtsd2siq %xmm4, %r12
	imul	%r11, %rcx
	cvtsd2siq %xmm5, %r11
	imul	%r10, %rdx
	cvtsd2siq %xmm6, %r10
#ifdef _WIN64
	mov	216(%rsp), %rbp		/* p */
#else
	mov	-24(%rsp), %rbp		/* p */
#endif
	imul	%rbp, %r15
	imul	%rbp, %r14
	imul	%rbp, %r13
	imul	%rbp, %r12
	imul	%rbp, %r11
	imul	%rbp, %r10
	sub	%r15, %rdi
	sub	%r14, %r8
	sub	%r13, %r9
	sub	%r12, %rbx
	sub	%r11, %rcx
	sub	%r10, %rdx

	mov	%rdi, %r15
	mov	%r8, %r14
	mov	%r9, %r13
	mov	%rbx, %r12
	mov	%rcx, %r11
	mov	%rdx, %r10

	sub	%rbp, %rdi
	cmovl	%r15, %rdi
	sub	%rbp, %r8
	cmovl	%r14, %r8
	sub	%rbp, %r9
	cmovl	%r13, %r9
	sub	%rbp, %rbx
	cmovl	%r12, %rbx
	sub	%rbp, %rcx
	cmovl	%r11, %rcx
	sub	%rbp, %rdx
	cmovl	%r10, %rdx

	mov	20(%rsi), %r15d
	cmp	%rdi, %r15
	jz	out
	mov	16(%rsi), %r14d
	cmp	%r8, %r14
	jz	out
	mov	12(%rsi), %r13d
	cmp	%r9, %r13
	jz	out
	mov	8(%rsi), %r12d
	cmp	%rbx, %r12
	jz	out
	mov	4(%rsi), %r11d
	cmp	%rcx, %r11
	jz	out
	mov	(%rsi), %r10d
	cmp	%rdx, %r10
	jz	out
#ifdef SMALL_P
	sub	%ebp, %r15d
	cmp	%rdi, %r15
	jz	out
	sub	%ebp, %r14d
	cmp	%r8, %r14
	jz	out
	sub	%ebp, %r13d
	cmp	%r9, %r13
	jz	out
	sub	%ebp, %r12d
	cmp	%rbx, %r12
	jz	out
	sub	%ebp, %r11d
	cmp	%rcx, %r11
	jz	out
	sub	%ebp, %r10d
	cmp	%rdx, %r10
	jz	out
#endif
test:
	dec	%eax
	lea	-48(%rsi), %rsi
	jge	loop
out:
#ifdef _WIN64
	mov	200(%rsp), %rsi		/* D */
#else
	mov	-16(%rsp), %rsi		/* D */
#endif
	mov	%rdi, 40(%rsi)
	mov	%r8, 32(%rsi)
	mov	%r9, 24(%rsi)
	mov	%rbx, 16(%rsi)
	mov	%rcx, 8(%rsi)
	mov	%rdx, (%rsi)

#ifdef _WIN64
	movdqa	(%rsp), %xmm6
	movdqa	16(%rsp), %xmm7
	movdqa	32(%rsp), %xmm8
	movdqa	48(%rsp), %xmm9
	movdqa	64(%rsp), %xmm10
	movdqa	80(%rsp), %xmm11
	movdqa	96(%rsp), %xmm12

	add	$120, %rsp
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	pop	%r12
	pop	%r13
	pop	%r14
	pop	%r15
	ret
