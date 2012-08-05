/* mulmod-x86-64.S -- (C) Geoffrey Reynolds, September 2007.

   Modular multiplication routines for x86-64 using SSE2 floating point,
   modulus is limited to 2^51.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define SSE_MODE_BITS (0x6000)	/* round to zero. */

/* uint16_t vec_mulmod64_initp_x86_64(uint64_t p);

     Setup for multiplying modulo p, where 1 < p < 2^51.
     Sets SSE mode to round-to-zero.
     Sets one_over_p = 1.0/p (computed with new settings) and mulmod_p = p.
     Returns old SSE mode rounding bits.


   void vec_mulmod64_initb_x86_64(uint64_t b);

     Setup for multiplying by b modulo p, where b < p < 2^51.
     Sets b_over_p = b/p and mulmod_b = b.

     Assumes SSE mode is round to zero.
     Assumes one_over_p = 1.0/p computed with above settings.


   void vec<N>_mulmod64_x86_64(const uint64_t *X, uint64_t *Y, int count);

     Assign Y[i] <-- X[i]*b (mod p) for 0 <= i < lim, where X[i] < p < 2^51,
     and where lim is the least positive multiple of N such that count <= lim.
     X and Y must be 8-aligned, and may overlap by multiples of N elements.

     Note that count may be negative!

     Assumes vec_mulmod64_initb_x86_64(b) has been called.


   void vec_mulmod64_finib_x86_64(void);

     Release resources ready for another call to vec_mulmod64_initb_x86_64().
     (Declared inline in asm-x86-64-gcc.h).

     Assumes vec_mulmod64_initb_x86_64(b) has been called.


   void vec_mulmod64_finip_x86_64(uint16_t mode);

     Release resources ready for another call to vec_mulmod64_initp_x86_64().
     restores SSE mode to mode.

     Assumes vec_mulmod64_initp_x86_64(p) has been called.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define mulmod64_init_x86_64		_mulmod64_init_x86_64
#define mulmod64_fini_x86_64		_mulmod64_fini_x86_64
#define vec_mulmod64_initp_x86_64	_vec_mulmod64_initp_x86_64
#define vec_mulmod64_finip_x86_64	_vec_mulmod64_finip_x86_64
#define vec_mulmod64_initb_x86_64	_vec_mulmod64_initb_x86_64
#define vec2_mulmod64_x86_64		_vec2_mulmod64_x86_64
#define vec4_mulmod64_x86_64		_vec4_mulmod64_x86_64
#define vec6_mulmod64_x86_64		_vec6_mulmod64_x86_64
#define vec8_mulmod64_x86_64		_vec8_mulmod64_x86_64
#define one_over_p			_one_over_p
#define b_over_p			_b_over_p
#endif

	.comm	mulmod_p,8
	.comm	mulmod_b,8

	.globl	one_over_p
	.globl	b_over_p

#ifdef _WIN64
#define ARG1 %rcx
#define ARG1l %ecx
#define ARG1w %cx
#define ARG2 %rdx
#define ARG2l %edx
#define ARG2w %dx
#define ARG3 %r8
#define ARG3l %r8d
#define ARG3w %r8w
#define REG4 %rsi
#define REG5 %rdi
#else
#define ARG1 %rdi
#define ARG1l %edi
#define ARG1w %di
#define ARG2 %rsi
#define ARG2l %esi
#define ARG2w %si
#define ARG3 %rdx
#define ARG3l %edx
#define ARG3w %dx
#define REG4 %r8
#define REG5 %rcx
#endif

	.text
	.p2align 4,,7
	.globl	vec_mulmod64_initp_x86_64
	.globl	mulmod64_init_x86_64

vec_mulmod64_initp_x86_64:
#ifdef _WIN64
#define VAR1 8(%rsp)
#else
#define VAR1 -8(%rsp)
#endif

	mov	ARG1, mulmod_p(%rip)

mulmod64_init_x86_64:

	stmxcsr	VAR1
	movzwl	VAR1, %eax
	orw	$SSE_MODE_BITS, VAR1
	ldmxcsr	VAR1

	cvtsi2sdq ARG1, %xmm0
	mov	$1, %edx
	cvtsi2sd %edx, %xmm1
	divsd	%xmm0, %xmm1
	movsd	%xmm1, one_over_p(%rip)

	ret

#undef VAR1


	.p2align 4,,7
	.globl	vec_mulmod64_finip_x86_64
	.globl	mulmod64_fini_x86_64

vec_mulmod64_finip_x86_64:
mulmod64_fini_x86_64:
#ifdef _WIN64
#define VAR1 8(%rsp)
#else
#define VAR1 -8(%rsp)
#endif

	stmxcsr	VAR1
	and	$SSE_MODE_BITS, ARG1w
	andw	$~SSE_MODE_BITS, VAR1
	or	ARG1w, VAR1
	ldmxcsr	VAR1

	ret

#undef VAR1

	
	.p2align 4,,15
	.globl	vec_mulmod64_initb_x86_64

vec_mulmod64_initb_x86_64:
	cvtsi2sdq ARG1, %xmm0
	mov	ARG1, mulmod_b(%rip)
	mulsd	one_over_p(%rip), %xmm0
	movsd	%xmm0, b_over_p(%rip)

	ret


	.p2align 4,,15
	.globl	vec2_mulmod64_x86_64

vec2_mulmod64_x86_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#endif
	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9
	movsd	b_over_p(%rip), %xmm0

	.p2align 4,,15
loop2:
	mov	(ARG1), %rax
	mov	8(ARG1), %r10
	cvtsi2sdq %rax, %xmm1
	cvtsi2sdq %r10, %xmm2
	imul	REG4, %rax
	imul	REG4, %r10
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	cvtsd2siq %xmm1, REG5
	cvtsd2siq %xmm2, %r11
	imul	%r9, REG5
	imul	%r9, %r11
	sub	REG5, %rax
	sub	%r11, %r10
	mov	%rax, (ARG2)
	mov	%r10, 8(ARG2)
	sub	%r9, %rax
	jl	0f
	mov	%rax, (ARG2)
0:	sub	%r9, %r10
	jl	1f
	mov	%r10, 8(ARG2)
1:
	lea	16(ARG1), ARG1
	lea	16(ARG2), ARG2
	sub	$2, ARG3l
	jg	loop2

#ifdef _WIN64
	pop	REG5
	pop	REG4
#endif
	ret


	.p2align 4,,15
	.globl	vec4_mulmod64_x86_64

vec4_mulmod64_x86_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13

	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9
	movsd	b_over_p(%rip), %xmm0

	.p2align 4,,15
loop4:
	mov	(ARG1), %rax
	mov	8(ARG1), %rbx
	mov	16(ARG1), %r10
	mov	24(ARG1), %r11
	cvtsi2sdq %rax, %xmm1
	cvtsi2sdq %rbx, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	cvtsd2siq %xmm1, REG5
	cvtsd2siq %xmm2, %rbp
	cvtsd2siq %xmm3, %r12
	cvtsd2siq %xmm4, %r13
	imul	%r9, REG5
	imul	%r9, %rbp
	imul	%r9, %r12
	imul	%r9, %r13
	sub	REG5, %rax
	sub	%rbp, %rbx
	sub	%r12, %r10
	sub	%r13, %r11
	mov	%rax, (ARG2)
	mov	%rbx, 8(ARG2)
	mov	%r10, 16(ARG2)
	mov	%r11, 24(ARG2)

	sub	%r9, %rax
	jl	0f
	mov	%rax, (ARG2)
0:	sub	%r9, %rbx
	jl	1f
	mov	%rbx, 8(ARG2)
1:	sub	%r9, %r10
	jl	2f
	mov	%r10, 16(ARG2)
2:	sub	%r9, %r11
	jl	3f
	mov	%r11, 24(ARG2)
3:
	lea	32(ARG1), ARG1
	lea	32(ARG2), ARG2
	sub	$4, ARG3l
	jg	loop4

	pop	%r13
	pop	%r12
	pop	%rbx
	pop	%rbp
#ifdef _WIN64
	pop	REG5
	pop	REG4
#endif
	ret


	.p2align 4,,15
	.globl	vec6_mulmod64_x86_64

vec6_mulmod64_x86_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#define VAR1 (%rsp)
#define VAR2 8(%rsp)
#define VAR3 16(%rsp)
#else
#define VAR1 -24(%rsp)
#define VAR2 -16(%rsp)
#define VAR3 -8(%rsp)
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15

#ifdef _WIN64
	sub	$24, %rsp
	movdqa	%xmm6, 96(%rsp)
#endif
	movsx	ARG3l, ARG3		/* Allow for negative count */
	lea	(ARG1,ARG3,8), ARG3
	mov	mulmod_b(%rip), %rax
	mov	mulmod_p(%rip), %rbx
	movsd	b_over_p(%rip), %xmm0
	mov	ARG3, VAR1
	mov	%rax, VAR2
	mov	%rbx, VAR3

	.p2align 4,,15
