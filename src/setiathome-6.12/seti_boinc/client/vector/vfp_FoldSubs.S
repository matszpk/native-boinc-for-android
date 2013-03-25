/*
 * vfp_FoldSubs.S
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
                /*****
                 * fold array by 3 
                 ******/
.Lzeros:
                .float 0.0,0.0
                
                .global vfp_foldArrayBy3_ll31
                .type vfp_foldArrayBy3_ll31, %function
vfp_foldArrayBy3_ll31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r0,r4,lsl #2 // end
                
                fldd d12,.Lzeros
                sub r6,r6,#4*7
                cmp r0,r6
                bhs .Lendf3loop2
.Lf3loop2:
.macro FOLDBY3_CORE
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22,s23}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fadds s7,s7,s23
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6,s7}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcmpes s25,s7
                fcpysmi s24,s6
                fmstat
                fcpysmi s25,s7
.endm
                FOLDBY3_CORE

                cmp r0,r6
                blo .Lf3loop2
.Lendf3loop2:
                and r4,r4,#7
                cmp r4,#4
                blo .Lf3lt4
                beq .Lf3eq4

                cmp r4,#6
                blo .Lf3lt6                
                beq .Lf3eq6
                
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcpysmi s24,s6
                b .Lf3end
.Lf3eq6:
                fldmias r0!,{s0,s1,s2,s3,s4,s5}
                fldmias r2!,{s8,s9,s10,s11,s12,s13}
                fldmias r3!,{s16,s17,s18,s19,s20,s21}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fstmias r5!,{s0,s1,s2,s3,s4,s5}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcpysmi s25,s5
                b .Lf3end
.Lf3lt6:
                fldmias r0!,{s0,s1,s2,s3,s4}
                fldmias r2!,{s8,s9,s10,s11,s12}
                fldmias r3!,{s16,s17,s18,s19,s20}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fstmias r5!,{s0,s1,s2,s3,s4}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcpysmi s24,s4
                b .Lf3end
.Lf3eq4:
                fldmias r0!,{s0,s1,s2,s3}
                fldmias r2!,{s8,s9,s10,s11}
                fldmias r3!,{s16,s17,s18,s19}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fstmias r5!,{s0,s1,s2,s3}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcpysmi s25,s3
                b .Lf3end
.Lf3lt4:
                cmp r4,#2
                blo .Lf3lt2
                beq .Lf3eq2
                
                fldmias r0!,{s0,s1,s2}
                fldmias r2!,{s8,s9,s10}
                fldmias r3!,{s16,s17,s18}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fstmias r5!,{s0,s1,s2}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcpysmi s24,s2
                b .Lf3end
.Lf3eq2:
                fldmias r0!,{s0,s1}
                fldmias r2!,{s8,s9}
                fldmias r3!,{s16,s17}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s0,s0,s16
                fadds s1,s1,s17
                fstmias r5!,{s0,s1}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcpysmi s25,s1
                b .Lf3end
.Lf3lt2:
                cmp r4,#0
                beq .Lf3eq0
                fldmias r0!,{s0}
                fldmias r2!,{s1}
                fldmias r3!,{s2}
                
                fadds s0,s0,s1
                fadds s0,s0,s2
                fstmias r5!,{s0}
                
                fcmpes s24,s0
                fmstat
                fcpysmi s24,s0
.Lf3eq0:

.Lf3end:
                fcmpes s24,s25
                fmstat
                fcpysmi s24,s25
                
                fmrs r0,s24
                vpop {d8,d9,d10,d11,d12}
                pop {r4,r5,r6,lr}
                bx lr
                
.Lzeros1:
                .float 0.0,0.0
                
                .global vfp_foldArrayBy3_lge31
                .type vfp_foldArrayBy3_lge31, %function
vfp_foldArrayBy3_lge31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r0,r4,lsl #2 // end
                
                fldd d12,.Lzeros1
                sub r6,r6,#4*31
                cmp r0,r6
                bhs .Lendf3loop4
