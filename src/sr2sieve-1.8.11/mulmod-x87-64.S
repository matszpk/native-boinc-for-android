/* mulmod-x87-64.S -- (C) Geoffrey Reynolds, September 2007.

   Modular multiplication routines for x86-64 using x87 floating point.
   modulus is limited to 2^62.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define FPU_MODE_BITS (0x0F00)	/* 64-bit precision, round to zero. */

/* uint16_t vec_mulmod64_initp_x87_64(uint64_t p);

     Setup for multiplying modulo p, where 1 < p < 2^62.
     Sets FPU mode to double extended precision and round-to-zero.
     Pushes 1.0/p (computed with new settings) into the FPU stack.
     Sets mulmod_p = p.
     Returns old FPU mode precision and rounding bits.


   void vec_mulmod64_initb_x87_64(uint64_t b);

     Setup for multiplying by b modulo p, where b < p < 2^62.
     Pushes b/p onto the FPU stack and sets mulmod_b = b.

     Assumes FPU mode is double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.


   void vec<N>_mulmod64_x87_64(const uint64_t *X, uint64_t *Y, int count);

     Assign Y[i] <-- X[i]*b (mod p) for 0 <= i < lim, where X[i] < p < 2^62,
     and where lim is the least positive multiple of N such that count <= lim.
     X and Y must be 8-aligned, and may overlap by multiples of N elements.

     Note that count may be negative!

     Assumes vec_mulmod64_initb_x87_64(b) has been called.


   void vec_mulmod64_finib_x87_64(void);

     Release resources ready for another call to vec_mulmod64_initb_x87_64().
     (Declared inline in asm-x86-64-gcc.h).

     Assumes vec_mulmod64_initb_x87_64(b) has been called.


   void vec_mulmod64_finip_x87_64(uint16_t mode);

     Release resources ready for another call to vec_mulmod64_initp_x87_64().
     restores FPU mode to mode.

     Assumes vec_mulmod64_initp_x87_64(p) has been called.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define mulmod64_init_x87_64		_mulmod64_init_x87_64
#define mulmod64_fini_x87_64		_mulmod64_fini_x87_64
#define vec_mulmod64_initp_x87_64	_vec_mulmod64_initp_x87_64
#define vec_mulmod64_finip_x87_64	_vec_mulmod64_finip_x87_64
#define vec_mulmod64_initb_x87_64	_vec_mulmod64_initb_x87_64
#define vec2_mulmod64_x87_64		_vec2_mulmod64_x87_64
#define vec4_mulmod64_x87_64		_vec4_mulmod64_x87_64
#define vec6_mulmod64_x87_64		_vec6_mulmod64_x87_64
#define vec8_mulmod64_x87_64		_vec8_mulmod64_x87_64
#endif

	.comm	mulmod_p,8
	.comm	mulmod_b,8

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
	.globl	vec_mulmod64_initp_x87_64
	.globl	mulmod64_init_x87_64

vec_mulmod64_initp_x87_64:
mulmod64_init_x87_64:
#ifdef _WIN64
#define VAR1 8(%rsp)
#else
#define VAR1 -8(%rsp)
#endif

	fnstcw	VAR1
	mov	ARG1, mulmod_p(%rip)
	movzwl	VAR1, %eax		/* Return old mode */
	orw	$FPU_MODE_BITS, VAR1	/* Set new mode bits */
	fldcw   VAR1

	fildll	mulmod_p(%rip)		/* p */
	fld1				/* 1.0, p */
	fdivp   %st(0), %st(1)		/* 1.0/p, using round to zero. */

	ret

#undef VAR1


	.p2align 4,,7
	.globl	vec_mulmod64_finip_x87_64
	.globl	mulmod64_fini_x87_64

vec_mulmod64_finip_x87_64:
mulmod64_fini_x87_64:
#ifdef _WIN64
#define VAR1 8(%rsp)
#else
#define VAR1 -8(%rsp)
#endif

	fstp	%st(0)

	fnstcw	VAR1
	and	$FPU_MODE_BITS, ARG1w
	andw	$~FPU_MODE_BITS, VAR1	/* Clear bits */
	or	ARG1w, VAR1		/* Restore old bits */
	fldcw	VAR1

	ret

#undef VAR1

	
	.p2align 4,,15
	.globl	vec_mulmod64_initb_x87_64

vec_mulmod64_initb_x87_64:
	mov	ARG1, mulmod_b(%rip)
	fildll	mulmod_b(%rip)
	fmul	%st(1), %st(0)

	ret


	.p2align 4,,15
	.globl	vec2_mulmod64_x87_64

vec2_mulmod64_x87_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#define VAR1 24(%rsp)
#define VAR2 32(%rsp)
#else
#define VAR1 -16(%rsp)
#define VAR2 -8(%rsp)
#endif

	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9

	.p2align 4,,15
