/* lookup-i386.S -- (C) Geoffrey Reynolds, January 2009.

   Calculate Legendre symbol lookup table indices using precomputed data.


   struct ind_t
   {
     long double  inv;
     uint16_t     pad;
     uint32_t     mod;
   }

   void init_lookup_ind_i386(struct ind_t *IND, const uint32_t *X, uint32_t len) __attribute__((regparm(3)));

   Assign IND[i].inv <-- 1.0L/X[i] and IND[i].mod <-- X[i],
   computed in round-to-zero mode for 0 <= i < LIM, where LIM is the least
   multiple of 2 such that len <= LIM.

   Assumes IND[] has room for LIM entries.
   Assumes len > 0.


   void gen_lookup_ind_i386(uint32_t *X, const struct ind_t *IND,
                            uint32_t len, uint64_t n) __attribute__((regparm(3)));

   Assign X[i] <-- n mod IND[i].mod for 0 <= i < LIM, where LIM is
   the least multiple of 2 such that len <= LIM.

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
#define init_lookup_ind_i386	_init_lookup_ind_i386
#define gen_lookup_ind_i386	_gen_lookup_ind_i386
#endif


	.text

	.p2align 4,,15
	.globl	init_lookup_ind_i386

init_lookup_ind_i386:
	push	%ebx
	sub	$16, %esp

	/* Set FPU mode to round-to-zero and extended precision */
	fnstcw	(%esp)
	movzwl	(%esp), %ebx
	or	$0x0F00, %bx
	mov	%bx, 4(%esp)
	fldcw	4(%esp)

	/* Round up to next multiple of 2 */
	add	$1, %ecx
	and	$-2, %ecx

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
	.globl	gen_lookup_ind_i386

gen_lookup_ind_i386:
	push	%ebx
	push	%ebp
	push	%edi
	push	%esi
	sub	$28, %esp

	/* %st(0) <-- -1.0*n */
	fildll	48(%esp)
	fldz
	fsubp

	mov	%eax, %edi
	mov	%edx, %esi

	.p2align 4,,7
gen_loop:
	/* (%esp) <-- -floor(n/mod1) */
	fldt	(%esi)
	fmul	%st(1), %st(0)
	fistpll	(%esp)

	/* 8(%esp) <-- -floor(n/mod2) */
	fldt	16(%esi)
	fmul	%st(1), %st(0)
	fistpll	8(%esp)


	/* %edx:%eax <-- -mod*floor(n/mod1) */
	mov	12(%esi), %ebp
	mov	4(%esp), %ebx
	mov	(%esp), %eax
	imul	%ebp, %ebx
	mul	%ebp
	add	%ebx, %edx

	/* %edx:%eax <-- n - mod1*floor(n/mod1) */
	add	48(%esp), %eax
	adc	52(%esp), %edx

	/* Correct rounding so that 0 <= %eax < mod1 */
	mov	%eax, %ebx
	sub	%ebp, %ebx
	sbb	$0, %edx	/* CF=0 predicted */
	jc	0f
	mov	%ebx, %eax
0:
	/* (%edi) <-- n%mod1 */
	mov	%eax, (%edi)


	/* %edx:%eax <-- -mod2*floor(n/mod2) */
	mov	28(%esi), %ebp
	mov	12(%esp), %ebx
	mov	8(%esp), %eax
	imul	%ebp, %ebx
	mul	%ebp
	add	%ebx, %edx

	/* %edx:%eax <-- n - mod2*floor(n/mod2) */
	add	48(%esp), %eax
	adc	52(%esp), %edx

	/* Correct rounding so that 0 <= %eax < mod2 */
	mov	%eax, %ebx
	sub	%ebp, %ebx
	sbb	$0, %edx	/* CF=0 predicted */
	jc	0f
	mov	%ebx, %eax
0:
	/* 4(%edi) <-- n%mod2 */
	mov	%eax, 4(%edi)


	lea	32(%esi), %esi
	lea	8(%edi), %edi
	sub	$2, %ecx
	ja	gen_loop


	fstp	%st(0)

	add	$28, %esp
	pop	%esi
	pop	%edi
	pop	%ebp
	pop	%ebx
	ret