loop6:
	mov	(ARG1), %rax
	mov	8(ARG1), %rbx
	mov	16(ARG1), ARG3
	mov	24(ARG1), %r9
	mov	32(ARG1), %r10
	mov	40(ARG1), %r11
	mov	VAR2, REG4		/* b */
	cvtsi2sdq %rax, %xmm1
	cvtsi2sdq %rbx, %xmm2
	cvtsi2sdq ARG3, %xmm3
	cvtsi2sdq %r9, %xmm4
	cvtsi2sdq %r10, %xmm5
	cvtsi2sdq %r11, %xmm6
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, ARG3
	imul	REG4, %r9
	imul	REG4, %r10
	imul	REG4, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	mulsd	%xmm0, %xmm5
	mulsd	%xmm0, %xmm6
	mov	VAR3, REG4		/* p */
	cvtsd2siq %xmm1, REG5
	cvtsd2siq %xmm2, %rbp
	cvtsd2siq %xmm3, %r12
	cvtsd2siq %xmm4, %r13
	cvtsd2siq %xmm5, %r14
	cvtsd2siq %xmm6, %r15
	imul	REG4, REG5
	imul	REG4, %rbp
	imul	REG4, %r12
	imul	REG4, %r13
	imul	REG4, %r14
	imul	REG4, %r15
	sub	REG5, %rax
	sub	%rbp, %rbx
	sub	%r12, ARG3
	sub	%r13, %r9
	sub	%r14, %r10
	sub	%r15, %r11
	mov	%rax, (ARG2)
	mov	%rbx, 8(ARG2)
	mov	ARG3, 16(ARG2)
	mov	%r9, 24(ARG2)
	mov	%r10, 32(ARG2)
	mov	%r11, 40(ARG2)

	sub	REG4, %rax
	jl	0f
	mov	%rax, (ARG2)
