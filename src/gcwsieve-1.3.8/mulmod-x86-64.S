/* mulmod-x86-64.S -- (C) Geoffrey Reynolds, November 2006.

   Modular multiplication for x86-64/SSE2.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define SSE_MODE_BITS (0x6000)		/* Round To Zero */


/* void mod64_init_x86_64(uint64_t p);

     Set SSE rounding mode to round-to-zero.
     Set one_over_p to 1.0/p, computed in round-to-zero mode.
     Saves the old rounding mode in mod64_rnd.


   void mod64_fini_x86_64(void);

     Restore the previous SSE rounding mode from mod64_rnd.


   uint64_t mulmod64_x86_64(uint64_t a, uint64_t b, uint64_t p);

     returns a*b (mod p), where a,b < p < 2^51.

     Assumes one_over_p = 1.0/p computed in round-to-zero mode.
     Assumes current SSE rounding mode is round-to-zero.
*/


#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN64 1
#endif

#if NEED_UNDERSCORE
#define one_over_p _one_over_p
#define mod64_rnd _mod64_rnd
#define mod64_init_x86_64 _mod64_init_x86_64
#define mod64_fini_x86_64 _mod64_fini_x86_64
#define mulmod64_x86_64 _mulmod64_x86_64
#endif

	.globl	one_over_p
	.globl	mod64_rnd


	.text

	.globl	mod64_init_x86_64
	.p2align 4,,7

mod64_init_x86_64:
#ifdef _WIN64
	stmxcsr	8(%rsp)
	mov	8(%rsp), %eax
	and	$SSE_MODE_BITS, %eax
	orl	$SSE_MODE_BITS, 8(%rsp)
	mov	%eax, mod64_rnd(%rip)	/* Save old bits */
	ldmxcsr	8(%rsp)
	cvtsi2sdq %rcx, %xmm0
#else
	stmxcsr	-8(%rsp)
	mov	-8(%rsp), %eax
	and	$SSE_MODE_BITS, %eax
	orl	$SSE_MODE_BITS, -8(%rsp)
	mov	%eax, mod64_rnd(%rip)	/* Save old bits */
	ldmxcsr	-8(%rsp)
	cvtsi2sdq %rdi, %xmm0
#endif
	mov	$1, %eax
	cvtsi2sd %eax, %xmm1
	divsd	%xmm0, %xmm1
	movsd	%xmm1, one_over_p(%rip)

	ret


	.globl	mod64_fini_x86_64
	.p2align 4,,7

mod64_fini_x86_64:
#ifdef _WIN64
	stmxcsr	8(%rsp)
	mov	8(%rsp), %eax
	and	$~SSE_MODE_BITS, %eax
	or	mod64_rnd(%rip), %eax	/* Restore old bits */
	mov	%eax, 8(%rsp)
	ldmxcsr	8(%rsp)
#else
	stmxcsr	-8(%rsp)
	mov	-8(%rsp), %eax
	and	$~SSE_MODE_BITS, %eax
	or	mod64_rnd(%rip), %eax	/* Restore old bits */
	mov	%eax, -8(%rsp)
	ldmxcsr	-8(%rsp)
#endif
	ret


	.globl	mulmod64_x86_64
	.p2align 4

mulmod64_x86_64:
#ifdef _WIN64
	cvtsi2sdq %rcx, %xmm0
	cvtsi2sdq %rdx, %xmm1
	mulsd	one_over_p(%rip), %xmm0
	mulsd	%xmm0, %xmm1
	cvtsd2siq %xmm1, %rax
	imul	%rdx, %rcx
	imul	%r8, %rax
	sub	%rax, %rcx
	mov	%rcx, %rax
	sub	%r8, %rcx
	jl	0f
	mov	%rcx, %rax
#else
	cvtsi2sdq %rdi, %xmm0
	cvtsi2sdq %rsi, %xmm1
	mulsd	one_over_p(%rip), %xmm0
	mulsd	%xmm0, %xmm1
	cvtsd2siq %xmm1, %rax
	imul	%rsi, %rdi
	imul	%rdx, %rax
	sub	%rax, %rdi
	mov	%rdi, %rax
	sub	%rdx, %rdi
	jl	0f
	mov	%rdi, %rax
#endif
0:	ret
