/*
 * powmod-neon.S
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
                
#define pshifted_st 0
#define onep_st 8
#define pptimes41_st 16
#define ppshifted1_st 20
#define clza_st 24
#define ppmask1_st 28
#define newa_abyp_st 32
// shifted half of n and mask
#define nshift_st 128
#define nlpart_st 136
#define npartn_st 140
#define vector_st 144
#define n_st 152


#ifndef SKIP_VFP_SAVE
#define st_vfp_size 64
#else
#define st_vfp_size 0
#endif

                .text
                .align 2
                .globl  vec_powmod64_neon_shifted
                .type   vec_powmod64_neon_shifted, %function
vec_powmod64_neon_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                sub sp,sp,#160
                
                add r1,r0,r1,lsl #3
                strd r0,[sp,#vector_st]
                strd r2,[sp,#n_st]

                ldr r10,.LGOT1
.LPIC1:
                add r10,pc,r10
                ldr r9,.LGOT1+4
                ldr r8,[r10,r9]          // mod64_init_data
                
                ldr r6,[r8,#clzp_idx]
                sub r6,r6,#2
                str r6,[sp,#clza_st]
                
                ldrd r4,[r8,#pshifted_idx]
                strd r4,[sp,#pshifted_st]
                ldrd r4,[r8,#onep_idx]
                adds r4,r4,#14
                adc r5,r5,#0
                strd r4,[sp,#onep_st]
                
                ldr r9,.LGOT1+8
                ldr r8,[r10,r9]          // mod64_neon_init_data
                add r9,r8,#16
                add r12,r8,#32          // pmask
                strd r8,[sp,#pptimes41_st]
                
                mov r4,r0
                mov r5,r1
                cmp r4,r5
                beq .Lendvecloop
.Lvecloop:
                ldrd r2,[sp,#n_st]
                //ldrd r0,[r4]
                vldmia.i32 r4,{q12,q13,q14}
                
                cmp r3,#0
                bhi .Lnoend2
                cmpeq r2,#1
                bhi .Lnoend2 // if end
                beq .Lend2 // if end
                // set to one (n==0)
                mov r0,#1
                lsl r0,r0,r6
                mov r1,#0
                vmov d24,r0,r1
                vmov d25,d24
                vmov q13,q12
                vmov q14,q12
                bls .Lend2
.Lnoend2:
                vmov r0,r1,d24
                
                tst r3,r3               // clz(b)
                beq .Lonlylowpart2
                clz r4,r3
                teq r3,#1
                bne .Lnolowlast2
                mov r6,#0x80000000
                b .Lsavenparts2
.Lnolowlast2:
                rsb r4,r4,#30
                mov r6,#1
                str r6,[sp,#npartn_st]
                lsl r6,r6,r4
                
                str r3,[sp,#nshift_st+4]        // n
                str r6,[sp,#nshift_st]
                str r2,[sp,#nlpart_st]
                b .Lskip3
.Lonlylowpart2:
                clz r4,r2
                rsb r4,r4,#30
                mov r6,#1
                lsl r6,r6,r4
.Lsavenparts2:
                mov r3,#0
                str r2,[sp,#nshift_st+4]        // n
                str r6,[sp,#nshift_st]
                str r3,[sp,#nlpart_st]
                str r3,[sp,#npartn_st]
.Lskip3:
                /*
                 *first iteration
                 */
                // first part square mod
                
                ldrd r2,[sp,#pshifted_st]       // pshifted
                ldrd r10,[sp,#onep_st]       // onep+fac
.macro PARTSQRMOD1 J,DT,D0,D1
                //vmov r0,r1,\DT
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd1_\J
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd1_\J:
                vmov r0,r1,\DT
                mov r7,#0
                umull lr,r4,r8,r10
                umull r5,r6,r8,r11
                adds r5,r5,r4
                umull lr,r4,r9,r10
                adcs r6,r6,#0
                adc r7,r7,#0
                adds r5,r5,lr
                adcs r6,r6,r4
                
                ldr r4,[sp,#clza_st]         // clza
                adc r7,r7,#0
                rsb r5,r4,#32
                umlal r6,r7,r9,r11
                lsr r8,r8,r4
                orr r8,r8,r9,lsl r5
                lsr r9,r9,r4
                lsr r6,r6,#1    // high trick
                vmov \D0,r8,r9   // newa
                vmov \D1,r6,r7   // abyp
.endm
                PARTSQRMOD1 0,d25,d6,d7
                PARTSQRMOD1 1,d26,d8,d9
                PARTSQRMOD1 2,d27,d10,d11
                PARTSQRMOD1 3,d28,d12,d13
                PARTSQRMOD1 4,d29,d14,d15
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd1_5
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd1_5:
                mov r7,#0
                umull lr,r4,r8,r10
                umull r5,r6,r8,r11
                adds r5,r5,r4
                umull lr,r4,r9,r10
                adcs r6,r6,#0
                adc r7,r7,#0
                adds r5,r5,lr
                adcs r6,r6,r4
                
                ldr r4,[sp,#clza_st]         // clza
                adc r7,r7,#0
                rsb r5,r4,#32
                umlal r6,r7,r9,r11
                lsr r8,r8,r4
                orr r8,r8,r9,lsl r5
                lsr r9,r9,r4
                lsr r6,r6,#1    // high trick
                vmov d16,r8,r9   // newa
                vmov d17,r6,r7   // abyp

                add r6,sp,#newa_abyp_st
                vzip.i32 q3,q4
                vzip.i32 q5,q6
                vzip.i32 q7,q8
                // save to bbyp,abyp in stack
                vstmia.i32 r6,{q3,q4,q5,q6,q7,q8}
                
                ldrd r10,[sp,#pptimes41_st]
                vld1.32 {q15},[r11]
.macro MULMOD1
                /* r0 - T
                 * r1 - D->R+i (start)
                 * r2 - D->R+0 (end)
                 * q0,q1,q2 - X's (for 6 swizzle)
                 *    (low 32bits, high 32 bits)
                 *    X[0] - (q0.i32[0],q1.i32[0])
                 *    X[1] - (q0.i32[1],q1.i32[1])
                 *    X[2] - (q0.i32[2],q1.i32[2])
                 *    X[3] - (q0.i32[3],q1.i32[3])
                 *    X[4] - (q2.i32[0],q2.i32[2])
                 *    X[5] - (q2.i32[1],q2.i32[3])
                 *  q15 - p,
                 *  Warning: T_t.bbyp[0:31] must be shifted right (???)
                 */
                // add and set as new X0
                vadd.u64 q0,q12,q15
                vadd.u64 q1,q13,q15
                vadd.u64 q2,q14,q15
                // final
                vuzp.i32 q0,q1
                vuzp.i32 d4,d5
                // load b,bbyp (partial square)
                // pmask
                vld1.32 {q15},[r12]
                // forming optimized structures
                // (structures is ok)
                // q3=[T0,bbyp0],q4=[T1,bbyp1] -> q3=[T0L,T1L,T0H,T1H],
                // q4=[bbyp0L,bbyp1L,bbyp0H,bbyp1H]
                /* high(newa*bbyp) */
                vmull.u32 q9,d2,d8      // d1=(newaH0,newaH1), d8=(bbyp0L,bbyp1L)
                vmull.u32 q12,d0,d9     // d0=(newaL0,newaL1), d9=(bbyp0H,bbyp1H)
                vmull.u32 q10,d3,d12
                vmull.u32 q13,d1,d13
                vmull.u32 q11,d5,d16
                vmull.u32 q14,d4,d17
                vsra.u64 q9,q12,#1      // (newaH*bbypL>>1)+(newaL*bbypH>>1)
                vsra.u64 q10,q13,#1
                vsra.u64 q11,q14,#1
                vshr.u64 q9,q9,#31
                vshr.u64 q10,q10,#31
                vshr.u64 q11,q11,#31
                vmlal.u32 q9,d2,d9      // +(newaH*bbypH)
                vmlal.u32 q10,d3,d13
                vmlal.u32 q11,d5,d17
                
                vuzp.u32 q9,q10
                vuzp.u32 d22,d23
                /* result of prod:
                 * q9=[tmp0L,tmp1L,tmp2L,tmp3L]
                 * q10=[tmp0H,tmp1H,tmp2H,tmp3H]
                 * q11=[tmp4L,tmp5L,tmp4H,tmp5H]
                 */
                vand q9,q9,q15
                vand d22,d22,d30
                /* results in: q9,q11,q13
                 * low(newa*newb), T=newb
                 * q0,q1,q2 - newa q3,q5,q7 - newb
                 * low(tmp*p), T=
                 * q9,q11,q13 - tmp, q15-[p*4L,p*4L,p*4H,p*4H]
                 */
                vmull.u32 q12,d2,d6
                vmull.u32 q13,d3,d10
                vmull.u32 q14,d5,d14
                vld1.32 {q15},[r10]
                //PRINT128Q64 d30,d31
                vmlal.u32 q12,d0,d7
                vmlal.u32 q13,d1,d11
                vmlal.u32 q14,d4,d15
                vmlsl.u32 q12,d20,d30
                vmlsl.u32 q13,d21,d30
                vmlsl.u32 q14,d23,d30
                vmlsl.u32 q12,d18,d31
                vmlsl.u32 q13,d19,d31
                vmlsl.u32 q14,d22,d31
                vshl.u64 q12,q12,#32
                vshl.u64 q13,q13,#32
                vshl.u64 q14,q14,#32
                vmlal.u32 q12,d0,d6
                vmlal.u32 q13,d1,d10
                vmlal.u32 q14,d4,d14
                vmlsl.u32 q12,d18,d30
                vmlsl.u32 q13,d19,d30
                vmlsl.u32 q14,d22,d30
                
                vld1.32 {q15},[r11]
                // q15=[pshifted,pshidted]
                vshr.s64 q9,q12,#63
                vshr.s64 q10,q13,#63
                vshr.s64 q11,q14,#63
                vand.i64 q9,q9,q15
                vand.i64 q10,q10,q15
                vand.i64 q11,q11,q15
                vadd.i64 q12,q9,q12
                vadd.i64 q13,q10,q13
                vadd.i64 q14,q11,q14
.endm
                MULMOD1
                
                /* end sqrmod b */
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod3
                
                MULMOD1
.Lnomulmod3:
                tst r4,r4
                str r4,[sp,#nshift_st]
                beq .Lendloop2
                /**
                 * end of first iteration
                 */

                /* higher part of n */
                /* main loop */
.Lloop2:
                
.macro PARTSQRMOD J,DT,D0,D1
                //vmov r0,r1,\DT
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd2_\J
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd2_\J:
                vmov r0,r1,\DT
                mov r7,#0
                umull lr,r4,r8,r10
                umull r5,r6,r8,r11
                adds r5,r5,r4
                umull lr,r4,r9,r10
                adcs r6,r6,#0
                adc r7,r7,#0
                adds r5,r5,lr
                adcs r6,r6,r4
                
                ldr r4,[sp,#24]         // clza
                adc r7,r7,#0
                rsb r5,r4,#32
                umlal r6,r7,r9,r11
                lsr r8,r8,r4
                orr r8,r8,r9,lsl r5
                lsr r9,r9,r4
                lsr r6,r6,#1    // high trick
                vmov \D0,r8,r9   // newa
                vmov \D1,r6,r7   // abyp
.endm
                vmov r0,r1,d24
                ldrd r2,[sp,#pshifted_st]    // pshifted
                ldrd r10,[sp,#onep_st]       // onep+fac
                PARTSQRMOD 0,d25,d6,d7
                PARTSQRMOD 1,d26,d8,d9
                PARTSQRMOD 2,d27,d10,d11
                PARTSQRMOD 3,d28,d12,d13
                PARTSQRMOD 4,d29,d14,d15
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd2_5
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd2_5:
                mov r7,#0
                umull lr,r4,r8,r10
                umull r5,r6,r8,r11
                adds r5,r5,r4
                umull lr,r4,r9,r10
                adcs r6,r6,#0
                adc r7,r7,#0
                adds r5,r5,lr
                adcs r6,r6,r4
                
                ldr r4,[sp,#24]         // clza
                adc r7,r7,#0
                rsb r5,r4,#32
                umlal r6,r7,r9,r11
                lsr r8,r8,r4
                orr r8,r8,r9,lsl r5
                lsr r9,r9,r4
                lsr r6,r6,#1    // high trick
                vmov d16,r8,r9   // newa
                vmov d17,r6,r7   // abyp
                
                vzip.i32 q3,q4
                vzip.i32 q5,q6
                vzip.i32 q7,q8
                
                ldrd r10,[sp,#pptimes41_st]
                MULMOD1
                
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod4
                
                add r6,sp,#newa_abyp_st
                vldmia.i32 r6,{q3,q4,q5,q6,q7,q8}
                MULMOD1
.Lnomulmod4:
                tst r4,r4
                str r4,[sp,#nshift_st]
                bne .Lloop2
.Lendloop2:
                ldr r8,[sp,#npartn_st]
                subs r8,#1
                blo .Lend2
                str r8,[sp,#npartn_st]
                
                mov r10,#0x80000000
                ldr r11,[sp,#nlpart_st]
                strd r10,[sp,#nshift_st]
                b .Lloop2
.Lend2:
                ldrd r4,[sp,#vector_st]
                strd r0,[r4]
                vstmia.i32 r4!,{q12,q13,q14}
                str r4,[sp,#vector_st]
                cmp r4,r5
                blo .Lvecloop
.Lendvecloop:
                add sp,sp,#160
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
                .word mod64_neon_init_data(GOT)
                