0:	sub	REG4, %rbx
	jl	1f
	mov	%rbx, 8(ARG2)
1:	sub	REG4, ARG3
	jl	2f
	mov	ARG3, 16(ARG2)
2:	sub	REG4, %r9
	jl	3f
	mov	%r9, 24(ARG2)
3:	sub	REG4, %r10
	jl	4f
	mov	%r10, 32(ARG2)
4:	sub	REG4, %r11
	jl	5f
	mov	%r11, 40(ARG2)
5:
	lea	48(ARG1), ARG1
	lea	48(ARG2), ARG2
	cmp	ARG1, VAR1
	jg	loop6

#ifdef _WIN64
	movdqa	96(%rsp), %xmm6
	add	$24, %rsp
#endif

	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbx
	pop	%rbp
#ifdef _WIN64
	pop	REG5
	pop	REG4
#endif
	ret

#undef VAR1
#undef VAR2
#undef VAR3


	.p2align 4,,15
	.globl	vec8_mulmod64_x86_64

vec8_mulmod64_x86_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13

	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9
	movsd	b_over_p(%rip), %xmm0

	.p2align 4,,15
loop8:
	mov	(ARG1), %rax
	mov	8(ARG1), %rbx
	mov	16(ARG1), %r10
	mov	24(ARG1), %r11
	cvtsi2sdq %rax, %xmm1
	cvtsi2sdq %rbx, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	cvtsd2siq %xmm1, REG5
	cvtsd2siq %xmm2, %rbp
	cvtsd2siq %xmm3, %r12
	cvtsd2siq %xmm4, %r13
	imul	%r9, REG5
	imul	%r9, %rbp
	imul	%r9, %r12
	imul	%r9, %r13
	sub	REG5, %rax
	sub	%rbp, %rbx
	sub	%r12, %r10
	sub	%r13, %r11
	mov	%rax, (ARG2)
	mov	%rbx, 8(ARG2)
	mov	%r10, 16(ARG2)
	mov	%r11, 24(ARG2)

	sub	%r9, %rax
	jl	0f
	mov	%rax, (ARG2)
0:	sub	%r9, %rbx
	jl	1f
	mov	%rbx, 8(ARG2)
1:	sub	%r9, %r10
	jl	2f
	mov	%r10, 16(ARG2)
2:	sub	%r9, %r11
	jl	3f
	mov	%r11, 24(ARG2)
3:
	mov	32(ARG1), %rax
	mov	40(ARG1), %rbx
	mov	48(ARG1), %r10
	mov	56(ARG1), %r11
	cvtsi2sdq %rax, %xmm1
	cvtsi2sdq %rbx, %xmm2
	cvtsi2sdq %r10, %xmm3
	cvtsi2sdq %r11, %xmm4
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11
	mulsd	%xmm0, %xmm1
	mulsd	%xmm0, %xmm2
	mulsd	%xmm0, %xmm3
	mulsd	%xmm0, %xmm4
	cvtsd2siq %xmm1, REG5
	cvtsd2siq %xmm2, %rbp
	cvtsd2siq %xmm3, %r12
	cvtsd2siq %xmm4, %r13
	imul	%r9, REG5
	imul	%r9, %rbp
	imul	%r9, %r12
	imul	%r9, %r13
	sub	REG5, %rax
	sub	%rbp, %rbx
	sub	%r12, %r10
	sub	%r13, %r11
	mov	%rax, 32(ARG2)
	mov	%rbx, 40(ARG2)
	mov	%r10, 48(ARG2)
	mov	%r11, 56(ARG2)

	sub	%r9, %rax
	jl	0f
	mov	%rax, 32(ARG2)
0:	sub	%r9, %rbx
	jl	1f
	mov	%rbx, 40(ARG2)
1:	sub	%r9, %r10
	jl	2f
	mov	%r10, 48(ARG2)
2:	sub	%r9, %r11
	jl	3f
	mov	%r11, 56(ARG2)
3:
	lea	64(ARG1), ARG1
	lea	64(ARG2), ARG2
	sub	$8, ARG3l
	jg	loop8

	pop	%r13
	pop	%r12
	pop	%rbx
	pop	%rbp
#ifdef _WIN64
	pop	REG5
	pop	REG4
#endif
	ret
