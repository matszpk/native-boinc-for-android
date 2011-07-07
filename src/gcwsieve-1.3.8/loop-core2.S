/* loop-core2.S -- (C) Geoffrey Reynolds, August 2007.

   Main loop for x86-64 machines, optimised for Intel Core 2.

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
#define PREFETCH_ITER 8


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


/* This loop is optimised for the Core 2 processor, which appears to be
   limited only by instruction throughput, and in practice achieves a
   throughput of about 8 clock cycles per term.

   int swizzle_loop4_core2(const uint64_t *T, loop_data_t *D, int i,
                           uint64_t p);

   Assumes that D was composed with SWIZZLE=4.
   Assumes that one_over_p = 1.0/p computed in round-to-zero mode.
   Assumes that the current SSE rounding mode is round-to-zero.
*/

#if NEED_UNDERSCORE
#define one_over_p _one_over_p
#define swizzle_loop4_core2 _swizzle_loop4_core2
#define swizzle_loop4_core2_prefetch _swizzle_loop4_core2_prefetch
#endif

	.globl	one_over_p


	.text
#if USE_PREFETCH
	.globl	swizzle_loop4_core2_prefetch
#else
	.globl	swizzle_loop4_core2
#endif
	.p2align 4

#if USE_PREFETCH
swizzle_loop4_core2_prefetch:
#else
swizzle_loop4_core2:
#endif

	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
	sub	$56, %rsp

	mov	%rcx, %rdi
	mov	%rdx, %rsi
	mov	%r8, %rdx
	mov	%r9, %rcx
	movdqa	%xmm6, (%rsp)
	movdqa	%xmm7, 16(%rsp)
	movdqa	%xmm8, 32(%rsp)
#endif
	movsd	one_over_p(%rip), %xmm0	/* 1.0/p */
#ifdef _WIN64
	mov	%rdi, 96(%rsp)		/* T */
	mov	%rsi, 104(%rsp)		/* D */
	mov	%edx, %eax		/* i */
	mov	%rcx, 120(%rsp)		/* p */
#else
	mov	%rdi, -8(%rsp)		/* T */
	mov	%rsi, -16(%rsp)		/* D */
	mov	%edx, %eax		/* i */
	mov	%rcx, -24(%rsp)		/* p */
#endif
	mov	(%rsi), %r8		/* D->X[0] */
	mov	8(%rsi), %r9		/* D->X[1] */
	mov	16(%rsi), %r10		/* D->X[2] */
	mov	24(%rsi), %r11		/* D->X[3] */
	sal	$5, %rdx
	lea	32(%rsi,%rdx), %rsi	/* &(D->R[i]) */
	jmp	test

	.p2align 4
loop:
#ifdef _WIN64
	mov	96(%rsp), %rbp		/* T */
#else
	mov	-8(%rsp), %rbp		/* T */
#endif
#if USE_PREFETCH
	prefetchnta -32*PREFETCH_ITER(%rsi)
#endif
	mov	16(%rsi), %edi
	mov	20(%rsi), %ebx
	mov	24(%rsi), %ecx
	mov	28(%rsi), %edx
	mov	(%rbp,%rdi,8), %rdi
	mov	(%rbp,%rbx,8), %rbx
	mov	(%rbp,%rcx,8), %rcx
	mov	(%rbp,%rdx,8), %rdx

	cvtsi2sdq %r8, %xmm1
	cvtsi2sdq %r9, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	imul	%rdi, %r8
	imul	%rbx, %r9
	imul	%rcx, %r10
	imul	%rdx, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	cvtsi2sdq %rdi, %xmm5
	cvtsi2sdq %rbx, %xmm6
	cvtsi2sdq %rcx, %xmm7
	cvtsi2sdq %rdx, %xmm8
	mulsd	%xmm5, %xmm1
	mulsd	%xmm6, %xmm2
	mulsd	%xmm7, %xmm3
	mulsd	%xmm8, %xmm4
	cvtsd2siq %xmm1, %rdi
	cvtsd2siq %xmm2, %rbx
	cvtsd2siq %xmm3, %rcx
	cvtsd2siq %xmm4, %rdx
#ifdef _WIN64
	mov	120(%rsp), %rbp		/* p */
#else
	mov	-24(%rsp), %rbp		/* p */
#endif
	imul	%rbp, %rdi
	imul	%rbp, %rbx
	imul	%rbp, %rcx
	imul	%rbp, %rdx
	sub	%rdi, %r8
	sub	%rbx, %r9
	sub	%rcx, %r10
	sub	%rdx, %r11

	mov	%r8, %rdi
	mov	%r9, %rbx
	mov	%r10, %rcx
	mov	%r11, %rdx

	sub	%rbp, %rdi
	jl	0f
	mov	%rdi, %r8
0:	sub	%rbp, %rbx
	jl	1f
	mov	%rbx, %r9
1:	sub	%rbp, %rcx
	jl	2f
	mov	%rcx, %r10
2:	sub	%rbp, %rdx
	jl	3f
	mov	%rdx, %r11
3:
	mov	(%rsi), %edi
	mov	4(%rsi), %ebx
	mov	8(%rsi), %ecx
	mov	12(%rsi), %edx
	cmp	%r8, %rdi
	jz	out
	cmp	%r9, %rbx
	jz	out
	cmp	%r10, %rcx
	jz	out
	cmp	%r11, %rdx
	jz	out
#ifdef SMALL_P
	sub	%ebp, %edi
	sub	%ebp, %ebx
	sub	%ebp, %ecx
	sub	%ebp, %edx
	cmp	%r8, %rdi
	jz	out
	cmp	%r9, %rbx
	jz	out
	cmp	%r10, %rcx
	jz	out
	cmp	%r11, %rdx
	jz	out
#endif
test:
	dec	%eax			/* --i */
	lea	-32(%rsi), %rsi		/* &(D->R[i]) */
	jge	loop
out:
#ifdef _WIN64
	mov	104(%rsp), %rsi		/* D */
#else
	mov	-16(%rsp), %rsi		/* D */
#endif
	mov	%r8, (%rsi)		/* D->X[0] */
	mov	%r9, 8(%rsi)		/* D->X[1] */
	mov	%r10, 16(%rsi)		/* D->X[2] */
	mov	%r11, 24(%rsi)		/* D->X[3] */

#ifdef _WIN64
	movdqa	(%rsp), %xmm6
	movdqa	16(%rsp), %xmm7
	movdqa	32(%rsp), %xmm8

	add	$56, %rsp
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	ret
