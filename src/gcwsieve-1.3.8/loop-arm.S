/*
 * loop-arm.S
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
                .align  2
                .global swizzle_loop2_arm
                .type   swizzle_loop2_arm, %function
swizzle_loop2_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,#48
                ldr r4,.LGOT1
.LPIC1:
                add r4,pc,r4
                ldr r5,.LGOT1+4
                ldr r6,[r4,r5]          // mod64_init_data
                ldrd r4,[r6,#16]        // p<<clzb
                ldrd r8,[r6,#8]         // p<<2
                ldr r10,[r6,#28]
                strd r4,[sp]
                strd r8,[sp,#8]
                str r10,[sp,#40]
                ldrd r8,[r1]            // D->X[0]
                ldrd r10,[r1,#8]            // D->X[1]
                adds r8,r8,r4             // X[0]+p
                adc r9,r9,r5
                adds r10,r10,r4             // X[1]+p
                adc r11,r11,r5
                strd r8,[sp,#16]
                strd r10,[sp,#24]
                add r1,r1,#16            // to D->R
                add r2,r2,lsl #1
                add r2,r1,r2,lsl #3            // to D->R[i]
                str r1,[sp,#36]
                sub r2,r2,#96
                cmp r2,r1
                blo .Ltoloop2           // if i<4
.Lloop1:
                pld [r2,#-256]
                /* MainCore:
                 * r0 - D->R+i
                 * r1 - T
                 * r2 - D->R+0
                 * input:
                 * r6,r7 - new A (shifted)
                 * r8,r9 - new B; // can be modified
                 * r4,r5 - p<<2
                 * r10,r11 - bbyp
                 * //r12 - and mask//
                 * output -
                 * r6,r7 - shifted modulo
                 */
                ldrd r6,[sp,#16]            // D->X[0]
                
                /* first iteration */
                ldr r14,[r2,#16+72]        // D->G[i][0]
                ldrd r8,[r14]            // T[D->G[i][0]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][0]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r1,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#72]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd1_1l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1_1l:
                // X0 == D->R[i].N[0]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak1_1
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* second iteration */
                ldr r14,[r2,#16+48]        // D->G[i][0]
                ldrd r8,[r14]            // T[D->G[i][0]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][0]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r1,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#48]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd1_2l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1_2l:
                // X0 == D->R[i].N[0]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak1_2
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* third iteration */
                ldr r14,[r2,#16+24]        // D->G[i][0]
                ldrd r8,[r14]            // T[D->G[i][0]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][0]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r1,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#24]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd1_3l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1_3l:
                // X0 == D->R[i].N[0]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak1_3
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* fourth iteration */
                ldr r14,[r2,#16]        // D->G[i][0]
                ldrd r8,[r14]            // T[D->G[i][0]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][0]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r1,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd1_4l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1_4l:
                // X0 == D->R[i].N[0]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak1_4
                adds r6,r6,r8
                adc r7,r7,r9
                
                strd r6,[sp,#16]    // save X0
                /*
                 * SECOND SWIZZLE
                 */
                ldrd r6,[sp,#24]            // D->X[1]
                /* first iteration */
                ldr r14,[r2,#20+72]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8+72]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd2_1l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2_1l:
                // X1 == D->R[i].N[1]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak2_1
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* second iteration */
                ldr r14,[r2,#20+48]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8+48]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd2_2l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2_2l:
                // X1 == D->R[i].N[1]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak2_2
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* third iteration */
                ldr r14,[r2,#20+24]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8+24]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd2_3l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2_3l:
                // X1 == D->R[i].N[1]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak2_3
                adds r6,r6,r8
                adc r7,r7,r9
                
                /* fourth iteration */
                ldr r14,[r2,#20]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd2_4l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2_4l:
                // X1 == D->R[i].N[1]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak2_4
                adds r6,r6,r8
                adc r7,r7,r9
                
                strd r6,[sp,#24]    // save X1
                
                // decrease pointer
                ldr r1,[sp,#36]
                sub r2,r2,#96
                cmp r2,r1
                bhs .Lloop1
                b .Ltoloop2
                
                /* undoing loop iters */
.Lbreak1_1:
                mov r3,#1
                b .Ltoundo1
.Lbreak1_2:
                mov r3,#2
                b .Ltoundo1
.Lbreak1_3:
                mov r3,#3
                b .Ltoundo1
.Lbreak1_4:
                mov r3,#4
.Ltoundo1:
                str r2,[sp,#32]
                sub sp,sp,#8
                mov r0,r6
                mov r1,r7       // X0
                ldrd r4,[sp,#96]        // orig p
                strd r4,[sp]
                bl revert_swizzle_loop_0(PLT)
                add sp,sp,#8
                strd r0,[sp,#16]        // save X0
                ldr r2,[sp,#32]
                ldr r1,[sp,#36]
                b .Ltoloop2

.Lbreak2_1:
                mov r3,#1
                b .Ltoundo2
.Lbreak2_2:
                mov r3,#2
                b .Ltoundo2
.Lbreak2_3:
                mov r3,#3
                b .Ltoundo2
.Lbreak2_4:
                mov r3,#4
.Ltoundo2:
                str r2,[sp,#32]
                sub sp,sp,#8
                mov r0,r6
                mov r1,r7       // X1
                ldrd r4,[sp,#96]        // orig p
                strd r4,[sp]
                bl revert_swizzle_loop_1(PLT)
                strd r0,[sp,#32]        // save X1
                
                ldrd r0,[sp,#24]        // load X0
                mov r3,#4
                ldr r2,[sp,#40]         // LR
                ldrd r4,[sp,#96]        // orig p
                strd r4,[sp]
                bl revert_swizzle_loop_0(PLT)
                strd r0,[sp,#24]        // save X1
                add sp,sp,#8
                ldr r2,[sp,#32]
                ldr r1,[sp,#36]
                b .Ltoloop2

                /*
                 * single iter loop
                 */
.Ltoloop2:
                add r2,r2,#72
                cmp r2,r1
                blo .Lnoloop2
.Lloop2:
                ldrd r6,[sp,#16]            // D->X[0]
                ldr r14,[r2,#16]        // D->G[i][0]
                ldrd r8,[r14]            // T[D->G[i][0]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][0]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r1,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd1l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l:
                // X0 == D->R[i].N[0]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak1
                adds r6,r6,r8
                adc r7,r7,r9
                strd r6,[sp,#16]    // save X0
                /*
                 * SECOND SWIZZLE
                 */
                ldrd r6,[sp,#24]            // D->X[1]
                ldr r14,[r2,#20]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd2l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l:
                // X1 == D->R[i].N[1]
                teq r6,r10
                teqeq r7,r11
                beq .Lbreak2
                adds r6,r6,r8
                adc r7,r7,r9
                strd r6,[sp,#24]    // save X1
                
                // decrease pointer
                ldr r1,[sp,#36]
                sub r2,r2,#24
                cmp r2,r1
                bhs .Lloop2
.Lnoloop2:
                ldrd r4,[sp,#16]
                ldrd r6,[sp,#24]
                subs r4,r4,r8           // X[0]-p
                sbc r5,r5,r9
                subs r6,r6,r8           // X[1]-p
                sbc r7,r7,r9
                b .Laftercorrect
.Lbreak1:
                strd r6,[sp,#16]    // save X0
                // compute second swizzle
                ldrd r6,[sp,#24]            // D->X[1]
                ldr r14,[r2,#20]        // D->G[i][1]
                ldrd r8,[r14]            // T[D->G[i][1]].T (newb)
                ldrd r10,[r14,#8]        // T[D->G[i][1]].bbyp
                ldrd r4,[sp,#8]         // p<<2
                // main op
                umull r1,r3,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r3,r3,r14
                adc r11,r11,#0
                adds r10,r10,r3
                adc r11,r11,#0            // result in r10,r11
                ldr r12,[sp,#40]
                and r10,r10,r12            // and masking
                umull r1,r3,r6,r8        // newa*newb
                mla r3,r7,r8,r3
                mla r3,r6,r9,r3       // result in r2,r3
                umull r6,r7,r4,r10       // tmp*p
                mla r7,r4,r11,r7
                mla r7,r5,r10,r7         // result in r6,r7
                ldrd r8,[sp]            // pshifted
                ldrd r10,[r2,#8]
                subs r6,r1,r6
                sbcs r7,r3,r7           // newa*newb-tmp*p
                bpl .Lnoadd3
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd3:
                ldr r1,[sp,#36]
                ldrd r4,[sp,#16]        // X0
                b .Laftercorrect
.Lbreak2:
                ldr r1,[sp,#36]
                ldrd r4,[sp,#16]        // X0+p
                subs r4,r4,r8           // (X0+p)-p
                sbc r5,r5,r9
.Laftercorrect:
                strd r4,[r1,#-16]         // D->X[0]
                strd r6,[r1,#-8]        // D->X[1]
                
                // compute i
                sub r0,r2,r1
                ldr r3,one_by_3
                smmulr r0,r0,r3          // (I/3)
                asr r0,r0,#3
                add sp,#48
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
.Lend1:
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
one_by_3:
                .word 1431655765
