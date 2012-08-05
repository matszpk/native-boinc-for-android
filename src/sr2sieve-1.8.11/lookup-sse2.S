/* lookup-sse2.S -- (C) Geoffrey Reynolds, January 2009.

   Calculate Legendre symbol lookup table indices using precomputed data.


   struct ind_t
   {
     long double  inv;
     uint16_t     pad;
     uint32_t     mod;
   }

   void init_lookup_ind_sse2(struct ind_t *IND, const uint32_t *X, uint32_t len) __attribute__((regparm(3)));

   Assign IND[i].inv <-- 1.0L/X[i] and IND[i].mod <-- X[i],
   computed in round-to-zero mode for 0 <= i < LIM, where LIM is the least
   multiple of 4 such that len <= LIM.

   Assumes IND[] has room for LIM entries.
   Assumes len > 0.


   void gen_lookup_ind_sse2(uint32_t *X, const struct ind_t *IND,
                            uint32_t len, uint64_t n) __attribute__((regparm(3)));

   Assign X[i] <-- n mod IND[i].mod for 0 <= i < LIM, where LIM is
   the least multiple of 4 such that len <= LIM.

   Assumes FPU mode is round-to-zero and extended precision.
   Assumes IND[j].inv = 1.0L/IND[j].mod computed in round-to-zero mode.
   Assumes n < 2^51.
   Assumes len > 0


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"


#if NEED_UNDERSCORE
#define init_lookup_ind_sse2	_init_lookup_ind_sse2
#define gen_lookup_ind_sse2	_gen_lookup_ind_sse2
#endif


	.text

	.p2align 4,,15
	.globl	init_lookup_ind_sse2

init_lookup_ind_sse2:
	push	%ebx
	sub	$16, %esp

	/* Set FPU mode to round-to-zero and extended precision */
	fnstcw	(%esp)
	movzwl	(%esp), %ebx
	or	$0x0F00, %bx
	mov	%bx, 4(%esp)
	fldcw	4(%esp)

	/* Round up to next multiple of 4 */
	add	$3, %ecx
	and	$-4, %ecx

	.p2align 4,,7
init_loop:
	mov	(%edx), %ebx
	mov	%ebx, 8(%esp)
	movl	$0, 12(%esp)
	fildll	8(%esp)
	fld1
	fdivp
	fstpt	(%eax)
	mov	%ebx, 12(%eax)

	lea	16(%eax), %eax
	lea	4(%edx), %edx

	dec	%ecx
	jnz	init_loop


	/* Restore FPU mode */
	fldcw	(%esp)

	add	$16, %esp
	pop	%ebx
	ret




	.p2align 4,,15
	.globl	gen_lookup_ind_sse2

gen_lookup_ind_sse2:
	sub	$44, %esp

	/* %st(0) <-- -1.0*n */
	fildll	48(%esp)
	fldz
	fsubp

	/* %xmm6 <-- {n,n} */
	pshufd	$0x44, 48(%esp), %xmm6


	.p2align 4,,7
gen_loop:
	fldt	(%edx)
	movd	12(%edx), %xmm0
	fmul	%st(1), %st(0)
	fistpll	(%esp)
	fldt	16(%edx)
	movd	28(%edx), %xmm1
	fmul	%st(1), %st(0)
	fistpll	8(%esp)
	fldt	32(%edx)
	movd	44(%edx), %xmm2
	fmul	%st(1), %st(0)
	fistpll	16(%esp)
	fldt	48(%edx)
	movd	60(%edx), %xmm3
	fmul	%st(1), %st(0)
	fistpll	24(%esp)

	punpcklqdq %xmm2, %xmm0
	punpcklqdq %xmm3, %xmm1

	movq	(%esp), %xmm2
	movq	8(%esp), %xmm3
	movhps	16(%esp), %xmm2
	movhps	24(%esp), %xmm3
	pshufd	$0xF5, %xmm2, %xmm4
	pshufd	$0xF5, %xmm3, %xmm5
	pmuludq	%xmm0, %xmm2
	pmuludq	%xmm1, %xmm3
	pmuludq	%xmm0, %xmm4
	pmuludq	%xmm1, %xmm5
	paddq	%xmm6, %xmm2
	paddq	%xmm6, %xmm3
	psllq   $32, %xmm4
	psllq   $32, %xmm5
	psubq	%xmm0, %xmm2
	psubq	%xmm1, %xmm3
	paddq   %xmm4, %xmm2
	paddq   %xmm5, %xmm3

	pxor    %xmm4, %xmm4
	pxor    %xmm5, %xmm5
	pcmpgtd %xmm2, %xmm4
	pcmpgtd %xmm3, %xmm5
	pshufd  $0xF5, %xmm4, %xmm4
	pshufd  $0xF5, %xmm5, %xmm5
	pand    %xmm0, %xmm4
	pand    %xmm1, %xmm5
	paddq	%xmm4, %xmm2
	paddq	%xmm5, %xmm3

	psllq	$32, %xmm3
	por	%xmm3, %xmm2
	movdqa	%xmm2, (%eax)

	lea	64(%edx), %edx
	lea	16(%eax), %eax
	sub	$4, %ecx
	ja	gen_loop


	fstp	%st(0)

	add	$44, %esp
	ret
