/* loop-x86-sse2.S -- (C) Geoffrey Reynolds, August 2007.

   Main loop for x86 machines with SSE2.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

/* Prefetch data for this number of loop iterations in advance.
*/
#define PREFETCH_ITER 4


/* struct loop_rec_t { uint32_t N[4]; uint32_t G[4]; };
   struct loop_data_t { uint64_t X[4]; struct loop_rec_t R[0]; };

   int swizzle_loop4(const uint64_t *T, loop_data_t *D, int i, uint64_t p)
   {
     while (--i >= 0)
     {
       int j;

       for (j = 0; j < 4; j++)
         D->X[j] = mulmod64(D->X[j],T[D->R[i].G[j]],p);

       for (j = 0; j < 4; j++)
         if (D->X[j] == D->R[i].N[j])
           return i;
     }

     return i;
   }
*/

/* int swizzle_loop4_x86_sse2(int i, const uint64_t *T, loop_data_t *D,
	                      uint64_t p);

   Assumes FPU is set to double extended precision and round to zero.
   Assumes %st(0) contains 1.0/p computed with above settings.
   Assumes that i < 2^28.
   Assumes that D is 16-aligned. Idealy both T and D should be 32-aligned.
   Assumes stack is 16-aligned.
   First argument passed in %eax [attribute regparm(1)].

   If ASSEMBLE_FOR_MSC is defined then the prototype is changed to:
   int swizzle_loop4_x86_sse2(const uint64_t *T, loop_data_t *D,
	                      uint64_t p, int i);
   with all arguments passed on the stack.
	
   The comments below use this notation, where 0 <= j < 4, 0 <= k < i :
   Xj = D->X[j],  Njk = D->R[k].N[j],  Gjk = D->R[k].G[j]
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN32 1
#define UNALIGNED_STACK 1
#endif

#if NEED_UNDERSCORE
#define swizzle_loop4_x86_sse2 _swizzle_loop4_x86_sse2
#define swizzle_loop4_x86_sse2_prefetch _swizzle_loop4_x86_sse2_prefetch
#endif

	.text
#if USE_PREFETCH
	.globl	swizzle_loop4_x86_sse2_prefetch
#else
	.globl	swizzle_loop4_x86_sse2
#endif
	.p2align 4

#if USE_PREFETCH
swizzle_loop4_x86_sse2_prefetch:
#else
swizzle_loop4_x86_sse2:
#endif
#if UNALIGNED_STACK
	mov	%esp, %ecx
	and	$~15, %esp	/* 0 mod 16 */
#if ASSEMBLE_FOR_MSC
	sub	$12, %esp
	pushl	20(%ecx)	/* 0 mod 16 */
#endif
	pushl	16(%ecx)
	pushl	12(%ecx)
	pushl	8(%ecx)
	pushl	4(%ecx)		/* 0 mod 16 */
	push	%ecx		/* 12 mod 16 */
#endif
	push	%esi
	push	%edi
	sub	$36, %esp

#if ASSEMBLE_FOR_MSC
	mov	64(%esp), %eax
#endif
	sal	$3, %eax	/* i*8 */
	mov	48(%esp), %esi	/* T */
	mov	52(%esp), %edi	/* D */
	movq    56(%esp), %xmm0 /* p */

	punpcklqdq %xmm0, %xmm0	/* {p,p} */
	movdqa	%xmm0, (%esp)
	psrlq	$32, %xmm0	/* {p>>32,p>>32} */
	movdqa	%xmm0, 16(%esp)

	movdqa	(%edi), %xmm2	/* {X0,X1} */
	movdqa	16(%edi), %xmm3	/* {X2,X3} */

	jmp	test

	.p2align 4
loop:
	mov	48(%edi,%eax,4), %ecx	/* G0i */
	mov	52(%edi,%eax,4), %edx	/* G1i */

#if USE_PREFETCH
	prefetchnta 32-32*PREFETCH_ITER(%edi,%eax,4)
