/* loop-x86.S -- (C) Geoffrey Reynolds, August 2007.

   Main loop for x86 machines without SSE2.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

/* Prefetch data for this number of loop iterations in advance.
*/
#define PREFETCH_ITER 4


/* struct loop_rec_t { uint32_t N[2]; uint32_t G[2]; };
   struct loop_data_t { uint64_t X[2]; struct loop_rec_t R[0]; };

   int swizzle_loop2(const uint64_t *T, loop_data_t *D, int i, uint64_t p)
   {
     while (--i >= 0)
     {
       D->X[0] = mulmod64(D->X[0],T[D->R[i].G[0]],p);
       D->X[1] = mulmod64(D->X[1],T[D->R[i].G[1]],p);

       if (D->X[0] == D->R[i].N[0] || D->X[1] == D->R[i].N[1])
         break;
     }

     return i;
   }
*/

/* int swizzle_loop2_x86(const uint64_t *T, loop_data_t *D, int i, uint64_t p);

   Assumes FPU is set to double extended precision and round to zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
   Idealy both T and D should be 16-aligned.
   Assumes stack is 8-aligned, unless UNALIGNED_STACK=1.

   The comments below use this notation, where 0 <= j < 4, 0 <= k < i :
   Xj = D->X[j],  Njk = D->R[k].N[j],  Gjk = D->R[k].G[j]
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN32 1
#define UNALIGNED_STACK 1
#endif

#if NEED_UNDERSCORE
#define swizzle_loop2_x86 _swizzle_loop2_x86
#define swizzle_loop2_x86_prefetch _swizzle_loop2_x86_prefetch
#endif

	.text
#if USE_PREFETCH
	.globl	swizzle_loop2_x86_prefetch
#else
	.globl	swizzle_loop2_x86
#endif
	.p2align 4

#if USE_PREFETCH
swizzle_loop2_x86_prefetch:
#else
swizzle_loop2_x86:
#endif
#if UNALIGNED_STACK
	mov	%esp, %eax
	and	$~7, %esp	/* 0 mod 8 */
	sub	$4, %esp
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
	sub	$60, %esp

	mov	80(%esp), %edi		/* T */
	mov	84(%esp), %ebx		/* D */
	mov	88(%esp), %eax		/* i */
	mov	92(%esp), %esi		/* p lo */

	/* Move X0,X1 onto the stack so they can be addressed by %esp
	*/
	mov	(%ebx), %ecx		/* X0 lo */
	mov	4(%ebx), %edx		/* X0 hi */
	mov	%ecx, (%esp)
	mov	%edx, 4(%esp)
	mov	8(%ebx), %ecx		/* X1 lo */
	mov	12(%ebx), %edx		/* X1 hi */
	mov	%ecx, 8(%esp)
	mov	%edx, 12(%esp)

	sal	$4, %eax		/* 16*i */
	lea	16(%ebx,%eax), %ebp	/* D->R+i */
	jmp	test

	.p2align 4
loop:
	mov	8(%ebp), %edx		/* G0i */
#if USE_PREFETCH
	prefetchnta -16*PREFETCH_ITER(%ebp)
#endif
	fildll	(%esp)			/* X0 */
	fildll	(%edi,%edx,8)		/* T[G0i] */
	mov	(%edi,%edx,8), %eax
	mov	4(%edi,%edx,8), %ebx
	mov	%eax, %ecx
	mull	(%esp)
	fmulp	%st(0), %st(1)
	imul	(%esp), %ebx
	fmul	%st(1), %st(0)
	imul	4(%esp), %ecx
	mov	%eax, 36(%esp)
	add	%ebx, %ecx
	fistpll	16(%esp)
	add	%ecx, %edx
	mov	%edx, 32(%esp)

	mov	12(%ebp), %edx		/* G1i */

	fildll	8(%esp)			/* X1 */
	fildll	(%edi,%edx,8)		/* T[G1i] hi */
	mov	(%edi,%edx,8), %eax
	mov	4(%edi,%edx,8), %ebx
	mov	%eax, %ecx
	mull	8(%esp)
	fmulp	%st(0), %st(1)
	imul	8(%esp), %ebx
	fmul	%st(1), %st(0)
	imul	12(%esp), %ecx
	mov	%eax, 44(%esp)
	add	%ebx, %ecx
	fistpll	24(%esp)
	add	%ecx, %edx
	mov	%edx, 40(%esp)

	mov	16(%esp), %eax
	mov	96(%esp), %ebx
	mov	20(%esp), %ecx
	imul	%eax, %ebx
	imul	%esi, %ecx
	mul	%esi
	add	%ebx, %ecx
	mov	32(%esp), %ebx
	add	%ecx,%edx
	mov	36(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, (%esp)
	mov	%ebx, 4(%esp)
	sub	%esi, %ecx
	sbb	96(%esp), %ebx
	jl	0f
	mov	%ecx, (%esp)
	mov	%ebx, 4(%esp)
0:
	mov	24(%esp), %eax
	mov	96(%esp), %ebx
	mov	28(%esp), %ecx
	imul	%eax, %ebx
	imul	%esi, %ecx
	mul	%esi
	add	%ebx, %ecx
	mov	40(%esp), %ebx
	add	%ecx,%edx
	mov	44(%esp), %ecx
	sub	%eax, %ecx
	sbb	%edx, %ebx
	mov	%ecx, %eax
	mov	%ebx, %edx
	sub	%esi, %ecx
	sbb	96(%esp), %ebx
	jl	0f
	mov	%ecx, %eax
	mov	%ebx, %edx
0:
	mov	%eax, 8(%esp)
	mov	%edx, 12(%esp)
	mov	(%ebp), %ecx		/* N0i */
	mov	4(%ebp), %ebx		/* N1i */
	xor	(%esp), %ecx
	xor	%eax, %ebx
	or	4(%esp), %ecx
	jz	out			/* X0 == N0i */
	or	%edx, %ebx
	jz	out			/* X1 == N1i */

#ifdef SMALL_P
	mov	(%ebp), %ecx		/* N0i */
	mov	4(%ebp), %ebx		/* N1i */
	sub	%esi, %ecx		/* N0i-p */
	sub	%esi, %ebx		/* N1i-p */
	cmp	(%esp), %ecx
	jz	out			/* X0 == N0i */
	cmp	%eax, %ebx
	jz	out			/* X1 == N1i */
#endif

test:
	decl	88(%esp)
	lea	-16(%ebp), %ebp
	jge	loop

out:
	/* Move X0,X1 back into D->X[].
	*/
	mov	84(%esp), %ecx		/* D->X */
	mov	(%esp), %eax
	mov	4(%esp), %edx
	mov	%eax, (%ecx)
	mov	%edx, 4(%ecx)
	mov	8(%esp), %eax
	mov	12(%esp), %edx
	mov	%eax, 8(%ecx)
	mov	%edx, 12(%ecx)

	mov	88(%esp), %eax
	add	$60, %esp
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%ebp
#if UNALIGNED_STACK
	pop	%esp
#endif
	ret