loop2:
	mov	(ARG1), %rax
	mov	8(ARG1), %r10
	fildll	(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR1
	fildll	8(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR2
	imul	REG4, %rax
	imul	REG4, %r10
	mov	VAR1, REG5
	mov	VAR2, %r11
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

#undef VAR1
#undef VAR2


	.p2align 4,,15
	.globl	vec4_mulmod64_x87_64

vec4_mulmod64_x87_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#define VAR1 56(%rsp)
#define VAR2 64(%rsp)
#define VAR3 72(%rsp)
#define VAR4 80(%rsp)
#else
#define VAR1 -32(%rsp)
#define VAR2 -24(%rsp)
#define VAR3 -16(%rsp)
#define VAR4 -8(%rsp)
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13

	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9

	.p2align 4,,15
loop4:
	mov	(ARG1), %rax
	mov	8(ARG1), %rbx
	mov	16(ARG1), %r10
	mov	24(ARG1), %r11
	fildll	(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR1
	fildll	8(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR2
	fildll	16(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR3
	fildll	24(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR4
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11
	mov	VAR1, REG5
	mov	VAR2, %rbp
	mov	VAR3, %r12
	mov	VAR4, %r13
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

#undef VAR1
#undef VAR2
#undef VAR3
#undef VAR4


	.p2align 4,,15
	.globl	vec6_mulmod64_x87_64

vec6_mulmod64_x87_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#define VAR1 48(%rsp)
#define VAR2 56(%rsp)
#define VAR3 64(%rsp)
#define VAR4 (%rsp)
#define VAR5 8(%rsp)
#define VAR6 16(%rsp)
#define VAR7 24(%rsp)
#define VAR8 32(%rsp)
#define VAR9 40(%rsp)
#else
#define VAR1 -72(%rsp)
#define VAR2 -64(%rsp)
#define VAR3 -56(%rsp)
#define VAR4 -48(%rsp)
#define VAR5 -40(%rsp)
#define VAR6 -32(%rsp)
#define VAR7 -24(%rsp)
#define VAR8 -16(%rsp)
#define VAR9 -8(%rsp)
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15
#ifdef _WIN64
	sub	$72, %rsp
#endif
	movsx	ARG3l, ARG3		/* Allow for negative count */
	lea	(ARG1,ARG3,8), ARG3
	mov	mulmod_b(%rip), %rax
	mov	mulmod_p(%rip), %rbx
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
	mov	VAR2, REG4
	fildll	(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR4
	fildll	8(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR5
	fildll	16(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR6
	fildll	24(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR7
	fildll	32(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR8
	fildll	40(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR9
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, ARG3
	imul	REG4, %r9
	imul	REG4, %r10
	imul	REG4, %r11
	mov	VAR3, REG4
	mov	VAR4, REG5
	mov	VAR5, %rbp
	mov	VAR6, %r12
	mov	VAR7, %r13
	mov	VAR8, %r14
	mov	VAR9, %r15
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
	add	$72, %rsp
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
#undef VAR4
#undef VAR5
#undef VAR6
#undef VAR7
#undef VAR8
#undef VAR9

	
	.p2align 4,,15
	.globl	vec8_mulmod64_x87_64

vec8_mulmod64_x87_64:
#ifdef _WIN64
	push	REG4
	push	REG5
#define VAR1 (%rsp)
#define VAR2 8(%rsp)
#define VAR3 16(%rsp)
#define VAR4 24(%rsp)
#define VAR5 32(%rsp)
#define VAR6 40(%rsp)
#define VAR7 48(%rsp)
#define VAR8 56(%rsp)
#else
#define VAR1 -64(%rsp)
#define VAR2 -56(%rsp)
#define VAR3 -48(%rsp)
#define VAR4 -40(%rsp)
#define VAR5 -32(%rsp)
#define VAR6 -24(%rsp)
#define VAR7 -16(%rsp)
#define VAR8 -8(%rsp)
#endif
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13
#ifdef _WIN64
	sub	$72, %rsp
#endif
	mov	mulmod_b(%rip), REG4
	mov	mulmod_p(%rip), %r9

	.p2align 4,,15
loop8:
	fildll	(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR1
	fildll	8(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR2
	fildll	16(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR3
	fildll	24(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR4

	mov	(ARG1), %rax
	mov	8(ARG1), %rbx
	mov	16(ARG1), %r10
	mov	24(ARG1), %r11
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11

	fildll	32(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR5
	fildll	40(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR6
	fildll	48(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR7
	fildll	56(ARG1)
	fmul	%st(1), %st(0)
	fistpll	VAR8

	mov	VAR1, REG5
	mov	VAR2, %rbp
	mov	VAR3, %r12
	mov	VAR4, %r13
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
	imul	REG4, %rax
	imul	REG4, %rbx
	imul	REG4, %r10
	imul	REG4, %r11
	mov	VAR5, REG5
	mov	VAR6, %rbp
	mov	VAR7, %r12
	mov	VAR8, %r13
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

#ifdef _WIN64
	add	$72, %rsp
#endif
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
#undef VAR4
#undef VAR5
#undef VAR6
#undef VAR7
#undef VAR8
