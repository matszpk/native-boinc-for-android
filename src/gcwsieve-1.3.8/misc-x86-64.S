/* misc-x86-64.S -- (C) Geoffrey Reynolds, September 2007.

   Misc x86-64 routines.

   uint64_t timestamp(void);
     Returns the CPU timestamp counter.

   uint32_t my_cpuid(uint32_t OP, uint32_t *R);
     Stores the results of cpuid function OP:
     R[0] <-- %eax
     R[1] <-- %ebx
     R[2] <-- %ecx
     R[3] <-- %edx
     Returns the value in R[0].

	
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#if ASSEMBLE_FOR_MSC
#define _WIN64 1
#endif

#if NEED_UNDERSCORE
#define timestamp _timestamp
#define my_cpuid _my_cpuid
#endif

	.text

	.globl	timestamp
	.p2align 4,,7

timestamp:
	push	%rbx

	xor	%eax, %eax
	cpuid
	rdtsc
	shl	$32, %rdx
	or	%rdx, %rax

	pop	%rbx
	ret


	.globl	my_cpuid
	.p2align 4,,7

my_cpuid:
	push	%rbx

#ifdef _WIN64
	mov	%ecx, %eax
	mov	%rdx, %r8
	cpuid
	mov	%eax, (%r8)
	mov	%ebx, 4(%r8)
	mov	%ecx, 8(%r8)
	mov	%edx, 12(%r8)
#else
	mov	%edi, %eax
	cpuid
	mov	%eax, (%rsi)
	mov	%ebx, 4(%rsi)
	mov	%ecx, 8(%rsi)
	mov	%edx, 12(%rsi)
#endif

	pop	%rbx
	ret