.Lf3loop4:
                FOLDBY3_CORE
                FOLDBY3_CORE
                FOLDBY3_CORE
                FOLDBY3_CORE
                
                cmp r0,r6
                blo .Lf3loop4
.Lendf3loop4:
                add r6,r6,#4*24
                cmp r0,r6
                bhs .Lendf3loop2
.Lf3loop5:
                FOLDBY3_CORE
                
                cmp r0,r6
                blo .Lf3loop5
.Lendf3loop5:
                b .Lendf3loop2                
                
.Lfoldby3sel:
.rept 31
                .word vfp_foldArrayBy3_ll31
.endr
                .word vfp_foldArrayBy3_lge31
                
.Lzeros2:
                .float 0.0,0.0
                /*****
                 * fold array by 4
                 ******/
                .global vfp_foldArrayBy4_ll31
                .type vfp_foldArrayBy4_ll31, %function
vfp_foldArrayBy4_ll31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldr r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                ldrd r4,[r1]  // di,dest
                add r7,r0,r4,lsl #2 // end
                
                fldd d12,.Lzeros2
                sub r7,r7,#4*7
                cmp r0,r7
                bhs .Lendf4loop2
.Lf4loop2:
.macro FOLDBY4_CORE
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22,s23}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fldmias r6!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fadds s7,s7,s23
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6,s7}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcmpes s25,s7
                fcpysmi s24,s6
                fmstat
                fcpysmi s25,s7
.endm
                FOLDBY4_CORE

                cmp r0,r7
                blo .Lf4loop2
.Lendf4loop2:
                and r4,r4,#7
                cmp r4,#4
                blo .Lf4lt4
                beq .Lf4eq4

                cmp r4,#6
                blo .Lf4lt6                
                beq .Lf4eq6
                
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fldmias r6!,{s16,s17,s18,s19,s20,s21,s22}
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcpysmi s24,s6
                b .Lf4end
.Lf4eq6:
                fldmias r0!,{s0,s1,s2,s3,s4,s5}
                fldmias r2!,{s6,s7,s8,s9,s10,s11}
                fldmias r3!,{s12,s13,s14,s15,s16,s17}
                fldmias r6!,{s18,s19,s20,s21,s22,s23}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s3,s3,s9
                fadds s4,s4,s10
                fadds s5,s5,s11
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s4,s4,s16
                fadds s5,s5,s17
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fadds s5,s5,s23
                fstmias r5!,{s0,s1,s2,s3,s4,s5}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcpysmi s25,s5
                b .Lf4end
.Lf4lt6:
                fldmias r0!,{s0,s1,s2,s3,s4}
                fldmias r2!,{s6,s7,s8,s9,s10}
                fldmias r3!,{s12,s13,s14,s15,s16}
                fldmias r6!,{s18,s19,s20,s21,s22}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s3,s3,s9
                fadds s4,s4,s10
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s4,s4,s16
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fstmias r5!,{s0,s1,s2,s3,s4}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcpysmi s24,s4
                b .Lf4end
.Lf4eq4:
                fldmias r0!,{s0,s1,s2,s3}
                fldmias r2!,{s6,s7,s8,s9}
                fldmias r3!,{s12,s13,s14,s15}
                fldmias r6!,{s18,s19,s20,s21}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s3,s3,s9
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fstmias r5!,{s0,s1,s2,s3}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcpysmi s25,s3
                b .Lf4end
.Lf4lt4:
                cmp r4,#2
                blo .Lf4lt2
                beq .Lf4eq2
                
                fldmias r0!,{s0,s1,s2}
                fldmias r2!,{s6,s7,s8}
                fldmias r3!,{s12,s13,s14}
                fldmias r6!,{s18,s19,s20}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fstmias r5!,{s0,s1,s2}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcpysmi s24,s2
                b .Lf4end
