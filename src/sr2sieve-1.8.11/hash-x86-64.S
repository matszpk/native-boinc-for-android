/* hash-x86-64.S -- (C) Geoffrey Reynolds, October 2007.

   Hash table routines for x86-64. See prototypes in bsgs.c.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define	BJ64		_BJ64
#define	build_hashtable_x86_64	_build_hashtable_x86_64
#define	search_hashtable_x86_64	_search_hashtable_x86_64
#endif

	.globl	BJ64

	.text

	.globl	build_hashtable_x86_64
	.p2align 4,,15

build_hashtable_x86_64:
	push	%rbx
	push	%rbp
#ifdef _WIN64
	push	%rdi
	push	%rsi
	mov	%ecx, %r9d
#else
	mov	%edi, %r9d
#endif
	mov	BJ64(%rip), %rsi		/* BJ64 */
	mov	-8(%rsi), %r8			/* htable */
	mov	-16(%rsi), %r10			/* olist */

	mov	(%rsi), %r11			/* bj0 */
	mov	%r11, (%rsi,%r9,8)		/* BJ64[m] <-- bj0 */

	/* Clear hashtable */
	mov	%r8, %rdi
	mov	-24(%rsi), %ecx			/* hsize/4 */
	mov	-40(%rsi), %rax			/* empty_slot pattern */
	rep
	stosq					/* Clear the main table */

	movzwl	%ax, %ebp			/* empty_slot */
	mov	-32(%rsi), %edi			/* hsize_minus1 */
	movq	$-1, (%rsi,%rbp,8)		/* BJ[empty_slot] <-- -1 */
	xor	%ebx, %ebx
	mov	%r11d, %edx			/* bj0 */
	jmp	build_table

	.p2align 4,,7
build_overflow_list:
	xor	$0x8000, %eax
	mov	%ax, (%r10,%rbx,2)
	mov	%ebx, %eax
	inc	%ebx
	or	$0x8000, %eax
	mov	%ax, (%r8,%rcx,2)
	cmp	%rdx, %r11
	je	build_done

	.p2align 4,,7
build_table:
	mov	%edx, %ecx
	mov	8(%rsi,%rbx,8), %rdx
	and	%edi, %ecx
	movzwl	(%r8,%rcx,2), %eax
	cmp	%ebp, %eax			/* empty_slot */
	jne	build_overflow_list
	mov	%bx, (%r8,%rcx,2)
	inc	%ebx
	cmp	%rdx, %r11
	jne	build_table

	.p2align 4,,7
build_done:
	xor	%eax, %eax
	cmp	%r9d, %ebx
	jae	0f
	mov	%ebx, %eax
0:
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbp
	pop	%rbx
	ret



	/* Assumes cc > 0 */
	/* Returns index i < cc of first match, or i=cc if none. */

	.globl	search_hashtable_x86_64
	.p2align 4,,15

search_hashtable_x86_64:
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
	mov	%rcx, %rdi
	mov	%edx, %r9d
#else
	mov	%esi, %r9d
#endif
	mov	BJ64(%rip), %rsi
	xor	%ebx, %ebx
	mov	-8(%rsi), %r11			/* htable */
	mov	-16(%rsi), %r10			/* olist */
	mov	-32(%rsi), %r8d			/* hsize_minus1 */

	.p2align 4,,7
search_loop:
	mov	%r8d, %eax
	mov	(%rdi,%rbx,8), %rcx
	and	%ecx, %eax
	movzwl	(%r11,%rax,2), %edx
	mov	%edx, %eax
	and	$0x7FFF, %eax
	cmp	%rcx, (%rsi,%rax,8)
	je	search_done /* found */

	testw	%dx, %dx
	js	overflow

search_continue:
	inc	%ebx
	cmp	%r9d, %ebx
	jb	search_loop

search_done:
	mov	%ebx, %eax
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	ret

	.p2align 4,,7
overflow_loop:
	testw	%dx, %dx
	js	search_continue

overflow:
	movzwl	(%r10,%rax,2), %edx
	mov	%edx, %eax
	and	$0x7FFF, %eax
	cmp	%rcx, (%rsi,%rax,8)
	jne	overflow_loop
	jmp	search_done /* found */
