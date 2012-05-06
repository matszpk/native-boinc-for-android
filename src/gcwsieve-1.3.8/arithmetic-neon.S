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
                ldrd r0,[r4,#8]    // ptimes4
                str r0,[r3]
                str r0,[r3,#4]
                str r1,[r3,#8]
                str r1,[r3,#12]
                ldrd r0,[r4,#16]
                strd r0,[r3,#16]    // pshifted_1/2
                strd r0,[r3,#24]
                ldr r0,[r4,#28]     // pmask
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
                
                .align 4
                .comm mod64_neon_init_data, 48

                
                .align  2
                .global premod64_neon_init
                .type   premod64_neon_init, %function
premod64_neon_init:
                push {r4,r5,r6,r7,r8,r9,r10,r11}
                ldr r4,.LGOT2
.LPIC2:
                add r4,pc,r4
                ldr r5,.LGOT2+4
                ldr r1,[r4,r5]
                ldrd r4,[r0]
                adds r4,r4,r2           // newb=b+p
                adc r5,r5,r3
                ldr r6,[r1,#24]
                rsb r8,r6,#33
                mov r7,#1
                lsl r7,r7,r8            // mybit
                cmp r5,r7               // mybit>=newb
                bhs .Lnoadd2
                adds r4,r4,r2           // newb+=p
                adc r5,r5,r3
.Lnoadd2:
                strd r4,[r0]
                sub r6,r6,#2
                rsb r7,r6,#32
                lsl r5,r5,r6            // bc=newb<<clzb
                orr r5,r4,lsr r7
                lsl r4,r4,r6
                ldrd r2,[r1]            // onep
                adds r2,r2,#14          // add factor
                adc r3,r3,#0
                // onep*bc
                umull r6,r7,r2,r4
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
                strd r2,[r0,#8]
                pop {r4,r5,r6,r7,r8,r9,r10,r11}
                bx lr
.Lend2:
                .align  2
.LGOT2:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC2+8)
                .word mod64_init_data(GOT)
                /* onep,
                 * ptimes4 - p*4
                 * pshifted - pshifted<<(clzp-2)
                 * clzp
                 * pmask
                 */