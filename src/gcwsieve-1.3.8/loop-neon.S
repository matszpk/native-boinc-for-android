/*
 * loop-neon.S
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

#ifndef SKIP_VFP_SAVE
#define st_vfp_size 64
#else
#define st_vfp_size 0
#endif                

                .text
                .align 2
                .global swizzle_loop6_neon
                .type   swizzle_loop6_neon, %function
swizzle_loop6_neon:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                sub sp,sp,#24
                
                vldmia r1,{q0,q1,q2}
                
                add r0,r1,#48
                mov r4,#72
                
                and r5,r2,#7
                mla r3,r5,r4,r0
                
                mla r1,r2,r4,r0
                sub r1,r1,#72
                
                ldr r5,.LGOT1
.LPIC1:
                add r5,pc,r5
                ldr r6,.LGOT1+4
                ldr r4,[r5,r6]  // mod64_neon_init_data
                add r5,r4,#16   // pshifted
                add lr,r4,#32   // pmask
                mov r12,sp  // comparer
                tst r12,#8
                addne r12,r12,#8    // aligning
                vmov.i32 q4,#0
                vst1.i32 {q4},[r12]    // zeroing
                
                // initialize X0
                vld1.32 {q15},[r5]
                vadd.i64 q0,q0,q15 // add pshifted
                vadd.i64 q1,q1,q15
                vadd.i64 q2,q2,q15
                vuzp.u32 q0,q1
                vuzp.u32 d4,d5
                
                cmp r3,r1
                bhi .Ltoloop2
              
/// MACRO
.macro LOOP_CORE
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
                ldrd r6,[r1,#48]    // load D->G[0:1]
                ldrd r8,[r1,#56]    // load D->G[2:3]
                ldrd r10,[r1,#64]    // load D->G[4:5]
                vld1.32 {q3},[r6]    // load all T structure
                vld1.32 {q4},[r7]
                vld1.32 {q5},[r8]
                vld1.32 {q6},[r9]
                vld1.32 {q7},[r10]
                vld1.32 {q8},[r11]
                // forming optimized structures
                // (structures is ok)
                // q3=[T0,bbyp0],q4=[T1,bbyp1] -> q3=[T0L,T1L,T0H,T1H],
                // q4=[bbyp0L,bbyp1L,bbyp0H,bbyp1H]
                vzip.i32 q3,q4
                vzip.i32 q5,q6
                vzip.i32 q7,q8
                vld1.32 {q15},[lr]
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
                vld1.32 {q15},[r4]
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
                
                vld1.32 {q15},[r5]
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
                
                // add and set as new X0
                vadd.u64 q0,q12,q15
                vadd.u64 q1,q13,q15
                vadd.u64 q2,q14,q15
                // final
                vuzp.i32 q0,q1
                vuzp.i32 d4,d5
                
                /// compare xxxxxxxxxxxxx
                vldmia r1,{q3,q4,q5}
                vld1.32 {q9},[r12]  // comparer
                vceq.i32 q10,q12,q3
                vceq.i32 q11,q13,q4
                vceq.i32 q15,q14,q5
                vorr q9,q9,q10
                vorr q9,q9,q11
                vorr q9,q9,q15
                vst1.32 {q9},[r12]
.endm
/// MACRO
              
.Lloop:
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                sub r1,r1,#72
                pld [r1,#-768]
                LOOP_CORE
                
                vorr d16,d18,d19
                
                vmov r6,r7,d16
                orrs r6,r6,r7
                bne .Lcheck_result
                
                sub r1,r1,#72
                
                cmp r3,r1
                bls .Lloop
                
.Ltoloop2:
                cmp r0,r1
                bhi .Lendloop
                
.Lloop2:
                /* fourth iteration */
                LOOP_CORE
                
                vorr d16,d18,d19
                
                vmov r6,r7,d16
                orrs r6,r6,r7
                bne .Lcheck_result2
                
                sub r1,r1,#72
                
                cmp r0,r1
                bls .Lloop2
