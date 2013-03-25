/*
 * neon_FoldSubs.S
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
                /*****
                 * fold array by 3
                 ******/
                .global neon_foldArrayBy3_ll31
                .type neon_foldArrayBy3_ll31, %function
neon_foldArrayBy3_ll31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r6,r6,#4*15
                cmp r0,r6
                bhs .Lendf3loop1
.Lf3loop1:
.macro FOLDBY3_CORE
                vldmia r0!,{q0,q1,q2,q3}
                vldmia r2!,{q4,q5,q6,q7}
                vldmia r3!,{q8,q9,q10,q11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q3,q3,q7
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vstmia r5!,{q0,q1,q2,q3}
                
                vmax.f32 q13,q0,q1
                vmax.f32 q14,q2,q3
                vmax.f32 q12,q12,q13
                vmax.f32 q12,q12,q14
.endm
                FOLDBY3_CORE

                cmp r0,r6
                blo .Lf3loop1
.Lendf3loop1:
                and r4,r4,#15
                cmp r4,#8
                blo .Lf3lt8
                beq .Lf3eq8
                
                cmp r4,#12
                blo .Lf3lt12
                beq .Lf3eq12
                
                cmp r4,#14
                blo .Lf3lt14
                beq .Lf3eq14
                // 15 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12,s13,s14}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s28,s29,s30}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                vldmia r3!, {s15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 s14,s14,s30
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vadd.f32 s14,s14,s15
                
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12,s13,s14}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 q12,q12,q3
                
                b .Lf3end
.Lf3eq14:
                // 14 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{d6}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{d14}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{d6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf3end
