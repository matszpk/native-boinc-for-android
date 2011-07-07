/* powmod-i386.S -- (C) Geoffrey Reynolds, November 2006.

   uint64_t powmod64(uint64_t a, uint64_t n, uint64_t p);

     Returns a^n (mod p), where a < p < 2^62.

     Assumes FPU is set to double extended precision and round to zero.
     Assumes %st(0) contains 1.0/p computed with above settings.
     Assumes that the stack is 8-aligned, unless UNALIGNED_STACK=1.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN32 1
#define UNALIGNED_STACK 1
#endif

#if NEED_UNDERSCORE
#define powmod64 _powmod64
#endif

	.text
	.p2align 4
	.globl	powmod64

powmod64:
#if UNALIGNED_STACK
	mov	%esp, %eax
	and	$~7, %esp	/* 0 mod 8 */
	pushl	24(%eax)
	pushl	20(%eax)	/* 0 mod 8 */
	pushl	16(%eax)
	pushl	12(%eax)	/* 0 mod 8 */
	pushl	8(%eax)
	pushl	4(%eax)		/* 0 mod 8 */
	push	%eax		/* 4 mod 8 */
#endif
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
#if UNALIGNED_STACK
	pop	%esp
#endif
	ret
