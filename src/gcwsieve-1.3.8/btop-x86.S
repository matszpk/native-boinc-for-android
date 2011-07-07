/* btop-x86.S -- (C) Geoffrey Reynolds, September 2007.

   Build table of powers for x86 machines without SSE2.

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

#define VECTOR_LENGTH 4

/* void btop_x86(const uint32_t *L, uint64_t *T, uint32_t lmin,
                 uint32_t llen, uint64_t p);


   Given T[0], assigns T[j] <-- T[0]^(j+1) (mod p) for 0 < j <= lmin,
   and, if llen > 0, also assigns T[k] <-- T[0]^(k+1) (mod p) for those
   k in {lmin+L[i] : 0 <= i < llen}.

   Assumes T[0] < p < 2^62.
   Assumes that L was composed with VECTOR_LENGTH 4.
   Assumes FPU is set to double extended precision and round to zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
   Assumes that the stack is 8-aligned, unless UNALIGNED_STACK=1
*/

#if NEED_UNDERSCORE
#define btop_x86 _btop_x86
#endif

	.text
	.p2align 4
	.globl	btop_x86

btop_x86:
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
	push	%edi
	push	%esi
	push	%ebx
	sub	$28, %esp

	mov	52(%esp), %ebp				/* T */
	fildll	(%ebp)					/* T[0] */
	fmul	%st(1),%st(0)

	lea	8*(VECTOR_LENGTH-1)(%ebp), %edi		/* T+VECTOR_LENGTH-1 */
	mov	%ebp, %esi
	mov	(%ebp), %eax
	mov	4(%ebp), %edx