.Lf3lt14:
                // 13 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s13}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{s14}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                fadds s12,s12,s13
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                fadds s12,s12,s14
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf3end
.Lf3eq12:
                // 12 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r2!,{q4,q5,q6}
                vldmia r3!,{q8,q9,q10}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vstmia r5!,{q0,q1,q2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf3end
.Lf3lt12:
                cmp r4,#10
                blo .Lf3lt10
                beq .Lf3eq10
                // 11 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8,s9,s10}
                vldmia r2!,{q4,q5}
                vldmia r2!,{s24,s25,s26}
                vldmia r3!,{q8,q9}
                vldmia r3!,{d20}
                vldmia r3!,{s11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vadd.f32 s10,s10,s26
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 d4,d4,d20
                vadd.f32 s10,s10,s11
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8,s9,s10}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf3end
.Lf3eq10:
                // 10 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{d4}
                vldmia r2!,{q4,q5}
                vldmia r2!,{d12}
                vldmia r3!,{q8,q9}
                vldmia r3!,{d20}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 d4,d4,d20
                vstmia r5!,{q0,q1}
                vstmia r5!,{d4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4
                
                b .Lf3end
.Lf3lt10:
                // 9 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8}
                vldmia r2!,{q4,q5}
                vldmia r2!,{s9}
                vldmia r3!,{q8,q9}
                vldmia r3!,{s10}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                fadds s8,s8,s9
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                fadds s8,s8,s10
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4

                b .Lf3end
.Lf3eq8:
                // 8 elems
                vldmia r0!,{q0,q1}
                vldmia r2!,{q4,q5}
                vldmia r3!,{q8,q9}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vstmia r5!,{q0,q1}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf3end
.Lf3lt8:
                cmp r4,#4
                blo .Lf3lt4
                beq .Lf3eq4
                
                cmp r4,#6
                blo .Lf3lt6
                beq .Lf3eq6
                // 7 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4,s5,s6}
                vldmia r2!,{q4}
                vldmia r2!,{s20,s21,s22}
                vldmia r3!,{q8}
                vldmia r3!,{d18}
                vldmia r3!,{s7}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 s6,s6,s22
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d18
                vadd.f32 s6,s6,s7
                vstmia r5!,{q0}
                vstmia r5!,{s4,s5,s6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf3end
.Lf3eq6:
                // 6 elems
                vldmia r0!,{q0}
                vldmia r0!,{d2}
                vldmia r2!,{q4}
                vldmia r2!,{d10}
                vldmia r3!,{q8}
                vldmia r3!,{d18}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d18
                vstmia r5!,{q0}
                vstmia r5!,{d2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf3end
.Lf3lt6:
                // 5 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4}
                vldmia r2!,{q4}
                vldmia r2!,{s20}
                vldmia r3!,{q8}
                vldmia r3!,{s5}
                
                vadd.f32 q0,q0,q4
                fadds s4,s4,s20
                vadd.f32 q0,q0,q8
                fadds s4,s4,s5
                vstmia r5!,{q0}
                vstmia r5!,{s4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf3end
.Lf3eq4:
                // 4 elems
                vldmia r0!,{q0}
                vldmia r2!,{q4}
                vldmia r3!,{q8}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q0,q0,q8
                vstmia r5!,{q0}
                
                vmax.f32 q12,q12,q0
                
                b .Lf3end
.Lf3lt4:
                cmp r4,#2
                blo .Lf3lt2
                beq .Lf3eq2
                // 3 elems
                vldmia r0!,{s0,s1,s2}
                vldmia r2!,{s16,s17,s18}
                vldmia r3!,{d16}
                vldmia r3!,{s3}
                
                vadd.f32 d0,d0,d8
                vadd.f32 s2,s2,s18
                vadd.f32 d0,d0,d16
                vadd.f32 s2,s2,s3
                vstmia r5!,{s0,s1,s2}
                
                vmax.f32 q12,q12,q0
                
                b .Lf3end
.Lf3eq2:
                // 2 elems
                vldmia r0!,{d0}
                vldmia r2!,{d4}
                vldmia r3!,{d8}
                
                vadd.f32 d0,d0,d4
                vadd.f32 d0,d0,d8
                vstmia r5!,{d0}
                
                vmax.f32 d24,d24,d0
                
                b .Lf3end
.Lf3lt2:
                cmp r4,#0
                beq .Lf3eq0
                
                vldmia r0!,{s0}
                vldmia r2!,{s1}
                vldmia r3!,{s2}
                
                vadd.f32 s0,s0,s1
                vadd.f32 s0,s0,s2
                vstmia r5!,{s0}
                
                vmax.f32 d24,d24,d0
.Lf3eq0:
.Lf3end:
                vpmax.f32 d24,d24,d25
                vpmax.f32 d0,d24,d24

                fmrs r0,s0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,lr}
                bx lr
                
                
                .global neon_foldArrayBy3_lge31
                .type neon_foldArrayBy3_lge31, %function
neon_foldArrayBy3_lge31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r6,r6,#4*31
                cmp r0,r6
                bhs .Lendf3loop2
.Lf3loop2:
                FOLDBY3_CORE
                FOLDBY3_CORE
                
                cmp r0,r6
                blo .Lf3loop2
.Lendf3loop2:                
                add r6,r6,#4*16
                cmp r0,r6
                bhs .Lendf3loop1
.Lf3loop3:
                FOLDBY3_CORE

                cmp r0,r6
                blo .Lf3loop3
.Lendf3loop3:
                b .Lendf3loop1
               
.Lfoldby3sel:
.rept 31
                .word neon_foldArrayBy3_ll31
.endr
                .word neon_foldArrayBy3_lge31
               
                /******
                 * fold array by 4
                 ******/
                .global neon_foldArrayBy4_ll31
                .type neon_foldArrayBy4_ll31, %function
neon_foldArrayBy4_ll31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldr r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                ldrd r4,[r1]  // di,dest
                add r7,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r7,r7,#4*15
                cmp r0,r7
                bhs .Lendf4loop1
.Lf4loop1:
.macro FOLDBY4_CORE
                vldmia r0!,{q0,q1,q2,q3}
                vldmia r2!,{q4,q5,q6,q7}
                vldmia r3!,{q8,q9,q10,q11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q3,q3,q7
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vldmia r6!,{q8,q9,q10,q11}
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vstmia r5!,{q0,q1,q2,q3}
                
                vmax.f32 q13,q0,q1
                vmax.f32 q14,q2,q3
                vmax.f32 q12,q12,q13
                vmax.f32 q12,q12,q14
.endm
                FOLDBY4_CORE

                cmp r0,r7
                blo .Lf4loop1
.Lendf4loop1:
                and r4,r4,#15
                cmp r4,#8
                blo .Lf4lt8
                beq .Lf4eq8
                
                cmp r4,#12
                blo .Lf4lt12
                beq .Lf4eq12
                
                cmp r4,#14
                blo .Lf4lt14
                beq .Lf4eq14
                // 15 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12,s13,s14}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s28,s29,s30}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                vldmia r3, {s15}
                vldmia r6!,{q13,q14,q15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 s14,s14,s30
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vadd.f32 s14,s14,s15
                vldmia r6!,{s16,s17,s18}
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d8
                vadd.f32 s14,s14,s18
                
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12,s13,s14}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 q12,q12,q3
                
                b .Lf4end
.Lf4eq14:
                // 14 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{d6}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{d14}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                vldmia r6!,{q13,q14,q15}
                vldmia r6!,{d7}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d7
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{d6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf4end
.Lf4lt14:
                // 13 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s13}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{s14}
                vldmia r6!,{q13,q14,q15}
                vldmia r6!,{s15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                fadds s12,s12,s13
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                fadds s12,s12,s14
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                fadds s12,s12,s15
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf4end
.Lf4eq12:
                // 12 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r2!,{q4,q5,q6}
                vldmia r3!,{q8,q9,q10}
                vldmia r6!,{q13,q14,q15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vstmia r5!,{q0,q1,q2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf4end
.Lf4lt12:
                cmp r4,#10
                blo .Lf4lt10
                beq .Lf4eq10
                // 11 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8,s9,s10}
                vldmia r2!,{q4,q5}
                vldmia r2!,{s24,s25,s26}
                vldmia r3!,{q8,q9}
                vldmia r3!,{d20}
                vldmia r3!,{s11}
                vldmia r6!,{q13,q14}
                vldmia r6!,{s12,s13,s14}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vadd.f32 s10,s10,s26
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 d4,d4,d20
                vadd.f32 s10,s10,s11
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 d4,d4,d6
                vadd.f32 s10,s10,s14
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8,s9,s10}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf4end
.Lf4eq10:
                // 10 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{d4}
                vldmia r2!,{q4,q5}
                vldmia r2!,{d12}
                vldmia r3!,{q8,q9}
                vldmia r3!,{d20}
                vldmia r6!,{q13,q14}
                vldmia r6!,{d5}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 d4,d4,d20
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 d4,d4,d5
                vstmia r5!,{q0,q1}
                vstmia r5!,{d4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4
                
                b .Lf4end
.Lf4lt10:
                // 9 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8}
                vldmia r2!,{q4,q5}
                vldmia r2!,{s9}
                vldmia r3!,{q8,q9}
                vldmia r3!,{s10}
                vldmia r6!,{q13,q14}
                vldmia r6!,{s11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                fadds s8,s8,s9
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                fadds s8,s8,s10
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                fadds s8,s8,s11
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4

                b .Lf4end
.Lf4eq8:
                // 8 elems
                vldmia r0!,{q0,q1}
                vldmia r2!,{q4,q5}
                vldmia r3!,{q8,q9}
                vldmia r6!,{q13,q14}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vstmia r5!,{q0,q1}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf4end
.Lf4lt8:
                cmp r4,#4
                blo .Lf4lt4
                beq .Lf4eq4
                
                cmp r4,#6
                blo .Lf4lt6
                beq .Lf4eq6
                // 7 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4,s5,s6}
                vldmia r2!,{q4}
                vldmia r2!,{s20,s21,s22}
                vldmia r3!,{q8}
                vldmia r3!,{d18}
                vldmia r3!,{s7}
                vldmia r6!,{q13}
                vldmia r6!,{s8,s9,s10}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 s6,s6,s22
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d18
                vadd.f32 s6,s6,s7
                vadd.f32 q0,q0,q13
                vadd.f32 d2,d2,d4
                vadd.f32 s6,s6,s10
                vstmia r5!,{q0}
                vstmia r5!,{s4,s5,s6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf4end
.Lf4eq6:
                // 6 elems
                vldmia r0!,{q0}
                vldmia r0!,{d2}
                vldmia r2!,{q4}
                vldmia r2!,{d10}
                vldmia r3!,{q8}
                vldmia r3!,{d18}
                vldmia r6!,{q13}
                vldmia r6!,{d3}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d18
                vadd.f32 q0,q0,q13
                vadd.f32 d2,d2,d3
                vstmia r5!,{q0}
                vstmia r5!,{d2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf4end
.Lf4lt6:
                // 5 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4}
                vldmia r2!,{q4}
                vldmia r2!,{s20}
                vldmia r3!,{q8}
                vldmia r3!,{s5}
                vldmia r6!,{q13}
                vldmia r6!,{s6}
                
                vadd.f32 q0,q0,q4
                fadds s4,s4,s20
                vadd.f32 q0,q0,q8
                fadds s4,s4,s5
                vadd.f32 q0,q0,q13
                fadds s4,s4,s6
                vstmia r5!,{q0}
                vstmia r5!,{s4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf4end
.Lf4eq4:
                // 4 elems
                vldmia r0!,{q0}
                vldmia r2!,{q4}
                vldmia r3!,{q8}
                vldmia r6!,{q13}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q0,q0,q8
                vadd.f32 q0,q0,q13
                vstmia r5!,{q0}
                
                vmax.f32 q12,q12,q0
                
                b .Lf4end
.Lf4lt4:
                cmp r4,#2
                blo .Lf4lt2
                beq .Lf4eq2
                // 3 elems
                vldmia r0!,{s0,s1,s2}
                vldmia r2!,{s16,s17,s18}
                vldmia r3!,{d16}
                vldmia r3!,{s3}
                vldmia r6!,{s4,s5,s6}
                
                vadd.f32 d0,d0,d8
                vadd.f32 s2,s2,s18
                vadd.f32 d0,d0,d16
                vadd.f32 s2,s2,s3
                vadd.f32 d0,d0,d2
                vadd.f32 s2,s2,s6
                vstmia r5!,{s0,s1,s2}
                
                vmax.f32 q12,q12,q0
                
                b .Lf4end
.Lf4eq2:
                // 2 elems
                vldmia r0!,{d0}
                vldmia r2!,{d4}
                vldmia r3!,{d8}
                vldmia r6!,{d12}
                
                vadd.f32 d0,d0,d4
                vadd.f32 d0,d0,d8
                vadd.f32 d0,d0,d12
                vstmia r5!,{d0}
                
                vmax.f32 d24,d24,d0
                
                b .Lf4end
.Lf4lt2:
                cmp r4,#0
                beq .Lf4eq0
                
                vldmia r0!,{s0}
                vldmia r2!,{s1}
                vldmia r3!,{s2}
                vldmia r6!,{s3}
                
                vadd.f32 s0,s0,s1
                vadd.f32 s0,s0,s2
                vadd.f32 s0,s0,s3
                vstmia r5!,{s0}
                
                vmax.f32 d24,d24,d0
.Lf4eq0:
.Lf4end:
                vpmax.f32 d24,d24,d25
                vpmax.f32 d0,d24,d24

                fmrs r0,s0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,r7,r8,lr}
                bx lr
                
                
                .global neon_foldArrayBy4_lge31
                .type neon_foldArrayBy4_lge31, %function
neon_foldArrayBy4_lge31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldr r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                ldrd r4,[r1]  // di,dest
                add r7,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r7,r7,#4*31
                cmp r0,r7
                bhs .Lendf4loop2
.Lf4loop2:
                FOLDBY4_CORE
                FOLDBY4_CORE
                
                cmp r0,r7
                blo .Lf4loop2
.Lendf4loop2:
                add r7,r7,#4*16
                cmp r0,r7
                bhs .Lendf4loop1
.Lf4loop3:
                FOLDBY4_CORE

                cmp r0,r7
                blo .Lf4loop3
.Lendf4loop3:
                b .Lendf4loop1
                
.Lfoldby4sel:
.rept 31
                .word neon_foldArrayBy4_ll31
.endr
                .word neon_foldArrayBy4_lge31

                /******
                 * fold array by 5
                 ******/
                .global neon_foldArrayBy5_ll31
                .type neon_foldArrayBy5_ll31, %function
neon_foldArrayBy5_ll31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldrd r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                add r7,r0,r7,lsl #2
                ldrd r4,[r1]  // di,dest
                add r8,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r8,r8,#4*15
                cmp r0,r8
                bhs .Lendf5loop1
.Lf5loop1:
.macro FOLDBY5_CORE
                vldmia r0!,{q0,q1,q2,q3}
                vldmia r2!,{q4,q5,q6,q7}
                vldmia r3!,{q8,q9,q10,q11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q3,q3,q7
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vldmia r6!,{q8,q9,q10,q11}
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vldmia r7!,{q8,q9,q10,q11}
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 q3,q3,q11
                vstmia r5!,{q0,q1,q2,q3}
                
                vmax.f32 q13,q0,q1
                vmax.f32 q14,q2,q3
                vmax.f32 q12,q12,q13
                vmax.f32 q12,q12,q14
.endm
                FOLDBY5_CORE

                cmp r0,r8
                blo .Lf5loop1
.Lendf5loop1:
                and r4,r4,#15
                cmp r4,#8
                blo .Lf5lt8
                beq .Lf5eq8
                
                cmp r4,#12
                blo .Lf5lt12
                beq .Lf5eq12
                
                cmp r4,#14
                blo .Lf5lt14
                beq .Lf5eq14
                // 15 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12,s13,s14}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s28,s29,s30}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                vldmia r3, {s15}
                vldmia r6!,{q13,q14,q15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 s14,s14,s30
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vadd.f32 s14,s14,s15
                vldmia r6!,{s16,s17,s18}
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d8
                vadd.f32 s14,s14,s18
                vldmia r7!,{q13,q14,q15}
                vldmia r7!,{s16,s17,s18}
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d8
                vadd.f32 s14,s14,s18
                
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12,s13,s14}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 q12,q12,q3
                
                b .Lf5end
.Lf5eq14:
                // 14 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{d6}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{d14}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{d22}
                vldmia r6!,{q13,q14,q15}
                vldmia r6!,{d7}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                vadd.f32 d6,d6,d22
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d7
                vldmia r7!,{q13,q14,q15}
                vldmia r7!,{d7}
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vadd.f32 d6,d6,d7
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{d6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf5end
.Lf5lt14:
                // 13 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r0!,{s12}
                vldmia r2!,{q4,q5,q6}
                vldmia r2!,{s13}
                vldmia r3!,{q8,q9,q10}
                vldmia r3!,{s14}
                vldmia r6!,{q13,q14,q15}
                vldmia r6!,{s15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                fadds s12,s12,s13
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q2,q2,q10
                fadds s12,s12,s14
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                fadds s12,s12,s15
                vldmia r7!,{q13,q14,q15}
                vldmia r7!,{s15}
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                fadds s12,s12,s15
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf5end
.Lf5eq12:
                // 12 elems
                vldmia r0!,{q0,q1,q2}
                vldmia r2!,{q3,q4,q5}
                vldmia r3!,{q6,q7,q8}
                vldmia r6!,{q9,q10,q11}
                vldmia r7!,{q13,q14,q15}
                
                vadd.f32 q0,q0,q3
                vadd.f32 q1,q1,q4
                vadd.f32 q2,q2,q5
                vadd.f32 q0,q0,q6
                vadd.f32 q1,q1,q7
                vadd.f32 q2,q2,q8
                vadd.f32 q0,q0,q9
                vadd.f32 q1,q1,q10
                vadd.f32 q2,q2,q11
                vadd.f32 q0,q0,q13
                vadd.f32 q1,q1,q14
                vadd.f32 q2,q2,q15
                vstmia r5!,{q0,q1,q2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf5end
.Lf5lt12:
                cmp r4,#10
                blo .Lf5lt10
                beq .Lf5eq10
                // 11 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8,s9,s10}
                vldmia r2!,{q7,q8}
                vldmia r2!,{d6}
                vldmia r2!,{s11}
                vldmia r3!,{q9,q10}
                vldmia r3!,{s14,s15,s16}
                vldmia r6!,{q11}
                vldmia r6!,{q13}
                vldmia r6!,{s18,s19,s20}
                vldmia r7!,{q14}
                vldmia r7!,{q15}
                vldmia r7!,{s22,s23,s24}

                vadd.f32 q0,q0,q7
                vadd.f32 q1,q1,q8
                vadd.f32 d4,d4,d6
                vadd.f32 s10,s10,s11
                vadd.f32 q0,q0,q9
                vadd.f32 q1,q1,q10
                vadd.f32 d4,d4,d7
                vadd.f32 s10,s10,s16
                vadd.f32 q0,q0,q11
                vadd.f32 q1,q1,q13
                vadd.f32 d4,d4,d9
                vadd.f32 s10,s10,s20
                vadd.f32 q0,q0,q14
                vadd.f32 q1,q1,q15
                vadd.f32 d4,d4,d11
                vadd.f32 s10,s10,s24
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8,s9,s10}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf5end
.Lf5eq10:
                // 10 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{d4}
                vldmia r2!,{q3,q4}
                vldmia r2!,{d5}
                vldmia r3!,{q5,q6}
                vldmia r3!,{d14}
                vldmia r6!,{q8,q9}
                vldmia r6!,{d15}
                vldmia r7!,{q10,q11}
                vldmia r7!,{d26}
                
                vadd.f32 q0,q0,q3
                vadd.f32 q1,q1,q4
                vadd.f32 d4,d4,d5
                vadd.f32 q0,q0,q5
                vadd.f32 q1,q1,q6
                vadd.f32 d4,d4,d14
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 d4,d4,d15
                vadd.f32 q0,q0,q10
                vadd.f32 q1,q1,q11
                vadd.f32 d4,d4,d26
                vstmia r5!,{q0,q1}
                vstmia r5!,{d4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4
                
                b .Lf5end
.Lf5lt10:
                // 9 elems
                vldmia r0!,{q0,q1}
                vldmia r0!,{s8}
                vldmia r2!,{q4,q5}
                vldmia r2!,{s9}
                vldmia r3!,{q6,q7}
                vldmia r3!,{s10}
                vldmia r6!,{q8,q9}
                vldmia r6!,{s11}
                vldmia r7!,{q10,q11}
                vldmia r7!,{s12}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                fadds s8,s8,s9
                vadd.f32 q0,q0,q6
                vadd.f32 q1,q1,q7
                fadds s8,s8,s10
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                fadds s8,s8,s11
                vadd.f32 q0,q0,q10
                vadd.f32 q1,q1,q11
                fadds s8,s8,s12
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4

                b .Lf5end
.Lf5eq8:
                // 8 elems
                vldmia r0!,{q0,q1}
                vldmia r2!,{q2,q3}
                vldmia r3!,{q4,q5}
                vldmia r6!,{q8,q9}
                vldmia r7!,{q10,q11}
                
                vadd.f32 q0,q0,q2
                vadd.f32 q1,q1,q3
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q0,q0,q8
                vadd.f32 q1,q1,q9
                vadd.f32 q0,q0,q10
                vadd.f32 q1,q1,q11
                vstmia r5!,{q0,q1}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf5end
.Lf5lt8:
                cmp r4,#4
                blo .Lf5lt4
                beq .Lf5eq4
                
                cmp r4,#6
                blo .Lf5lt6
                beq .Lf5eq6
                // 7 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4,s5,s6}
                vldmia r2!,{q2}
                vldmia r2!,{d6}
                vldmia r2!,{s7}
                vldmia r3!,{q4}
                vldmia r3!,{s20,s21,s22}
                vldmia r6!,{q6}
                vldmia r6!,{s28,s29,s30}
                vldmia r7!,{q8}
                vldmia r7!,{d7}
                vldmia r7!,{s31}
                
                vadd.f32 q0,q0,q2
                vadd.f32 d2,d2,d6
                vadd.f32 s6,s6,s7
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 s6,s6,s22
                vadd.f32 q0,q0,q6
                vadd.f32 d2,d2,d14
                vadd.f32 s6,s6,s30
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d7
                vadd.f32 s6,s6,s31
                vstmia r5!,{q0}
                vstmia r5!,{s4,s5,s6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf5end
.Lf5eq6:
                // 6 elems
                vldmia r0!,{q0}
                vldmia r0!,{d2}
                vldmia r2!,{q2}
                vldmia r2!,{d6}
                vldmia r3!,{q4}
                vldmia r3!,{d10}
                vldmia r6!,{q6}
                vldmia r6!,{d14}
                vldmia r7!,{q8}
                vldmia r7!,{d15}
                
                vadd.f32 q0,q0,q2
                vadd.f32 d2,d2,d6
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 q0,q0,q6
                vadd.f32 d2,d2,d14
                vadd.f32 q0,q0,q8
                vadd.f32 d2,d2,d15
                vstmia r5!,{q0}
                vstmia r5!,{d2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf5end
.Lf5lt6:
                // 5 elems
                vldmia r0!,{q0}
                vldmia r0!,{s4}
                vldmia r2!,{q2}
                vldmia r2!,{s5}
                vldmia r3!,{q4}
                vldmia r3!,{s6}
                vldmia r6!,{q6}
                vldmia r6!,{s7}
                vldmia r7!,{q8}
                vldmia r7!,{s12}
                
                vadd.f32 q0,q0,q2
                fadds s4,s4,s5
                vadd.f32 q0,q0,q4
                fadds s4,s4,s6
                vadd.f32 q0,q0,q6
                fadds s4,s4,s7
                vadd.f32 q0,q0,q8
                fadds s4,s4,s12
                vstmia r5!,{q0}
                vstmia r5!,{s4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf5end
.Lf5eq4:
                // 4 elems
                vldmia r0!,{q0}
                vldmia r2!,{q4}
                vldmia r3!,{q8}
                vldmia r6!,{q13}
                vldmia r7!,{q14}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q0,q0,q8
                vadd.f32 q0,q0,q13
                vadd.f32 q0,q0,q14
                vstmia r5!,{q0}
                
                vmax.f32 q12,q12,q0
                
                b .Lf5end
.Lf5lt4:
                cmp r4,#2
                blo .Lf5lt2
                beq .Lf5eq2
                // 3 elems
                vldmia r0!,{s0,s1,s2}
                vldmia r2!,{d2}
                vldmia r2!,{s3}
                vldmia r3!,{s6,s7,s8}
                vldmia r6!,{s10,s11,s12}
                vldmia r7!,{s14,s15,s16}
                
                vadd.f32 d0,d0,d2
                vadd.f32 s2,s2,s3
                vadd.f32 d0,d0,d3
                vadd.f32 s2,s2,s8
                vadd.f32 d0,d0,d5
                vadd.f32 s2,s2,s12
                vadd.f32 d0,d0,d7
                vadd.f32 s2,s2,s16
                vstmia r5!,{s0,s1,s2}
                
                vmax.f32 q12,q12,q0
                
                b .Lf5end
.Lf5eq2:
                // 2 elems
                vldmia r0!,{d0}
                vldmia r2!,{d4}
                vldmia r3!,{d8}
                vldmia r6!,{d12}
                vldmia r7!,{d13}
                
                vadd.f32 d0,d0,d4
                vadd.f32 d0,d0,d8
                vadd.f32 d0,d0,d12
                vadd.f32 d0,d0,d13
                vstmia r5!,{d0}
                
                vmax.f32 d24,d24,d0
                
                b .Lf5end
.Lf5lt2:
                cmp r4,#0
                beq .Lf5eq0
                
                vldmia r0!,{s0}
                vldmia r2!,{s1}
                vldmia r3!,{s2}
                vldmia r6!,{s3}
                vldmia r7!,{s4}
                
                vadd.f32 s0,s0,s1
                vadd.f32 s0,s0,s2
                vadd.f32 s0,s0,s3
                vadd.f32 s0,s0,s4
                vstmia r5!,{s0}
                
                vmax.f32 d24,d24,d0
.Lf5eq0:
.Lf5end:
                vpmax.f32 d24,d24,d25
                vpmax.f32 d0,d24,d24

                fmrs r0,s0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,r7,r8,lr}
                bx lr
                
.Lfoldby5sel:
.rept 31
                .word neon_foldArrayBy5_ll31
.endr
                .word neon_foldArrayBy5_lge31

                
                .global neon_foldArrayBy5_lge31
                .type neon_foldArrayBy5_lge31, %function
neon_foldArrayBy5_lge31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldrd r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                add r7,r0,r7,lsl #2
                ldrd r4,[r1]  // di,dest
                add r8,r0,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r8,r8,#4*31
                cmp r0,r8
                bhs .Lendf5loop2
.Lf5loop2:
                FOLDBY5_CORE
                FOLDBY5_CORE
                
                cmp r0,r8
                blo .Lf5loop2
.Lendf5loop2:
                add r8,r8,#4*16
                cmp r0,r8
                bhs .Lendf5loop1
.Lf5loop3:
                FOLDBY5_CORE

                cmp r0,r8
                blo .Lf5loop3
.Lendf5loop3:
                b .Lendf5loop1
                

                /*****
                 * fold array by 2
                 ******/
                .global neon_foldArrayBy2_ll31
                .type neon_foldArrayBy2_ll31, %function
neon_foldArrayBy2_ll31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0,#4] // ss1
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r2,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r6,r6,#4*15
                cmp r2,r6
                bhs .Lendf2loop1
.Lf2loop1:
.macro FOLDBY2_CORE
                vldmia r2!,{q0,q1,q2,q3}
                vldmia r3!,{q4,q5,q6,q7}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 q3,q3,q7
                vstmia r5!,{q0,q1,q2,q3}
                
                vmax.f32 q13,q0,q1
                vmax.f32 q14,q2,q3
                vmax.f32 q12,q12,q13
                vmax.f32 q12,q12,q14
.endm
                FOLDBY2_CORE

                cmp r2,r6
                blo .Lf2loop1
.Lendf2loop1:
                and r4,r4,#15
                cmp r4,#8
                blo .Lf2lt8
                beq .Lf2eq8
                
                cmp r4,#12
                blo .Lf2lt12
                beq .Lf2eq12
                
                cmp r4,#14
                blo .Lf2lt14
                beq .Lf2eq14
                // 15 elems
                vldmia r2!,{q0,q1,q2}
                vldmia r2!,{s12,s13,s14}
                vldmia r3!,{q4,q5,q6}
                vldmia r3!,{s28,s29}
                vldmia r3!,{s15}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vadd.f32 s14,s14,s15
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12,s13,s14}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 q12,q12,q3
                
                b .Lf2end
.Lf2eq14:
                // 14 elems
                vldmia r2!,{q0,q1,q2}
                vldmia r2!,{d6}
                vldmia r3!,{q4,q5,q6}
                vldmia r3!,{d14}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vadd.f32 d6,d6,d14
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{d6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf2end
.Lf2lt14:
                // 13 elems
                vldmia r2!,{q0,q1,q2}
                vldmia r2!,{s12}
                vldmia r3!,{q4,q5,q6}
                vldmia r3!,{s13}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                fadds s12,s12,s13
                vstmia r5!,{q0,q1,q2}
                vstmia r5!,{s12}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                vmax.f32 d24,d24,d6
                
                b .Lf2end
.Lf2eq12:
                // 12 elems
                vldmia r2!,{q0,q1,q2}
                vldmia r3!,{q4,q5,q6}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 q2,q2,q6
                vstmia r5!,{q0,q1,q2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf2end
.Lf2lt12:
                cmp r4,#10
                blo .Lf2lt10
                beq .Lf2eq10
                // 11 elems
                vldmia r2!,{q0,q1}
                vldmia r2!,{s8,s9,s10}
                vldmia r3!,{q4,q5}
                vldmia r3!,{s24,s25}
                vldmia r3!,{s11}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vadd.f32 s10,s10,s11
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8,s9,s10}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 q12,q12,q2
                
                b .Lf2end
.Lf2eq10:
                // 10 elems
                vldmia r2!,{q0,q1}
                vldmia r2!,{d4}
                vldmia r3!,{q4,q5}
                vldmia r3!,{d12}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vadd.f32 d4,d4,d12
                vstmia r5!,{q0,q1}
                vstmia r5!,{d4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4
                
                b .Lf2end
.Lf2lt10:
                // 9 elems
                vldmia r2!,{q0,q1}
                vldmia r2!,{s8}
                vldmia r3!,{q4,q5}
                vldmia r3!,{s9}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                fadds s8,s8,s9
                vstmia r5!,{q0,q1}
                vstmia r5!,{s8}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                vmax.f32 d24,d24,d4

                b .Lf2end
.Lf2eq8:
                // 8 elems
                vldmia r2!,{q0,q1}
                vldmia r3!,{q4,q5}
                
                vadd.f32 q0,q0,q4
                vadd.f32 q1,q1,q5
                vstmia r5!,{q0,q1}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf2end
.Lf2lt8:
                cmp r4,#4
                blo .Lf2lt4
                beq .Lf2eq4
                
                cmp r4,#6
                blo .Lf2lt6
                beq .Lf2eq6
                // 7 elems
                vldmia r2!,{q0}
                vldmia r2!,{s4,s5,s6}
                vldmia r3!,{q4}
                vldmia r3!,{s20,s21}
                vldmia r3!,{s7}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vadd.f32 s6,s6,s7
                vstmia r5!,{q0}
                vstmia r5!,{s4,s5,s6}
                
                vmax.f32 q12,q12,q0
                vmax.f32 q12,q12,q1
                
                b .Lf2end
.Lf2eq6:
                // 6 elems
                vldmia r2!,{q0}
                vldmia r2!,{d2}
                vldmia r3!,{q4}
                vldmia r3!,{d10}
                
                vadd.f32 q0,q0,q4
                vadd.f32 d2,d2,d10
                vstmia r5!,{q0}
                vstmia r5!,{d2}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf2end
.Lf2lt6:
                // 5 elems
                vldmia r2!,{q0}
                vldmia r2!,{s4}
                vldmia r3!,{q4}
                vldmia r3!,{s5}
                
                vadd.f32 q0,q0,q4
                fadds s4,s4,s5
                vstmia r5!,{q0}
                vstmia r5!,{s4}
                
                vmax.f32 q12,q12,q0
                vmax.f32 d24,d24,d2
                
                b .Lf2end
.Lf2eq4:
                // 4 elems
                vldmia r2!,{q0}
                vldmia r3!,{q4}
                
                vadd.f32 q0,q0,q4
                vstmia r5!,{q0}
                
                vmax.f32 q12,q12,q0
                
                b .Lf2end
.Lf2lt4:
                cmp r4,#2
                blo .Lf2lt2
                beq .Lf2eq2
                // 3 elems
                vldmia r2!,{s0,s1,s2}
                vldmia r3!,{s16,s17}
                vldmia r3!,{s3}
                
                vadd.f32 d0,d0,d8
                vadd.f32 s2,s2,s3
                vstmia r5!,{s0,s1,s2}
                
                vmax.f32 q12,q12,q0
                
                b .Lf2end
.Lf2eq2:
                // 2 elems
                vldmia r2!,{d0}
                vldmia r3!,{d4}
                
                vadd.f32 d0,d0,d4
                vstmia r5!,{d0}
                
                vmax.f32 d24,d24,d0
                
                b .Lf2end
.Lf2lt2:
                cmp r4,#0
                beq .Lf2eq0
                
                vldmia r2!,{s0}
                vldmia r3!,{s1}
                
                vadd.f32 s0,s0,s1
                vstmia r5!,{s0}
                
                vmax.f32 d24,d24,d0
.Lf2eq0:
.Lf2end:
                vpmax.f32 d24,d24,d25
                vpmax.f32 d0,d24,d24

                fmrs r0,s0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,lr}
                bx lr
                
                
                .global neon_foldArrayBy2_lge31
                .type neon_foldArrayBy2_lge31, %function
neon_foldArrayBy2_lge31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
                
                ldr r0,[r0,#4] // ss1
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r2,r4,lsl #2 // end
                
                veor.i32 q12,q12,q12
                sub r6,r6,#4*31
                cmp r2,r6
                bhs .Lendf2loop2
.Lf2loop2:
                FOLDBY2_CORE
                FOLDBY2_CORE
                
                cmp r2,r6
                blo .Lf2loop2
.Lendf2loop2:
                add r6,r6,#4*16
                cmp r2,r6
                bhs .Lendf2loop1
.Lf2loop3:
                FOLDBY2_CORE

                cmp r2,r6
                blo .Lf2loop3
.Lendf2loop3:
                b .Lendf2loop1
                
.Lfoldby2sel:
.rept 31
                .word neon_foldArrayBy2_ll31
.endr
                .word neon_foldArrayBy2_lge31
                
                .align 2
.Lname:
                .string "opt NEON"
                
                .align 2
                .global neonFoldMain
neonFoldMain:
                .word .Lfoldby3sel
                .word .Lfoldby4sel
                .word .Lfoldby5sel
                .word .Lfoldby2sel
                .word .Lfoldby2sel
                .word .Lname
                