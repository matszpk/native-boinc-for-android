/* misc-i386.S -- (C) Geoffrey Reynolds, September 2007.

   Misc x86 routines.

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

#if NEED_UNDERSCORE
#define timestamp _timestamp
#define my_cpuid _my_cpuid
#endif

	.text

	.globl	timestamp
	.p2align 4,,7

timestamp:
	push	%ebx

	xor	%eax, %eax
	cpuid
	rdtsc

	pop	%ebx
	ret


	.globl	my_cpuid
	.p2align 4,,7

my_cpuid:
	push	%ebx
	push	%edi

	mov	12(%esp), %eax
	mov	16(%esp), %edi
	cpuid
	mov	%eax, (%edi)
	mov	%ebx, 4(%edi)
	mov	%ecx, 8(%edi)
	mov	%edx, 12(%edi)

	pop	%edi
	pop	%ebx
	ret