fill:
	mov	(%ebp), %ebx				/* T[0] lo */
	mov	4(%ebp), %ecx				/* T[0] hi */
	fildll	(%esi)
	imul	%edx, %ebx
	fmul	%st(1), %st(0)
	imul	%eax, %ecx
	fistpll	8(%esp)
	mull	(%ebp)
	mov	%eax, 4(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	8(%esp), %eax
	mov	%edx, (%esp)
	mov	12(%esp), %edx
	mov	68(%esp), %ebx				/* p hi */
	mov	64(%esp), %ecx				/* p lo */
	imul	%eax, %ebx
	imul	%edx, %ecx
	mull	64(%esp)				/* p lo */
	add	%ebx, %ecx
	mov	(%esp), %ebx
	add	%ecx,%edx
	mov	4(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	64(%esp), %ecx				/* p lo */
	sbb	68(%esp), %ebx				/* p hi */
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	mov	%eax, 8(%esi)
	mov	%edx, 12(%esi)
	add	$8, %esi
	cmp	%edi, %esi
	jb	fill

	fstp	%st(0)
	cmpl	$(VECTOR_LENGTH), 56(%esp)
	jb	ladder

	fildll	8*(VECTOR_LENGTH-1)(%ebp)		/* T[3] */
	fmul	%st(1),%st(0)

	mov	56(%esp), %edi				/* lmin */
	mov	%ebp, %esi
	lea	-8*VECTOR_LENGTH(%ebp,%edi,8), %edi

	.p2align 4,,7
next:
	fildll	(%esi)
	fmul	%st(1), %st(0)
	fistpll	32(%esi)
	fildll	8(%esi)
	fmul	%st(1), %st(0)
	fistpll	40(%esi)
	fildll	16(%esi)
	fmul	%st(1), %st(0)
	fistpll	48(%esi)
	fildll	24(%esi)
	fmul	%st(1), %st(0)
	fistpll	56(%esi)

	mov	(%esi), %ebx
	mov	4(%esi), %ecx
	mov	24(%ebp), %eax
	imul	28(%ebp), %ebx
	imul	%eax, %ecx
	mull	(%esi)
	mov	%eax, (%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	32(%esi), %eax
	mov	%edx, 4(%esp)
	mov	68(%esp), %ebx
	mov	64(%esp), %ecx
	imul	%eax, %ebx
	imul	36(%esi), %ecx
	mull	64(%esp)
	add	%ebx, %ecx
	mov	4(%esp), %ebx
	add	%ecx,%edx
	mov	(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 32(%esi)
	mov	%ebx, 36(%esi)
	sub	64(%esp), %ecx
	sbb	68(%esp), %ebx
	jl	0f
	mov	%ecx, 32(%esi)
	mov	%ebx, 36(%esi)
0:	mov	8(%esi), %ebx
	mov	12(%esi), %ecx
	mov	24(%ebp), %eax
	imul	28(%ebp), %ebx
	imul	%eax, %ecx
	mull	8(%esi)
	mov	%eax, (%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	40(%esi), %eax
	mov	%edx, 4(%esp)
	mov	68(%esp), %ebx
	mov	64(%esp), %ecx
	imul	%eax, %ebx
	imul	44(%esi), %ecx
	mull	64(%esp)
	add	%ebx, %ecx
	mov	4(%esp), %ebx
	add	%ecx,%edx
	mov	(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 40(%esi)
	mov	%ebx, 44(%esi)
	sub	64(%esp), %ecx
	sbb	68(%esp), %ebx
	jl	1f
	mov	%ecx, 40(%esi)
	mov	%ebx, 44(%esi)
1:	mov	16(%esi), %ebx
	mov	20(%esi), %ecx
	mov	24(%ebp), %eax
	imul	28(%ebp), %ebx
	imul	%eax, %ecx
	mull	16(%esi)
	mov	%eax, (%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	48(%esi), %eax
	mov	%edx, 4(%esp)
	mov	68(%esp), %ebx
	mov	64(%esp), %ecx
	imul	%eax, %ebx
	imul	52(%esi), %ecx
	mull	64(%esp)
	add	%ebx, %ecx
	mov	4(%esp), %ebx
	add	%ecx,%edx
	mov	(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 48(%esi)
	mov	%ebx, 52(%esi)
	sub	64(%esp), %ecx
	sbb	68(%esp), %ebx
	jl	2f
	mov	%ecx, 48(%esi)
	mov	%ebx, 52(%esi)
2:	mov	24(%esi), %ebx
	mov	28(%esi), %ecx
	mov	24(%ebp), %eax
	imul	28(%ebp), %ebx
	imul	%eax, %ecx
	mull	24(%esi)
	mov	%eax, (%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	56(%esi), %eax
	mov	%edx, 4(%esp)
	mov	68(%esp), %ebx
	mov	64(%esp), %ecx
	imul	%eax, %ebx
	imul	60(%esi), %ecx
	mull	64(%esp)
	add	%ebx, %ecx
	mov	4(%esp), %ebx
	add	%ecx,%edx
	mov	(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, 56(%esi)
	mov	%ebx, 60(%esi)
	sub	64(%esp), %ecx
	sbb	68(%esp), %ebx
	jl	3f
	mov	%ecx, 56(%esi)
	mov	%ebx, 60(%esi)
3:
	add	$(8*VECTOR_LENGTH), %esi
	cmp	%edi, %esi
	jbe	next

	fstp	%st(0)

ladder:
	mov	60(%esp), %eax				/* llen */
	test	%eax, %eax
	jz	out

	mov	56(%esp), %ecx				/* lmin */
	fildll	(%ebp,%ecx,8)
	fmul	%st(1),%st(0)
	xor	%esi, %esi

	.p2align 4,,7
step:
	mov	48(%esp), %edi
	mov	56(%esp), %ecx
	mov	(%edi,%esi,4), %edi
	mov	(%ebp,%ecx,8), %eax
	mov	4(%ebp,%ecx,8), %edx
	inc	%esi
	add	%edi, %ecx
	mov	%ecx, 16(%esp)

	fildll	(%ebp,%edi,8)
	mov	(%ebp,%edi,8), %ebx
	mov	4(%ebp,%edi,8), %ecx
	imul	%edx, %ebx
	fmul	%st(1), %st(0)
	imul	%eax, %ecx
	fistpll	8(%esp)
	mull	(%ebp,%edi,8)
	mov	%eax, 4(%esp)
	add	%ecx, %ebx
	add	%ebx, %edx
	mov	8(%esp), %eax
	mov	%edx, (%esp)
	mov	12(%esp), %edx
	mov	68(%esp), %ebx
	mov	64(%esp), %ecx
	imul	%eax, %ebx
	imul	%edx, %ecx
	mull	64(%esp)
	add	%ebx, %ecx
	mov	(%esp), %ebx
	add	%ecx,%edx
	mov	4(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	64(%esp), %ecx
	sbb	68(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	mov	16(%esp), %edi
	cmp	60(%esp), %esi
	mov	%eax, 8(%ebp,%edi,8)
	mov	%edx, 12(%ebp,%edi,8)
	jb	step

	fstp	%st(0)

out:
	add	$28, %esp
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%ebp
#if UNALIGNED_STACK
	pop	%esp
#endif
	ret
