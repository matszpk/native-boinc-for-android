/* lookup-x86_64.S -- (C) Geoffrey Reynolds, January 2009.

   Calculate Legendre symbol lookup table indices using precomputed data.


   struct ind_t
   {
     double    inv[4];
     uint32_t  mod[4];
   }

   void init_lookup_ind_x86_64(struct ind_t *IND, const uint32_t *X,
                               uint32_t len);

   Assign IND[i/4].inv[i%4] <-- 1.0/X[i] and IND[i/4].mod[i%4] <-- X[i],
   computed in round-to-zero mode for 0 <= i < LIM, where LIM is the least
   multiple of 4 such that len <= LIM.

   Assumes IND[] is 16-aligned and has room for LIM/4 entries.
   Assumes len > 0.


   void gen_lookup_ind_x86_64(uint32_t *X, const struct ind_t *IND,
	                      uint32_t len, uint64_t n);

   Assign X[i] <-- n mod IND[i/4].mod[i%4] for 0 <= i < LIM, where LIM is
   the least multiple of 4 such that len <= LIM.

   Assumes IND is 16-aligned.
   Assumes SSE rounding mode is round-to-zero.
   Assumes IND[j].inv[j] = 1.0/IND[j].mod[j] computed in round-to-zero mode.
   Assumes n < 2^51.
   Assumes len > 0


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"


#if NEED_UNDERSCORE
#define init_lookup_ind_x86_64	_init_lookup_ind_x86_64
#define gen_lookup_ind_x86_64	_gen_lookup_ind_x86_64
#endif

	.text

	.align	16
onepd:
	.double	1.0
	.double	1.0


	.text

	.p2align 4,,15
	.globl	init_lookup_ind_x86_64

#ifdef _WIN64
# define TMP1 8(%rsp)
# define TMP2 12(%rsp)
# define ARG1 %rcx
# define ARG2 %rdx
# define ARG3l %r8d
# define REG1 %r9
# define REG1l %r9d
#else
# define TMP1 -8(%rsp)
# define TMP2 -4(%rsp)
# define ARG1 %rdi
# define ARG2 %rsi
# define ARG3l %edx
# define REG1 %rcx
# define REG1l %ecx
#endif

init_lookup_ind_x86_64:

	/* Set SSE rounding mode to round-to-zero */
	stmxcsr	TMP1
	mov	TMP1, %eax
	or	$0x6000, %eax
	mov	%eax, TMP2
	ldmxcsr	TMP2

	/* %xmm0 <-- {1.0,1.0} */
	movapd	onepd(%rip), %xmm0


	.p2align 4,,7
init_loop:
	mov	(ARG2), %eax
	mov	4(ARG2), REG1l
	cvtsi2sd %rax, %xmm1
	cvtsi2sd REG1, %xmm3
	mov	%eax, 32(ARG1)
	mov	REG1l, 36(ARG1)
	mov	8(ARG2), %eax
	mov	12(ARG2), REG1l
	cvtsi2sd %rax, %xmm2
	cvtsi2sd REG1, %xmm4
	mov	%eax, 40(ARG1)
	mov	REG1l, 44(ARG1)
	unpcklpd %xmm3, %xmm1
	unpcklpd %xmm4, %xmm2
	movapd	%xmm0, %xmm3
	movapd	%xmm0, %xmm4
	divpd	%xmm1, %xmm3
	divpd	%xmm2, %xmm4
	movapd	%xmm3, (ARG1)
	movapd	%xmm4, 16(ARG1)

	lea	48(ARG1), ARG1
	lea	16(ARG2), ARG2
	sub	$4, ARG3l
	ja	init_loop


	/* Restore SSE rounding mode */
	ldmxcsr	TMP1

	ret

#undef TMP1
#undef TMP2
#undef ARG1
#undef ARG2
#undef ARG3l
#undef REG1
#undef REG1l


	.p2align 4,,15
	.globl	gen_lookup_ind_x86_64

#ifdef _WIN64
# define ARG1 %rcx
# define ARG2 %rdx
# define ARG3l %r8d
# define ARG4 %r9
# define REG1 %rdi
# define REG1l %edi
# define REG2 %rsi
# define REG2l %esi
#else
# define ARG1 %rdi
# define ARG2 %rsi
# define ARG3l %edx
# define ARG4 %rcx
# define REG1 %r8
# define REG1l %r8d
# define REG2 %r9
# define REG2l %r9d
#endif

gen_lookup_ind_x86_64:
	push	%rbp
	push	%rbx
#ifdef _WIN64
	push	%rdi
	push	%rsi
#endif
	push	%r12

	/* %xmm0 <-- {-1.0*n,-1.0*n} */
	cvtsi2sdq ARG4, %xmm3
	xorpd	%xmm0, %xmm0
	unpcklpd %xmm3, %xmm3
	subpd	%xmm3, %xmm0


	.p2align 4,,15
gen_loop:
	movapd	(ARG2), %xmm1
	movapd	16(ARG2), %xmm2
	mulpd	%xmm0, %xmm1
	mulpd	%xmm0, %xmm2
	cvtsd2siq %xmm1, %rax
	cvtsd2siq %xmm2, %rbp
	unpckhpd %xmm1, %xmm1
	unpckhpd %xmm2, %xmm2
	cvtsd2siq %xmm1, %rbx
	cvtsd2siq %xmm2, %r12

	mov	32(ARG2), REG1l
	mov	36(ARG2), REG2l
	mov	40(ARG2), %r10d
	mov	44(ARG2), %r11d
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

	lea	48(ARG2), ARG2
	lea	16(ARG1), ARG1
	sub	$4, ARG3l
	ja	gen_loop


	pop	%r12
#ifdef _WIN64
	pop	%rsi
	pop	%rdi
#endif
	pop	%rbx
	pop	%rbp
	ret
