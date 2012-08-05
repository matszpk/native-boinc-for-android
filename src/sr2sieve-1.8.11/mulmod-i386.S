/* mulmod-i386.S -- (C) Geoffrey Reynolds, June 2007.

   Modular multiplication routines for x86.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/


#define FPU_MODE_BITS (0x0F00)	/* 64-bit precision, round to zero. */


/* uint16_t vec_mulmod64_initp_i386(const uint64_t *p);

     Setup for multiplying modulo p, where 1 < p < 2^62.
     Sets FPU to double extended precision and round-to-zero.
     Pushes 1.0/p (computed with new settings) onto the FPU stack.
     Returns old FPU precision and rounding bits.

     Argument passed in %eax [function attribute regparm(1)].


   void vec_mulmod64_initb_i386(const uint64_t *b);

     Setup for multiplying by b modulo p, where b < p < 2^62.
     Pushes b/p onto the FPU stack.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.

     Argument passed in %eax [function attribute regparm(1)].


   uint64_t mulmod64_i386(uint64_t a, uint64_t b, uint64_t p);

     returns a*b (mod p), where a,b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes stack is 8-aligned.


   void vec<N>_mulmod64_i386(const uint64_t *X, uint64_t *Y, int count);

     Assign Y[i] <-- X[i]*b (mod p) for 0 <= i < lim, where X[i] < p < 2^62,
     and where lim is the least positive multiple of N such that count <= lim.
     X and Y must be 8-aligned, and may overlap by multiples of N elements.

     Assumes vec_mulmod64_initb_[i386|sse2](b) has been called.
     Assumes stack is 8-aligned.
     Arguments passed in %eax,%edx,%ecx [function attribute regparm(3)].


   void vec_fill_i386(uint64_t *b, uint64_t *X, uint32_t n);

     Assign X[i] <-- X[0]*b^i (mod p) for 0 < i < n, where X[0],b < p < 2^62.

     Assumes vec_mulmod64_initp_[i386|sse2](p) has been called.
     Assumes X and stack are 8-aligned.
     Arguments passed in %eax,%edx,%ecx [function attribute regparm(3)].


   void vec_mulmod64_finib_i386(void);

     Release resources ready for another call to vec_mulmod64_initb_i386().
     Pops b/p off the FPU stack. (Declared inline in asm-i386-gcc.h).

     Assumes vec_mulmod64_initb_i386(b) has been called.


   void vec_mulmod64_finip_i386(uint16_t mode);

     Release resources ready for another call to vec_mulmod64_initp_i386().
     Pops 1.0/p off the FPU stack and restores FPU to mode.

     Assumes vec_mulmod64_initp_i386(p) has been called.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define mulmod64_init_i386	_mulmod64_init_i386
#define mulmod64_i386		_mulmod64_i386
#define mulmod64_fini_i386	_mulmod64_fini_i386
#define vec_mulmod64_initp_i386	_vec_mulmod64_initp_i386
#define vec_mulmod64_finip_i386	_vec_mulmod64_finip_i386
#define vec_mulmod64_initb_i386	_vec_mulmod64_initb_i386
#define vec2_mulmod64_i386	_vec2_mulmod64_i386
#define vec4_mulmod64_i386	_vec4_mulmod64_i386
#define vec8_mulmod64_i386	_vec8_mulmod64_i386
#define vec_fill_i386		_vec_fill_i386
#endif

	.comm	mulmod_data,64

#define mulmod_b	mulmod_data
#define mulmod_bh	16+mulmod_data
#define mulmod_p	32+mulmod_data
#define mulmod_ph	48+mulmod_data

	.text
	.p2align 4
	.globl	vec_mulmod64_initp_i386
	.globl	mulmod64_init_i386

vec_mulmod64_initp_i386:
	mov	(%eax), %ecx
	mov	4(%eax), %edx
	mov	%ecx, mulmod_p
	mov	%edx, 4+mulmod_p

mulmod64_init_i386:
	sub	$4, %esp

	fildll	(%eax)		/* p */
	fld1			/* 1.0, p */

	fnstcw	(%esp)
	movzwl	(%esp), %eax
	orw	$FPU_MODE_BITS, (%esp)	/* Set new mode bits */
	fldcw   (%esp)

	fdivp   %st(0), %st(1)	/* 1.0/p, using round to zero. */

	add	$4, %esp
	ret


