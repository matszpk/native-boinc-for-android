/* powmod-i386.S -- (C) Geoffrey Reynolds, November 2006.

   uint64_t powmod64_i386(uint64_t b, uint64_t n, uint64_t p);
     Returns b^n (mod p), where 0 <= b < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 8-aligned.

   void vec_powmod64_i386(uint64_t *B, int len, uint64_t n, uint64_t p);
     Assigns B[i] <-- B[i]^n (mod p) for 0 <= i < LIM, where 0 <= b < p < 2^62
     and LIM is the least multiple of 2 satisfying LIM >= len.

     Assumes that len > 0, n > 0.
     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 8-aligned.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define powmod64_i386 _powmod64_i386
#define vec_powmod64_i386 _vec_powmod64_i386
#endif

	.text
	.p2align 4
	.globl	powmod64_i386

powmod64_i386:
	push	%ebp
	push	%ebx
	sub	$36, %esp
	movl	$1, 24(%esp)
	movl	$0, 28(%esp)
	mov	48(%esp), %eax
	mov	52(%esp), %edx
	mov	56(%esp), %ebx
	mov	60(%esp), %ecx
	mov	64(%esp), %ebp
	jmp	test

	.p2align 4
sqr:
	mov	48(%esp), %eax
	mov	52(%esp), %ecx
	imul	%eax, %ecx
	fmul	%st(1), %st(0)
	fistpll	16(%esp)
	mul	%eax
	mov	%eax, 12(%esp)
	lea	(%edx,%ecx,2), %edx
	mov	16(%esp), %eax
	mov	%edx, 8(%esp)
	mov	20(%esp), %edx
	mov	68(%esp), %ebx
	mov	%ebp, %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mul	%ebp
	add	%ebx, %ecx
	mov	8(%esp), %ebx
	add	%ecx,%edx
	mov	12(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	%ebp, %ecx
	sbb	68(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:	mov	%eax, 48(%esp)
	mov	%edx, 52(%esp)

	fstp    %st(0)
	mov	60(%esp), %ecx
	mov	56(%esp), %ebx
	shrd	$1, %ecx, %ebx
	shr	%ecx
	mov	%ebx, 56(%esp)
	mov	%ecx, 60(%esp)
test:
	or	%ebx, %ecx
	je	out
	fildll	48(%esp)
	fld	%st(1)
	test	$1, %bl
	fmul	%st(1),%st(0)
	je	sqr

	fildll	24(%esp)
	mov	24(%esp), %ebx
	mov	28(%esp), %ecx
	imul	%edx, %ebx
	fmul	%st(1), %st(0)
	imul	%eax, %ecx
	fistpll	16(%esp)
	mull	24(%esp)
	mov	%eax, 8(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	16(%esp), %eax
	mov	%edx, 12(%esp)
	mov	20(%esp), %edx
	mov	68(%esp), %ebx
	mov	%ebp, %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mul	%ebp
	add	%ebx, %ecx
	mov	12(%esp), %ebx
	add	%ecx,%edx
	mov	8(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	%ebp, %ecx
	sbb	68(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:	mov	%eax, 24(%esp)
	mov	%edx, 28(%esp)
	jmp	sqr
out:
	mov	24(%esp), %eax
	mov	28(%esp), %edx
	add	$36, %esp
	pop	%ebx
	pop	%ebp
	ret


	.text
	.globl vec_powmod64_i386

	.p2align 4,,15

vec_powmod64_i386:
	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

	sub	$60, %esp
	mov	%esp, %ebp

	mov	80(%ebp), %esi		/* B */
	mov	84(%ebp), %ecx		/* len */
	lea	(%esi,%ecx,8), %eax
	mov	%eax, 32(%ebp)		/* B+len */

	inc	%ecx
	and	$-2, %ecx		/* LIM */
	shl	$3, %ecx
	sub	%ecx, %esp		/* A */

	shr	$2, %ecx
	mov	%esp, %edi
	rep
	movsl				/* A[i] <-- B[i] for 0 <= i < LIM */

	mov	92(%ebp), %edx		/* high dword of n */
	bsr	%edx, %ecx		/* ZF predictable */
	setz	36(%ebp)		/* true iff high dword done */
	jnz	0f
	mov	88(%ebp), %edx		/* low dword of n */
	bsr	%edx, %ecx		/* ZF=0 assumed */
0:
	mov	%edx, 40(%ebp)		/* Current dword of n */
	dec	%ecx			/* SF=0 predicted */
	jl	vec_next
	mov	$1, %eax
	shl	%cl, %eax
	mov	%eax, 44(%ebp)		/* x <-- second highest bit of n */


	.p2align 4,,2

vec_loop:
	mov	80(%ebp), %edi		/* B */
	mov	96(%ebp), %esi		/* pl */

	.p2align 4,,7