.Lf4eq2:
                fldmias r0!,{s0,s1}
                fldmias r2!,{s6,s7}
                fldmias r3!,{s12,s13}
                fldmias r6!,{s18,s19}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s0,s0,s18
                fadds s1,s1,s19
                fstmias r5!,{s0,s1}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcpysmi s25,s1
                b .Lf4end
.Lf4lt2:
                cmp r4,#0
                beq .Lf4eq0
                fldmias r0!,{s0}
                fldmias r2!,{s1}
                fldmias r3!,{s2}
                fldmias r6!,{s3}
                
                fadds s0,s0,s1
                fadds s0,s0,s2
                fadds s0,s0,s3
                fstmias r5!,{s0}
                
                fcmpes s24,s0
                fmstat
                fcpysmi s24,s0
.Lf4eq0:
                
.Lf4end:
                fcmpes s24,s25
                fmstat
                fcpysmi s24,s25
                
                fmrs r0,s24
                vpop {d8,d9,d10,d11,d12}
                pop {r4,r5,r6,r7,r8,lr}
                bx lr
                
.Lzeros2_1:
                .float 0.0,0.0
                .global vfp_foldArrayBy4_lge31
                .type vfp_foldArrayBy4_lge31, %function
vfp_foldArrayBy4_lge31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldr r6,[r1,#16] // tmp2
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                ldrd r4,[r1]  // di,dest
                add r7,r0,r4,lsl #2 // end
               
                fldd d12,.Lzeros2_1
                sub r7,r7,#4*31
                cmp r0,r7
                bhs .Lendf4loop4
.Lf4loop4:
                FOLDBY4_CORE
                FOLDBY4_CORE
                FOLDBY4_CORE
                FOLDBY4_CORE

                cmp r0,r7
                blo .Lf4loop4
.Lendf4loop4:
                add r7,r7,#4*24
                cmp r0,r7
                bhs .Lendf4loop2
.Lf4loop5:
                FOLDBY4_CORE
                
                cmp r0,r7
                blo .Lf4loop5
.Lendf4loop5:
                b .Lendf4loop2
                
.Lfoldby4sel:
.rept 31
                .word vfp_foldArrayBy4_ll31
.endr
                .word vfp_foldArrayBy4_lge31

.Lzeros3:
                .float 0.0,0.0
                /*****
                 * fold array by 5 
                 ******/
                .global vfp_foldArrayBy5_ll31
                .type vfp_foldArrayBy5_ll31, %function
vfp_foldArrayBy5_ll31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldrd r6,[r1,#16] // tmp2,tmp3
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                add r7,r0,r7,lsl #2
                ldrd r4,[r1]  // di,dest
                add r8,r0,r4,lsl #2 // end
                
                fldd d12,.Lzeros3
                sub r8,r8,#4*7
                cmp r0,r8
                bhs .Lendf5loop2
.Lf5loop2:
.macro FOLDBY5_CORE
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6,s7}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22,s23}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fldmias r6!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fadds s7,s7,s23
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fldmias r7!,{s16,s17,s18,s19,s20,s21,s22,s23}
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fadds s7,s7,s23
                
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6,s7}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcmpes s25,s7
                fcpysmi s24,s6
                fmstat
                fcpysmi s25,s7
.endm
                FOLDBY5_CORE
                
                cmp r0,r8
                blo .Lf5loop2
.Lendf5loop2:
                and r4,r4,#7
                cmp r4,#4
                blo .Lf5lt4
                beq .Lf5eq4

                cmp r4,#6
                blo .Lf5lt6                
                beq .Lf5eq6
                
                fldmias r0!,{s0,s1,s2,s3,s4,s5,s6}
                fldmias r2!,{s8,s9,s10,s11,s12,s13,s14}
                fldmias r3!,{s16,s17,s18,s19,s20,s21,s22}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fldmias r6!,{s16,s17,s18,s19,s20,s21,s22}
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fldmias r7!,{s16,s17,s18,s19,s20,s21,s22}
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fadds s4,s4,s20
                fadds s5,s5,s21
                fadds s6,s6,s22
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcpysmi s24,s6
                b .Lf5end
