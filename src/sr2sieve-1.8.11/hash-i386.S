/* hash-i386.S -- (C) Geoffrey Reynolds, October 2007.

   Hash table routines for x86. See prototypes in bsgs.c.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if NEED_UNDERSCORE
#define	BJ64		_BJ64
#define	build_hashtable_i386	_build_hashtable_i386
#define	search_hashtable_i386	_search_hashtable_i386
#endif

	.globl	BJ64

	.text

	.globl	build_hashtable_i386
	.p2align 4,,15

build_hashtable_i386:
	push	%edi
	push	%esi
	push	%ebp
	push	%ebx
	push	%eax			/* m */

	mov	BJ64, %esi		/* BJ64 */
	mov	-4(%esi), %edi		/* htable */
	mov	-12(%esi), %ecx		/* hsize/2 */
	mov	-20(%esi), %eax		/* empty_slot pattern */
	rep
	stosl				/* Clear the main table. */

	movzwl	%ax, %eax		/* empty_slot */
	push	%eax
	movl	$-1, (%esi,%eax,8)	/* Fill empty slot */
	movl	$-1, 4(%esi,%eax,8)

	mov	-4(%esi), %edi		/* htable */
	mov	-16(%esi), %ebp		/* hsize_minus1 */
	mov	4(%esp), %eax		/* m */
	mov	(%esi), %edx		/* BJ64[0] */
	mov	4(%esi), %ecx
	mov	%edx, (%esi,%eax,8)	/* BJ64[m] */
	mov	%ecx, 4(%esi,%eax,8)

	xor	%ebx, %ebx		/* i <-- 0 */
	jmp	build_table

	.p2align 3,,7
build_overflow_list:
	mov	-8(%esi), %ecx		/* olist */
	xor	$0x8000, %eax
	mov	%ax, (%ecx,%ebx,2)
	mov	%ebx, %eax
	inc	%ebx
	or	$0x8000, %eax
	mov	4(%esi), %ecx
	mov	%ax, (%edi,%edx,2)
	mov	(%esi,%ebx,8), %edx
	cmp	(%esi), %edx
	jne	build_table
	cmp	4(%esi,%ebx,8), %ecx
	je	build_done

	.p2align 4,,7
build_table:
	and	%ebp, %edx
	movzwl	(%edi,%edx,2), %eax
	cmp	(%esp), %eax		/* empty_slot */
	jne	build_overflow_list
	mov	%bx, (%edi,%edx,2)
	inc	%ebx
	mov	(%esi,%ebx,8), %edx
	cmp	(%esi), %edx
	jne	build_table
	cmp	4(%esi,%ebx,8), %ecx
	jne	build_table

	.p2align 4,,7
build_done:
	pop	%edx			/* empty_slot */
	pop	%ecx			/* m */
	xor	%eax, %eax
	cmp	%ecx, %ebx
	jae	0f
	mov	%ebx, %eax
0:
	pop	%ebx
	pop	%ebp
	pop	%esi
	pop	%edi
	ret


	/* Assumes cc > 0 */
	/* Returns index i < cc of first match, or i=cc if none. */

	.globl	search_hashtable_i386
	.p2align 4,,15

search_hashtable_i386:
	push	%ebp
	push	%edi
	push	%esi
	push	%ebx

	mov	BJ64, %ecx
	mov	20(%esp), %edx		/* D64 */
	xor	%ebp, %ebp		/* i <-- 0 */

	.p2align 4,,7
search_loop:
	mov	(%edx), %esi		/* D64[i] */
	mov	-16(%ecx), %eax		/* hsize_minus1 */
	mov	-4(%ecx), %ebx		/* htable */
	and	%esi, %eax		/* slot */
	movzwl	(%ebx,%eax,2), %ebx	/* htable[slot] */
	mov	%ebx, %eax
	and	$0x7FFF, %eax
	mov	4(%edx), %edi
	cmp	(%ecx,%eax,8), %esi
	jne	check_overflow
	cmp	4(%ecx,%eax,8), %edi
	je	search_done /* found */

check_overflow:
	testw	%bx, %bx
	js	overflow

search_continue:
	inc	%ebp
	mov	20(%esp), %edx		/* D64 */
	cmp	24(%esp), %ebp		/* cc == i? */
	lea	(%edx,%ebp,8), %edx
	jb	search_loop

	.p2align 3
search_done:
	mov	%ebp, %eax		/* i */
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%ebp
	ret

	.p2align 4,,15
overflow_loop:
	testw	%bx, %bx
	js	search_continue

overflow:
	mov	-8(%ecx), %edx			/* olist */
	movzwl	(%edx,%eax,2), %eax
	mov	%eax, %ebx
	and	$0x7FFF, %eax
	cmp	(%ecx,%eax,8), %esi
	jne	overflow_loop
	cmp	4(%ecx,%eax,8), %edi
	jne	overflow_loop

	jmp	search_done /* found */
