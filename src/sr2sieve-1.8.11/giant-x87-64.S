/* giant-x87-64.S -- (C) Geoffrey Reynolds, October 2007.

   One-pass giant step routine for x86-64 using x87 floating point,
   modulus is limited to 2^62.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

/* giant4_x87_64(const uint64_t *D, uint32_t cc, uint32_t M, void (*fun)(,,,),
                 uint64_t b, uint64_t p);
*/
 
#if NEED_UNDERSCORE
#define giant4_x87_64	_giant4_x87_64
#define BJ64		_BJ64
#endif

	.globl	BJ64

	.text

	.p2align 4,,15
	.globl	giant4_x87_64

giant4_x87_64:
	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
#endif
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	sub	$56, %rsp

#ifdef _WIN64
	mov	%rcx, %rdi
	mov	%rdx, %rsi
	mov	%r8, %rdx
	mov	%r9, %rcx
	mov	56+64+8+32+0(%rsp), %r8
	mov	56+64+8+32+8(%rsp), %r9
#endif

#ifdef _WIN64
	fildll	56+64+8+32+0(%rsp)		/* b */
#else
	mov	%r8, (%rsp)			/* b */
	fildll	(%rsp)
#endif
	fmul	%st(1), %st(0)			/* b/p */

	mov	%rcx, 48(%rsp)			/* fun */
	mov	%esi, 44(%rsp)			/* cc */
	dec	%edx
	mov	%edx, 40(%rsp)			/* M-1 */

	mov	%rdi, 32(%rsp)			/* D */
	dec	%esi
	mov	%esi, %eax			/* cc-1 */
	and	$-4, %esi
	lea	(%rdi,%rsi,8), %rdi		/* D+(cc-1)-(cc-1)%4 */

	mov	BJ64(%rip), %rsi		/* BJ64 */
	mov	-16(%rsi), %r15			/* olist */

	test	$2, %al
	jnz	cc_loop4			/* cc == 0 or 3 mod 4 */

cc_loop2:
	mov	(%rdi), %rax			/* D[i] */
	mov	8(%rdi), %rbx			/* D[i+1] */
	mov	%rax, (%rsp)
	mov	%rbx, 8(%rsp)
	mov	40(%rsp), %r14d			/* M-1 */
	mov	-32(%rsi), %r12d		/* hsize_minus1 */
	mov	-8(%rsi), %r13			/* htable */

	.p2align 4,,7
M_loop2:
	fildll	(%rsp)
	fmul	%st(1), %st(0)
	fistpll	(%rsp)
	fildll	8(%rsp)
	fmul	%st(1), %st(0)
	fistpll	8(%rsp)
	imul	%r8, %rax
	imul	%r8, %rbx
	mov	(%rsp), %rcx
	mov	8(%rsp), %rdx
	imul	%r9, %rcx
	imul	%r9, %rdx
	sub	%rcx, %rax
	sub	%rdx, %rbx

	mov	%rax, %rcx
	mov	%rbx, %rdx
#if USE_CMOV
	sub	%r9, %rax
	cmovl	%rcx, %rax
	mov     %rax, (%rsp)
	sub	%r9, %rbx
	cmovl	%rdx, %rbx
	mov     %rbx, 8(%rsp)
#else
	sub	%r9, %rcx
	jl	0f
	mov	%rcx, %rax
0:	mov     %rax, (%rsp)
	sub	%r9, %rdx
	jl	1f
	mov	%rdx, %rbx
1:	mov     %rbx, 8(%rsp)
#endif

	mov	%eax, %ecx
	mov	%ebx, %edx
	and	%r12d, %ecx
	and	%r12d, %edx
	movzwl	(%r13,%rcx,2), %ecx
	movzwl	(%r13,%rdx,2), %edx

search2_0:
	mov	%ecx, %ebp
	and	$0x7FFF, %ebp
	cmp	%rax, (%rsi,%rbp,8)
	je	search2_found0
	test	%cx, %cx
	jns	search2_1

	.p2align 4,,4
overflow2_0:
	movzwl	(%r15,%rbp,2), %ecx
	mov	%ecx, %ebp
	and	$0x7FFF, %ebp
	cmp	%rax, (%rsi,%rbp,8)
	je	search2_found0
	test	%cx, %cx
	jns	overflow2_0

	.p2align 4,,1