.Lf5eq6:
                fldmias r0!,{s0,s1,s2,s3,s4,s5}
                fldmias r2!,{s6,s7,s8,s9,s10,s11}
                fldmias r3!,{s12,s13,s14,s15,s16,s17}
                fldmias r6!,{s18,s19,s20,s21,s22,s23}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s3,s3,s9
                fadds s4,s4,s10
                fadds s5,s5,s11
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s4,s4,s16
                fadds s5,s5,s17
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fadds s5,s5,s23
                fldmias r7!,{s18,s19,s20,s21,s22,s23}
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fadds s5,s5,s23
                fstmias r5!,{s0,s1,s2,s3,s4,s5}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcpysmi s25,s5
                b .Lf5end
.Lf5lt6:
                fldmias r0!,{s0,s1,s2,s3,s4}
                fldmias r2!,{s6,s7,s8,s9,s10}
                fldmias r3!,{s12,s13,s14,s15,s16}
                fldmias r6!,{s18,s19,s20,s21,s22}
                
                fadds s0,s0,s6
                fadds s1,s1,s7
                fadds s2,s2,s8
                fadds s3,s3,s9
                fadds s4,s4,s10
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s4,s4,s16
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fldmias r7!,{s18,s19,s20,s21,s22}
                fadds s0,s0,s18
                fadds s1,s1,s19
                fadds s2,s2,s20
                fadds s3,s3,s21
                fadds s4,s4,s22
                fstmias r5!,{s0,s1,s2,s3,s4}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcpysmi s24,s4
                b .Lf5end
.Lf5eq4:
                fldmias r0!,{s0,s1,s2,s3}
                fldmias r2!,{s4,s5,s6,s7}
                fldmias r3!,{s8,s9,s10,s11}
                fldmias r6!,{s12,s13,s14,s15}
                fldmias r7!,{s16,s17,s18,s19}
                
                fadds s0,s0,s4
                fadds s1,s1,s5
                fadds s2,s2,s6
                fadds s3,s3,s7
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s3,s3,s15
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fadds s3,s3,s19
                fstmias r5!,{s0,s1,s2,s3}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcpysmi s25,s3
                b .Lf5end
.Lf5lt4:
                cmp r4,#2
                blo .Lf5lt2
                beq .Lf5eq2
                
                fldmias r0!,{s0,s1,s2}
                fldmias r2!,{s4,s5,s6}
                fldmias r3!,{s8,s9,s10}
                fldmias r6!,{s12,s13,s14}
                fldmias r7!,{s16,s17,s18}
                
                fadds s0,s0,s4
                fadds s1,s1,s5
                fadds s2,s2,s6
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s2,s2,s14
                fadds s0,s0,s16
                fadds s1,s1,s17
                fadds s2,s2,s18
                fstmias r5!,{s0,s1,s2}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcpysmi s24,s2
                b .Lf5end
.Lf5eq2:
                fldmias r0!,{s0,s1}
                fldmias r2!,{s4,s5}
                fldmias r3!,{s8,s9}
                fldmias r6!,{s12,s13}
                fldmias r7!,{s16,s17}
                
                fadds s0,s0,s4
                fadds s1,s1,s5
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s0,s0,s12
                fadds s1,s1,s13
                fadds s0,s0,s16
                fadds s1,s1,s17
                fstmias r5!,{s0,s1}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcpysmi s25,s1
                b .Lf5end
