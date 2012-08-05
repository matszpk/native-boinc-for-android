/*
 * powseq-arm.S
 * Author: Mateusz Szpakowski
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
                .align 2
                .globl  powseq64_arm_shifted
                .type   powseq64_arm_shifted, %function
powseq64_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#32
                
                push {r0,r1,r2,r3}
                mov r0,r2
                mov r1,r3
                bl premulmod64_arm_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                ldr r4,.LGOT0
.LPIC0:
                add r4,pc,r4
                ldr r5,.LGOT0+4
                ldr r5,[r4,r5]
                
                ldrd r6,[r5,#48]        // pshifted_st
                ldrd r8,[r5,#56]        // bbyp2
                ldrd r10,[r5,#72]        // new b
                strd r6,[sp,#8]
                strd r8,[sp,#16]
                strd r10,[sp,#24]
                ldr r4,[r5,#64]        // pmask
                
                sub r8,r1,#1
                and r8,r8,#0xfffffffc
                add r9,r0,r1,lsl #3
                str r9,[sp,#4]
                
                add r1,r0,r8,lsl #3
                str r1,[sp]
                
                add r0,r0,#8
                ldrd r8,[sp,#8]          // pshifted
                ldrd r2,[r5,#40]        // ptimes4
                ldrd r6,[r0,#-8]           // load D64[i]
                
                cmp r0,r1
                bhs .Lloop1end
.Lloop1:
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r6,r7 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r10,r11 - bbyp
                 * r4 - and mask
                 * output -
                 * r6,r7 - shifted modulo
                 */
                /* iter 1 */
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd1l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l:
                strd r6,[r0]    // save to D64
                
                /* iter 2 */
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l:
                strd r6,[r0,#8]    // save to D64
                
                /* iter 3 */
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd3l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd3l:
                strd r6,[r0,#16]    // save to D64
                
                /* iter 4 */
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd4l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd4l:
                strd r6,[r0,#24]    // save to D64
                
                ldr r1,[sp]
                add r0,r0,#32
                cmp r0,r1       // cc
                blo .Lloop1
.Lloop1end:
                ldr r1,[sp,#4]
                cmp r0,r1
                bhs .Lloop2end
.Lloop2:
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd5l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd5l:
                strd r6,[r0]    // save to D64
                
                ldr r1,[sp,#4]
                add r0,r0,#8
                cmp r0,r1       // cc
                blo .Lloop2
.Lloop2end:
                add sp,sp,#32
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word mod64_init_data(GOT)