search2_1:
	mov	%edx, %ebp
	and	$0x7FFF, %ebp
	cmp	%rbx, (%rsi,%rbp,8)
	je	search2_found1
	test	%dx, %dx
	jns	search2_done

	.p2align 4,,4
overflow2_1:
	movzwl	(%r15,%rbp,2), %edx
	mov	%edx, %ebp
	and	$0x7FFF, %ebp
	cmp	%rbx, (%rsi,%rbp,8)
	je	search2_found1
	test	%dx, %dx
	jns	overflow2_1

	.p2align 4,,2
search2_done:
	dec	%r14d
	jnz	M_loop2
	jmp	cc_next4

	.p2align 4,,4
search2_found0:
	push	$0
	call	eliminate
	jmp	search2_1

	.p2align 4,,4
search2_found1:
	push	$1
	call	eliminate
	jmp	search2_done

	.p2align 4,,7
cc_loop4:
	mov	(%rdi), %r10			/* D[i] */
	mov	8(%rdi), %r11			/* D[i+1] */
	mov	16(%rdi), %r12			/* D[i+2] */
	mov	24(%rdi), %r13			/* D[i+3] */

	fildll	(%rdi)
	fmul	%st(1), %st(0)
	fistpll	(%rsp)
	fildll	8(%rdi)
	fmul	%st(1), %st(0)
	fistpll	8(%rsp)
	fildll	16(%rdi)
	fmul	%st(1), %st(0)
	fistpll	16(%rsp)
	fildll	24(%rdi)
	fmul	%st(1), %st(0)
	fistpll	24(%rsp)

	mov	40(%rsp), %r14d			/* M-1 */

	.p2align 4,,7
M_loop4:
	imul	%r8, %r10
	imul	%r8, %r11
	imul	%r8, %r12
	imul	%r8, %r13
	mov	(%rsp), %rax
	mov	8(%rsp), %rbx
	mov	16(%rsp), %rcx
	mov	24(%rsp), %rdx
	imul	%r9, %rax
	imul	%r9, %rbx
	imul	%r9, %rcx
	imul	%r9, %rdx
	sub	%rax, %r10
	sub	%rbx, %r11
	sub	%rcx, %r12
	sub	%rdx, %r13

	mov	%r10, %rax
	mov	%r11, %rbx
	mov	%r12, %rcx
	mov	%r13, %rdx
#if USE_CMOV
	sub	%r9, %r10
	cmovl	%rax, %r10
	mov	%r10, (%rsp)
	sub	%r9, %r11
	cmovl	%rbx, %r11
	mov	%r11, 8(%rsp)
	sub	%r9, %r12
	cmovl	%rcx, %r12
	mov	%r12, 16(%rsp)
	sub	%r9, %r13
	cmovl	%rdx, %r13
	mov	%r13, 24(%rsp)
#else
	sub	%r9, %rax
	jl	0f
	mov	%rax, %r10
0:	mov	%r10, (%rsp)
	sub	%r9, %rbx
	jl	1f
	mov	%rbx, %r11
1:	mov	%r11, 8(%rsp)
	sub	%r9, %rcx
	jl	2f
	mov	%rcx, %r12
2:	mov	%r12, 16(%rsp)
	sub	%r9, %rdx
	jl	3f
	mov	%rdx, %r13
3:	mov	%r13, 24(%rsp)
#endif

	mov	%r10d, %eax
	mov	%r11d, %ebx
	mov	%r12d, %ecx
	mov	%r13d, %edx

	mov	-32(%rsi), %ebp			/* hsize_minus1 */
	and	%ebp, %eax
	and	%ebp, %ebx
	and	%ebp, %ecx
	and	%ebp, %edx

	mov	-8(%rsi), %rbp			/* htable */
	movzwl	(%rbp,%rax,2), %eax
	movzwl	(%rbp,%rbx,2), %ebx
	movzwl	(%rbp,%rcx,2), %ecx
	movzwl	(%rbp,%rdx,2), %edx

	/* Schedule these FPU operations ready for the next iteration.
	*/
	fildll	(%rsp)
	fmul	%st(1), %st(0)
	fistpll	(%rsp)
	fildll	8(%rsp)
	fmul	%st(1), %st(0)
	fistpll	8(%rsp)
	fildll	16(%rsp)
	fmul	%st(1), %st(0)
	fistpll	16(%rsp)
	fildll	24(%rsp)
	fmul	%st(1), %st(0)
	fistpll	24(%rsp)