vec_sqr:
	/* %edi = B+i, %st(0) = 1.0/p */

	fildll	(%edi)
	mov	(%edi), %eax
	mov	4(%edi), %ebx
	fmul	%st(0), %st(0)
	imul	%eax, %ebx
	fmul	%st(1), %st(0)
	mul	%eax
	mov	%eax, 12(%ebp)
	add	%ebx, %edx
	fistpll	(%ebp)
	add	%ebx, %edx
	mov	%edx, 8(%ebp)

	fildll	8(%edi)
	mov	8(%edi), %eax
	mov	12(%edi), %ecx
	fmul	%st(0), %st(0)
	imul	%eax, %ecx
	fmul	%st(1), %st(0)
	mul	%eax
	mov	%eax, 28(%ebp)
	add	%ecx, %edx
	fistpll	16(%ebp)
	add	%ecx, %edx
	mov	%edx, 24(%ebp)

	mov	(%ebp), %eax
	mov	100(%ebp), %ebx
	mov	4(%ebp), %ecx
	imul	%eax, %ebx
	imul	%esi, %ecx
	mul	%esi
	add	%ebx, %ecx
	mov	8(%ebp), %ebx
	add	%ecx,%edx
	mov	12(%ebp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
	sub	%esi, %ecx
	sbb	100(%ebp), %ebx
	jl	0f
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
0:
	mov	16(%ebp), %eax
	mov	100(%ebp), %ebx
	mov	20(%ebp), %ecx
	imul	%eax, %ebx
	imul	%esi, %ecx
	mul	%esi
	add	%ebx, %ecx
	mov	24(%ebp), %ebx
	add	%ecx,%edx
	mov	28(%ebp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
	sub	%esi, %ecx
	sbb	100(%ebp), %ebx
	jl	1f
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
1:
	lea	16(%edi), %edi
	cmp	%edi, 32(%ebp)
	ja	vec_sqr

	mov	44(%ebp), %eax		/* x */
	test	%eax, 40(%ebp)		/* ZF unpredictable */
	jz	vec_shift

	mov	%esp, %esi		/* A */
	mov	80(%ebp), %edi		/* B */

	.p2align 4,,7

vec_mul:
	/* %edi = B+i, %esi = A+i, %st(0) = 1.0/p */

	fildll	(%edi)
	fildll	(%esi)
	mov	(%esi), %eax
	mov	4(%esi), %ebx
	mov	%eax, %ecx
	mull	(%edi)
	fmulp	%st(0), %st(1)
	imul	(%edi), %ebx
	fmul	%st(1), %st(0)
	imul	4(%edi), %ecx
	mov	%eax, 12(%ebp)
	add	%ebx, %ecx
	fistpll	(%ebp)
	add	%ecx, %edx
	mov	%edx, 8(%ebp)

	fildll	8(%edi)
	fildll	8(%esi)
	mov	8(%esi), %eax
	mov	12(%esi), %ebx
	mov	%eax, %ecx
	mull	8(%edi)
	fmulp	%st(0), %st(1)
	imul	8(%edi), %ebx
	fmul	%st(1), %st(0)
	imul	12(%edi), %ecx
	mov	%eax, 28(%ebp)
	add	%ebx, %ecx
	fistpll	16(%ebp)
	add	%ecx, %edx
	mov	%edx, 24(%ebp)

	mov	(%ebp), %eax
	mov	100(%ebp), %ebx
	mov	4(%ebp), %ecx
	imul	%eax, %ebx
	imul	96(%ebp), %ecx
	mull	96(%ebp)
	add	%ebx, %ecx
	mov	8(%ebp), %ebx
	add	%ecx,%edx
	mov	12(%ebp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
	sub	96(%ebp), %ecx
	sbb	100(%ebp), %ebx
	jl	0f
	mov	%ecx, (%edi)
	mov	%ebx, 4(%edi)
0:
	mov	16(%ebp), %eax
	mov	100(%ebp), %ebx
	mov	20(%ebp), %ecx
	imul	%eax, %ebx
	imul	96(%ebp), %ecx
	mull	96(%ebp)
	add	%ebx, %ecx
	mov	24(%ebp), %ebx
	add	%ecx,%edx
	mov	28(%ebp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
	sub	96(%ebp), %ecx
	sbb	100(%ebp), %ebx
	jl	1f
	mov	%ecx, 8(%edi)
	mov	%ebx, 12(%edi)
1:
	lea	16(%edi), %edi
	lea	16(%esi), %esi
	cmp	%edi, 32(%ebp)
	ja	vec_mul

	.p2align 4,,2

vec_shift:
	mov	44(%ebp), %eax
	shr	%eax
	mov	%eax, 44(%ebp)		/* x >>= 1 */
	jnz	vec_loop

vec_next:
	testb	$1, 36(%ebp)		/* 1 iff high dword done */
	mov	88(%ebp), %edx		/* low dword */
	movl	$0x80000000, 44(%ebp)	/* x <-- high bit of next dword */
	movb	$1, 36(%ebp)		/* high dword done */
	mov	%edx, 40(%ebp)		/* Current dword */
	jz	vec_loop

	.p2align 4,,2

vec_done:
	mov	%ebp, %esp
	add	$60, %esp
	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	ret
