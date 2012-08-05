/* lookup-x87_64.S -- (C) Geoffrey Reynolds, January 2009.

   Calculate Legendre symbol lookup table indices using precomputed data.


   struct ind_t
   {
     long double  inv;
     uint16_t     pad;
     uint32_t     mod;
   }

   void init_lookup_ind_x87_64(struct ind_t *IND, const uint32_t *X,
                               uint32_t len);

   Assign IND[i].inv <-- 1.0L/X[i] and IND[i].mod <-- X[i],
   computed in round-to-zero mode for 0 <= i < LIM, where LIM is the least
   multiple of 4 such that len <= LIM.

   Assumes IND[] is 16-aligned and has room for LIM entries.
   Assumes len > 0.


   void gen_lookup_ind_x87_64(uint32_t *X, const struct ind_t *IND,
	                      uint32_t len, uint64_t n);

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
#define init_lookup_ind_x87_64	_init_lookup_ind_x87_64
#define gen_lookup_ind_x87_64	_gen_lookup_ind_x87_64
#endif


	.text

	.p2align 4,,15
	.globl	init_lookup_ind_x87_64

#ifdef _WIN64
# define TMP1 8(%rsp)
# define TMP2 12(%rsp)
# define TMP3 16(%rsp)
# define ARG1 %rcx
# define ARG2 %rdx
# define ARG3l %r8d
#else
# define TMP1 -4(%rsp)
# define TMP2 -8(%rsp)
# define TMP3 -16(%rsp)
# define ARG1 %rdi
# define ARG2 %rsi
# define ARG3l %edx
#endif

init_lookup_ind_x87_64:

	/* Set FPU mode to round-to-zero and extended precision */
	fnstcw	TMP1
	movzwl	TMP1, %eax
	or	$0x0F00, %ax
	mov	%ax, TMP2
	fldcw	TMP2

	/* Round up to next multiple of 4 */
	add	$3, ARG3l
	and	$-4, ARG3l

	.p2align 4,,7
init_loop:
	mov	(ARG2), %eax
	mov	%rax, TMP3
	fildll	TMP3
	fld1
	fdivp
	fstpt	(ARG1)
	mov	%eax, 12(ARG1)

	lea	16(ARG1), ARG1
	lea	4(ARG2), ARG2
	dec	ARG3l
	jnz	init_loop


	/* Restore FPU mode */
	fldcw	TMP1

	ret

#undef TMP1
#undef TMP2
#undef TMP3
#undef ARG1
#undef ARG2
#undef ARG3l


	.p2align 4,,15
	.globl	gen_lookup_ind_x87_64

#ifdef _WIN64
# define TMP1 48(%rsp)
# define TMP2 56(%rsp)
# define TMP3 64(%rsp)
# define TMP4 72(%rsp)
# define ARG1 %rcx
# define ARG2 %rdx
# define ARG3l %r8d
# define ARG4 %r9
# define REG1 %rdi
# define REG1l %edi
# define REG2 %rsi
# define REG2l %esi
#else
# define TMP1 -32(%rsp)
# define TMP2 -24(%rsp)
# define TMP3 -16(%rsp)
# define TMP4 -8(%rsp)
# define ARG1 %rdi
# define ARG2 %rsi
# define ARG3l %edx
# define ARG4 %rcx
# define REG1 %r8
# define REG1l %r8d
# define REG2 %r9
# define REG2l %r9d
#endif

gen_lookup_ind_x87_64:
	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
#endif
	push	%r12

	/* %st(0) <-- -1.0*n */
	push	ARG4
	fildll	(%rsp)
	fldz
	fsubp
	pop	ARG4


	.p2align 4,,15
gen_loop:
	fldt	(ARG2)
	mov	12(ARG2), REG1l
	fmul	%st(1), %st(0)
	fistpll	TMP1
	fldt	16(ARG2)
	mov	28(ARG2), REG2l
	fmul	%st(1), %st(0)
	fistpll	TMP2
	fldt	32(ARG2)
	mov	44(ARG2), %r10d
	fmul	%st(1), %st(0)
	fistpll	TMP3
	fldt	48(ARG2)
	mov	60(ARG2), %r11d
	fmul	%st(1), %st(0)
	fistpll	TMP4

	mov	TMP1, %rax
	mov	TMP2, %rbx
	mov	TMP3, %rbp
	mov	TMP4, %r12
	imul	REG1, %rax
	imul	REG2, %rbx
	imul	%r10, %rbp
	imul	%r11, %r12
	add	ARG4, %rax
	add	ARG4, %rbx
	add	ARG4, %rbp
	add	ARG4, %r12

0:	mov	%eax, (ARG1)
	sub	REG1, %rax	/* CF=0 predicted */
	jae	0b
0:	mov	%ebx, 4(ARG1)
	sub	REG2, %rbx	/* CF=0 predicted */
	jae	0b
0:	mov	%ebp, 8(ARG1)
	sub	%r10, %rbp	/* CF=0 predicted */
	jae	0b
0:	mov	%r12d, 12(ARG1)
	sub	%r11, %r12	/* CF=0 predicted */
	jae	0b

	lea	64(ARG2), ARG2
	lea	16(ARG1), ARG1
	sub	$4, ARG3l
	ja	gen_loop


	fstp	%st(0)

	pop	%r12
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	ret
