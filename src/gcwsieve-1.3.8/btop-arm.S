/*
 * btop-arm.S
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
                .align  2
                .global btop_arm
                .type   btop_arm, %function
btop_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#48
                mov r4,r1
                mov r1,r3
                mov r3,r4
                strd r0,[sp,#32]
                str r2,[sp,#28]
                ldr r4,.LGOT1
.LPIC1:
                add r4,pc,r4
                ldr r5,.LGOT1+4
                ldr r4,[r4,r5]
                ldrd r6,[r4]            // save onep
                /*
                 * WARNING: this factor (adds r6,r6,#XX) is very important.
                 * shouldnt be lesser than 11. This factor amortizes partial results
                 */
                adds r6,r6,#14          // with factor
                adc r7,r7,#0
                strd r6,[sp]
                ldr r14,[r4,#24]        // clzp
                rsb r4,r14,#33
                mov r12,#1
                lsl r12,r12,r4            // mybit
                sub r14,r14,#2
                ldrd r4,[r3]            // load T[0].T
                add r3,r3,#16
                add r2,r3,r2,lsl #4
                ldrd r6,[sp,#88]            // p
                subs r4,r4,r6
                sbc r5,r5,r7            // newb-p
                subs r8,r4,r6
                sbcs r9,r5,r7           // newb-p-p
                movhs r4,r8
                movhs r5,r9
                teq r4,#2
                teqeq r5,#0
                bne .Lifnottwo
                /* for T[0]=2 */
.Lloop1:
                adds r4,r4,r4   // (b*2)%p
                adc r5,r5,r5
                subs r8,r4,r6   
                sbcs r9,r5,r7
                movhs r4,r8
                movhs r5,r9
                adds r8,r4,r6           // newb
                adc r9,r5,r7
                cmp r9,r12
                bhs .Lnoadd2
                adds r8,r8,r6
                adc r9,r9,r7
.Lnoadd2:
                strd r8,[r3]         // save newb
                rsb r11,r14,#32
                lsl r9,r9,r14            // bc=newb<<clzp
                orr r9,r9,r8,lsr r11
                lsl r8,r8,r14
                ldrd r6,[sp]            // onep+fac
                strd r4,[sp,#8]
                // mul bc by onep
                umull r4,r5,r8,r6
                umull r10,r11,r9,r6
                umull r0,r1,r8,r7
                umull r6,r7,r9,r7
                adds r10,r10,r5
                adcs r11,r11,#0
                adc r7,r7,#0
                adds r10,r10,r0
                adcs r11,r11,r1
                adc r7,r7,#0
                adds r6,r6,r11
                adc r7,r7,#0
                strd r6,[r3,#8]         // save T.bbyp
                ldrd r4,[sp,#8]
                ldrd r6,[sp,#88]        // restore p
                adds r3,r3,#16
                cmp r3,r2
                bne .Lloop1
                b .Lladder
                /* for T[0]!=2 */
.Lifnottwo:
                /*
                 * tmp = ((T[0].T<<(clzp-2))*T[i-1].bbyp)>>(clzp-4)
                 * tmp = resolv(T[0].T*T[i-1].T-tmp*p)
                 * T[i].T = correct(tmp)
                 * T[i].bbyp = (T[i].T<<(clzp-2))*onep
                 */
                ldrd r4,[r3,#-16]       // T[0].T
                strd r4,[sp,#8]
                ldrd r6,[r3,#-8]        // T[i-1].bbyp
                mov r8,r4               // T[i-1].T
                mov r9,r5
                str r12,[sp,#24]        // mybit
.Lloop2:
                umull r10,r11,r4,r8     // T[0].T*T[i-1].T
                mla r11,r4,r9,r11
                mla r11,r5,r8,r11
                rsb r12,r14,#32
                lsl r5,r5,r14           // T[0].T<<(clzp-2)
                orr r5,r5,r4,lsr r12
                lsl r4,r4,r14
                // (T[0].T<<(clzp-2))*T[i-1].bbyp
                umull r0,r1,r4,r7
                umull r8,r9,r5,r6
                umull r4,r5,r5,r7
                adds r0,r0,r8
                adcs r1,r1,r9
                adc r5,r5,#0
                adds r4,r4,r1
                adc r5,r5,#0
                // tmp>>(clzp-4)
                sub r14,r14,#2
                rsb r12,r14,#32
                lsr r4,r4,r14
                orr r4,r4,r5,lsl r12
                lsr r5,r5,r14
                ldrd r6,[sp,#88]        // p
                umull r0,r1,r4,r6       // tmp*p
                mla r1,r5,r6,r1
                mla r1,r4,r7,r1
                subs r8,r10,r0           // T[0].T*T[i-1].T-tmp*p
                sbcs r9,r11,r1
                // resolve
                bpl .Lnoadd3
                adds r8,r8,r6
                adc r9,r9,r7
.Lnoadd3:
                subs r10,r8,r6
                sbcs r11,r9,r7
                movpl r8,r10
                movpl r9,r11
                // correct
                adds r8,r8,r6
                adc r9,r9,r7
                ldr r12,[sp,#24]
                cmp r9,r12
                bhs .Lnoadd4
                adds r8,r6,r8
                adc r9,r7,r9
.Lnoadd4:
                add r14,r14,#2  // clzp-2
                strd r8,[r3]
                ldrd r10,[sp]   // onep
                // T[i].T<<(clzp-2)
                rsb r12,r14,#32
                lsl r7,r9,r14
                orr r7,r7,r8,lsr r12
                lsl r6,r8,r14
                // onep*bc
                umull r0,r1,r6,r10
                umull r4,r5,r7,r10
                umull r0,r12,r6,r11
                umull r6,r7,r7,r11
                adds r4,r4,r1
                adcs r5,r5,#0
                adc r7,r7,#0
                adds r4,r4,r0
                adcs r5,r5,r12
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0
                strd r6,[r3,#8]
                ldrd r4,[sp,#8]
                
                add r3,r3,#16
                cmp r3,r2
                bne .Lloop2
                ldr r12,[sp,#24]        // mybit
.Lladder:
                ldrd r0,[sp,#32]
                tst r1,r1
                beq .Lfinish
                ldr r2,[sp,#28]
                lsl r1,r1,#2
                add r1,r1,r0
                lsl r2,r2,#4
                /*
                 * tmp = ((T[L].T<<(clzp-2))*Ybbyp)>>(clzp-4)
                 * tmp = resolv(Y.T*T[L].T-tmp*p)
                 * T[L+lmin+1].T = correct(tmp)
                 * T[L+lmin+1].bbyp = (T[L].T<<(clzp-2))*onep
                 */
                str r3,[sp,#28]
                ldrd r4,[r3,#-16]         // T[lmin].T
                strd r4,[sp,#8]
                ldrd r6,[r3,#-8]          // T[lmin].bbyp
                strd r6,[sp,#16]
                sub r3,r3,r2    // pointer to T+1
                str r3,[sp,#40]
                str r12,[sp,#24]        // mybit
.Lloop3:
                ldr r10,[r0]
                add r12,r3,r10,lsl #4
                ldrd r8,[r12,#-16]         // T[L[j]]
                strd r0,[sp,#32]
                umull r10,r11,r4,r8     // T[L].T*T[lmin].T
                mla r11,r4,r9,r11
                mla r11,r5,r8,r11
                rsb r12,r14,#32
                lsl r5,r9,r14           // T[L].T<<(clzp-2)
                orr r5,r5,r8,lsr r12
                lsl r4,r8,r14
                // (T[L].T<<(clzp-2))*Ybbyp
                umull r0,r1,r4,r7
                umull r8,r9,r5,r6
                umull r4,r5,r5,r7
                adds r0,r0,r8
                adcs r1,r1,r9
                adc r5,r5,#0
                adds r4,r4,r1
                adc r5,r5,#0
                // tmp>>(clzp-4)
                sub r14,r14,#2
                rsb r12,r14,#32
                lsr r4,r4,r14
                orr r4,r4,r5,lsl r12
                lsr r5,r5,r14
                ldrd r6,[sp,#88]        // p
                umull r0,r1,r4,r6       // tmp*p
                mla r1,r5,r6,r1
                mla r1,r4,r7,r1
                subs r8,r10,r0           // T[0].T*T[i-1].T-tmp*p
                sbcs r9,r11,r1
                // resolve
                bpl .Lnoadd5
                adds r8,r8,r6
                adc r9,r9,r7
.Lnoadd5:
                subs r10,r8,r6
                sbcs r11,r9,r7
                movpl r8,r10
                movpl r9,r11
                // correct
                adds r8,r8,r6
                adc r9,r9,r7
                ldr r12,[sp,#24]
                cmp r9,r12
                bhs .Lnoadd6
                adds r8,r6,r8
                adc r9,r7,r9
.Lnoadd6:
                add r14,r14,#2  // clzp-2
                ldr r3,[sp,#28]        // T+lmin+1
                ldrd r0,[sp,#32]
                ldrd r10,[r0]
                add r12,r3,r10,lsl #4
                strd r8,[r12]
                ldrd r10,[sp]   // onep
                // T[i].T<<(clzp-2)
                rsb r4,r14,#32
                lsl r7,r9,r14
                orr r7,r7,r8,lsr r4
                lsl r6,r8,r14
                // onep*bc
                umull r8,r9,r6,r10
                umull r4,r5,r7,r10
                umull r8,r2,r6,r11
                umull r6,r7,r7,r11
                adds r4,r4,r9
                adcs r5,r5,#0
                adc r7,r7,#0
                adds r4,r4,r8
                adcs r5,r5,r2
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0
                strd r6,[r12,#8]
                ldrd r4,[sp,#8]
                ldrd r6,[sp,#16]
                ldr r3,[sp,#40]
                add r0,r0,#4
                cmp r0,r1
                bne .Lloop3
.Lfinish:
                add sp,sp,#48
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.Lend1:
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
                /* onep,
                 * ptimes4 - p*4
                 * pshifted - pshifted<<(clzp-2)
                 * clzp
                 * pmask
                 */