#if !USE_INLINE_MULMOD
	.p2align 4
	.globl	mulmod64_i386

mulmod64_i386:
	push	%ebx
	sub	$16, %esp

	fildll	32(%esp)
	fildll	24(%esp)
	mov	32(%esp), %eax
	mov	32(%esp), %ecx
	mov	36(%esp), %ebx
	mull	24(%esp)
	fmulp	%st(0), %st(1)
	imul	24(%esp), %ebx
	fmul	%st(1), %st(0)
	imul	28(%esp), %ecx
	mov	%eax, 8(%esp)
	add	%ebx, %ecx
	fistpll	(%esp)
	add	%ecx, %edx
	mov	%edx, 12(%esp)
	mov	(%esp), %eax
	mov	4(%esp), %edx
	mov	44(%esp), %ebx
	mov	40(%esp), %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mull	40(%esp)
	add	%ebx, %ecx
	mov	12(%esp), %ebx
	add	%ecx, %edx
	mov	8(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	40(%esp), %ecx
	sbb	44(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	add	$16, %esp
	pop	%ebx
	ret
#endif /* !USE_INLINE_MULMOD */


	.p2align 4
	.globl	vec_mulmod64_finip_i386
	.globl	mulmod64_fini_i386

vec_mulmod64_finip_i386:
mulmod64_fini_i386:
	sub	$4, %esp

	fstp	%st(0)
	fnstcw	(%esp)
	and	$FPU_MODE_BITS, %ax
	andw	$~FPU_MODE_BITS, (%esp)	/* Clear bits */
	or	%ax, (%esp)		/* Restore old bits */
	fldcw	(%esp)

	add	$4, %esp
	ret


	.p2align 4
	.globl	vec_mulmod64_initb_i386

vec_mulmod64_initb_i386:
	mov	(%eax), %ecx
	mov	4(%eax), %edx
	mov	%ecx, mulmod_b
	mov	%edx, 4+mulmod_b

	fildll	(%eax)
	fmul	%st(1), %st(0)

	ret


	.p2align 4
	.globl	vec2_mulmod64_i386

vec2_mulmod64_i386:
	push	%ebp
	push	%ebx
	push	%esi
	push	%edi
	sub	$12+32, %esp

	lea	(%eax,%ecx,8), %ecx
	mov	%ecx, 32(%esp)
	mov	%eax, %esi
	mov	%eax, 36(%esp)
	mov	%edx, 40(%esp)

	.p2align 4,,7
loop2:
	mov	mulmod_b, %ebp
	mov	4+mulmod_b, %edi

	fildll	(%esi)
	mov	%ebp, %eax
	mov	(%esi), %ebx
	mov	4(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll (%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 8(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 12(%esp)

	fildll	8(%esi)
	mov	%ebp, %eax
	mov	8(%esi), %ebx
	mov	12(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 16(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 24(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 28(%esp)

	mov	40(%esp), %edi
	mov	mulmod_p, %ebp
	mov	4+mulmod_p, %esi

	mov	(%esp), %eax
	mov	%esi, %ebx
	mov	4(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	8(%esp), %ecx
	add	%ebx, %edx
	mov	12(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
0:
	mov	16(%esp), %eax
	mov	%esi, %ebx
	mov	20(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	24(%esp), %ecx
	add	%ebx, %edx
	mov	28(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
0:
	mov	36(%esp), %esi
	lea	16(%esi), %esi
	lea	16(%edi), %edi
	cmp	%esi, 32(%esp)
	mov	%esi, 36(%esp)
	mov	%edi, 40(%esp)
	jg	loop2

	add	$12+32, %esp
	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret


	.p2align 4
	.globl	vec4_mulmod64_i386

vec4_mulmod64_i386:
	cmp	$2, %ecx
	jle	vec2_mulmod64_i386

	push	%ebp
	push	%ebx
	push	%esi
	push	%edi
	sub	$12+64, %esp

	lea	-16(%eax,%ecx,8), %ecx
	mov	%ecx, 64(%esp)
	mov	%eax, %esi
	mov	%eax, 68(%esp)
	mov	%edx, 72(%esp)

	.p2align 4,,7
loop4:
	mov	mulmod_b, %ebp
	mov	4+mulmod_b, %edi

	fildll	(%esi)
	mov	%ebp, %eax
	mov	(%esi), %ebx
	mov	4(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll (%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 8(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 12(%esp)

	fildll	8(%esi)
	mov	%ebp, %eax
	mov	8(%esi), %ebx
	mov	12(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 16(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 24(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 28(%esp)

	fildll	16(%esi)
	mov	%ebp, %eax
	mov	16(%esi), %ebx
	mov	20(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 32(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 40(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 44(%esp)

	fildll	24(%esi)
	mov	%ebp, %eax
	mov	24(%esi), %ebx
	mov	28(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 48(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 56(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 60(%esp)

	mov	72(%esp), %edi
	mov	mulmod_p, %ebp
	mov	4+mulmod_p, %esi

	mov	(%esp), %eax
	mov	%esi, %ebx
	mov	4(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	8(%esp), %ecx
	add	%ebx, %edx
	mov	12(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
0:
	mov	16(%esp), %eax
	mov	%esi, %ebx
	mov	20(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	24(%esp), %ecx
	add	%ebx, %edx
	mov	28(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
0:
	mov	32(%esp), %eax
	mov	%esi, %ebx
	mov	36(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	40(%esp), %ecx
	add	%ebx, %edx
	mov	44(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 16(%edi)
	mov	%ebx, 20(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 16(%edi)
	mov	%ebx, 20(%edi)
0:
	mov	48(%esp), %eax
	mov	%esi, %ebx
	mov	52(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	56(%esp), %ecx
	add	%ebx, %edx
	mov	60(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 24(%edi)
	mov	%ebx, 28(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 24(%edi)
	mov	%ebx, 28(%edi)
0:
	mov	64(%esp), %eax
	mov	68(%esp), %esi
	lea	32(%esi), %esi
	lea	32(%edi), %edi
	sub	%esi, %eax
	mov	%esi, 68(%esp)
	mov	%edi, 72(%esp)
	jg	loop4

	add	$32, %esp
	add	$16, %eax
	jg	loop2

	add	$12+32, %esp
	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret


	.p2align 4
	.globl	vec8_mulmod64_i386

vec8_mulmod64_i386:
	cmp	$4, %ecx
	jle	vec4_mulmod64_i386

	push	%ebp
	push	%ebx
	push	%esi
	push	%edi
	sub	$12+128, %esp

	lea	-32(%eax,%ecx,8), %ecx
	mov	%ecx, 128(%esp)
	mov	%eax, %esi
	mov	%eax, 132(%esp)
	mov	%edx, 136(%esp)

	.p2align 4,,7
loop8:
	mov	mulmod_b, %ebp
	mov	4+mulmod_b, %edi

	fildll	(%esi)
	mov	%ebp, %eax
	mov	(%esi), %ebx
	mov	4(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll (%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 8(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 12(%esp)

	fildll	8(%esi)
	mov	%ebp, %eax
	mov	8(%esi), %ebx
	mov	12(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 16(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 24(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 28(%esp)

	fildll	16(%esi)
	mov	%ebp, %eax
	mov	16(%esi), %ebx
	mov	20(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 32(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 40(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 44(%esp)

	fildll	24(%esi)
	mov	%ebp, %eax
	mov	24(%esi), %ebx
	mov	28(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 48(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 56(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 60(%esp)

	fildll	32(%esi)
	mov	%ebp, %eax
	mov	32(%esi), %ebx
	mov	36(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 64(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 72(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 76(%esp)

	fildll	40(%esi)
	mov	%ebp, %eax
	mov	40(%esi), %ebx
	mov	44(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 80(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 88(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 92(%esp)

	fildll	48(%esi)
	mov	%ebp, %eax
	mov	48(%esi), %ebx
	mov	52(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 96(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 104(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 108(%esp)

	fildll	56(%esi)
	mov	%ebp, %eax
	mov	56(%esi), %ebx
	mov	60(%esi), %ecx
	mull	%ebx
	fmul	%st(1), %st(0)
	fistpll 112(%esp)
	imul	%edi, %ebx
	imul	%ebp, %ecx
	mov	%eax, 120(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	%edx, 124(%esp)

	mov	136(%esp), %edi
	mov	mulmod_p, %ebp
	mov	4+mulmod_p, %esi

	mov	(%esp), %eax
	mov	%esi, %ebx
	mov	4(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	8(%esp), %ecx
	add	%ebx, %edx
	mov	12(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
0:
	mov	16(%esp), %eax
	mov	%esi, %ebx
	mov	20(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	24(%esp), %ecx
	add	%ebx, %edx
	mov	28(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
0:
	mov	32(%esp), %eax
	mov	%esi, %ebx
	mov	36(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	40(%esp), %ecx
	add	%ebx, %edx
	mov	44(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 16(%edi)
	mov	%ebx, 20(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 16(%edi)
	mov	%ebx, 20(%edi)
0:
	mov	48(%esp), %eax
	mov	%esi, %ebx
	mov	52(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	56(%esp), %ecx
	add	%ebx, %edx
	mov	60(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 24(%edi)
	mov	%ebx, 28(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 24(%edi)
	mov	%ebx, 28(%edi)
0:
	mov	64(%esp), %eax
	mov	%esi, %ebx
	mov	68(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	72(%esp), %ecx
	add	%ebx, %edx
	mov	76(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 32(%edi)
	mov	%ebx, 36(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 32(%edi)
	mov	%ebx, 36(%edi)
0:
	mov	80(%esp), %eax
	mov	%esi, %ebx
	mov	84(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	88(%esp), %ecx
	add	%ebx, %edx
	mov	92(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 40(%edi)
	mov	%ebx, 44(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 40(%edi)
	mov	%ebx, 44(%edi)
0:
	mov	96(%esp), %eax
	mov	%esi, %ebx
	mov	100(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	104(%esp), %ecx
	add	%ebx, %edx
	mov	108(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 48(%edi)
	mov	%ebx, 52(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 48(%edi)
	mov	%ebx, 52(%edi)
0:
	mov	112(%esp), %eax
	mov	%esi, %ebx
	mov	116(%esp), %ecx
	imul	%eax, %ebx
	imul	%ebp, %ecx
	mull	%ebp
	add	%ecx, %ebx
	mov	120(%esp), %ecx
	add	%ebx, %edx
	mov	124(%esp), %ebx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 56(%edi)
	mov	%ebx, 60(%edi)
	sub	%ebp, %ecx
	sbb	%esi, %ebx
	jl	0f
	mov	%ecx, 56(%edi)
	mov	%ebx, 60(%edi)
0:
	mov	128(%esp), %eax
	mov	132(%esp), %esi
	lea	64(%esi), %esi
	lea	64(%edi), %edi
	sub	%esi, %eax
	mov	%esi, 132(%esp)
	mov	%edi, 136(%esp)
	jg	loop8

	add	$64, %esp
	add	$32, %eax
	jg	loop4

	add	$12+64, %esp
	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret
