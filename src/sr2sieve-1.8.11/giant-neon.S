/*
 * giant-neon.S
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

#define mod64_init_data_IDX 4
#define mod64_neon_init_data_IDX 8
#define BJ64_IDX 12
#define hsize_IDX 16
#define htable_IDX 20
#define hsize_minus1_IDX 24
#define olist_IDX 28

#define BJ64_st 0
#define htable_st 4
#define clzbm1_st 8
#define hsize_minus1_st 12
#define olist_st 16
#define elimfun_st 20
#define ppointers_st 24

#define b_st 80
#define p_st 88
#define tmpdat_st 32

#ifndef SKIP_VFP_SAVE
#define st_vfp_size 64
#else
#define st_vfp_size 0
#endif

                .align 2
                .global giant4_neon
                .type   giant4_neon, %function
giant4_neon:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                sub sp,sp,#40

                /* r0 - D64
                 * r1 - cc
                 * r2 - M
                 * r3 - fun
                 * sp+80 - b
                 * sp+88 - p
                 */
                ldr r4,.LGOT0
.LPIC0:
                add r4,pc,r4
                ldr r5,.LGOT0+BJ64_IDX
                ldr r7,.LGOT0+hsize_minus1_IDX
                ldr r9,.LGOT0+htable_IDX
                ldr r11,.LGOT0+olist_IDX
                ldr r5,[r4,r5]
                ldr r7,[r4,r7]
                ldr r9,[r4,r9]
                ldr r11,[r4,r11]
                ldr r6,[r5]
                ldr r8,[r7]
                ldr r10,[r9]
                ldr r12,[r11]
                lsl r8,r8,#1
                str r6,[sp,#BJ64_st]
                str r8,[sp,#hsize_minus1_st]
                str r10,[sp,#htable_st]
                str r12,[sp,#olist_st]
                str r3,[sp,#elimfun_st]
                
                push {r0,r1,r2,r3}
                ldrd r0,[sp,#st_vfp_size+b_st+16]        // b
                bl premulmod64_neon_init_shifted_2(PLT)
                pop {r0,r1,r2,r3}

                ldr r5,.LGOT0+mod64_init_data_IDX
                ldr r5,[r4,r5]
                ldr r6,[r5,#32]
                sub r6,r6,#3
                str r6,[sp,#clzbm1_st]
                
                /*
                 * mod64_neon_init_data
                 */
                ldr r5,.LGOT0+mod64_neon_init_data_IDX
                ldr r6,[r4,r5]
                add r9,r6,#32   // pmask
                add r7,r6,#16   // pshifted
                add r8,r6,#48
                vld1.i32 {q6},[r9]
                vldmia.i32 r8,{q4,q5}   // b,bbyp
                strd r6,[sp,#ppointers_st]
                
                /* loop register usage:
                 * r0 - D64 start
                 * r1 - D64 end
                 * r2 - M // in stack
                 * r3 - i // in stack
                 * r4 - D64 pointer
                 */
                add r1,r0,r1,lsl #3
                mov r3,#1
                strd r2,[sp,#tmpdat_st]  // save regs
                
                ldr r9,[sp,#BJ64_st]

.Lloop1:
                mov r4,r0
                ldrd r6,[sp,#ppointers_st]
.Lloop2:
                vldmia.i32 r4,{q11,q12,q13,q14}
                vadd.u64 q0,q11,q15
                vadd.u64 q1,q12,q15
                vadd.u64 q2,q13,q15
                vadd.u64 q3,q14,q15
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
                vld1.32 {q15},[r6]
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
                vld1.32 {q15},[r7]
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
                vstmia.i32 r4!,{q11,q12,q13,q14}
                
                cmp r4,r1       // cc
                blo .Lloop2
                
                mov r4,r0
                ldrd r2,[sp,#clzbm1_st] // and hsize_minus1
                ldr r5,[sp,#htable_st]
.Lloop3:
                //PRINTOL
                // looking up on hashtable
                // D64[i] == bj
                // uses hsize_minus1, htable, BJ64,(slot,elt) (4 regs)
                // uses clzb to shift
                ldrd r6,[r4]     // load D64[i]
                //PRINT64 r6,r7
                and r8,r3,r6,lsr r2
                ldrh r8,[r5,r8]  // elt
                //PRINT16 r8
                bic r12,r8,#0x8000
                lsl lr,r12,#1    // elt*2
                lsl r12,lr,#2     // elt*8
                
                ldrd r10,[r9,r12]
                //PRINT64 r10,r11
                cmp r10,r6
                cmpeq r11,r7
                beq .Lfound
                
                tst r8,#0x8000
                beq .Lnotfound
                
                ldr r5,[sp,#olist_st]
.Loloop:
                ldrh r8,[r5,lr]
                //PRINT16 r8
                bic r12,r8,#0x8000
                lsl lr,r12,#1      // elt*2
                lsl r12,lr,#2     // elt*8
                
                ldrd r10,[r9,r12]
                //PRINT64 r10,r11
                cmp r10,r6
                cmpeq r11,r7
                beq .Lfound
                
                tst r8,#0x8000
                beq .Loloop
                ldr r5,[sp,#htable_st]
                b .Lnotfound
                
.Lfound:        // r8 - hash index
                bic r8,r8,#0x8000
                ldr r6,[sp,#elimfun_st]
                push {r0,r1,r2,r3}
                sub r2,r4,r0
                ldr r0,[sp,#tmpdat_st+4+16]
                mov r1,r8
                lsr r2,r2,#3
                ldrd r10,[sp,#st_vfp_size+p_st+16]
                
                adr lr,.Lreturnptr
                push {r10,r11}
                mov pc,r6       // elimfun call
                //bl r6
.Lreturnptr:
                add sp,sp,#8
                pop {r0,r1,r2,r3}
                ldr r5,[sp,#htable_st]
.Lnotfound:
                add r4,r4,#8
                cmp r4,r1       // cc
                blo .Lloop3
                
                ldrd r2,[sp,#tmpdat_st]
                add r3,r3,#1
                cmp r3,r2       // M
                str r3,[sp,#tmpdat_st+4]
                blo .Lloop1
                
                add sp,sp,#40
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word mod64_init_data(GOT)
                .word mod64_neon_init_data(GOT)
                .word BJ64(GOT)
                .word hsize(GOT)
                .word htable(GOT)
                .word hsize_minus1(GOT)
                .word olist(GOT)
