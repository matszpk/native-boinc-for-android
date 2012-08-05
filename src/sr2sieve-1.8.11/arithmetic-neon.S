/*
 * arithmetic-neon.S
 * Mateusz Szpakowski
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
                .global mod64_neon_init
                .type   mod64_neon_init, %function
mod64_neon_init:
                push {r4,lr}
                // execute arm init
                bl mod64_arm_init(PLT)
                pop {r4,lr}
                // copy some data to mod64_neon_init_data
                push {r4,r5}
                ldr r2,.LGOT1
.LPIC1:
                add r2,pc,r2
                ldr r3,.LGOT1+4     //
                ldr r4,.LGOT1+8
                ldr r3,[r3,r2]
                ldr r4,[r4,r2]
                ldrd r0,[r4,#40]    // ptimes4
                str r0,[r3]
                str r0,[r3,#4]
                str r1,[r3,#8]
                str r1,[r3,#12]
                ldrd r0,[r4,#48]
                strd r0,[r3,#16]    // pshifted_1/2
                strd r0,[r3,#24]
                ldr r0,[r4,#64]     // pmask
                mov r1,r0
                strd r0,[r3,#32]
                strd r0,[r3,#40]
                
                pop {r4,r5}
                bx lr
.Lend1:
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_neon_init_data(GOT)
                .word mod64_init_data(GOT)
                
                .comm mod64_neon_init_data, 80

                
                .align  2
                .global premulmod64_neon_init_shifted
                .type   premulmod64_neon_init_shifted, %function
premulmod64_neon_init_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                // TODO: must be TESTED
                ldr r4,.LGOT2
.LPIC2:
                add r4,pc,r4
                ldr r5,.LGOT2+4         // mod64_init_data
                ldr r6,[r4,r5]
                ldr r12,.LGOT2+8         // mod64_init_neon_data
                ldr lr,[r4,r12]
                //ldrd r4,[r0]
                ldrd r2,[r6,#48]        // pshifted
                adds r4,r0,r2           // newb=b+p
                adc r5,r1,r3
                ldr r1,[r6,#32]         // clzp
                sub r1,r1,#2            // clzb
                tst r5,#0x80000000      // mybit>=newb
                bmi .Lnoadd2
                adds r4,r4,r2           // newb+=p
                adc r5,r5,r3
.Lnoadd2:
                rsb r7,r1,#32
                lsr r8,r4,r1            // newb_shift>>clzb
                orr r8,r8,r5,lsl r7
                lsr r9,r5,r1
                str r8,[lr,#48]
                str r8,[lr,#52]
                str r9,[lr,#56]
                str r9,[lr,#60]
                ldrd r2,[r6]            // onep
                adds r2,r2,#14          // add factor
                adc r3,r3,#0
                // onep*bc
                umull r1,r7,r2,r4
                umull r8,r9,r3,r4
                umull r10,r11,r2,r5
                umull r2,r3,r3,r5
                adds r8,r8,r7
                adcs r9,r9,#0
                adc r3,r3,#0
                adds r8,r8,r10
                adcs r9,r9,r11
                adc r3,r3,#0
                adds r2,r2,r9
                adc r3,r3,#0
                // special optimization for NEON
                // this speedup multiply (high 64bits) of two 64bit values
                lsr r2,r2,#1
                //PRINT64 r2,r3
                str r2,[lr,#64]
                str r2,[lr,#68]
                str r3,[lr,#72]
                str r3,[lr,#76]                
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.Lend2:
                .align  2
.LGOT2:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC2+8)
                .word mod64_init_data(GOT)
                .word mod64_neon_init_data(GOT)
                
                
                /* version for unshifted b */
                .align  2
                .global premulmod64_neon_init_shifted_2
                .type   premulmod64_neon_init_shifted_2, %function
premulmod64_neon_init_shifted_2:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                ldr r4,.LGOT3
.LPIC3:
                add r4,pc,r4
                ldr r5,.LGOT3+4         // mod64_init_data
                ldr r6,[r4,r5]
                ldr r12,.LGOT3+8         // mod64_init_neon_data
                ldr lr,[r4,r12]
                //ldrd r4,[r0]
                ldrd r2,[r6,#16]        // p
                //PRINT64 r0,r1
                adds r4,r0,r2           // newb=b+p
                adc r5,r1,r3
                ldr r1,[r6,#32]         // clzp
                rsb r8,r1,#33
                mov r7,#1
                lsl r7,r7,r8            // mybit
                cmp r5,r7               // mybit>=newb
                bhs .Lnoadd3
                adds r4,r4,r2           // newb+=p
                adc r5,r5,r3
.Lnoadd3:
                //PRINT64 r4,r5
                //strd r4,[lr,#48]
                str r4,[lr,#48]
                str r4,[lr,#52]
                str r5,[lr,#56]
                str r5,[lr,#60]
                sub r1,r1,#2
                rsb r7,r1,#32
                lsl r5,r5,r1            // bc=newb<<clzb
                orr r5,r4,lsr r7
                lsl r4,r4,r1
                ldrd r2,[r6]            // onep
                adds r2,r2,#14          // add factor
                adc r3,r3,#0
                // onep*bc
                umull r1,r7,r2,r4
                umull r8,r9,r3,r4
                umull r10,r11,r2,r5
                umull r2,r3,r3,r5
                adds r8,r8,r7
                adcs r9,r9,#0
                adc r3,r3,#0
                adds r8,r8,r10
                adcs r9,r9,r11
                adc r3,r3,#0
                adds r2,r2,r9
                adc r3,r3,#0
                // special optimization for NEON
                // this speedup multiply (high 64bits) of two 64bit values
                lsr r2,r2,#1
                //PRINT64 r2,r3
                str r2,[lr,#64]
                str r2,[lr,#68]
                str r3,[lr,#72]
                str r3,[lr,#76]
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.Lend3:
                .align  2
.LGOT3:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC3+8)
                .word mod64_init_data(GOT)
                .word mod64_neon_init_data(GOT)
                