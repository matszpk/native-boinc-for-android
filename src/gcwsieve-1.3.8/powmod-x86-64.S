/* powmod-x86-64.S -- (C) Geoffrey Reynolds, January 2008.

   uint64_t powmod64_x86_64(uint64_t b, uint64_t n, uint64_t p, double invp);

     Returns b^n (mod p), where 0 <= b < p < 2^51.

     Assumes current SSE rounding mode is round-to-zero.
     Assumes invp = 1.0/p computed in round-to-zero mode.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN64 1
#endif

/* Set USE_CMOV=1 to use a conditional move instead of a branch, even when
   the branch is predictable. Slower on Intel, but maybe faster on AMD.
*/
#ifndef USE_CMOV
# define USE_CMOV 0
#endif

#if NEED_UNDERSCORE
#define powmod64_x86_64 _powmod64_x86_64
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
