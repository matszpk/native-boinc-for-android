/*
 * powseq-neon.S
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

#ifndef SKIP_VFP_SAVE
#define st_vfp_size 64
#else
#define st_vfp_size 0
#endif

                .align 2
.Lpow8mod64_arm_shifted:
                /* r0,r1 - a (shifted)
                 * r2,r3 - p
                 */
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#32
                
                mov r6,r0
                mov r7,r1
                
                ldr r0,.LGOT0
.LPIC0:
                add r0,pc,r0
                ldr r1,.LGOT0+4
                ldr r0,[r0,r1]          // mod64_init_data
                
                ldrd r2,[r0,#pshifted_idx]
                strd r2,[sp]
                ldrd r2,[r0,#ptimes4_idx]
                strd r2,[sp,#8]
                ldrd r2,[r0,#onep_idx]
                adds r2,r2,#14
                adc r3,r3,#0
                strd r2,[sp,#16]
                ldr r4,[r0,#clzp_idx]
                sub r4,r4,#2
                str r4,[sp,#24]
                ldr r4,[r0,#pmask_idx]
                str r4,[sp,#28]
                
.macro SQRMOD J
                /* sqrmod core begin */
                ldrd r2,[sp]
                
                ldrd r10,[sp,#16]       // onep+fac
                
                adds r6,r6,r2
                adc r7,r7,r3
                
                mov r8,r6
                mov r9,r7
                tst r7,#0x80000000        // mybit>=newb
                bmi .Lnoadd\J
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd\J:
                mov lr,#0
                umull r1,r4,r8,r10
                umull r5,r12,r8,r11
                adds r5,r5,r4
                umull r1,r4,r9,r10
                adcs r12,r12,#0
                adc lr,lr,#0
                adds r5,r5,r1
                adcs r12,r12,r4
                
                ldr r4,[sp,#24]         // clza
                adc lr,lr,#0
                rsb r5,r4,#32
                umlal r12,lr,r9,r11
                lsr r8,r8,r4
                orr r8,r8,r9,lsl r5
                ldrd r2,[sp,#8]         // ptimes4
                lsr r9,r9,r4
                ldr r0,[sp,#28]         // pmask
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r6,r7 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r12,r14 - bbyp
                 * r0 - and mask
                 * output -
                 * r6,r7 - shifted modulo
                 */
                // main op
                umull r1,r5,r6,lr     // newa*bbyp
                umull r10,r11,r7,r12
                umull r12,lr,r7,lr
                adds r1,r1,r10
                adcs r5,r5,r11
                adc lr,lr,#0
                adds r12,r12,r5
                adc lr,lr,#0           // result in r10,r11
                and r12,r12,r0           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r12       // tmp*p
                mla r7,r2,lr,r7
                mla r7,r3,r12,r7         // result in r6,r7
                ldrd r8,[sp]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2_\J
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2_\J:
                /* sqrmod core end */
.endm
                SQRMOD 10
                SQRMOD 20
                SQRMOD 30
                
                mov r0,r6
                mov r1,r7
                
                add sp,sp,#32
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word mod64_init_data(GOT)


.macro VEC_POWSEQ
                /* r0 - T
                 * r1 - D->R+i (start)
                 * r2 - D->R+0 (end)
                 * q0,q1,q2,q3 - X's (for 6 swizzle)
                 *    (low 32bits, high 32 bits)
                 *    X[0] - (q0.i32[0],q1.i32[0])
                 *    X[1] - (q0.i32[1],q1.i32[1])
                 *    X[2] - (q0.i32[2],q1.i32[2])
                 *    X[3] - (q0.i32[3],q1.i32[3])
                 *    X[4] - (q2.i32[0],q2.i32[2])
                 *    X[5] - (q2.i32[1],q2.i32[3])
                 * // forming optimized structures
                 * (structures is ok)
                 * q3=[T0,bbyp0],q4=[T1,bbyp1] -> q3=[T0L,T1L,T0H,T1H],
                 * q4=[bbyp0L,bbyp1L,bbyp0H,bbyp1H]
                 *  q15 - p,
                 *  Warning: T_t.bbyp[0:31] must be shifted right (???)
                 */
                vuzp.u32 q0,q1
                vuzp.u32 q2,q3
                /* pmask - q6 */
                //vld1.32 {q4},[r8]
                /* b and bbyp - q4,q5 */
                
                vmull.u32 q11,d0,d11     // d0=(newaL0,newaL1), d9=(bbyp0H,bbyp1H)
                vmull.u32 q12,d1,d11
                vmull.u32 q7,d2,d10      // d1=(newaH0,newaH1), d8=(bbyp0L,bbyp1L)
                vmull.u32 q8,d3,d10
                vmull.u32 q9,d6,d10
                vmull.u32 q13,d4,d11
                vmull.u32 q10,d7,d10
                vmull.u32 q14,d5,d11
                vsra.u64 q7,q11,#1      // (newaH*bbypL>>1)+(newaL*bbypH>>1)
                vsra.u64 q8,q12,#1
                vsra.u64 q9,q13,#1
                vsra.u64 q10,q14,#1
                vshr.u64 q7,q7,#31
                vshr.u64 q8,q8,#31
                vshr.u64 q9,q9,#31
                vshr.u64 q10,q10,#31
                vmlal.u32 q7,d2,d11      // +(newaH*bbypH)
                vmlal.u32 q8,d3,d11
                vmlal.u32 q9,d6,d11
                vmlal.u32 q10,d7,d11
                
                vuzp.u32 q7,q8
                vuzp.u32 q9,q10
                /* result of prod:
                 * q7=[tmp0L,tmp1L,tmp2L,tmp3L]
                 * q8=[tmp0H,tmp1H,tmp2H,tmp3H]
                 * q9=[tmp4L,tmp5L,tmp6L,tmp7L]
                 * q10=[tmp4H,tmp5H,tmp6H,tmp7H]
                 */
                vld1.32 {q15},[r4]
                vmull.u32 q11,d2,d8
                vmull.u32 q12,d3,d8
                vmull.u32 q13,d6,d8
                vmull.u32 q14,d7,d8
                
                vand q7,q7,q6
                vand q9,q9,q6
                /* results in: q7,q9,q11,q13
                 * low(newa*newb), T=newb
                 * q0,q1,q2,q3 - newa q5 - newb
                 * low(tmp*p), T=
                 * q7,q9,q11,q13 - tmp, q15-[p*4L,p*4L,p*4H,p*4H]
                 */
                vmlal.u32 q11,d0,d9
                vmlal.u32 q12,d1,d9
                vmlal.u32 q13,d4,d9
                vmlal.u32 q14,d5,d9
                vmlsl.u32 q11,d16,d30 // q15
                vmlsl.u32 q12,d17,d30
                vmlsl.u32 q13,d20,d30
                vmlsl.u32 q14,d21,d30
                vmlsl.u32 q11,d14,d31
                vmlsl.u32 q12,d15,d31
                vmlsl.u32 q13,d18,d31
                vmlsl.u32 q14,d19,d31
                vshl.u64 q11,q11,#32
                vshl.u64 q12,q12,#32
                vshl.u64 q13,q13,#32
                vshl.u64 q14,q14,#32
                vmlal.u32 q11,d0,d8
                vmlal.u32 q12,d1,d8
                vmlal.u32 q13,d4,d8
                vmlal.u32 q14,d5,d8
                vmlsl.u32 q11,d14,d30   // q15
                vmlsl.u32 q12,d15,d30
                vmlsl.u32 q13,d18,d30
                vmlsl.u32 q14,d19,d30
                
                // q15=[pshifted,pshidted]
                vld1.32 {q15},[r5]
                vshr.s64 q7,q11,#63
                vshr.s64 q8,q12,#63
                vshr.s64 q9,q13,#63
                vshr.s64 q10,q14,#63
                vand.i64 q7,q7,q15
                vand.i64 q8,q8,q15
                vand.i64 q9,q9,q15
                vand.i64 q10,q10,q15
                
                vadd.i64 q11,q7,q11
                vadd.i64 q12,q8,q12
                vadd.i64 q13,q9,q13
                vadd.i64 q14,q10,q14
                
                vadd.u64 q0,q11,q15
                vadd.u64 q1,q12,q15
                vadd.u64 q2,q13,q15
                vadd.u64 q3,q14,q15
                
                vstmia.i32 r0!,{q11,q12,q13,q14}
.endm

                .align 2
                .globl  powseq64_neon_shifted
                .type   powseq64_neon_shifted, %function
powseq64_neon_shifted:
                // if vector too short
                
                cmp r1,#18
                blo powseq64_arm_shifted(PLT)
                // real routine
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                /* preparing short vector */
                ldr r5,.LGOT1
.LPIC1:
                add r5,pc,r5
                ldr r6,.LGOT1+4
                ldr r8,[r5,r6]  // mod64_init_data
                ldr r10,[r8,#32]
                sub r10,r10,#2
                mov r9,#1
                lsl r9,r9,r10    // shifted one
                
                ldrd r6,[r0]
                
                cmp r6,r9
                cmpeq r7,#0
                bne .Lnobaseone
                
                push {r0,r1,r2,r3}
                mov r1,#10
                bl powseq64_arm_shifted(PLT)
                pop {r0,r1,r2,r3}
                ldrd r2,[r0,#8*8]       // new b (b^8)
                // shift right by clzp-2
                
                add r0,r0,#16
                sub r1,r1,#2
                b .Lshortvecend
.Lnobaseone:
                push {r0,r1,r2,r3}
                mov r1,#8
                bl powseq64_arm_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                rsb r9,r10,#32  // b<<clzb
                lsl r3,r3,r10
                orr r3,r3,r2,lsr r9
                lsl r2,r2,r10
                
                // create shifted b^3
                // r2,r2 - b
                push {r0,r1}
                mov r0,r2
                mov r1,r3
                bl .Lpow8mod64_arm_shifted
                mov r2,r0
                mov r3,r1
                pop {r0,r1}
.Lshortvecend:
                
                /* real neon loop */
                push {r0,r1,r2,r3}
                mov r0,r2
                mov r1,r3
                bl premulmod64_neon_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                add r7,r0,r1,lsl #3
                sub r6,r7,#8*8
                
                ldr r3,.LGOT1+8
                ldr r4,[r5,r3]  // mod64_neon_init_data
                add r5,r4,#16   // pshifted
                add r8,r4,#32   // pmask
                add r9,r4,#48   // b,bbyp
                
                // load data
                vldmia.i32 r0!,{q11,q12,q13,q14}
                vld1.32 {q15},[r5]
                vldmia.32 r9,{q4,q5}
                vld1.32 {q6},[r8]
                
                vadd.u64 q0,q11,q15
                vadd.u64 q1,q12,q15
                vadd.u64 q2,q13,q15
                vadd.u64 q3,q14,q15
                
                cmp r0,r6
                bhs .Lendloop1
.Lloop1:
                VEC_POWSEQ
                VEC_POWSEQ
                
                cmp r0,r6
                blo .Lloop1
.Lendloop1:
                cmp r0,r7
                bhs .Lnolast
                
                // last iteration
                VEC_POWSEQ
.Lnolast:
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr

                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
                .word mod64_neon_init_data(GOT)