#endif
	fildll  (%esi,%ecx,8)		/* T[G0i] */
	fmul    %st(1), %st(0)
	fildll  (%edi)			/* X0 */
	fmulp   %st(0), %st(1)
	fistpll (%edi)
	fildll  (%esi,%edx,8)		/* T[G1i] */
	fmul    %st(1), %st(0)
	fildll  8(%edi)			/* X1 */
	fmulp   %st(0), %st(1)
	fistpll 8(%edi)
	movq    (%esi,%ecx,8), %xmm0
	movq    (%esi,%edx,8), %xmm6
	punpcklqdq %xmm6, %xmm0		/* {T[G0i],T[G1i]} */

	mov	56(%edi,%eax,4), %ecx	/* G2i */
	mov	60(%edi,%eax,4), %edx	/* G3i */

	fildll  (%esi,%ecx,8)		/* T[G2i] */
	fmul    %st(1), %st(0)
	fildll  16(%edi)		/* X2 */
	fmulp   %st(0), %st(1)
	fistpll 16(%edi)
	fildll  (%esi,%edx,8)		/* T[G3i] */
	fmul    %st(1), %st(0)
	fildll  24(%edi)		/* X3 */
	fmulp   %st(0), %st(1)
	fistpll 24(%edi)
	movq    (%esi,%ecx,8), %xmm1
	movq    (%esi,%edx,8), %xmm7
	punpcklqdq %xmm7, %xmm1		/* {T[G2i],T[G3i]} */

	movdqa  %xmm0, %xmm6		/* {T[G0i],T[G1i]} */
	movdqa  %xmm2, %xmm4		/* {X0,X1} */
	movdqa  %xmm1, %xmm5		/* {T[G2i],T[G3i]} */
	movdqa  %xmm3, %xmm7		/* {X2,X3} */
	psrlq   $32, %xmm6
	psrlq   $32, %xmm4
	psrlq   $32, %xmm5
	psrlq   $32, %xmm7
	pmuludq %xmm2, %xmm6
	pmuludq %xmm3, %xmm5
	pmuludq %xmm0, %xmm4
	pmuludq %xmm1, %xmm7
	pmuludq %xmm0, %xmm2
	pmuludq %xmm1, %xmm3
	psllq   $32, %xmm6
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psllq   $32, %xmm7
	paddq   %xmm6, %xmm2
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3
	paddq   %xmm7, %xmm3

	movq    (%edi), %xmm6
	movhps  8(%edi), %xmm6
	movq    16(%edi), %xmm7
	movhps  24(%edi), %xmm7

	movdqa  16(%esp), %xmm0
	movdqa  %xmm0, %xmm1
	pshufd	$0xF5, %xmm6, %xmm4
	pshufd	$0xF5, %xmm7, %xmm5
	pmuludq %xmm6, %xmm0
	pmuludq %xmm7, %xmm1
	pmuludq (%esp), %xmm4
	pmuludq (%esp), %xmm5
	pmuludq (%esp), %xmm6
	pmuludq (%esp), %xmm7
	psllq   $32, %xmm0
	psllq   $32, %xmm1
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	paddq   %xmm0, %xmm6
	paddq   %xmm1, %xmm7
	paddq   %xmm4, %xmm6
	paddq   %xmm5, %xmm7

	psubq   (%esp), %xmm2
	psubq   (%esp), %xmm3
	psubq   %xmm6, %xmm2
	psubq   %xmm7, %xmm3
	pxor    %xmm0, %xmm0
	pxor    %xmm1, %xmm1
	pcmpgtd %xmm2, %xmm0
	pcmpgtd %xmm3, %xmm1
	pshufd  $0xF5,%xmm0,%xmm0
	pshufd  $0xF5,%xmm1,%xmm1
	pand    (%esp), %xmm0
	pand    (%esp), %xmm1
	paddq   %xmm0, %xmm2
	paddq   %xmm1, %xmm3

	movlps  %xmm2, (%edi)		/* X0 <- X0*T[G0i] (mod p) */
	movhps  %xmm2, 8(%edi)		/* X1 <- X1*T[G1i] (mod p) */
	movlps  %xmm3, 16(%edi)		/* X2 <- X2*T[G2i] (mod p) */
	movhps  %xmm3, 24(%edi)		/* X3 <- X3*T[G3i] (mod p) */

	movd    32(%edi,%eax,4), %xmm4	/* N0i */
	movd    36(%edi,%eax,4), %xmm6	/* N1i */
	punpcklqdq %xmm6, %xmm4		/* {N0i,N1i} */

	movd    40(%edi,%eax,4), %xmm5	/* N2i */
	movd    44(%edi,%eax,4), %xmm7	/* N3i */
	punpcklqdq %xmm7, %xmm5		/* {N2i,N3i} */
#ifdef SMALL_P
	movdqa	%xmm4, %xmm0
	movdqa	%xmm5, %xmm1
	psubq	(%esp), %xmm0		/* {N0i-p,N1i-p} */
	psubq	(%esp), %xmm1		/* {N2i-p,N3i-p} */
#endif
	pcmpeqd %xmm2, %xmm4
	pcmpeqd %xmm3, %xmm5
	pshufd  $0xB1,%xmm4,%xmm6
	pshufd  $0xB1,%xmm5,%xmm7
	pand    %xmm4, %xmm6
	pand    %xmm5, %xmm7
	por	%xmm6, %xmm7
#ifdef SMALL_P
	pcmpeqd %xmm2, %xmm0
	pcmpeqd %xmm3, %xmm1
	psllq	$32, %xmm0
	psrlq	$32, %xmm1
	por	%xmm0, %xmm7
	por	%xmm1, %xmm7
#endif
	pmovmskb %xmm7, %edx		/* zero iff Xj != Nj for each j */

	test	%edx, %edx
	jnz	out			/* Xj == Nj for at least one j */

test:
	sub	$8, %eax
	jge	loop			/* while (i >= 0) */
out:
	sar	$3, %eax		/* current i (negative if done) */
	add	$36, %esp
	pop	%edi
	pop	%esi
#if UNALIGNED_STACK
	pop	%esp
#endif
	ret
