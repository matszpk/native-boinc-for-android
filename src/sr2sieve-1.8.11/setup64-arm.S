/*
 * setup64-arm.S
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

#define onep_idx 0
#define bbyp_idx 8
#define p_idx 16
#define b_idx 24
#define clzp_idx 32
#define clzbbyp_idx 36
#define ptimes4_idx 40
#define pshifted_idx 48
#define bbyp2_idx 56
#define pmask_idx 64
#define b2_idx 72

#define POWER_RESIDUE_LCM 720
                
                .text
#define seq_t_size 24

                /**
                 * setup1_mulmod_arm_shifted
                 */
                .align 2
                .global setup1_mulmod_arm_shifted
                .type   setup1_mulmod_arm_shifted, %function
setup1_mulmod_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#48
                
                str r2,[sp,#40]
                /* r0 - i
                 * r1 - C32
                 * r2 - D64
                 */
                ldr r4,.LGOT1
.LPIC1:
                add r4,pc,r4
                ldr r5,.LGOT1+8
                ldr r6,.LGOT1+12
                ldr r7,.LGOT1+16
                ldr r8,[r4,r5]
                ldr r9,[r4,r6]
                ldr r10,[r4,r7]
                ldr r8,[r8]            // SEQ
                ldr r9,[r9]            // subseq_d
                ldr r12,[r10]            // BJ64
                
                ldr r11,.Lpowreslcm
                add r12,r12,r11,lsl #3
                add r8,r8,r0,lsl #4
                add r8,r8,r0,lsl #3     // SEQ+f
                
                ldrd r6,[r8]        // SEQ.k/c
                tst r7,r7
                bmi .Lnorev1
                ldrd r10,[sp,#40+40]    // p
                subs r10,r10,r6
                sbc r11,r11,#0
                b .Lafternegck_1
.Lnorev1:
                mov r10,r6
                mov r11,#0
.Lafternegck_1:
                push {r0,r1,r2,r3}
                mov r0,r10
                mov r1,r11
                bl premulmod64_arm_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                ldr r5,.LGOT1+4
                ldr r6,[r4,r5]          // mod64_init_data
                ldrd r10,[r6,#bbyp2_idx]
                strd r10,[sp,#8]         // bbyp2
                ldrd r10,[r6,#b2_idx]
                strd r10,[sp,#16]         // b2
                ldrd r10,[r6,#pshifted_idx]
                strd r10,[sp,#24]         // pshifted
                ldrd r10,[r6,#ptimes4_idx]
                strd r10,[sp,#32]         // ptimes4
                ldr r10,[r6,#pmask_idx]
                str r10,[sp,#44]         // pmask
                
                // r9 - subseq_d
                mov lr,r2      // old D64
                ldrd r4,[r8,#16]        // SEQ.first/last
                add r5,r9,r5,lsl #1
                add r9,r9,r4,lsl #1
                
                cmp r9,r5
                bhi .Lloopend1_1
.Lloop1_1:
                str r4,[r1]
                ldrh r6,[r9]   // subseq_d[h]
                add r7,r12,r6,lsl #3
                ldrd r10,[r7]
                strd r10,[r2]
                add r4,r4,#1
                
                add r2,r2,#8
                add r1,r1,#4
                add r9,r9,#2
                cmp r9,r5
                bls .Lloop1_1
.Lloopend1_1:
                str r2,[sp,#4]
                sub r3,r2,lr
                bic r4,r3,#0x8
                add r6,r4,lr
                str r6,[sp]
                mov r0,lr
                
                ldrd r2,[sp,#32]        // ptimes4
                ldr r4,[sp,#44]
                ldrd r8,[sp,#24]          // pshifted
                cmp r0,r6
                beq .Lloop2end_1
.Lloop2_1:
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
                ldrd r6,[r0]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd1l_1
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l_1:
                strd r6,[r0]    // save to D64
                
                /* iter 2 */
                ldrd r6,[r0,#8]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2l_1
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l_1:
                strd r6,[r0,#8]    // save to D64
                
                add r0,r0,#16
                ldr r1,[sp]
                cmp r0,r1
                bne .Lloop2_1
.Lloop2end_1:
                // last iteration
                ldr r1,[sp,#4]
                cmp r0,r1
                beq .Lnolast_1
                
                // last iter
                ldrd r6,[r0]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd3l_1
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd3l_1:
                strd r6,[r0]    // save to D64
                add r0,r0,#8
.Lnolast_1:
                ldr r3,[sp,#40]
                sub r0,r0,r3
                lsr r0,r0,#3    // result j
                
                add sp,sp,#48
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
                .word SEQ(GOT)
                .word subseq_d(GOT)
                .word BJ64(GOT)

                /**
                 * setup2_mulmod_arm_shifted
                 */
                
                .global setup2_mulmod_arm_shifted
                .type   setup2_mulmod_arm_shifted, %function
setup2_mulmod_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#40
                
                push {r0,r1,r2,r3}
                bl premulmod64_arm_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                ldr r4,.LGOT2
.LPIC2:
                add r4,pc,r4
                ldr r5,.LGOT2+4
                ldr r5,[r4,r5]
                
                ldrd r6,[r5,#48]        // pshifted_st
                strd r6,[sp,#8]
                ldrd r6,[r5,#56]        // bbyp2
                strd r6,[sp,#16]
                ldrd r6,[r5,#72]        // new b
                strd r6,[sp,#24]
                
                ldr r6,.LGOT2+8         // BJ64
                ldr r6,[r4,r6]
                ldr r2,[r6]
                str r2,[sp,#32]
                
                ldr r4,[r5,#32]        // clzp
                sub r4,r4,#2
                rsb r10,r4,#32
                mov r6,#1
                lsl r6,r6,r4
                mov r7,#0
                str r6,[sp]             // shifted one
                strd r6,[r2]
                
                // shift powval
                lsl r1,r1,r4
                orr r1,r1,r0,lsr r10
                lsl r0,r0,r4
                strd r0,[r2,#8]         // BJ64[1]
                
                add r2,r2,#8
                
                cmp r0,r6
                cmpeq r1,#0
                beq .Lloop1end_2
                
                ldr r4,[r5,#64]        // pmask
                ldrd r8,[sp,#8]          // pshifted
                ldrd r0,[r5,#40]        // ptimes4
                ldrd r6,[r2]           // load D64[i]
                add r2,r2,#8
.Lloop1_2:
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r0,r1 - p<<2
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
                umull r3,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r3,r3,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r3,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r3,r5
                umull r6,r7,r0,r10       // tmp*p
                mla r7,r0,r11,r7
                mla r7,r1,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r3,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd1l_2
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l_2:
                strd r6,[r2]    // save to D64
                ldr r14,[sp]    // shifted one
                cmp r6,r14
                cmpeq r7,#0
                beq .Lloop1end_2
                
                /* iter 2 */
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#16]        // bbyp2
                ldrd r8,[sp,#24]            // b
                // main op
                umull r3,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r3,r3,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r4           // and masking
                umull r3,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r3,r5
                umull r6,r7,r0,r10       // tmp*p
                mla r7,r0,r11,r7
                mla r7,r1,r10,r7         // result in r6,r7
                ldrd r8,[sp,#8]          // pshifted
                subs r6,r3,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2l_2
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l_2:
                strd r6,[r2,#8]    // save to D64
                add r2,r2,#16
                ldr r14,[sp]    // shifted one
                cmp r6,r14
                cmpeq r7,#0
                bne .Lloop1_2
                // fix for counter
                sub r2,r2,#8
.Lloop1end_2:
                ldr r3,[sp,#32]
                sub r0,r2,r3
                lsr r0,r0,#3

                add sp,sp,#40
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT2:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC2+8)
                .word mod64_init_data(GOT)
                .word BJ64(GOT)
                
.Lpowreslcm:
                .word ((POWER_RESIDUE_LCM+1)|7)
                /**
                 * setup3_mulmod_arm_shifted
                 */
                .global setup3_mulmod_arm_shifted
                .type   setup3_mulmod_arm_shifted, %function
setup3_mulmod_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#48
                
                str r3,[sp,#40]
                /* r0 - f
                 * r1 - list
                 * r2 - C32
                 * r3 - D64
                 */
                ldr r4,.LGOT3
.LPIC3:
                add r4,pc,r4
                ldr r5,.LGOT3+8
                ldr r6,.LGOT3+12
                ldr r7,.LGOT3+16
                ldr r8,[r4,r5]
                ldr r9,[r4,r6]
                ldr r10,[r4,r7]
                ldr r8,[r8]            // SEQ
                ldr r9,[r9]            // subseq_d
                ldr r12,[r10]            // BJ64
                ldr r11,.Lpowreslcm
                add r12,r12,r11,lsl #3
                add r8,r8,r0,lsl #4
                add r8,r8,r0,lsl #3     // SEQ+f
                
                ldrd r6,[r8]        // SEQ.k/c
                tst r7,r7
                bmi .Lnorev3
                ldrd r10,[sp,#40+40]    // p
                subs r10,r10,r6
                sbc r11,r11,#0
                b .Lafternegck_3
.Lnorev3:
                mov r10,r6
                mov r11,#0
.Lafternegck_3:
                push {r0,r1,r2,r3}
                mov r0,r10
                mov r1,r11
                bl premulmod64_arm_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                ldr r5,.LGOT3+4
                ldr r6,[r4,r5]          // mod64_init_data
                ldrd r10,[r6,#bbyp2_idx]
                strd r10,[sp,#8]         // bbyp2
                ldrd r10,[r6,#b2_idx]
                strd r10,[sp,#16]         // b2
                ldrd r10,[r6,#pshifted_idx]
                strd r10,[sp,#24]         // pshifted
                ldrd r10,[r6,#ptimes4_idx]
                strd r10,[sp,#32]         // ptimes4
                ldr r10,[r6,#pmask_idx]
                str r10,[sp,#44]         // pmask
                
                // r9 - subseq_d
                mov lr,r3       // old D64
.Lloop1_3:
                ldr r4,[r1]
                adds r5,r4,#1
                beq .Lloopend1_3
                str r4,[r2]
                add r10,r9,r4,lsl #1
                ldrh r6,[r10]   // subseq_d[h]
                add r7,r12,r6,lsl #3
                ldrd r10,[r7]
                strd r10,[r3]
                
                add r3,r3,#8
                
                ldr r4,[r1,#4]
                adds r5,r4,#1
                beq .Lloopend1_3
                str r4,[r2,#4]
                add r10,r9,r4,lsl #1
                ldrh r6,[r10]   // subseq_d[h]
                add r7,r12,r6,lsl #3
                ldrd r10,[r7]
                strd r10,[r3]
                
                add r2,r2,#8
                add r1,r1,#8
                add r3,r3,#8
                b .Lloop1_3
.Lloopend1_3:
                str r3,[sp,#4]
                sub r5,r3,lr
                bic r5,r5,#0x8
                add r6,lr,r5
                str r6,[sp]
                mov r0,lr
                
                ldrd r2,[sp,#32]        // ptimes4
                ldr r4,[sp,#44]
                ldrd r8,[sp,#24]          // pshifted
                
                cmp r0,r6
                beq .Lloop2end_3
.Lloop2_3:
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
                ldrd r6,[r0]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd1l_3
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l_3:
                strd r6,[r0]    // save to D64
                
                /* iter 2 */
                ldrd r6,[r0,#8]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2l_3
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l_3:
                strd r6,[r0,#8]    // save to D64
                
                add r0,r0,#16
                ldr r1,[sp]
                cmp r0,r1
                bne .Lloop2_3
.Lloop2end_3:                
                // last iteration
                ldr r1,[sp,#4]
                cmp r0,r1
                beq .Lnolast_3
                
                // last iter
                ldrd r6,[r0]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#8]        // bbyp2
                ldrd r8,[sp,#16]            // b
                
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
                ldrd r8,[sp,#24]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd3l_3
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd3l_3:
                strd r6,[r0]    // save to D64
                add r0,r0,#8
.Lnolast_3:
                ldr r3,[sp,#40]
                sub r0,r0,r3
                lsr r0,r0,#3    // result j
                
                add sp,sp,#48
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT3:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC3+8)
                .word mod64_init_data(GOT)
                .word SEQ(GOT)
                .word subseq_d(GOT)
                .word BJ64(GOT)