.Lf5lt2:
                cmp r4,#0
                beq .Lf5eq0
                fldmias r0!,{s0}
                fldmias r2!,{s1}
                fldmias r3!,{s2}
                fldmias r6!,{s3}
                fldmias r7!,{s4}
                
                fadds s0,s0,s1
                fadds s0,s0,s2
                fadds s0,s0,s3
                fadds s0,s0,s4
                fstmias r5!,{s0}
                
                fcmpes s24,s0
                fmstat
                fcpysmi s24,s0
.Lf5eq0:

.Lf5end:        
                fcmpes s24,s25
                fmstat
                fcpysmi s24,s25
                
                fmrs r0,s24
                vpop {d8,d9,d10,d11,d12}
                pop {r4,r5,r6,r7,r8,lr}
                bx lr

.Lzeros3_1:
                .float 0.0,0.0
                
                .global vfp_foldArrayBy5_lge31
                .type vfp_foldArrayBy5_lge31, %function
vfp_foldArrayBy5_lge31:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0] // ss0
                ldrd r2,[r1,#8] // tmp0
                ldrd r6,[r1,#16] // tmp2,tmp3
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                add r6,r0,r6,lsl #2
                add r7,r0,r7,lsl #2
                ldrd r4,[r1]  // di,dest
                add r8,r0,r4,lsl #2 // end
                
                fldd d12,.Lzeros3_1
                sub r8,r8,#4*31
                cmp r0,r8
                bhs .Lendf5loop4
.Lf5loop4:
                FOLDBY5_CORE
                FOLDBY5_CORE
                FOLDBY5_CORE
                FOLDBY5_CORE
                
                cmp r0,r8
                blo .Lf5loop4
.Lendf5loop4:
                add r8,r8,#4*24
                cmp r0,r8
                bhs .Lendf5loop2
.Lf5loop5:
                FOLDBY5_CORE
                
                cmp r0,r8
                blo .Lf5loop5
.Lendf5loop5:
                b .Lendf5loop2

.Lfoldby5sel:
.rept 31
                .word vfp_foldArrayBy5_ll31
.endr
                .word vfp_foldArrayBy5_lge31
                
.Lzeros4:
                .float 0.0,0.0
                /*****
                 * fold array by 2
                 ******/
                .global vfp_foldArrayBy2_ll31
                .type vfp_foldArrayBy2_ll31, %function
vfp_foldArrayBy2_ll31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0,#4] // ss1
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r2,r4,lsl #2 // end
                
                fldd d12,.Lzeros4
                sub r6,r6,#4*7
                cmp r2,r6
                bhs .Lendf2loop2
.Lf2loop2:
.macro FOLDBY2_CORE
                fldmias r2!,{s0,s1,s2,s3,s4,s5,s6,s7}
                fldmias r3!,{s8,s9,s10,s11,s12,s13,s14,s15}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fadds s7,s7,s15
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6,s7}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcmpes s25,s7
                fcpysmi s24,s6
                fmstat
                fcpysmi s25,s7
.endm
                FOLDBY2_CORE

                cmp r2,r6
                blo .Lf2loop2
.Lendf2loop2:
                and r4,r4,#7
                cmp r4,#4
                blo .Lf2lt4
                beq .Lf2eq4

                cmp r4,#6
                blo .Lf2lt6                
                beq .Lf2eq6
                
                fldmias r2!,{s0,s1,s2,s3,s4,s5,s6}
                fldmias r3!,{s8,s9,s10,s11,s12,s13,s14}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fadds s6,s6,s14
                fstmias r5!,{s0,s1,s2,s3,s4,s5,s6}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcmpes s24,s6
                fcpysmi s25,s5
                fmstat
                fcpysmi s24,s6
                b .Lf2end
.Lf2eq6:
                fldmias r2!,{s0,s1,s2,s3,s4,s5}
                fldmias r3!,{s8,s9,s10,s11,s12,s13}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                fadds s5,s5,s13
                fstmias r5!,{s0,s1,s2,s3,s4,s5}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcmpes s25,s5
                fcpysmi s24,s4
                fmstat
                fcpysmi s25,s5
                b .Lf2end
.Lf2lt6:
                fldmias r2!,{s0,s1,s2,s3,s4}
                fldmias r3!,{s8,s9,s10,s11,s12}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                fadds s4,s4,s12
                
                fstmias r5!,{s0,s1,s2,s3,s4}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcmpes s24,s4
                fcpysmi s25,s3
                fmstat
                fcpysmi s24,s4
                b .Lf2end
.Lf2eq4:
                fldmias r2!,{s0,s1,s2,s3}
                fldmias r3!,{s8,s9,s10,s11}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                fadds s3,s3,s11
                
                fstmias r5!,{s0,s1,s2,s3}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcmpes s25,s3
                fcpysmi s24,s2
                fmstat
                fcpysmi s25,s3
                b .Lf2end
.Lf2lt4:
                cmp r4,#2
                blo .Lf2lt2
                beq .Lf2eq2
                
                fldmias r2!,{s0,s1,s2}
                fldmias r3!,{s8,s9,s10}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                fadds s2,s2,s10
                
                fstmias r5!,{s0,s1,s2}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcmpes s24,s2
                fcpysmi s25,s1
                fmstat
                fcpysmi s24,s2
                b .Lf2end
.Lf2eq2:
                fldmias r2!,{s0,s1}
                fldmias r3!,{s8,s9}
                
                fadds s0,s0,s8
                fadds s1,s1,s9
                
                fstmias r5!,{s0,s1}
                
                fcmpes s24,s0
                fmstat
                fcmpes s25,s1
                fcpysmi s24,s0
                fmstat
                fcpysmi s25,s1
                b .Lf2end
.Lf2lt2:
                cmp r4,#0
                beq .Lf2eq0
                fldmias r2!,{s0}
                fldmias r3!,{s1}
                
                fadds s0,s0,s1
                fstmias r5!,{s0}
                
                fcmpes s24,s0
                fmstat
                fcpysmi s24,s0
.Lf2eq0:

.Lf2end:
                fcmpes s24,s25
                fmstat
                fcpysmi s24,s25
                
                fmrs r0,s24
                vpop {d8,d9,d10,d11,d12}
                pop {r4,r5,r6,lr}
                bx lr
                
                .global vfp_foldArrayBy2_lge31
                .type vfp_foldArrayBy2_lge31, %function
vfp_foldArrayBy2_lge31:
                push {r4,r5,r6,lr}
                vpush {d8,d9,d10,d11,d12}
                
                ldr r0,[r0,#4] // ss1
                ldrd r2,[r1,#8] // tmp0
                add r2,r0,r2,lsl #2
                add r3,r0,r3,lsl #2
                ldrd r4,[r1]  // di,dest
                add r6,r2,r4,lsl #2 // end
                
                fldd d12,.Lzeros4
                sub r6,r6,#4*31
                cmp r2,r6
                bhs .Lendf2loop4
.Lf2loop4:
                FOLDBY2_CORE
                FOLDBY2_CORE
                FOLDBY2_CORE
                FOLDBY2_CORE

                cmp r2,r6
                blo .Lf2loop4
.Lendf2loop4:
                add r6,r6,#4*24
                cmp r2,r6
                bhs .Lendf2loop2
.Lf2loop5:
                FOLDBY2_CORE
                
                cmp r2,r6
                blo .Lf2loop5
                b .Lendf2loop2
                
.Lfoldby2sel:
.rept 31
                .word vfp_foldArrayBy2_ll31
.endr
                .word vfp_foldArrayBy2_lge31
                
                .align 2
.Lname:
                .string "opt VFP"
                
                .align 2
                .global vfpFoldMain
vfpFoldMain:
                .word .Lfoldby3sel
                .word .Lfoldby4sel
                .word .Lfoldby5sel
                .word .Lfoldby2sel
                .word .Lfoldby2sel
                .word .Lname
                