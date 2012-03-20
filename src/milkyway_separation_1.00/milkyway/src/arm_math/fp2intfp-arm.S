/*
 * ifplog.S
 * Author: Mateusz Szpakowski
 * License: GPLv3
 */

                .arch armv6
                .fpu softvfp
                .eabi_attribute 20, 1
                .eabi_attribute 21, 1
                .eabi_attribute 23, 3
                .eabi_attribute 24, 1
                .eabi_attribute 25, 1
                .eabi_attribute 26, 2
                .eabi_attribute 30, 2
                .eabi_attribute 18, 4
                .text
                /* converts between IEEE-754 FP to INTFP.
                   IntFp format:
                   0-62 bit - fraction
                   63 bit - one (1)
                   64-79 bit - exponent (between -32768 and 32767)
                   80-95 bit - sign */
                
                .align 3
                .global fp_to_intfp
                .type   fp_to_intfp, %function
fp_to_intfp:
                push {r4,r5}
                
                lsl r5,r1,#11
                orr r5,r5,r0,lsr #21
                lsl r4,r0,#11
                orr r5,r5,#0x80000000
                
                bic r3,r1,#0x80000000
                tst r0,r0
                tsteq r3,r3
                bne .Lnozero
                mov r4,#0
                mov r5,#0
                mov r3,#0x8000
                b .Lconv_end
.Lnozero:
                ldr r0,.Lmask
                cmp r1,r0
                beq .Linf_or_nan
                lsr r3,r1,#20
                bic r3,r3,#0x800
                sub r3,r3,#0x3f0
                sub r3,r3,#0x0f
                ldr r0,.Lsignmask
                bic r3,r3,r0
                b .Lconv_end
.Linf_or_nan:
                ldr r3,.Linf
.Lconv_end:
                tst r1,r1
                ldrmi r0,.Lsignmask
                orrmi r3,r3,r0
                strd r4,[r2]
                str r3,[r2,#8]
                
                pop {r4,r5}
                bx lr
.Lmask:
                .word 0x7ff00000
.Lsignmask:
                .word 0xffff0000
.Linf:
                .word 0xffff
                
                .align 3
                .global intfp_to_fp
                .type   intfp_to_fp, %function
intfp_to_fp:
                push {r4,r5,r6,r7}
                
                ldrd r2,[r0]
                ldrsh r4,[r0,#8]
                
                adds r2,r2,#0x400   // rounding
                adcs r3,r3,#0
                bcc .Lifnocarry
                movs r3,r3,rrx
                mov r2,r2,rrx
                add r4,r4,#1
.Lifnocarry:
                ldr r5,.Lmaxmantisa
                cmp r4,r5
                blt .Lnotinf
                mov r6,#0
                ldr r7,.Lmask
                ldr r5,[r0,#8]
                tst r5,r5
                orrmi r7,r7,#0x80000000
                b .Lconv_end2
.Lnotinf:
                ldr r5,.Lminmantisa
                cmp r4,r5
                bgt .Lisnormal
                mov r6,#0
                mov r7,#0
                ldr r5,[r0,#8]
                tst r5,r5
                orrmi r7,r7,#0x80000000
                b .Lconv_end2
.Lisnormal:
                sub r4,r4,r5
                lsl r7,r4,#20
                bic r7,r7,#0x80000000
                
                bic r3,r3,#0x80000000
                
                lsr r2,r2,#11
                orr r6,r2,r3,lsl #21
                orr r7,r3,lsr #11
                
                ldr r5,[r0,#8]
                tst r5,r5
                orrmi r7,r7,#0x80000000
.Lconv_end2:
                strd r6,[r1]        // output
                
                pop {r4,r5,r6,r7}
                bx lr
.Lminmantisa:
                .word -0x3ff
.Lmaxmantisa:
                .word 0x3ff