search4_0:
	mov	%eax, %ebp
	and	$0x7FFF, %ebp
	cmp	%r10, (%rsi,%rbp,8)
	je	search4_found0
	test	%ax, %ax
	jns	search4_1

	.p2align 4,,4
overflow4_0:
	movzwl	(%r15,%rbp,2), %eax
	mov	%eax, %ebp
	and	$0x7FFF, %ebp
	cmp	%r10, (%rsi,%rbp,8)
	je	search4_found0
	test	%ax, %ax
	jns	overflow4_0

	.p2align 4,,1
search4_1:
	mov	%ebx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r11, (%rsi,%rbp,8)
	je	search4_found1
	test	%bx, %bx
	jns	search4_2

	.p2align 4,,4
overflow4_1:
	movzwl	(%r15,%rbp,2), %ebx
	mov	%ebx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r11, (%rsi,%rbp,8)
	je	search4_found1
	test	%bx, %bx
	jns	overflow4_1

	.p2align 4,,1
search4_2:
	mov	%ecx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r12, (%rsi,%rbp,8)
	je	search4_found2
	test	%cx, %cx
	jns	search4_3

	.p2align 4,,4
overflow4_2:
	movzwl	(%r15,%rbp,2), %ecx
	mov	%ecx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r12, (%rsi,%rbp,8)
	je	search4_found2
	test	%cx, %cx
	jns	overflow4_2

	.p2align 4,,1
search4_3:
	mov	%edx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r13, (%rsi,%rbp,8)
	je	search4_found3
	test	%dx, %dx
	jns	search4_done

	.p2align 4,,4
overflow4_3:
	movzwl	(%r15,%rbp,2), %edx
	mov	%edx, %ebp
	and	$0x7FFF, %ebp
	cmp	%r13, (%rsi,%rbp,8)
	je	search4_found3
	test	%dx, %dx
	jns	overflow4_3

	.p2align 4,,2
search4_done:
	dec	%r14d
	jnz	M_loop4

	.p2align 4,,3
cc_next4:
	lea	-32(%rdi), %rdi
	cmp	32(%rsp), %rdi
	jae	cc_loop4

out:
	fstp	%st(0)
	add	$56, %rsp
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	ret

	.p2align 4,,1
search4_found0:
	push	$0
	call	eliminate
	jmp	search4_1

	.p2align 4,,1
search4_found1:
	push	$1
	call	eliminate
	jmp	search4_2

	.p2align 4,,1
search4_found2:
	push	$2
	call	eliminate
	jmp	search4_3

	.p2align 4,,1
search4_found3:
	push	$3
	call	eliminate
	jmp	search4_done

/*	%ebp = j
	%r9 = p
	%r14d = i-1
	%rdi = D+k	*/

	.p2align 4,,3
eliminate:
	push	%rax
	push	%rcx
	push	%rdx
#ifndef _WIN64
	push	%rdi
	push	%rsi
#endif
	push	%r8
	push	%r9
	push	%r10
	push	%r11
#ifdef _WIN64
	lea	64(%rsp), %rax
	sub	$40, %rsp			/* align + shadow space */

	mov	%rdi, %r8
	sub	32+8(%rax), %r8
	shr	$3, %r8d
	add	(%rax), %r8d			/* k */
	cmp	44+8(%rax), %r8d
	jae	0f				/* Garbage due to overrun. */
	mov	40+8(%rax), %ecx
	sub	%r14d, %ecx
	inc	%ecx				/* i */
	mov	%ebp, %edx			/* j */
	call	*(48+8)(%rax)			/* fun(i,j,k,p) */
0:
	add	$40, %rsp
#else
	lea	80(%rsp), %rax
	sub	$8, %rsp			/* align stack */

	sub	32+8(%rax), %rdi
	shr	$3, %edi
	add	(%rax), %edi			/* k */
	cmp	44+8(%rax), %edi
	jae	0f				/* Garbage due to overrun. */
	mov	%edi, %edx
	mov	40+8(%rax), %edi
	sub	%r14d, %edi
	inc	%edi				/* i */
	mov	%ebp, %esi			/* j */
	mov	%r9, %rcx			/* p */
	call	*(48+8)(%rax)			/* fun(i,j,k,p) */
0:
	add	$8, %rsp
#endif
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
#ifndef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rdx
	pop	%rcx
	pop	%rax

	ret	$8
