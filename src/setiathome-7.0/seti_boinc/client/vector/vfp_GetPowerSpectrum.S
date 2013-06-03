/*
 * vfp_GetPowerSpectrum.S
 * Author: Mateusz Szpakowski
 */
 
                .arch armv6
                .fpu vfp
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
                .global vfp_GetPowerSpectrum
                .type   vfp_GetPowerSpectrum, %function
vfp_GetPowerSpectrum:
                push {r4,r5}
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
                
                add r2,r0,r2,lsl #3
                sub r2,r2,#15*8
                /* r0 - freqData
                 * r1 - PowerSpectrum
                 * r2 - end of freqData
                 */
                cmp r0,r2
                bhs .Lendmainloop
.Lmainloop:
                pld [r0,#64]
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15}
                fmuls s16,s0,s0
                fmuls s17,s2,s2
                fmuls s18,s4,s4
                fmuls s19,s6,s6
                fmuls s20,s8,s8
                fmuls s21,s10,s10
                fmuls s22,s12,s12
                fmuls s23,s14,s14
                fmacs s16,s1,s1
                fmacs s17,s3,s3
                fmacs s18,s5,s5
                fmacs s19,s7,s7
                fmacs s20,s9,s9
                fmacs s21,s11,s11
                fmacs s22,s13,s13
                fmacs s23,s15,s15
                
                pld [r0,#64]
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15}
                fstmias r1!,{s16,s17,s18,s19,s20,s21,s22,s23}
                
                fmuls s24,s0,s0
                fmuls s25,s2,s2
                fmuls s26,s4,s4
                fmuls s27,s6,s6
                fmuls s28,s8,s8
                fmuls s29,s10,s10
                fmuls s30,s12,s12
                fmuls s31,s14,s14
                fmacs s24,s1,s1
                fmacs s25,s3,s3
                fmacs s26,s5,s5
                fmacs s27,s7,s7
                fmacs s28,s9,s9
                fmacs s29,s11,s11
                fmacs s30,s13,s13
                fmacs s31,s15,s15
                fstmias r1!,{s24,s25,s26,s27,s28,s29,s30,s31}
                
                cmp r0,r2
                blo .Lmainloop
.Lendmainloop:
                add r2,r2,#8*12
                bhs .Lendsmallloop
.Lsmallloop:
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7}
                fmuls s16,s0,s0
                fmuls s17,s2,s2
                fmuls s18,s4,s4
                fmuls s19,s6,s6
                fmacs s16,s1,s1
                fmacs s17,s3,s3
                fmacs s18,s5,s5
                fmacs s19,s7,s7
                fstmias r1!,{s16,s17,s18,s19}
                cmp r0,r2
                blo .Lsmallloop
.Lendsmallloop:
                add r2,r2,#8*3
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
                pop {r4,r5}
                bx lr
                
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word analysis_state(GOT)
                