.Lendloop:
                ldr r3,.Lone_by_9
                sub r1,r1,r0
                asr r1,r1,#3
                
                sub r0,r0,#48
                
                vld1.32 {q15},[r5]
                vzip.i32 q0,q1
                vzip.i32 d4,d5
                // subtract pshifted
                vsub.i64 q0,q0,q15
                vsub.i64 q1,q1,q15
                vsub.i64 q2,q2,q15
                
                vstmia r0,{q0,q1,q2}
                
                smmulr r0,r1,r3
                
                add sp,sp,#24
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr

.Lcheck_result2:
                mov r2,r1
                push {r0,r1,r2,r3}
                mov r2,#1
                b .Lcheck_common
.Lcheck_result:
                mov r2,r1
                add r1,r1,#72*7
                push {r0,r1,r2,r3}
                sub r1,r1,#72*7
                mov r2,#8
.Lcheck_common:                
                sub r0,r0,#48   // X
                mov r8,r0
                ldrd r6,[sp,#80+st_vfp_size]    // p
                
                vzip.u32 q0,q1      // X0 vector
                vzip.u32 d4,d5
                vstmia r0,{q0,q1,q2}
                
                push {r6,r7}
                bl revert_swizzle_loop6(PLT)
                
                add lr,r4,#32   // restore pmask addr
                
                vldmia r8,{q0,q1,q2}
                vuzp.u32 q0,q1      // X0 vector
                vuzp.u32 d4,d5
                
                add sp,sp,#8
                pop {r0,r1,r2,r3}
                
.Lcheckloop1:
                ldrd r6,[r1,#48]    // load D->G[0:1]
                ldrd r8,[r1,#56]    // load D->G[2:3]
                ldrd r10,[r1,#64]    // load D->G[4:5]
                vld1.32 {q3},[r6]    // load all T structure
                vld1.32 {q4},[r7]
                vld1.32 {q5},[r8]
                vld1.32 {q6},[r9]
                vld1.32 {q7},[r10]
                vld1.32 {q8},[r11]
                vld1.32 {q15},[lr]
                // forming optimized structures
                // (structures is ok)
                // q3=[T0,bbyp0],q4=[T1,bbyp1] -> q3=[T0L,T1L,T0H,T1H],
                // q4=[bbyp0L,bbyp1L,bbyp0H,bbyp1H]
                vzip.i32 q3,q4
                vzip.i32 q5,q6
                vzip.i32 q7,q8
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
                vld1.32 {q15},[r4]
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
                
                vld1.32 {q15},[r5]
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
                
                // add and set as new X0
                vadd.u64 q0,q12,q15
                vadd.u64 q1,q13,q15
                vadd.u64 q2,q14,q15
                // final
                vuzp.i32 q0,q1
                vuzp.i32 d4,d5
                
                /// compare xxxxxxxxxxxxx
                vldmia r1,{q3,q4,q5}
                
                vmov r6,r7,d6
                vmov r8,r9,d24
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                vmov r6,r7,d7
                vmov r8,r9,d25
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                
                vmov r6,r7,d8
                vmov r8,r9,d26
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                vmov r6,r7,d9
                vmov r8,r9,d27
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                
                vmov r6,r7,d10
                vmov r8,r9,d28
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                vmov r6,r7,d11
                vmov r8,r9,d29
                teq r6,r8
                teqeq r7,r9
                beq .Lendloop
                
                sub r1,r1,#72
                
                cmp r2,r1
                bls .Lcheckloop1
                
                mov r12,sp  // comparer
                tst r12,#8
                addne r12,r12,#8    // aligning
                
                // reset comparer
                vmov.i32 q3,#0
                vst1.32 {q3},[r12]
                
                // check whether from loop2
                cmp r3,r1
                bls .Lloop // if not found
                cmp r0,r1
                bls .Lloop2
                // if endloop
                b .Lendloop
                
.Lone_by_9:
                .word 477218588
                
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_neon_init_data(GOT)
