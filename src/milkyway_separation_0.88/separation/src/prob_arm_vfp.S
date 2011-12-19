/*
 * prob_arm_vfp.S
 * Mateusz Szpakowski
 * License: GPLv3
 */
                .arch armv6
                .fpu vfp
                .eabi_attribute 20, 1
                .eabi_attribute 21, 1
                .eabi_attribute 23, 3
                .eabi_attribute 24, 1
                .eabi_attribute 25, 1
                .eabi_attribute 26, 2
                .eabi_attribute 30, 2
                .eabi_attribute 18, 4
                .text
                .align 3
#ifndef SKIP_VFP_SAVE
#define st_vfp_size 64
#else
#define st_vfp_size 0
#endif
                
                /* with aux_bg_profile */
.Lprob_aux_arm_vfp:
.Lmainloop1:
                /* r0 - ap,
                   r1 - sc,
                   r2 - sg_dx,
                   r3 - r_point 
                   sp - qw_r3_n 
                   sp+(8-39) - lbt
                   sp+40 - gPrime
                   sp+48 - reff_xr_rp3
                   sp+56 - streamTmps */
                mov r8,r1
                /* first quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3]            // rpoint
                fldd d1,[r3,#8]
                
                fldd d2,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d4,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                fldd d6,[r0]
                
                fmacd d5,d0,d2          // d5-d6 xyz.x[2]
                fmacd d6,d1,d2
                fmuld d7,d0,d3          // d7-d8 xyz.y[2]
                fmuld d8,d1,d3
                fmuld d9,d0,d4          // d9-d10 xyz.z[2]
                fmuld d10,d1,d4
                
                
                fldd d11,[r0,#16]       // q_inv_sqr
                /* rg_calc */
                fmuld d0,d5,d5
                fmuld d1,d6,d6
                fmacd d0,d7,d7
                fmacd d1,d8,d8
                fmuld d12,d9,d9
                fmuld d13,d10,d10
                fmacd d0,d12,d11
                fmacd d1,d13,d11
                
                fsqrtd d0,d0
                
                fldd d2,[r6]    // qw_r3_N
                fldd d4,[r6,#8]    // qw_r3_N
                fstd d5,[sp]
                fstd d6,[sp,#8]
                fstd d7,[sp,#16]
                fstd d8,[sp,#24]
                fstd d9,[sp,#32]
                fstd d10,[sp,#40]
                fldd d3,[r0,#24]        // r0
                
                fsqrtd d1,d1
                
                /* h_prob_fast */
                faddd d11,d3,d0
                faddd d12,d3,d1
                fmuld d13,d11,d11
                fmuld d14,d12,d12
                fmuld d13,d13,d11
                fmuld d14,d14,d12
                fmuld d13,d13,d0
                fmuld d14,d14,d1
                
                fdivd d13,d2,d13
                
                /* aux_bg_profile */
                fldd d12,[sp,#128+40+40+st_vfp_size]         // gprime
                fldd d0,[r2]            // sg_dx
                fldd d1,[r2,#8]
                
                fldd d3,[r0,#72]        // ap->bg_a
                fldd d5,[r0,#80]        // ap->bg_b
                faddd d0,d0,d12
                faddd d1,d1,d12
                fldd d6,[r0,#80]        // ap->bg_b
                fldd d7,[r0,#88]        // ap->bg_c
                fldd d8,[r0,#88]        // ap->bg_c
                
                fmacd d5,d0,d3
                fmacd d6,d1,d3
                fmacd d7,d0,d5
                fmacd d8,d1,d6
                
                fmacd d13,d7,d2
                faddd d15,d15,d13       // bg_prop+=h_prop
                
                fdivd d14,d4,d14
                
                /* second quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3,#16]            // rpoint
                fldd d1,[r3,#24]
                
                fldd d2,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d12,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                fldd d6,[r0]
                
                fmacd d5,d0,d2          // d5-d6 xyz.x[2]
                fmacd d6,d1,d2
                fmuld d7,d0,d3          // d7-d8 xyz.y[2]
                
                fmuld d13,d1,d3
                fmuld d9,d0,d12          // d9-d10 xyz.z[2]
                fmuld d10,d1,d12
                
                fmacd d14,d8,d4         // first phase end
                faddd d15,d15,d14
                
                fldd d11,[r0,#16]         // q_inv_sqr
                
                /* rg_calc */
                fmuld d0,d5,d5
                fmuld d1,d6,d6
                fmacd d0,d7,d7
                fmacd d1,d13,d13
                fmuld d12,d9,d9
                fmuld d8,d10,d10
                fmacd d0,d12,d11
                fmacd d1,d8,d11
                
                fsqrtd d0,d0
                
                fldd d2,[r6,#16]    // qw_r3_N
                fldd d4,[r6,#24]    // qw_r3_N
                fstd d5,[sp,#48]
                fstd d6,[sp,#56]
                fstd d7,[sp,#64]
                fstd d13,[sp,#72]
                fstd d9,[sp,#80]
                fstd d10,[sp,#88]
                fldd d3,[r0,#24]        // r0
                
                fsqrtd d1,d1
                
                /* h_prob_fast */
                faddd d11,d3,d0
                faddd d12,d3,d1
                fmuld d13,d11,d11
                fmuld d14,d12,d12
                fmuld d13,d13,d11
                fmuld d14,d14,d12
                fmuld d13,d13,d0
                fmuld d14,d14,d1
                
                fdivd d13,d2,d13
                
                /* aux_bg_profile */
                fldd d12,[sp,#128+40+40+st_vfp_size]         // gprime
                fldd d0,[r2,#16]            // sg_dx
                fldd d1,[r2,#24]
                
                fldd d3,[r0,#72]        // ap->bg_a
                fldd d5,[r0,#80]        // ap->bg_b
                faddd d0,d0,d12
                faddd d1,d1,d12
                fldd d6,[r0,#80]        // ap->bg_b
                fldd d7,[r0,#88]        // ap->bg_c
                fldd d8,[r0,#88]        // ap->bg_c
                
                fdivd d14,d4,d14
                
                fmacd d5,d0,d3
                fmacd d6,d1,d3
                fmacd d7,d0,d5
                fmacd d8,d1,d6
                
                fmacd d13,d7,d2
                fmacd d14,d8,d4
                
                faddd d15,d15,d13       // bg_prop+=h_prop
                faddd d15,d15,d14
                
                /*
                 * ok /-\
                 *    ||
                 */
                
                strd r2,[sp,#104]
                str r4,[sp,#112]
                str r7,[sp,#116]
                
                fstd d15,[sp,#96]
                
                str r0,[sp,#120]
                ldr r0,[sp,#128+40+56+st_vfp_size]      // streamTmps
.Lstreamloop1:
                /* first quad half */
                fldd d4,[sp]
                fldd d5,[sp,#8]
                fldd d6,[sp,#16]
                fldd d7,[sp,#24]
                fldd d8,[sp,#32]
                fldd d9,[sp,#40]
                // sc[i].a and sc[i].c
                fldd d10,[r8]
                fldd d11,[r8,#8]
                fldd d12,[r8,#16]
                fldd d13,[r8,#32]
                fldd d14,[r8,#40]
                fldd d15,[r8,#48]
                
                fsubd d4,d4,d13                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d13
                fsubd d6,d6,d14
                fsubd d7,d7,d14
                fsubd d8,d8,d15
                fsubd d9,d9,d15
                
                fmuld d0,d10,d4                 // dotted
                fmuld d1,d10,d5
                fmacd d0,d11,d6
                fmacd d1,d11,d7
                fmacd d0,d12,d8
                fmacd d1,d12,d9
                
                fnmacd d4,d0,d10        // xyzs-sc.a*dotted
                fnmacd d6,d0,d11
                fnmacd d8,d0,d12
                fnmacd d5,d1,d10
                fnmacd d7,d1,d11
                fnmacd d9,d1,d12
                
                fmuld d0,d4,d4          // norm
                fmuld d1,d5,d5
                fmacd d0,d6,d6
                fmacd d1,d7,d7
                fmacd d0,d8,d8
                fmacd d1,d9,d9
                
                /* second quad half */
                fldd d4,[sp,#48]
                fldd d5,[sp,#56]
                fldd d6,[sp,#64]
                fldd d7,[sp,#72]
                fldd d8,[sp,#80]
                fldd d9,[sp,#88]
                
                fsubd d4,d4,d13                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d13
                fsubd d6,d6,d14
                fsubd d7,d7,d14
                fsubd d8,d8,d15
                fsubd d9,d9,d15
                
                fmuld d2,d10,d4                 // dotted
                fmuld d3,d10,d5
                fmacd d2,d11,d6
                fmacd d3,d11,d7
                fmacd d2,d12,d8
                fmacd d3,d12,d9
                
                fnmacd d4,d2,d10        // xyzs-sc.a*dotted
                fnmacd d6,d2,d11
                fnmacd d8,d2,d12
                fnmacd d5,d3,d10
                fnmacd d7,d3,d11
                fnmacd d9,d3,d12
                
                fmuld d2,d4,d4          // norm
                fmuld d3,d5,d5
                fmacd d2,d6,d6
                fmacd d3,d7,d7
                fmacd d2,d8,d8
                fmacd d3,d9,d9
                
                /*
                 * OK ^
                 */
                
                fldd d4,[r8,#64]    // sigma_sq2_inv
                
                fnmuld d0,d0,d4
                fnmuld d1,d1,d4
                fnmuld d2,d2,d4
                fnmuld d3,d3,d4
                
                /* computing exp (4 reals) */
                // 2
                fldd d15,.Linvlog2_4
                
                // 4
                fmuld d4,d0,d15            // x/log(2)
                fmuld d5,d1,d15            // x/log(2)
                fmuld d6,d2,d15            // x/log(2)
                fmuld d7,d3,d15            // x/log(2)
                
                // 6
                ftosid s16,d4
                ftosid s17,d5
                ftosid s18,d6
                ftosid s19,d7
                // 7
                fmrs r2,s16
                fmrs r3,s17
                fmrs r4,s18
                fmrs r7,s19
                
                ldr r10,.Lminexp
                ldr r11,.Lmaxexp
                mov r9,#0
                // 11
                cmp r2,r10
                movle r9,#1
                cmp r2,r11
                movge r9,#1
                cmp r3,r10
                movle r9,#1
                cmp r3,r11
                movge r9,#1
                cmp r4,r10
                movle r9,#1
                cmp r4,r11
                movge r9,#1
                cmp r7,r10
                movle r9,#1
                cmp r7,r11
                movge r9,#1
                tst r9,r9       // if exception
                beq .Lnoexpexception1
                bl .Lexpexception
                b .Lexpend1
.Lnoexpexception1:
                fldd d4,.Llog2_4
                fldd d5,.Llog2_4+8
                fsitod d10,s16
                fsitod d11,s17
                fsitod d12,s18
                fsitod d13,s19
                
                // 15
                fnmacd d0,d4,d10           // fractional
                fnmacd d1,d4,d11           // fractional
                fnmacd d2,d4,d12           // fractional
                fnmacd d3,d4,d13           // fractional
                
                fnmacd d0,d5,d10           // fractional
                fnmacd d1,d5,d11           // fractional
                fnmacd d2,d5,d12           // fractional
                fnmacd d3,d5,d13           // fractional
                
                // 14
                fldd d4,.Lpolyexp4
                //fldd d5,.Lpolyexp4
                //fldd d6,.Lpolyexp4
                //fldd d7,.Lpolyexp4
                
                // evaluate polynomial
                
                /*
                 * code for ARMv6 VFP 
                 */
                // 35
                fldd d8,.Lpolyexp4+8
                fldd d9,.Lpolyexp4+8
                fldd d10,.Lpolyexp4+8
                fldd d11,.Lpolyexp4+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*2
                fmacd d9,d4,d1
                fldd d13,.Lpolyexp4+8*2
                fmacd d10,d4,d2
                fldd d14,.Lpolyexp4+8*2
                fmacd d11,d4,d3
                fldd d15,.Lpolyexp4+8*2
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*3
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp4+8*3
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp4+8*3
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp4+8*3
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*4
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp4+8*4
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp4+8*4
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp4+8*4
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*5
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp4+8*5
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp4+8*5
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp4+8*5
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*6
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp4+8*6
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp4+8*6
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp4+8*6
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*7
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp4+8*7
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp4+8*7
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp4+8*7
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*8
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp4+8*8
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp4+8*8
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp4+8*8
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*9
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp4+8*9
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp4+8*9
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp4+8*9
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*10
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp4+8*10
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp4+8*10
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp4+8*10
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*11
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp4+8*11
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp4+8*11
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp4+8*11
                
                fmacd d12,d8,d0
                fmacd d13,d9,d1
                fmacd d14,d10,d2
                fmacd d15,d11,d3
                
                // apply exponent
                // 37
                fmrdh r9,d12
                fmrdh r10,d13
                fmrdh r11,d14
                fmrdh r12,d15
                
                // 38
                add r9,r9,r2,lsl #20
                add r10,r10,r3,lsl #20
                add r11,r11,r4,lsl #20
                add r12,r12,r7,lsl #20
                
                // 40
                fmdhr d12,r9
                fmdhr d13,r10
                fmdhr d14,r11
                fmdhr d15,r12
.Lexpend1:
                fldd d4,[r6]    // qw_r3_N
                fldd d5,[r6,#8]    // qw_r3_N
                fldd d6,[r6,#16]    // qw_r3_N
                fldd d7,[r6,#24]    // qw_r3_N
                // result
                
                fldd d8,[r0]
                fmacd d8,d4,d12
                fmacd d8,d5,d13
                fmacd d8,d6,d14
                fmacd d8,d7,d15
                fstd d8,[r0]
                add r0,r0,#8
                
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamloop1
                
                ldrd r2,[sp,#104]
                ldr r4,[sp,#112]
                ldr r7,[sp,#116]
                ldr r0,[sp,#120]
                
                fldd d15,[sp,#96]
                
                add r3,r3,#32
                add r6,r6,#32
                add r2,r2,#32
                cmp r6,r4
                blo .Lmainloop1
                
                add r4,r4,#24
                
                cmp r6,r4
                bne .Lrestloop1
.Lafterrestloop1:
                /* multiply by reff_xr_rp3 */
                fldd d0,[sp,#128+40+48+st_vfp_size]
                fmuld d15,d15,d0
                
                ldr r8,[sp,#128+40+56+st_vfp_size]      // streamTmps
                ldr r9,[r0,#36]        // nstreams
                add r9,r8,r9, lsl #3
.Lfinishloop1:
                fldd d1,[r8]
                fmuld d1,d1,d0
                fstd d1,[r8]
                add r8,#8
                cmp r8,r9
                blo .Lfinishloop1
                
                fmrrd r0,r1,d15
                add sp,sp,#128
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
                .align 3
.Llog2_4:       /* two parts of log2 */
                .double 6.931457519531250000e-01
                .double 1.428606820309417256e-06
.Linvlog2_4:
                .double 1.44269504088896340737
.Lpolyexp4:
                .double 2.511003760596377693e-08
                .double 2.763263963904102862e-07
                .double 2.755724091857896965e-06
                .double 2.480148548232849389e-05
                .double 1.984126989004711308e-04
                .double 1.388888895231477506e-03
                .double 8.333333333319601147e-03
                .double 4.166666666648809886e-02
                .double 1.666666666666667962e-01
                .double 5.000000000000018874e-01
                .double 1.000000000000000000e+00
                .double 1.000000000000000000e+00
.Lzero4:
                .double 0.0
.Linf4:
                .quad 0x7ff0000000000000
.Lrestloop1:
                mov r8,r1
                fldd d0,[r3]            // rpoint
                
                fldd d2,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d4,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                
                fmacd d5,d0,d2          // d5 xyz.x
                fmuld d6,d0,d3          // d6 xyz.y
                fmuld d7,d0,d4          // d7 xyz.z
                fmuld d10,d1,d4
                
                fstmiad sp,{d5,d6,d7}
                
                fldd d11,[r0,#16]       // q_inv_sqr
                /* rg_calc */
                fmuld d0,d5,d5
                fldd d2,[r6]    // qw_r3_N
                fmacd d0,d6,d6
                fmuld d7,d7,d7
                fmacd d0,d7,d11
                fldd d3,[r0,#24]        // r0
                fsqrtd d0,d0
                
                /* h_prob_fast */
                faddd d11,d3,d0
                fmuld d12,d11,d11
                fmuld d12,d12,d11
                fmuld d12,d12,d0
                fdivd d12,d2,d12
                
                /* bg_aux_profile */
                fldd d11,[sp,#128+40+40+st_vfp_size]         // gprime
                fldd d0,[r2]            // sg_dx
                
                faddd d0,d0,d11
                
                fldd d1,[r0,#72]        // ap->bg_a
                fldd d3,[r0,#80]        // ap->bg_b
                fldd d4,[r0,#88]        // ap->bg_c
                
                fmacd d3,d0,d1
                fmacd d4,d0,d3
                
                fmacd d12,d4,d2
                
                faddd d15,d15,d12       // bg_prop+=h_prop
                
                str r3,[sp,#104]
                str r2,[sp,#108]
                ldr r12,[sp,#128+40+56+st_vfp_size]      // streamTmps
                
                ldr r10,.Lminexp
                ldr r11,.Lmaxexp
.Lstreamrestloop1:
                fldmiad sp,{d4,d5,d6}
                // sc[i].a and sc[i].c
                fldd d7,[r8]
                fldd d8,[r8,#8]
                fldd d9,[r8,#16]
                fldd d10,[r8,#32]
                fldd d11,[r8,#40]
                fldd d12,[r8,#48]
                
                fsubd d4,d4,d10                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d11
                fsubd d6,d6,d12
                
                fmuld d0,d7,d4                 // dotted
                fmacd d0,d8,d5
                fmacd d0,d9,d6
                
                fnmacd d4,d0,d7        // xyzs-sc.a*dotted
                fnmacd d5,d0,d8
                fnmacd d6,d0,d9
                
                fmuld d0,d4,d4          // norm
                fmacd d0,d5,d5
                fmacd d0,d6,d6
                
                fldd d4,[r8,#64]    // sigma_sq2_inv
                fnmuld d0,d0,d4
                
                /* compute exponent */
                fldd d13,.Linvlog2_4
                fmuld d4,d0,d13            // x/log(2)
                ftosid s16,d4
                fmrs r2,s16
                
                cmp r2,r10
                bgt .Lrestnolow1
                fldd d12,.Lzero4
                b .Lrestendexp1
.Lrestnolow1:
                cmp r2,r11
                blt .Lrestcomputeexp1
                fldd d12,.Linf4
                b .Lrestendexp1
.Lrestcomputeexp1:
                fldd d4,.Llog2_4
                fldd d5,.Llog2_4+8
                fsitod d12,s16
                
                fnmacd d0,d4,d12           // fractional
                fnmacd d0,d5,d12           // fractional
                fldd d4,.Lpolyexp4
                
                fldd d8,.Lpolyexp4+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*2
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*3
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*4
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*5
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*6
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*7
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*8
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp4+8*9
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp4+8*10
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp4+8*11
                fmacd d12,d8,d0
                // apply exponent
                fmrdh r9,d12
                add r9,r9,r2,lsl #20
                fmdhr d12,r9
.Lrestendexp1:
                fldd d4,[r6]    // qw_r3_N
                fldd d8,[r12]
                fmacd d8,d4,d12
                fstd d8,[r12]
                add r12,r12,#8
                
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamrestloop1
                
                ldr r3,[sp,#104]
                ldr r2,[sp,#108]
                
                add r3,r3,#8
                add r6,r6,#8
                add r2,r2,#8
                cmp r6,r4
                blo .Lrestloop1
                b .Lafterrestloop1
                
                .align 3
.Lzero3:
                .double 0.0
                
                
                .global prob_arm_vfp
                .type   prob_arm_vfp,%function
                /* bg_probability_fast_hprob for ARM architecture */
prob_arm_vfp:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                
                /* r0 - ap,
                   r1 - sc,
                   r2 - sg_dx,
                   r3 - r_point 
                   sp - qw_r3_n 
                   sp+(8-39) - lbt
                   sp+40 - gPrime
                   sp+48 - reff_xr_rp3
                   sp+56 - streamTmps */
                sub sp,sp,#128
                
                ldr r4,[r0,#32]        // convolve
                //sub r4,r4,#1
                ldr r5,[r0,#36]        // nstreams
                ldr r9,[sp,#128+40+56+st_vfp_size]      // streamTmps
                
                lsl r8,r5,#3
                sub r8,r8,#8
                mov r6,#0
                mov r7,#0
.Lzeroloop1:
                strd r6,[r9,r8]
                subs r8,r8,#8
                bhs .Lzeroloop1
                
                ldr r6,[sp,#128+40+st_vfp_size]         // qw_r3_N
                sub r4,r4,#3
                add r4,r6,r4,lsl #3     // qw_r3_N+convolve
                add r5,r1,r5,lsl #7     // sc+nstreams
                fldd d15,.Lzero3         // bg_prop
                
                ldr r10,[r0,#44]         // aux_bg_profile
                tst r10,r10
                beq .Lprob_noaux_arm_vfp
                bne .Lprob_aux_arm_vfp
                
                .align 3
.Lzero:
                .double 0.0
                
.Lprob_noaux_arm_vfp:
                /* r0 - ap,
                   r1 - sc,
                   r2 - sg_dx,
                   r3 - r_point 
                   sp - qw_r3_n 
                   sp+(8-39) - lbt
                   sp+40 - gPrime
                   sp+48 - reff_xr_rp3
                   sp+56 - streamTmps */
.Lmainloop2:
                mov r8,r1
                /* first quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3]            // rpoint
                fldd d1,[r3,#8]
                
                fldd d2,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d4,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                fldd d6,[r0]
                
                fmacd d5,d0,d2          // d5-d6 xyz.x[2]
                fmacd d6,d1,d2
                fmuld d7,d0,d3          // d7-d8 xyz.y[2]
                fmuld d8,d1,d3
                fmuld d9,d0,d4          // d9-d10 xyz.z[2]
                fmuld d10,d1,d4
                
                fldd d11,[r0,#16]       // q_inv_sqr
                /* rg_calc */
                fmuld d0,d5,d5
                fmuld d1,d6,d6
                fmacd d0,d7,d7
                fmacd d1,d8,d8
                fmuld d12,d9,d9
                fmuld d13,d10,d10
                fmacd d0,d12,d11
                fmacd d1,d13,d11
                
                fsqrtd d0,d0
                
                fldd d2,[r6]    // qw_r3_N
                fldd d4,[r6,#8]    // qw_r3_N
                fstd d5,[sp]
                fstd d6,[sp,#8]
                fstd d7,[sp,#16]
                fstd d8,[sp,#24]
                fstd d9,[sp,#32]
                fstd d10,[sp,#40]
                fldd d3,[r0,#24]        // r0
                
                fsqrtd d1,d1
                
                /* h_prob_fast */
                faddd d11,d3,d0
                faddd d12,d3,d1
                fmuld d13,d11,d11
                fmuld d14,d12,d12
                fmuld d13,d13,d11
                fmuld d14,d14,d12
                fmuld d13,d13,d0
                fmuld d14,d14,d1
                
                fdivd d13,d2,d13
                
                /* second quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3,#16]            // rpoint
                fldd d1,[r3,#24]
                
                fldd d11,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d12,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                fldd d6,[r0]
                
                fmacd d5,d0,d11          // d5-d6 xyz.x[2]
                fmacd d6,d1,d11
                fmuld d7,d0,d3          // d7-d8 xyz.y[2]
                fmuld d8,d1,d3
                fmuld d9,d0,d12          // d9-d10 xyz.z[2]
                fmuld d10,d1,d12
                
                faddd d15,d15,d13       // bg_prop+=h_prop
                
                fdivd d14,d4,d14
                
                
                fldd d11,[r0,#16]         // q_inv_sqr
                /* rg_calc */
                fmuld d0,d5,d5
                fmuld d1,d6,d6
                fmacd d0,d7,d7
                fmacd d1,d8,d8
                fmuld d12,d9,d9
                fmuld d13,d10,d10
                fmacd d0,d12,d11
                fmacd d1,d13,d11
                
                faddd d15,d15,d14
                
                fsqrtd d0,d0
                
                fldd d2,[r6,#16]    // qw_r3_N
                fldd d4,[r6,#24]    // qw_r3_N
                fstd d5,[sp,#48]
                fstd d6,[sp,#56]
                fstd d7,[sp,#64]
                fstd d8,[sp,#72]
                fstd d9,[sp,#80]
                fstd d10,[sp,#88]
                fldd d3,[r0,#24]        // r0
                
                fsqrtd d1,d1
                
                /* h_prob_fast */
                faddd d11,d3,d0
                faddd d12,d3,d1
                fmuld d13,d11,d11
                fmuld d14,d12,d12
                fmuld d13,d13,d11
                fmuld d14,d14,d12
                fmuld d13,d13,d0
                fmuld d14,d14,d1
                fdivd d13,d2,d13
                fdivd d14,d4,d14
                
                faddd d15,d15,d13       // bg_prop+=h_prop
                faddd d15,d15,d14
                
                /*
                 * ok /-\
                 *    ||
                 */
                
                strd r2,[sp,#104]
                str r4,[sp,#112]
                str r7,[sp,#116]
                
                fstd d15,[sp,#96]
                
                str r0,[sp,#120]
                ldr r0,[sp,#128+40+56+st_vfp_size]      // streamTmps
.Lstreamloop2:
                /* first quad half */
                fldd d4,[sp]
                fldd d5,[sp,#8]
                fldd d6,[sp,#16]
                fldd d7,[sp,#24]
                fldd d8,[sp,#32]
                fldd d9,[sp,#40]
                // sc[i].a and sc[i].c
                fldd d10,[r8]
                fldd d11,[r8,#8]
                fldd d12,[r8,#16]
                fldd d13,[r8,#32]
                fldd d14,[r8,#40]
                fldd d15,[r8,#48]
                
                fsubd d4,d4,d13                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d13
                fsubd d6,d6,d14
                fsubd d7,d7,d14
                fsubd d8,d8,d15
                fsubd d9,d9,d15
                
                fmuld d0,d10,d4                 // dotted
                fmuld d1,d10,d5
                fmacd d0,d11,d6
                fmacd d1,d11,d7
                fmacd d0,d12,d8
                fmacd d1,d12,d9
                
                fnmacd d4,d0,d10        // xyzs-sc.a*dotted
                fnmacd d6,d0,d11
                fnmacd d8,d0,d12
                fnmacd d5,d1,d10
                fnmacd d7,d1,d11
                fnmacd d9,d1,d12
                
                fmuld d0,d4,d4          // norm
                fmuld d1,d5,d5
                fmacd d0,d6,d6
                fmacd d1,d7,d7
                fmacd d0,d8,d8
                fmacd d1,d9,d9
                
                /* second quad half */
                fldd d4,[sp,#48]
                fldd d5,[sp,#56]
                fldd d6,[sp,#64]
                fldd d7,[sp,#72]
                fldd d8,[sp,#80]
                fldd d9,[sp,#88]
                
                fsubd d4,d4,d13                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d13
                fsubd d6,d6,d14
                fsubd d7,d7,d14
                fsubd d8,d8,d15
                fsubd d9,d9,d15
                
                fmuld d2,d10,d4                 // dotted
                fmuld d3,d10,d5
                fmacd d2,d11,d6
                fmacd d3,d11,d7
                fmacd d2,d12,d8
                fmacd d3,d12,d9
                
                fnmacd d4,d2,d10        // xyzs-sc.a*dotted
                fnmacd d6,d2,d11
                fnmacd d8,d2,d12
                fnmacd d5,d3,d10
                fnmacd d7,d3,d11
                fnmacd d9,d3,d12
                
                fmuld d2,d4,d4          // norm
                fmuld d3,d5,d5
                fmacd d2,d6,d6
                fmacd d3,d7,d7
                fmacd d2,d8,d8
                fmacd d3,d9,d9
                
                /*
                 * OK ^
                 */
                
                fldd d4,[r8,#64]    // sigma_sq2_inv
                
                fnmuld d0,d0,d4
                fnmuld d1,d1,d4
                fnmuld d2,d2,d4
                fnmuld d3,d3,d4
                
                /* computing exp (4 reals) */
                // 2
                fldd d15,.Linvlog2
                
                // 4
                fmuld d4,d0,d15            // x/log(2)
                fmuld d5,d1,d15            // x/log(2)
                fmuld d6,d2,d15            // x/log(2)
                fmuld d7,d3,d15            // x/log(2)
                
                // 6
                ftosid s16,d4
                ftosid s17,d5
                ftosid s18,d6
                ftosid s19,d7
                // 7
                fmrs r2,s16
                fmrs r3,s17
                fmrs r4,s18
                fmrs r7,s19
                
                ldr r10,.Lminexp
                ldr r11,.Lmaxexp
                mov r9,#0
                // 11
                cmp r2,r10
                movle r9,#1
                cmp r2,r11
                movge r9,#1
                cmp r3,r10
                movle r9,#1
                cmp r3,r11
                movge r9,#1
                cmp r4,r10
                movle r9,#1
                cmp r4,r11
                movge r9,#1
                cmp r7,r10
                movle r9,#1
                cmp r7,r11
                movge r9,#1
                tst r9,r9       // if exception
                beq .Lnoexpexception2
                bl .Lexpexception
                b .Lexpend2
.Lnoexpexception2:
                fldd d4,.Llog2
                fldd d5,.Llog2+8
                
                fsitod d10,s16
                fsitod d11,s17
                fsitod d12,s18
                fsitod d13,s19
                
                // 15
                fnmacd d0,d4,d10           // fractional
                fnmacd d1,d4,d11           // fractional
                fnmacd d2,d4,d12           // fractional
                fnmacd d3,d4,d13           // fractional
                
                fnmacd d0,d5,d10           // fractional
                fnmacd d1,d5,d11           // fractional
                fnmacd d2,d5,d12           // fractional
                fnmacd d3,d5,d13           // fractional
                
                // 14
                fldd d4,.Lpolyexp2
                //fldd d5,.Lpolyexp2
                //fldd d6,.Lpolyexp2
                //fldd d7,.Lpolyexp2
                
                // evaluate polynomial
                
                /*
                 * code for ARMv6 VFP 
                 */
                // 35
                fldd d8,.Lpolyexp2+8
                fldd d9,.Lpolyexp2+8
                fldd d10,.Lpolyexp2+8
                fldd d11,.Lpolyexp2+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*2
                fmacd d9,d4,d1
                fldd d13,.Lpolyexp2+8*2
                fmacd d10,d4,d2
                fldd d14,.Lpolyexp2+8*2
                fmacd d11,d4,d3
                fldd d15,.Lpolyexp2+8*2
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*3
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp2+8*3
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp2+8*3
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp2+8*3
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*4
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp2+8*4
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp2+8*4
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp2+8*4
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*5
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp2+8*5
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp2+8*5
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp2+8*5
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*6
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp2+8*6
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp2+8*6
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp2+8*6
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*7
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp2+8*7
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp2+8*7
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp2+8*7
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*8
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp2+8*8
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp2+8*8
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp2+8*8
                
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*9
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp2+8*9
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp2+8*9
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp2+8*9
                
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*10
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp2+8*10
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp2+8*10
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp2+8*10
                
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*11
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp2+8*11
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp2+8*11
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp2+8*11
                
                fmacd d12,d8,d0
                fmacd d13,d9,d1
                fmacd d14,d10,d2
                fmacd d15,d11,d3
                
                // apply exponent
                // 37
                fmrdh r9,d12
                fmrdh r10,d13
                fmrdh r11,d14
                fmrdh r12,d15
                
                // 38
                add r9,r9,r2,lsl #20
                add r10,r10,r3,lsl #20
                add r11,r11,r4,lsl #20
                add r12,r12,r7,lsl #20
                
                // 40
                fmdhr d12,r9
                fmdhr d13,r10
                fmdhr d14,r11
                fmdhr d15,r12
.Lexpend2:
                fldd d4,[r6]    // qw_r3_N
                fldd d5,[r6,#8]    // qw_r3_N
                fldd d6,[r6,#16]    // qw_r3_N
                fldd d7,[r6,#24]    // qw_r3_N
                // result
                
                fldd d8,[r0]
                fmacd d8,d4,d12
                fmacd d8,d5,d13
                fmacd d8,d6,d14
                fmacd d8,d7,d15
                fstd d8,[r0]
                add r0,r0,#8
                
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamloop2
                
                ldrd r2,[sp,#104]
                ldr r4,[sp,#112]
                ldr r7,[sp,#116]
                ldr r0,[sp,#120]
                
                fldd d15,[sp,#96]
                
                add r3,r3,#32
                add r6,r6,#32
                cmp r6,r4
                blo .Lmainloop2
                
                add r4,r4,#24
                
                cmp r6,r4
                bne .Lrestloop2
.Lafterrestloop2:
                /* multiply by reff_xr_rp3 */
                fldd d0,[sp,#128+40+48+st_vfp_size]
                fmuld d15,d15,d0
                
                ldr r8,[sp,#128+40+56+st_vfp_size]      // streamTmps
                ldr r9,[r0,#36]        // nstreams
                add r9,r8,r9, lsl #3
.Lfinishloop2:
                fldd d1,[r8]
                fmuld d1,d1,d0
                fstd d1,[r8]
                add r8,#8
                cmp r8,r9
                blo .Lfinishloop2
                
                fmrrd r0,r1,d15
                add sp,sp,#128
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr

                .align 3
.Llog2:          /* two parts of log2 */
                .double 6.931457519531250000e-01
                .double 1.428606820309417256e-06
.Linvlog2:
                .double 1.44269504088896340737
.Lpolyexp2:
                .double 2.511003760596377693e-08
                .double 2.763263963904102862e-07
                .double 2.755724091857896965e-06
                .double 2.480148548232849389e-05
                .double 1.984126989004711308e-04
                .double 1.388888895231477506e-03
                .double 8.333333333319601147e-03
                .double 4.166666666648809886e-02
                .double 1.666666666666667962e-01
                .double 5.000000000000018874e-01
                .double 1.000000000000000000e+00
                .double 1.000000000000000000e+00

.Lrestloop2:
                mov r8,r1
                fldd d0,[r3]            // rpoint
                
                fldd d2,[sp,#128+48+st_vfp_size]        // lbt
                fldd d3,[sp,#128+56+st_vfp_size]
                fldd d4,[sp,#128+64+st_vfp_size]
                fldd d5,[r0]    // m_sun_r0
                
                fmacd d5,d0,d2          // d5 xyz.x
                fmuld d6,d0,d3          // d6 xyz.y
                fmuld d7,d0,d4          // d7 xyz.z
                fmuld d10,d1,d4
                
                fstmiad sp,{d5,d6,d7}
                
                fldd d11,[r0,#16]       // q_inv_sqr
                /* rg_calc */
                fmuld d0,d5,d5
                fldd d2,[r6]    // qw_r3_N
                fmacd d0,d6,d6
                fmuld d7,d7,d7
                fmacd d0,d7,d11
                fldd d3,[r0,#24]        // r0
                fsqrtd d0,d0
                
                /* h_prob_fast */
                faddd d11,d3,d0
                fmuld d12,d11,d11
                fmuld d12,d12,d11
                fmuld d12,d12,d0
                fdivd d12,d2,d12
                
                faddd d15,d15,d12       // bg_prop+=h_prop
                
                str r3,[sp,#104]
                str r2,[sp,#108]
                ldr r12,[sp,#128+40+56+st_vfp_size]      // streamTmps
                
                ldr r10,.Lminexp
                ldr r11,.Lmaxexp
.Lstreamrestloop2:
                fldmiad sp,{d4,d5,d6}
                // sc[i].a and sc[i].c
                fldd d7,[r8]
                fldd d8,[r8,#8]
                fldd d9,[r8,#16]
                fldd d10,[r8,#32]
                fldd d11,[r8,#40]
                fldd d12,[r8,#48]
                
                fsubd d4,d4,d10                  // mw_subv(xyz,sc.c)
                fsubd d5,d5,d11
                fsubd d6,d6,d12
                
                fmuld d0,d7,d4                 // dotted
                fmacd d0,d8,d5
                fmacd d0,d9,d6
                
                fnmacd d4,d0,d7        // xyzs-sc.a*dotted
                fnmacd d5,d0,d8
                fnmacd d6,d0,d9
                
                fmuld d0,d4,d4          // norm
                fmacd d0,d5,d5
                fmacd d0,d6,d6
                
                fldd d4,[r8,#64]    // sigma_sq2_inv
                fnmuld d0,d0,d4
                
                /* compute exponent */
                fldd d13,.Linvlog2
                fmuld d4,d0,d13            // x/log(2)
                ftosid s16,d4
                fmrs r2,s16
                
                cmp r2,r10
                bgt .Lrestnolow2
                fldd d12,.Lzero2
                b .Lrestendexp2
.Lrestnolow2:
                cmp r2,r11
                blt .Lrestcomputeexp2
                fldd d12,.Linf
                b .Lrestendexp2
.Lrestcomputeexp2:
                fldd d4,.Llog2
                fldd d5,.Llog2+8
                fsitod d12,s16
                
                fnmacd d0,d4,d12           // fractional
                fnmacd d0,d5,d12           // fractional
                fldd d4,.Lpolyexp
                
                fldd d8,.Lpolyexp+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*2
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*3
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*4
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*5
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*6
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*7
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*8
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*9
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*10
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*11
                fmacd d12,d8,d0
                // apply exponent
                fmrdh r9,d12
                add r9,r9,r2,lsl #20
                fmdhr d12,r9
.Lrestendexp2:
                fldd d4,[r6]    // qw_r3_N
                fldd d8,[r12]
                fmacd d8,d4,d12
                fstd d8,[r12]
                add r12,r12,#8
                
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamrestloop2
                
                ldr r3,[sp,#104]
                ldr r2,[sp,#108]
                
                add r3,r3,#8
                add r6,r6,#8
                cmp r6,r4
                blo .Lrestloop2
                b .Lafterrestloop2
                
                .align 3
.Llog2_2:       /* two parts of log2 */
                .double 6.931457519531250000e-01
                .double 1.428606820309417256e-06
.Lzero2:
                .double 0.0
.Linf:
                .quad 0x7ff0000000000000
.Lpolyexp:
                .double 2.511003760596377693e-08
                .double 2.763263963904102862e-07
                .double 2.755724091857896965e-06
                .double 2.480148548232849389e-05
                .double 1.984126989004711308e-04
                .double 1.388888895231477506e-03
                .double 8.333333333319601147e-03
                .double 4.166666666648809886e-02
                .double 1.666666666666667962e-01
                .double 5.000000000000018874e-01
                .double 1.000000000000000000e+00
                .double 1.000000000000000000e+00
.Lminexp:
                .int -1022
.Lmaxexp:
                .int 1022
/* hanbdle exp exception (out of range) */
.Lexpexception:
                sub sp,sp,#8
                cmp r2,r10
                bgt .Lnolow1
                fldd d12,.Lzero2
                b .Ltoexp2
.Lnolow1:
                cmp r2,r11
                blt .Lcomputeexp1
                fldd d12,.Linf
                b .Ltoexp2
.Lcomputeexp1:
                fldd d4,.Llog2_2
                fldd d5,.Llog2_2+8
                fsitod d10,s16
                fnmacd d0,d4,d10           // fractional
                fnmacd d0,d5,d10           // fractional
                
                fldd d4,.Lpolyexp
                fsts s17,[sp,#4]
                
                fldd d8,.Lpolyexp+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*2
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*3
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*4
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*5
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*6
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*7
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*8
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp+8*9
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp+8*10
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp+8*11
                fmacd d12,d8,d0
                // apply exponent
                fmrdh r9,d12
                add r9,r9,r2,lsl #20
                fmdhr d12,r9
                
                flds s17,[sp,#4]
.Ltoexp2:
                cmp r3,r10
                bgt .Lnolow2
                fldd d13,.Lzero2
                b .Ltoexp3
.Lnolow2:
                cmp r3,r11
                blt .Lcomputeexp2
                fldd d13,.Linf
                b .Ltoexp3
.Lcomputeexp2:
                fldd d4,.Llog2_2
                fldd d5,.Llog2_2+8
                fsitod d10,s17
                fnmacd d1,d4,d10           // fractional
                fnmacd d1,d5,d10           // fractional
                fldd d5,.Lpolyexp
                
                fsts s18,[sp]
                fsts s19,[sp,#4]
                
                fldd d9,.Lpolyexp+8
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp+8*2
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp+8*3
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp+8*4
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp+8*5
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp+8*6
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp+8*7
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp+8*8
                fmacd d13,d9,d1
                fldd d5,.Lpolyexp+8*9
                fmacd d5,d13,d1
                fldd d9,.Lpolyexp+8*10
                fmacd d9,d5,d1
                fldd d13,.Lpolyexp+8*11
                fmacd d13,d9,d1
                // apply exponent
                fmrdh r9,d13
                add r9,r9,r3,lsl #20
                fmdhr d13,r9
                
                flds s18,[sp]
                flds s19,[sp,#4]
.Ltoexp3:
                cmp r4,r10
                bgt .Lnolow3
                fldd d14,.Lzero2
                b .Ltoexp4
.Lnolow3:
                cmp r4,r11
                blt .Lcomputeexp3
                fldd d14,.Linf
                b .Ltoexp4
.Lcomputeexp3:
                fldd d4,.Llog2_2
                fldd d5,.Llog2_2+8
                fsitod d10,s18
                fnmacd d2,d4,d10           // fractional
                fnmacd d2,d5,d10           // fractional
                fldd d6,.Lpolyexp
                
                fldd d10,.Lpolyexp+8
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp+8*2
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp+8*3
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp+8*4
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp+8*5
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp+8*6
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp+8*7
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp+8*8
                fmacd d14,d10,d2
                fldd d6,.Lpolyexp+8*9
                fmacd d6,d14,d2
                fldd d10,.Lpolyexp+8*10
                fmacd d10,d6,d2
                fldd d14,.Lpolyexp+8*11
                fmacd d14,d10,d2
                // apply exponent
                fmrdh r9,d14
                add r9,r9,r4,lsl #20
                fmdhr d14,r9
.Ltoexp4:
                cmp r7,r10
                bgt .Lnolow4
                fldd d15,.Lzero2
                b .Ltoexceptend
.Lnolow4:
                cmp r7,r11
                blt .Lcomputeexp4
                fldd d15,.Linf
                b .Ltoexceptend
.Lcomputeexp4:
                fldd d4,.Llog2_2
                fldd d5,.Llog2_2+8
                fsitod d10,s19
                fnmacd d3,d4,d10           // fractional
                fnmacd d3,d5,d10           // fractional
                fldd d7,.Lpolyexp
                
                fldd d11,.Lpolyexp+8
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp+8*2
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp+8*3
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp+8*4
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp+8*5
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp+8*6
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp+8*7
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp+8*8
                fmacd d15,d11,d3
                fldd d7,.Lpolyexp+8*9
                fmacd d7,d15,d3
                fldd d11,.Lpolyexp+8*10
                fmacd d11,d7,d3
                fldd d15,.Lpolyexp+8*11
                fmacd d15,d11,d3
                // apply exponent
                fmrdh r9,d15
                add r9,r9,r7,lsl #20
                fmdhr d15,r9
.Ltoexceptend:
                add sp,sp,#8
                bx lr