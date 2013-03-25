/*
 * neon_GetPowerSpectrum.S
 * Author: Mateusz Szpakowski
 */
 
                .arch armv7-a
                .fpu neon
                .eabi_attribute 20, 1
                .eabi_attribute 21, 1
                .eabi_attribute 23, 3
                .eabi_attribute 24, 1
                .eabi_attribute 25, 1
                .eabi_attribute 26, 2
                .eabi_attribute 30, 2
                .eabi_attribute 18, 4
                .text
                .align  2
                .global neon_GetPowerSpectrum
                .type   neon_GetPowerSpectrum, %function
neon_GetPowerSpectrum:
                push {r4,r5,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                ldr r3,.LGOT1
.LPIC1:
                add r3,pc,r3
                ldr r4,.LGOT1+4
                ldr r4,[r3,r4]
                add r5,r2,r2,lsl #1
                fldd d0,[r4,#32]
                fmsr s4,r5
                fuitod d1,s4
                faddd d0,d0,d1
                fstd d0,[r4,#32]
                
                cmp r2,#4096
                blo .Lsecondversion
                
                add r2,r0,r2, lsl #3
                sub r2,r2,#31*8
                /* r0 - freqData
                 * r1 - PowerSpectrum
                 * r2 - end of freqData
                 */
                cmp r0,r2
                bhs .Lendmainloop
.Lmainloop:
                pld [r0,#128]
                vldmia r0!,{q0,q1,q2,q3,q4,q5,q6,q7}
                vmul.f32 q0,q0,q0
                vmul.f32 q1,q1,q1
                vmul.f32 q2,q2,q2
                vmul.f32 q3,q3,q3
                vmul.f32 q4,q4,q4
                vmul.f32 q5,q5,q5
                vmul.f32 q6,q6,q6
                vmul.f32 q7,q7,q7
                vpadd.f32 d16,d0,d1
                vpadd.f32 d17,d2,d3
                vpadd.f32 d18,d4,d5
                vpadd.f32 d19,d6,d7
                vpadd.f32 d20,d8,d9
                vpadd.f32 d21,d10,d11
                vpadd.f32 d22,d12,d13
                vpadd.f32 d23,d14,d15
                
                pld [r0,#128]
                vldmia r0!,{q0,q1,q2,q3,q4,q5,q6,q7}
                vstmia r1!,{q8,q9,q10,q11}
                vmul.f32 q0,q0,q0
                vmul.f32 q1,q1,q1
                vmul.f32 q2,q2,q2
                vmul.f32 q3,q3,q3
                vmul.f32 q4,q4,q4
                vmul.f32 q5,q5,q5
                vmul.f32 q6,q6,q6
                vmul.f32 q7,q7,q7
                vpadd.f32 d16,d0,d1
                vpadd.f32 d17,d2,d3
                vpadd.f32 d18,d4,d5
                vpadd.f32 d19,d6,d7
                vpadd.f32 d20,d8,d9
                vpadd.f32 d21,d10,d11
                vpadd.f32 d22,d12,d13
                vpadd.f32 d23,d14,d15
                vstmia r1!,{q8,q9,q10,q11}
                
                cmp r0,r2
                blo .Lmainloop

.Lendmainloop:
                add r2,r2,#8*24
                bhs .Lendsmallloop
.Lsmallloop:
                vldmia r0!,{q0,q1,q2,q3}
                vmul.f32 q0,q0,q0
                vmul.f32 q1,q1,q1
                vmul.f32 q2,q2,q2
                vmul.f32 q3,q3,q3
                vpadd.f32 d16,d0,d1
                vpadd.f32 d17,d2,d3
                vpadd.f32 d18,d4,d5
                vpadd.f32 d19,d6,d7
                vstmia r1!,{q8,q9,q10,q11}
                cmp r0,r2
                blo .Lsmallloop
.Lendsmallloop:
                add r2,r2,#8*7
                cmp r0,r2                
                beq .Lendmicroloop
.Lmicroloop:
                fldmias r0!,{s0,s1}
                fmuls s2,s0,s0
                fmacs s2,s1,s1
                fstmias r1!,{s2}
                
                cmp r0,r2
                blo .Lmicroloop
.Lendmicroloop:
                mov r0,#0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,lr}
                bx lr
                

                /*
                 * second version
                 */
.Lsecondversion:
                add r2,r0,r2, lsl #3
                sub r2,r2,#31*8
                /* r0 - freqData
                 * r1 - PowerSpectrum
                 * r2 - end of freqData
                 */
                cmp r0,r2
                bhs .Lendmainloop2
.Lmainloop2:
                vldmia r0!,{q0,q1,q2,q3,q4,q5,q6,q7}
                vldmia r0!,{q8,q9,q10,q11,q12,q13,q14,q15}
                vuzp.32 q0,q1
                vuzp.32 q2,q3
                vuzp.32 q4,q5
                vuzp.32 q6,q7
                vuzp.32 q8,q9
                vuzp.32 q10,q11
                vuzp.32 q12,q13
                vuzp.32 q14,q15
                vmul.f32 q0,q0,q0
                vmul.f32 q2,q2,q2
                vmul.f32 q4,q4,q4
                vmul.f32 q6,q6,q6
                vmul.f32 q8,q8,q8
                vmul.f32 q10,q10,q10
                vmul.f32 q12,q12,q12
                vmul.f32 q14,q14,q14
                vmla.f32 q0,q1,q1
                vmla.f32 q2,q3,q3
                vmla.f32 q4,q5,q5
                vmla.f32 q6,q7,q7
                vmla.f32 q8,q9,q9
                vmla.f32 q10,q11,q11
                vmla.f32 q12,q13,q13
                vmla.f32 q14,q15,q15
                vstmia r1!,{q0}
                vstmia r1!,{q2}
                vstmia r1!,{q4}
                vstmia r1!,{q6}
                vstmia r1!,{q8}
                vstmia r1!,{q10}
                vstmia r1!,{q12}
                vstmia r1!,{q14}
                
                cmp r0,r2
                blo .Lmainloop2

.Lendmainloop2:
                add r2,r2,#8*24
                bhs .Lendsmallloop2
.Lsmallloop2:
                vldmia r0!,{q0,q1,q2,q3}
                vmul.f32 q0,q0,q0
                vmul.f32 q1,q1,q1
                vmul.f32 q2,q2,q2
                vmul.f32 q3,q3,q3
                vpadd.f32 d16,d0,d1
                vpadd.f32 d17,d2,d3
                vpadd.f32 d18,d4,d5
                vpadd.f32 d19,d6,d7
                vstmia r1!,{q8,q9,q10,q11}
                cmp r0,r2
                blo .Lsmallloop2
.Lendsmallloop2:
                add r2,r2,#8*7
                cmp r0,r2                
                beq .Lendmicroloop
.Lmicroloop2:
                fldmias r0!,{s0,s1}
                fmuls s2,s0,s0
                fmacs s2,s1,s1
                fstmias r1!,{s2}
                
                cmp r0,r2
                blo .Lmicroloop2
                b .Lendmicroloop
                
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word analysis_state(GOT)
