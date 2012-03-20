/*
 * prob_arm_vfpv3.S
 * Mateusz Szpakowski
 * License: GPLv3
 */
                .arch armv7-a
                .fpu vfpv3
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
                .align 3
.Llog2_3:
                .double 6.931457519531250000e-01
                .double 1.428606820309417256e-06
                .double 1.44269504088896340737
                
                
                /* with aux_bg_profile */
.Lprob_aux_arm_vfpv3:
                /* r0 - ap,
                   r1 - sc,
                   r2 - sg_dx,
                   r3 - r_point 
                   sp - qw_r3_n 
                   sp+(8-39) - lbt
                   sp+40 - gPrime
                   sp+48 - reff_xr_rp3
                   sp+56 - streamTmps */
.Lmainloop1:
                mov r8,r1
                /* first quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3]            // rpoint
                fldd d1,[r3,#8]
                fldd d2,[r3,#16]
                fldd d3,[r3,#24]
                
                fldd d4,[sp,#232+48+st_vfp_size]        // lbt
                fldd d5,[sp,#232+56+st_vfp_size]
                fldd d6,[sp,#232+64+st_vfp_size]
                fldd d7,[r0]    // m_sun_r0
                fldd d8,[r0]
                fldd d9,[r0]
                fldd d10,[r0]
                
                fmacd d7,d0,d4          // d5-d6 xyz.x[2]
                fmacd d8,d1,d4
                fmacd d9,d2,d4
                fmacd d10,d3,d4
                fmuld d11,d0,d5          // d7-d8 xyz.y[2]
                fmuld d12,d1,d5
                fmuld d13,d2,d5
                fmuld d14,d3,d5
                fmuld d15,d0,d6          // d9-d10 xyz.z[2]
                fmuld d16,d1,d6
                fmuld d17,d2,d6
                fmuld d18,d3,d6
                
                fstd d7,[sp]
                fstd d8,[sp,#8]
                fstd d9,[sp,#16]
                fstd d10,[sp,#24]
                fstd d11,[sp,#32]
                fstd d12,[sp,#40]
                fstd d13,[sp,#48]
                fstd d14,[sp,#56]
                fstd d15,[sp,#64]
                fstd d16,[sp,#72]
                fstd d17,[sp,#80]
                fstd d18,[sp,#88]
                
                /* rg_calc */
                fldd d19,[r0,#16]       // q_inv_sqr
                fmuld d0,d7,d7
                fmuld d1,d8,d8
                fmuld d2,d9,d9
                fmuld d3,d10,d10
                fldd d4,[r6]        // qw_r3_N
                fldd d5,[r6,#8]    // qw_r3_N
                fldd d6,[r6,#16]    // qw_r3_N
                fldd d20,[r6,#24]    // qw_r3_N
                fmacd d0,d11,d11
                fmacd d1,d12,d12
                fmacd d2,d13,d13
                fmacd d3,d14,d14
                fmuld d15,d15,d15
                fmuld d16,d16,d16
                fmuld d17,d17,d17
                fmuld d18,d18,d18
                fmacd d0,d15,d19
                fmacd d1,d16,d19
                fmacd d2,d17,d19
                fmacd d3,d18,d19
                fldd d21,[r0,#24]        // r0
                fsqrtd d0,d0
                fsqrtd d1,d1
                fsqrtd d2,d2
                fsqrtd d3,d3
                
                /* h_prob_fast */
                faddd d22,d21,d0
                faddd d23,d21,d1
                faddd d24,d21,d2
                faddd d25,d21,d3
                fmuld d26,d22,d22
                fmuld d27,d23,d23
                fmuld d28,d24,d24
                fmuld d29,d25,d25
                fmuld d26,d26,d22
                fmuld d27,d27,d23
                fmuld d28,d28,d24
                fmuld d29,d29,d25
                fmuld d26,d26,d0
                fmuld d27,d27,d1
                fmuld d28,d28,d2
                fmuld d29,d29,d3
                fdivd d26,d4,d26
                fdivd d27,d5,d27
                fdivd d28,d6,d28
                fdivd d29,d20,d29
                
                /* aux_bg_profile */
                fldd d24,[sp,#232+40+40+st_vfp_size]         // gprime
                fldd d0,[r2]            // sg_dx
                fldd d1,[r2,#8]
                fldd d2,[r2,#16]
                fldd d3,[r2,#24]
                
                fldd d7,[r0,#72]        // ap->bg_a
                fldd d8,[r0,#80]        // ap->bg_b
                fldd d9,[r0,#80]
                fldd d10,[r0,#80]
                fldd d11,[r0,#80]
                
                faddd d0,d0,d24
                faddd d1,d1,d24
                faddd d2,d2,d24
                faddd d3,d3,d24
                
                fldd d12,[r0,#88]        // ap->bg_c
                fldd d13,[r0,#88]
                fldd d14,[r0,#88]
                fldd d15,[r0,#88]
                
                fmacd d8,d0,d7
                fmacd d9,d1,d7
                fmacd d10,d2,d7
                fmacd d11,d3,d7
                
                fmacd d12,d0,d8
                fmacd d13,d1,d9
                fmacd d14,d2,d10
                fmacd d15,d3,d11
                
                fmacd d26,d12,d4
                fmacd d27,d13,d5
                fmacd d28,d14,d6
                fmacd d29,d15,d20
                
                faddd d30,d30,d26       // bg_prop+=h_prop
                faddd d30,d30,d27
                faddd d30,d30,d28
                faddd d30,d30,d29
                
                /* second quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3,#32]            // rpoint
                fldd d1,[r3,#40]
                fldd d2,[r3,#48]
                fldd d3,[r3,#56]
                
                fldd d4,[sp,#232+48+st_vfp_size]        // lbt
                fldd d5,[sp,#232+56+st_vfp_size]
                fldd d6,[sp,#232+64+st_vfp_size]
                fldd d7,[r0]    // m_sun_r0
                fldd d8,[r0]
                fldd d9,[r0]
                fldd d10,[r0]
                
                fmacd d7,d0,d4          // d5-d6 xyz.x[2]
                fmacd d8,d1,d4
                fmacd d9,d2,d4
                fmacd d10,d3,d4
                fmuld d11,d0,d5          // d7-d8 xyz.y[2]
                fmuld d12,d1,d5
                fmuld d13,d2,d5
                fmuld d14,d3,d5
                fmuld d15,d0,d6          // d9-d10 xyz.z[2]
                fmuld d16,d1,d6
                fmuld d17,d2,d6
                fmuld d18,d3,d6
                
                fstd d7,[sp,#96]
                fstd d8,[sp,#104]
                fstd d9,[sp,#112]
                fstd d10,[sp,#120]
                fstd d11,[sp,#128]
                fstd d12,[sp,#136]
                fstd d13,[sp,#144]
                fstd d14,[sp,#152]
                fstd d15,[sp,#160]
                fstd d16,[sp,#168]
                fstd d17,[sp,#176]
                fstd d18,[sp,#184]
                
                /* rg_calc */
                fldd d19,[r0,#16]       // q_inv_sqr
                fmuld d0,d7,d7
                fmuld d1,d8,d8
                fmuld d2,d9,d9
                fmuld d3,d10,d10
                fldd d4,[r6,#32]        // qw_r3_N
                fldd d5,[r6,#40]    // qw_r3_N
                fldd d6,[r6,#48]    // qw_r3_N
                fldd d20,[r6,#56]    // qw_r3_N
                fmacd d0,d11,d11
                fmacd d1,d12,d12
                fmacd d2,d13,d13
                fmacd d3,d14,d14
                fmuld d15,d15,d15
                fmuld d16,d16,d16
                fmuld d17,d17,d17
                fmuld d18,d18,d18
                fmacd d0,d15,d19
                fmacd d1,d16,d19
                fmacd d2,d17,d19
                fmacd d3,d18,d19
                fldd d21,[r0,#24]        // r0
                fsqrtd d0,d0
                fsqrtd d1,d1
                fsqrtd d2,d2
                fsqrtd d3,d3
                
                /* h_prob_fast */
                faddd d22,d21,d0
                faddd d23,d21,d1
                faddd d24,d21,d2
                faddd d25,d21,d3
                fmuld d26,d22,d22
                fmuld d27,d23,d23
                fmuld d28,d24,d24
                fmuld d29,d25,d25
                fmuld d26,d26,d22
                fmuld d27,d27,d23
                fmuld d28,d28,d24
                fmuld d29,d29,d25
                fmuld d26,d26,d0
                fmuld d27,d27,d1
                fmuld d28,d28,d2
                fmuld d29,d29,d3
                fdivd d26,d4,d26
                fdivd d27,d5,d27
                fdivd d28,d6,d28
                fdivd d29,d20,d29
                
                /* aux_bg_profile */
                fldd d24,[sp,#232+40+40+st_vfp_size]         // gprime
                fldd d0,[r2,#32]            // sg_dx
                fldd d1,[r2,#40]
                fldd d2,[r2,#48]
                fldd d3,[r2,#56]
                
                fldd d7,[r0,#72]        // ap->bg_a
                fldd d8,[r0,#80]        // ap->bg_b
                fldd d9,[r0,#80]
                fldd d10,[r0,#80]
                fldd d11,[r0,#80]
                
                faddd d0,d0,d24
                faddd d1,d1,d24
                faddd d2,d2,d24
                faddd d3,d3,d24
                
                fldd d12,[r0,#88]        // ap->bg_c
                fldd d13,[r0,#88]
                fldd d14,[r0,#88]
                fldd d15,[r0,#88]
                
                fmacd d8,d0,d7
                fmacd d9,d1,d7
                fmacd d10,d2,d7
                fmacd d11,d3,d7
                
                fmacd d12,d0,d8
                fmacd d13,d1,d9
                fmacd d14,d2,d10
                fmacd d15,d3,d11
                
                fmacd d26,d12,d4
                fmacd d27,d13,d5
                fmacd d28,d14,d6
                fmacd d29,d15,d20
                
                faddd d30,d30,d26       // bg_prop+=h_prop
                faddd d30,d30,d27
                faddd d30,d30,d28
                faddd d30,d30,d29
                
                fstd d30,[sp,#192]
                
                /*
                 * ok /-\
                 *    ||
                 */
                strd r0,[sp,#200]
                strd r2,[sp,#208]
                strd r4,[sp,#216]
                str r7,[sp,#224]
                ldr r14,[sp,#232+40+56+st_vfp_size]      // streamTmps
                str r14,[sp,#228]
.Lstreamloop1:
                /* first quad half */
                adr r12,.Llog2_3
                
                fldd d8,[sp]
                fldd d9,[sp,#8]
                fldd d10,[sp,#16]
                fldd d11,[sp,#24]
                fldd d12,[sp,#32]
                fldd d13,[sp,#40]
                fldd d14,[sp,#48]
                fldd d15,[sp,#56]
                fldd d16,[sp,#64]
                fldd d17,[sp,#72]
                fldd d18,[sp,#80]
                fldd d19,[sp,#88]
                // sc[i].a and sc[i].c
                fldd d20,[r8]
                fldd d21,[r8,#8]
                fldd d22,[r8,#16]
                fldd d23,[r8,#32]
                fldd d24,[r8,#40]
                fldd d25,[r8,#48]
                
                fsubd d8,d8,d23                  // mw_subv(xyz,sc.c)
                fsubd d9,d9,d23
                fsubd d10,d10,d23
                fsubd d11,d11,d23
                fsubd d12,d12,d24
                fsubd d13,d13,d24
                fsubd d14,d14,d24
                fsubd d15,d15,d24
                fsubd d16,d16,d25
                fsubd d17,d17,d25
                fsubd d18,d18,d25
                fsubd d19,d19,d25
                
                fmuld d0,d20,d8                 // dotted
                fmuld d1,d20,d9
                fmuld d2,d20,d10
                fmuld d3,d20,d11
                fmacd d0,d21,d12
                fmacd d1,d21,d13
                fmacd d2,d21,d14
                fmacd d3,d21,d15
                fmacd d0,d22,d16
                fmacd d1,d22,d17
                fmacd d2,d22,d18
                fmacd d3,d22,d19
                
                fnmacd d8,d0,d20        // xyzs-sc.a*dotted
                fnmacd d12,d0,d21
                fnmacd d16,d0,d22
                fnmacd d9,d1,d20
                fnmacd d13,d1,d21
                fnmacd d17,d1,d22
                fnmacd d10,d2,d20
                fnmacd d14,d2,d21
                fnmacd d18,d2,d22
                fnmacd d11,d3,d20
                fnmacd d15,d3,d21
                fnmacd d19,d3,d22
                
                fmuld d0,d8,d8          // norm
                fmuld d1,d9,d9
                fmuld d2,d10,d10
                fmuld d3,d11,d11
                fmacd d0,d12,d12
                fmacd d1,d13,d13
                fmacd d2,d14,d14
                fmacd d3,d15,d15
                fmacd d0,d16,d16
                fmacd d1,d17,d17
                fmacd d2,d18,d18
                fmacd d3,d19,d19
                
                /* second quad half */
                fldd d8,[sp,#96]
                fldd d9,[sp,#104]
                fldd d10,[sp,#112]
                fldd d11,[sp,#120]
                fldd d12,[sp,#128]
                fldd d13,[sp,#136]
                fldd d14,[sp,#144]
                fldd d15,[sp,#152]
                fldd d16,[sp,#160]
                fldd d17,[sp,#168]
                fldd d18,[sp,#176]
                fldd d19,[sp,#184]
                
                fsubd d8,d8,d23                  // mw_subv(xyz,sc.c)
                fsubd d9,d9,d23
                fsubd d10,d10,d23
                fsubd d11,d11,d23
                fsubd d12,d12,d24
                fsubd d13,d13,d24
                fsubd d14,d14,d24
                fsubd d15,d15,d24
                fsubd d16,d16,d25
                fsubd d17,d17,d25
                fsubd d18,d18,d25
                fsubd d19,d19,d25
                
                fmuld d4,d20,d8                 // dotted
                fmuld d5,d20,d9
                fmuld d6,d20,d10
                fmuld d7,d20,d11
                fmacd d4,d21,d12
                fmacd d5,d21,d13
                fmacd d6,d21,d14
                fmacd d7,d21,d15
                fmacd d4,d22,d16
                fmacd d5,d22,d17
                fmacd d6,d22,d18
                fmacd d7,d22,d19
                
                fnmacd d8,d4,d20        // xyzs-sc.a*dotted
                fnmacd d12,d4,d21
                fnmacd d16,d4,d22
                fnmacd d9,d5,d20
                fnmacd d13,d5,d21
                fnmacd d17,d5,d22
                fnmacd d10,d6,d20
                fnmacd d14,d6,d21
                fnmacd d18,d6,d22
                fnmacd d11,d7,d20
                fnmacd d15,d7,d21
                fnmacd d19,d7,d22
                
                fmuld d4,d8,d8          // norm
                fmuld d5,d9,d9
                fmuld d6,d10,d10
                fmuld d7,d11,d11
                fmacd d4,d12,d12
                fmacd d5,d13,d13
                fmacd d6,d14,d14
                fmacd d7,d15,d15
                fmacd d4,d16,d16
                fmacd d5,d17,d17
                fmacd d6,d18,d18
                fmacd d7,d19,d19
                
                /*
                 * OK ^
                 */
                
                fldd d8,[r8,#64]    // sigma_sq2_inv
                
                fnmuld d0,d0,d8
                fnmuld d1,d1,d8
                fnmuld d2,d2,d8
                fnmuld d3,d3,d8
                fnmuld d4,d4,d8
                fnmuld d5,d5,d8
                fnmuld d6,d6,d8
                fnmuld d7,d7,d8
                
                fldd d31,[r12,#16]       // invlog2
                
                /*
                 * computing exp
                 */
                
                ldr r10,.Lminexp2
                ldr r11,.Lmaxexp2
                
                fmuld d20,d0,d31            // x/log(2)
                fmuld d21,d1,d31            // x/log(2)
                fmuld d22,d2,d31            // x/log(2)
                fmuld d23,d3,d31            // x/log(2)
                fmuld d24,d4,d31            // x/log(2)
                fmuld d25,d5,d31            // x/log(2)
                fmuld d26,d6,d31            // x/log(2)
                fmuld d27,d7,d31            // x/log(2)
                
                ftosid s16,d20
                ftosid s17,d21
                ftosid s18,d22
                ftosid s19,d23
                ftosid s20,d24
                ftosid s21,d25
                ftosid s22,d26
                ftosid s23,d27
                fmrs r0,s16
                fmrs r1,s17
                fmrs r2,s18
                fmrs r3,s19
                fmrs r4,s20
                fmrs r5,s21
                fmrs r7,s22
                fmrs r9,s23
                
                fldd d20,[r12] // .Llog2
                fldd d21,[r12,#8] // .Llog2 second part
                mov r12,#0
                cmp r0,r10
                movle r12,#1
                cmp r0,r11
                movge r12,#1
                cmp r1,r10
                movle r12,#1
                cmp r1,r11
                movge r12,#1
                cmp r2,r10
                movle r12,#1
                cmp r2,r11
                movge r12,#1
                cmp r3,r10
                movle r12,#1
                cmp r3,r11
                movge r12,#1
                
                cmp r4,r10
                movle r12,#1
                cmp r4,r11
                movge r12,#1
                cmp r5,r10
                movle r12,#1
                cmp r5,r11
                movge r12,#1
                cmp r7,r10
                movle r12,#1
                cmp r7,r11
                movge r12,#1
                cmp r9,r10
                movle r12,#1
                cmp r9,r11
                movge r12,#1
                
                tst r12,r12       // if exception
                beq .Lnoexpexception1
                push {r14}
                bl .Lexpexception
                pop {r14}
                b .Lexpend1
.Lnoexpexception1:
                
                fsitod d12,s16
                fsitod d13,s17
                fsitod d14,s18
                fsitod d15,s19
                fsitod d16,s20
                fsitod d17,s21
                fsitod d18,s22
                fsitod d19,s23
                
                fnmacd d0,d20,d12           // fractional
                fnmacd d1,d20,d13           // fractional
                fnmacd d2,d20,d14           // fractional
                fnmacd d3,d20,d15           // fractional
                fnmacd d4,d20,d16           // fractional
                fnmacd d5,d20,d17           // fractional
                fnmacd d6,d20,d18           // fractional
                fnmacd d7,d20,d19           // fractional
                fnmacd d0,d21,d12           // fractional
                fnmacd d1,d21,d13           // fractional
                fnmacd d2,d21,d14           // fractional
                fnmacd d3,d21,d15           // fractional
                fnmacd d4,d21,d16           // fractional
                fnmacd d5,d21,d17           // fractional
                fnmacd d6,d21,d18           // fractional
                fnmacd d7,d21,d19           // fractional
                
                fldd d20,.Lpolyexp4
                
                // evaluate polynomial
                
                fldd d8,.Lpolyexp4+8
                fldd d9,.Lpolyexp4+8
                fldd d10,.Lpolyexp4+8
                fldd d11,.Lpolyexp4+8
                fldd d12,.Lpolyexp4+8
                fldd d13,.Lpolyexp4+8
                fldd d14,.Lpolyexp4+8
                fldd d15,.Lpolyexp4+8
                fmacd d8,d20,d0
                fmacd d9,d20,d1
                fmacd d10,d20,d2
                fmacd d11,d20,d3
                fmacd d12,d20,d4
                fmacd d13,d20,d5
                fmacd d14,d20,d6
                fmacd d15,d20,d7
                fldd d20,.Lpolyexp4+8*2
                fldd d21,.Lpolyexp4+8*2
                fldd d22,.Lpolyexp4+8*2
                fldd d23,.Lpolyexp4+8*2
                fldd d24,.Lpolyexp4+8*2
                fldd d25,.Lpolyexp4+8*2
                fldd d26,.Lpolyexp4+8*2
                fldd d27,.Lpolyexp4+8*2
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp4+8*3
                fldd d9,.Lpolyexp4+8*3
                fldd d10,.Lpolyexp4+8*3
                fldd d11,.Lpolyexp4+8*3
                fldd d12,.Lpolyexp4+8*3
                fldd d13,.Lpolyexp4+8*3
                fldd d14,.Lpolyexp4+8*3
                fldd d15,.Lpolyexp4+8*3
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp4+8*4
                fldd d21,.Lpolyexp4+8*4
                fldd d22,.Lpolyexp4+8*4
                fldd d23,.Lpolyexp4+8*4
                fldd d24,.Lpolyexp4+8*4
                fldd d25,.Lpolyexp4+8*4
                fldd d26,.Lpolyexp4+8*4
                fldd d27,.Lpolyexp4+8*4
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp4+8*5
                fldd d9,.Lpolyexp4+8*5
                fldd d10,.Lpolyexp4+8*5
                fldd d11,.Lpolyexp4+8*5
                fldd d12,.Lpolyexp4+8*5
                fldd d13,.Lpolyexp4+8*5
                fldd d14,.Lpolyexp4+8*5
                fldd d15,.Lpolyexp4+8*5
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp4+8*6
                fldd d21,.Lpolyexp4+8*6
                fldd d22,.Lpolyexp4+8*6
                fldd d23,.Lpolyexp4+8*6
                fldd d24,.Lpolyexp4+8*6
                fldd d25,.Lpolyexp4+8*6
                fldd d26,.Lpolyexp4+8*6
                fldd d27,.Lpolyexp4+8*6
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp4+8*7
                fldd d9,.Lpolyexp4+8*7
                fldd d10,.Lpolyexp4+8*7
                fldd d11,.Lpolyexp4+8*7
                fldd d12,.Lpolyexp4+8*7
                fldd d13,.Lpolyexp4+8*7
                fldd d14,.Lpolyexp4+8*7
                fldd d15,.Lpolyexp4+8*7
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp4+8*8
                fldd d21,.Lpolyexp4+8*8
                fldd d22,.Lpolyexp4+8*8
                fldd d23,.Lpolyexp4+8*8
                fldd d24,.Lpolyexp4+8*8
                fldd d25,.Lpolyexp4+8*8
                fldd d26,.Lpolyexp4+8*8
                fldd d27,.Lpolyexp4+8*8
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp4+8*9
                fldd d9,.Lpolyexp4+8*9
                fldd d10,.Lpolyexp4+8*9
                fldd d11,.Lpolyexp4+8*9
                fldd d12,.Lpolyexp4+8*9
                fldd d13,.Lpolyexp4+8*9
                fldd d14,.Lpolyexp4+8*9
                fldd d15,.Lpolyexp4+8*9
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp4+8*10
                fldd d21,.Lpolyexp4+8*10
                fldd d22,.Lpolyexp4+8*10
                fldd d23,.Lpolyexp4+8*10
                fldd d24,.Lpolyexp4+8*10
                fldd d25,.Lpolyexp4+8*10
                fldd d26,.Lpolyexp4+8*10
                fldd d27,.Lpolyexp4+8*10
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp4+8*11
                fldd d9,.Lpolyexp4+8*11
                fldd d10,.Lpolyexp4+8*11
                fldd d11,.Lpolyexp4+8*11
                fldd d12,.Lpolyexp4+8*11
                fldd d13,.Lpolyexp4+8*11
                fldd d14,.Lpolyexp4+8*11
                fldd d15,.Lpolyexp4+8*11
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                
                // apply exponent
                fmrdh r10,d8
                fmrdh r11,d9
                fmrdh r12,d10
                fmrdh r14,d11
                
                add r10,r10,r0,lsl #20
                add r11,r11,r1,lsl #20
                add r12,r12,r2,lsl #20
                add r14,r14,r3,lsl #20
                
                fmdhr d8,r10
                fmdhr d9,r11
                fmdhr d10,r12
                fmdhr d11,r14
                
                fmrdh r10,d12
                fmrdh r11,d13
                fmrdh r12,d14
                fmrdh r14,d15
                
                add r10,r10,r4,lsl #20
                add r11,r11,r5,lsl #20
                add r12,r12,r7,lsl #20
                add r14,r14,r9,lsl #20
                
                fmdhr d12,r10
                fmdhr d13,r11
                fmdhr d14,r12
                fmdhr d15,r14
                
.Lexpend1:
                /* end of streamloop */
                fldd d0,[r6]    // qw_r3_N
                fldd d1,[r6,#8]    // qw_r3_N
                fldd d2,[r6,#16]    // qw_r3_N
                fldd d3,[r6,#24]    // qw_r3_N
                fldd d4,[r6,#32]    // qw_r3_N
                fldd d5,[r6,#40]    // qw_r3_N
                fldd d6,[r6,#48]    // qw_r3_N
                fldd d7,[r6,#56]    // qw_r3_N
                // result
                
                ldr r14,[sp,#228]
                fldd d16,[r14]
                fmacd d16,d0,d8
                fmuld d17,d1,d9
                fmuld d18,d2,d10
                fmuld d19,d3,d11
                fmuld d20,d4,d12
                fmuld d21,d5,d13
                fmuld d22,d6,d14
                fmuld d23,d7,d15
                
                faddd d16,d16,d17
                faddd d16,d16,d18
                faddd d16,d16,d19
                faddd d16,d16,d20
                faddd d16,d16,d21
                faddd d16,d16,d22
                faddd d16,d16,d23
                
                fstd d16,[r14]
                add r14,r14,#8
                str r14,[sp,#228]
                
                ldr r5,[sp,#220]
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamloop1
                
                ldrd r0,[sp,#200]
                ldrd r2,[sp,#208]
                ldrd r4,[sp,#216]
                ldr r7,[sp,#224]
                
                fldd d30,[sp,#192]
                
                add r2,r2,#64
                add r3,r3,#64
                add r6,r6,#64
                cmp r6,r4
                blo .Lmainloop1
                
                b .Ltoresthandle1
                
                .align 3
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
.Lminexp2:
                .int -1022
.Lmaxexp2:
                .int 1022

.Ltoresthandle1:
                add r4,r4,#56
                
                cmp r6,r4
                beq .Lafterrestloop1
.Lrestloop1:
                mov r8,r1
                fldd d0,[r3]            // rpoint
                
                fldd d2,[sp,#232+48+st_vfp_size]        // lbt
                fldd d3,[sp,#232+56+st_vfp_size]
                fldd d4,[sp,#232+64+st_vfp_size]
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
                fldd d11,[sp,#232+40+40+st_vfp_size]         // gprime
                fldd d0,[r2]            // sg_dx
                
                faddd d0,d0,d11
                
                fldd d1,[r0,#72]        // ap->bg_a
                fldd d3,[r0,#80]        // ap->bg_b
                fldd d4,[r0,#88]        // ap->bg_c
                
                fmacd d3,d0,d1
                fmacd d4,d0,d3
                
                fmacd d12,d4,d2
                
                faddd d30,d30,d12       // bg_prop+=h_prop
                
                str r3,[sp,#104]
                str r2,[sp,#108]
                ldr r12,[sp,#232+40+56+st_vfp_size]      // streamTmps
                
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
                fldd d4,.Llog2_2
                fldd d5,.Llog2_2+8
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
.Lafterrestloop1:
                /* multiply by reff_xr_rp3 */
                fldd d0,[sp,#232+40+48+st_vfp_size]
                fmuld d30,d30,d0
                
                ldr r8,[sp,#232+40+56+st_vfp_size]      // streamTmps
                ldr r9,[r0,#36]        // nstreams
                add r9,r8,r9, lsl #3
.Lfinishloop1:
                fldd d1,[r8]
                fmuld d1,d1,d0
                fstd d1,[r8]
                add r8,#8
                cmp r8,r9
                blo .Lfinishloop1
                
                fmrrd r0,r1,d30
                add sp,sp,#232
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif                
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
                .align 3
.Lzero3:
                .double 0.0
.Llog2_2:
                .double 6.931457519531250000e-01
                .double 1.428606820309417256e-06
//.Linvlog2:
                .double 1.44269504088896340737

                .global prob_arm_vfpv3
                .type   prob_arm_vfpv3,%function
                /* bg_probability_fast_hprob for ARM architecture */
prob_arm_vfpv3:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
#ifndef SKIP_VFP_SAVE
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                sub sp,sp,#232
                
                ldr r4,[r0,#32]        // convolve
                //sub r4,r4,#1
                ldr r5,[r0,#36]        // nstreams
                ldr r9,[sp,#232+40+56+st_vfp_size]      // streamTmps
                
                lsl r8,r5,#3
                sub r8,r8,#8
                mov r6,#0
                mov r7,#0
.Lzeroloop1:
                strd r6,[r9,r8]
                subs r8,r8,#8
                bhs .Lzeroloop1
                
                ldr r6,[sp,#232+40+st_vfp_size]         // qw_r3_N
                sub r4,r4,#7
                add r4,r6,r4,lsl #3     // qw_r3_N+convolve
                add r5,r1,r5,lsl #7     // sc+nstreams
                fldd d30,.Lzero3         // bg_prop
                
                ldr r10,[r0,#44]         // aux_bg_profile
                tst r10,r10
                beq .Lprob_noaux_arm_vfpv3
                bne .Lprob_aux_arm_vfpv3

.Lprob_noaux_arm_vfpv3:
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
                fldd d2,[r3,#16]
                fldd d3,[r3,#24]
                
                fldd d4,[sp,#232+48+st_vfp_size]        // lbt
                fldd d5,[sp,#232+56+st_vfp_size]
                fldd d6,[sp,#232+64+st_vfp_size]
                fldd d7,[r0]    // m_sun_r0
                fldd d8,[r0]
                fldd d9,[r0]
                fldd d10,[r0]
                
                fmacd d7,d0,d4          // d5-d6 xyz.x[2]
                fmacd d8,d1,d4
                fmacd d9,d2,d4
                fmacd d10,d3,d4
                fmuld d11,d0,d5          // d7-d8 xyz.y[2]
                fmuld d12,d1,d5
                fmuld d13,d2,d5
                fmuld d14,d3,d5
                fmuld d15,d0,d6          // d9-d10 xyz.z[2]
                fmuld d16,d1,d6
                fmuld d17,d2,d6
                fmuld d18,d3,d6
                
                /* rg_calc */
                fldd d19,[r0,#16]       // q_inv_sqr
                fmuld d0,d7,d7
                fmuld d1,d8,d8
                fmuld d2,d9,d9
                fmuld d3,d10,d10
                fldd d4,[r6]        // qw_r3_N
                fldd d5,[r6,#8]    // qw_r3_N
                fldd d6,[r6,#16]    // qw_r3_N
                fldd d20,[r6,#24]    // qw_r3_N
                fmacd d0,d11,d11
                fmacd d1,d12,d12
                fmacd d2,d13,d13
                fmacd d3,d14,d14
                fmuld d22,d15,d15
                fmuld d23,d16,d16
                fmuld d24,d17,d17
                fmuld d25,d18,d18
                fmacd d0,d22,d19
                fmacd d1,d23,d19
                fmacd d2,d24,d19
                fmacd d3,d25,d19
                fldd d21,[r0,#24]        // r0
                fsqrtd d0,d0
                
                fstd d7,[sp]
                fstd d8,[sp,#8]
                fstd d9,[sp,#16]
                fstd d10,[sp,#24]
                fstd d11,[sp,#32]
                fstd d12,[sp,#40]
                fstd d13,[sp,#48]
                fstd d14,[sp,#56]
                fstd d15,[sp,#64]
                fstd d16,[sp,#72]
                fstd d17,[sp,#80]
                fstd d18,[sp,#88]
                
                fsqrtd d1,d1
                fsqrtd d2,d2
                fsqrtd d3,d3
                
                /* h_prob_fast */
                faddd d22,d21,d0
                faddd d23,d21,d1
                faddd d24,d21,d2
                faddd d25,d21,d3
                fmuld d26,d22,d22
                fmuld d27,d23,d23
                fmuld d28,d24,d24
                fmuld d29,d25,d25
                fmuld d26,d26,d22
                fmuld d27,d27,d23
                fmuld d28,d28,d24
                fmuld d29,d29,d25
                fmuld d26,d26,d0
                fmuld d27,d27,d1
                fmuld d28,d28,d2
                fmuld d29,d29,d3
                fdivd d26,d4,d26
                fdivd d27,d5,d27
                fdivd d28,d6,d28
                fdivd d29,d20,d29
                
                faddd d30,d30,d26
                faddd d30,d30,d27
                faddd d30,d30,d28
                faddd d30,d30,d29
                
                /* second quad half */
                /* lbr2xyz_2 */
                fldd d0,[r3,#32]            // rpoint
                fldd d1,[r3,#40]
                fldd d2,[r3,#48]
                fldd d3,[r3,#56]
                
                fldd d4,[sp,#232+48+st_vfp_size]        // lbt
                fldd d5,[sp,#232+56+st_vfp_size]
                fldd d6,[sp,#232+64+st_vfp_size]
                fldd d7,[r0]    // m_sun_r0
                fldd d8,[r0]
                fldd d9,[r0]
                fldd d10,[r0]
                
                fmacd d7,d0,d4          // d5-d6 xyz.x[2]
                fmacd d8,d1,d4
                fmacd d9,d2,d4
                fmacd d10,d3,d4
                fmuld d11,d0,d5          // d7-d8 xyz.y[2]
                fmuld d12,d1,d5
                fmuld d13,d2,d5
                fmuld d14,d3,d5
                fmuld d15,d0,d6          // d9-d10 xyz.z[2]
                fmuld d16,d1,d6
                fmuld d17,d2,d6
                fmuld d18,d3,d6
                
                /* rg_calc */
                fldd d19,[r0,#16]       // q_inv_sqr
                fmuld d0,d7,d7
                fmuld d1,d8,d8
                fmuld d2,d9,d9
                fmuld d3,d10,d10
                fldd d4,[r6,#32]        // qw_r3_N
                fldd d5,[r6,#40]    // qw_r3_N
                fldd d6,[r6,#48]    // qw_r3_N
                fldd d20,[r6,#56]    // qw_r3_N
                fmacd d0,d11,d11
                fmacd d1,d12,d12
                fmacd d2,d13,d13
                fmacd d3,d14,d14
                fmuld d22,d15,d15
                fmuld d23,d16,d16
                fmuld d24,d17,d17
                fmuld d25,d18,d18
                fmacd d0,d22,d19
                fmacd d1,d23,d19
                fmacd d2,d24,d19
                fmacd d3,d25,d19
                fldd d21,[r0,#24]        // r0
                fsqrtd d0,d0
                
                fstd d7,[sp,#96]
                fstd d8,[sp,#104]
                fstd d9,[sp,#112]
                fstd d10,[sp,#120]
                fstd d11,[sp,#128]
                fstd d12,[sp,#136]
                fstd d13,[sp,#144]
                fstd d14,[sp,#152]
                fstd d15,[sp,#160]
                fstd d16,[sp,#168]
                fstd d17,[sp,#176]
                fstd d18,[sp,#184]
                
                fsqrtd d1,d1
                fsqrtd d2,d2
                fsqrtd d3,d3
                
                /* h_prob_fast */
                faddd d22,d21,d0
                faddd d23,d21,d1
                faddd d24,d21,d2
                faddd d25,d21,d3
                fmuld d26,d22,d22
                fmuld d27,d23,d23
                fmuld d28,d24,d24
                fmuld d29,d25,d25
                fmuld d26,d26,d22
                fmuld d27,d27,d23
                fmuld d28,d28,d24
                fmuld d29,d29,d25
                fmuld d26,d26,d0
                fmuld d27,d27,d1
                fmuld d28,d28,d2
                fmuld d29,d29,d3
                fdivd d26,d4,d26
                fdivd d27,d5,d27
                fdivd d28,d6,d28
                fdivd d29,d20,d29
                
                faddd d30,d30,d26
                faddd d30,d30,d27
                faddd d30,d30,d28
                faddd d30,d30,d29
                
                fstd d30,[sp,#192]
                
                /*
                 * ok /-\
                 *    ||
                 */
                strd r0,[sp,#200]
                strd r2,[sp,#208]
                strd r4,[sp,#216]
                str r7,[sp,#224]
                ldr r14,[sp,#232+40+56+st_vfp_size]      // streamTmps
                str r14,[sp,#228]
.Lstreamloop2:
                /* first quad half */
                adr r12,.Llog2_2
                
                fldd d8,[sp]
                fldd d9,[sp,#8]
                fldd d10,[sp,#16]
                fldd d11,[sp,#24]
                fldd d12,[sp,#32]
                fldd d13,[sp,#40]
                fldd d14,[sp,#48]
                fldd d15,[sp,#56]
                fldd d16,[sp,#64]
                fldd d17,[sp,#72]
                fldd d18,[sp,#80]
                fldd d19,[sp,#88]
                // sc[i].a and sc[i].c
                fldd d20,[r8]
                fldd d21,[r8,#8]
                fldd d22,[r8,#16]
                fldd d23,[r8,#32]
                fldd d24,[r8,#40]
                fldd d25,[r8,#48]
                
                fsubd d8,d8,d23                  // mw_subv(xyz,sc.c)
                fsubd d9,d9,d23
                fsubd d10,d10,d23
                fsubd d11,d11,d23
                fsubd d12,d12,d24
                fsubd d13,d13,d24
                fsubd d14,d14,d24
                fsubd d15,d15,d24
                fsubd d16,d16,d25
                fsubd d17,d17,d25
                fsubd d18,d18,d25
                fsubd d19,d19,d25
                
                fmuld d0,d20,d8                 // dotted
                fmuld d1,d20,d9
                fmuld d2,d20,d10
                fmuld d3,d20,d11
                fmacd d0,d21,d12
                fmacd d1,d21,d13
                fmacd d2,d21,d14
                fmacd d3,d21,d15
                fmacd d0,d22,d16
                fmacd d1,d22,d17
                fmacd d2,d22,d18
                fmacd d3,d22,d19
                
                fnmacd d8,d0,d20        // xyzs-sc.a*dotted
                fnmacd d12,d0,d21
                fnmacd d16,d0,d22
                fnmacd d9,d1,d20
                fnmacd d13,d1,d21
                fnmacd d17,d1,d22
                fnmacd d10,d2,d20
                fnmacd d14,d2,d21
                fnmacd d18,d2,d22
                fnmacd d11,d3,d20
                fnmacd d15,d3,d21
                fnmacd d19,d3,d22
                
                fmuld d0,d8,d8          // norm
                fmuld d1,d9,d9
                fmuld d2,d10,d10
                fmuld d3,d11,d11
                fmacd d0,d12,d12
                fmacd d1,d13,d13
                fmacd d2,d14,d14
                fmacd d3,d15,d15
                fmacd d0,d16,d16
                fmacd d1,d17,d17
                fmacd d2,d18,d18
                fmacd d3,d19,d19
                
                /* second quad half */
                fldd d8,[sp,#96]
                fldd d9,[sp,#104]
                fldd d10,[sp,#112]
                fldd d11,[sp,#120]
                fldd d12,[sp,#128]
                fldd d13,[sp,#136]
                fldd d14,[sp,#144]
                fldd d15,[sp,#152]
                fldd d16,[sp,#160]
                fldd d17,[sp,#168]
                fldd d18,[sp,#176]
                fldd d19,[sp,#184]
                
                fsubd d8,d8,d23                  // mw_subv(xyz,sc.c)
                fsubd d9,d9,d23
                fsubd d10,d10,d23
                fsubd d11,d11,d23
                fsubd d12,d12,d24
                fsubd d13,d13,d24
                fsubd d14,d14,d24
                fsubd d15,d15,d24
                fsubd d16,d16,d25
                fsubd d17,d17,d25
                fsubd d18,d18,d25
                fsubd d19,d19,d25
                
                fmuld d4,d20,d8                 // dotted
                fmuld d5,d20,d9
                fmuld d6,d20,d10
                fmuld d7,d20,d11
                fmacd d4,d21,d12
                fmacd d5,d21,d13
                fmacd d6,d21,d14
                fmacd d7,d21,d15
                fmacd d4,d22,d16
                fmacd d5,d22,d17
                fmacd d6,d22,d18
                fmacd d7,d22,d19
                
                fnmacd d8,d4,d20        // xyzs-sc.a*dotted
                fnmacd d12,d4,d21
                fnmacd d16,d4,d22
                fnmacd d9,d5,d20
                fnmacd d13,d5,d21
                fnmacd d17,d5,d22
                fnmacd d10,d6,d20
                fnmacd d14,d6,d21
                fnmacd d18,d6,d22
                fnmacd d11,d7,d20
                fnmacd d15,d7,d21
                fnmacd d19,d7,d22
                
                fmuld d4,d8,d8          // norm
                fmuld d5,d9,d9
                fmuld d6,d10,d10
                fmuld d7,d11,d11
                fmacd d4,d12,d12
                fmacd d5,d13,d13
                fmacd d6,d14,d14
                fmacd d7,d15,d15
                fmacd d4,d16,d16
                fmacd d5,d17,d17
                fmacd d6,d18,d18
                fmacd d7,d19,d19
                
                /*
                 * OK ^
                 */
                
                fldd d8,[r8,#64]    // sigma_sq2_inv
                
                fnmuld d0,d0,d8
                fnmuld d1,d1,d8
                fnmuld d2,d2,d8
                fnmuld d3,d3,d8
                fnmuld d4,d4,d8
                fnmuld d5,d5,d8
                fnmuld d6,d6,d8
                fnmuld d7,d7,d8
                
                fldd d31,[r12,#16]       // invlog2
                
                /*
                 * computing exp
                 */
                
                ldr r10,.Lminexp
                ldr r11,.Lmaxexp
                
                fmuld d20,d0,d31            // x/log(2)
                fmuld d21,d1,d31            // x/log(2)
                fmuld d22,d2,d31            // x/log(2)
                fmuld d23,d3,d31            // x/log(2)
                fmuld d24,d4,d31            // x/log(2)
                fmuld d25,d5,d31            // x/log(2)
                fmuld d26,d6,d31            // x/log(2)
                fmuld d27,d7,d31            // x/log(2)
                
                ftosid s16,d20
                ftosid s17,d21
                ftosid s18,d22
                ftosid s19,d23
                ftosid s20,d24
                ftosid s21,d25
                ftosid s22,d26
                ftosid s23,d27
                fmrs r0,s16
                fmrs r1,s17
                fmrs r2,s18
                fmrs r3,s19
                fmrs r4,s20
                fmrs r5,s21
                fmrs r7,s22
                fmrs r9,s23
                
                fldd d20,[r12] // .Llog2
                fldd d21,[r12,#8] // .Llog2
                mov r12,#0
                cmp r0,r10
                movle r12,#1
                cmp r0,r11
                movge r12,#1
                cmp r1,r10
                movle r12,#1
                cmp r1,r11
                movge r12,#1
                cmp r2,r10
                movle r12,#1
                cmp r2,r11
                movge r12,#1
                cmp r3,r10
                movle r12,#1
                cmp r3,r11
                movge r12,#1
                
                cmp r4,r10
                movle r12,#1
                cmp r4,r11
                movge r12,#1
                cmp r5,r10
                movle r12,#1
                cmp r5,r11
                movge r12,#1
                cmp r7,r10
                movle r12,#1
                cmp r7,r11
                movge r12,#1
                cmp r9,r10
                movle r12,#1
                cmp r9,r11
                movge r12,#1
                
                tst r12,r12       // if exception
                beq .Lnoexpexception2
                push {r14}
                bl .Lexpexception
                pop {r14}
                b .Lexpend2
.Lnoexpexception2:
                
                fsitod d12,s16
                fsitod d13,s17
                fsitod d14,s18
                fsitod d15,s19
                fsitod d16,s20
                fsitod d17,s21
                fsitod d18,s22
                fsitod d19,s23
                
                fnmacd d0,d20,d12           // fractional
                fnmacd d1,d20,d13           // fractional
                fnmacd d2,d20,d14           // fractional
                fnmacd d3,d20,d15           // fractional
                fnmacd d4,d20,d16           // fractional
                fnmacd d5,d20,d17           // fractional
                fnmacd d6,d20,d18           // fractional
                fnmacd d7,d20,d19           // fractional
                fnmacd d0,d21,d12           // fractional
                fnmacd d1,d21,d13           // fractional
                fnmacd d2,d21,d14           // fractional
                fnmacd d3,d21,d15           // fractional
                fnmacd d4,d21,d16           // fractional
                fnmacd d5,d21,d17           // fractional
                fnmacd d6,d21,d18           // fractional
                fnmacd d7,d21,d19           // fractional
                
                fldd d20,.Lpolyexp2
                
                // evaluate polynomial
                
                fldd d8,.Lpolyexp2+8
                fldd d9,.Lpolyexp2+8
                fldd d10,.Lpolyexp2+8
                fldd d11,.Lpolyexp2+8
                fldd d12,.Lpolyexp2+8
                fldd d13,.Lpolyexp2+8
                fldd d14,.Lpolyexp2+8
                fldd d15,.Lpolyexp2+8
                fmacd d8,d20,d0
                fmacd d9,d20,d1
                fmacd d10,d20,d2
                fmacd d11,d20,d3
                fmacd d12,d20,d4
                fmacd d13,d20,d5
                fmacd d14,d20,d6
                fmacd d15,d20,d7
                fldd d20,.Lpolyexp2+8*2
                fldd d21,.Lpolyexp2+8*2
                fldd d22,.Lpolyexp2+8*2
                fldd d23,.Lpolyexp2+8*2
                fldd d24,.Lpolyexp2+8*2
                fldd d25,.Lpolyexp2+8*2
                fldd d26,.Lpolyexp2+8*2
                fldd d27,.Lpolyexp2+8*2
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp2+8*3
                fldd d9,.Lpolyexp2+8*3
                fldd d10,.Lpolyexp2+8*3
                fldd d11,.Lpolyexp2+8*3
                fldd d12,.Lpolyexp2+8*3
                fldd d13,.Lpolyexp2+8*3
                fldd d14,.Lpolyexp2+8*3
                fldd d15,.Lpolyexp2+8*3
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp2+8*4
                fldd d21,.Lpolyexp2+8*4
                fldd d22,.Lpolyexp2+8*4
                fldd d23,.Lpolyexp2+8*4
                fldd d24,.Lpolyexp2+8*4
                fldd d25,.Lpolyexp2+8*4
                fldd d26,.Lpolyexp2+8*4
                fldd d27,.Lpolyexp2+8*4
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp2+8*5
                fldd d9,.Lpolyexp2+8*5
                fldd d10,.Lpolyexp2+8*5
                fldd d11,.Lpolyexp2+8*5
                fldd d12,.Lpolyexp2+8*5
                fldd d13,.Lpolyexp2+8*5
                fldd d14,.Lpolyexp2+8*5
                fldd d15,.Lpolyexp2+8*5
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp2+8*6
                fldd d21,.Lpolyexp2+8*6
                fldd d22,.Lpolyexp2+8*6
                fldd d23,.Lpolyexp2+8*6
                fldd d24,.Lpolyexp2+8*6
                fldd d25,.Lpolyexp2+8*6
                fldd d26,.Lpolyexp2+8*6
                fldd d27,.Lpolyexp2+8*6
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp2+8*7
                fldd d9,.Lpolyexp2+8*7
                fldd d10,.Lpolyexp2+8*7
                fldd d11,.Lpolyexp2+8*7
                fldd d12,.Lpolyexp2+8*7
                fldd d13,.Lpolyexp2+8*7
                fldd d14,.Lpolyexp2+8*7
                fldd d15,.Lpolyexp2+8*7
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp2+8*8
                fldd d21,.Lpolyexp2+8*8
                fldd d22,.Lpolyexp2+8*8
                fldd d23,.Lpolyexp2+8*8
                fldd d24,.Lpolyexp2+8*8
                fldd d25,.Lpolyexp2+8*8
                fldd d26,.Lpolyexp2+8*8
                fldd d27,.Lpolyexp2+8*8
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp2+8*9
                fldd d9,.Lpolyexp2+8*9
                fldd d10,.Lpolyexp2+8*9
                fldd d11,.Lpolyexp2+8*9
                fldd d12,.Lpolyexp2+8*9
                fldd d13,.Lpolyexp2+8*9
                fldd d14,.Lpolyexp2+8*9
                fldd d15,.Lpolyexp2+8*9
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                fldd d20,.Lpolyexp2+8*10
                fldd d21,.Lpolyexp2+8*10
                fldd d22,.Lpolyexp2+8*10
                fldd d23,.Lpolyexp2+8*10
                fldd d24,.Lpolyexp2+8*10
                fldd d25,.Lpolyexp2+8*10
                fldd d26,.Lpolyexp2+8*10
                fldd d27,.Lpolyexp2+8*10
                fmacd d20,d8,d0
                fmacd d21,d9,d1
                fmacd d22,d10,d2
                fmacd d23,d11,d3
                fmacd d24,d12,d4
                fmacd d25,d13,d5
                fmacd d26,d14,d6
                fmacd d27,d15,d7
                fldd d8,.Lpolyexp2+8*11
                fldd d9,.Lpolyexp2+8*11
                fldd d10,.Lpolyexp2+8*11
                fldd d11,.Lpolyexp2+8*11
                fldd d12,.Lpolyexp2+8*11
                fldd d13,.Lpolyexp2+8*11
                fldd d14,.Lpolyexp2+8*11
                fldd d15,.Lpolyexp2+8*11
                fmacd d8,d20,d0
                fmacd d9,d21,d1
                fmacd d10,d22,d2
                fmacd d11,d23,d3
                fmacd d12,d24,d4
                fmacd d13,d25,d5
                fmacd d14,d26,d6
                fmacd d15,d27,d7
                
                // apply exponent
                fmrdh r10,d8
                fmrdh r11,d9
                fmrdh r12,d10
                fmrdh r14,d11
                
                add r10,r10,r0,lsl #20
                add r11,r11,r1,lsl #20
                add r12,r12,r2,lsl #20
                add r14,r14,r3,lsl #20
                
                fmdhr d8,r10
                fmdhr d9,r11
                fmdhr d10,r12
                fmdhr d11,r14
                
                fmrdh r10,d12
                fmrdh r11,d13
                fmrdh r12,d14
                fmrdh r14,d15
                
                add r10,r10,r4,lsl #20
                add r11,r11,r5,lsl #20
                add r12,r12,r7,lsl #20
                add r14,r14,r9,lsl #20
                
                fmdhr d12,r10
                fmdhr d13,r11
                fmdhr d14,r12
                fmdhr d15,r14
                
.Lexpend2:
                /* end of streamloop */
                fldd d0,[r6]    // qw_r3_N
                fldd d1,[r6,#8]    // qw_r3_N
                fldd d2,[r6,#16]    // qw_r3_N
                fldd d3,[r6,#24]    // qw_r3_N
                fldd d4,[r6,#32]    // qw_r3_N
                fldd d5,[r6,#40]    // qw_r3_N
                fldd d6,[r6,#48]    // qw_r3_N
                fldd d7,[r6,#56]    // qw_r3_N
                // result
                
                ldr r14,[sp,#228]
                fldd d16,[r14]
                fmacd d16,d0,d8
                fmuld d17,d1,d9
                fmuld d18,d2,d10
                fmuld d19,d3,d11
                fmuld d20,d4,d12
                fmuld d21,d5,d13
                fmuld d22,d6,d14
                fmuld d23,d7,d15
                
                faddd d16,d16,d17
                faddd d16,d16,d18
                faddd d16,d16,d19
                faddd d16,d16,d20
                faddd d16,d16,d21
                faddd d16,d16,d22
                faddd d16,d16,d23
                
                fstd d16,[r14]
                add r14,r14,#8
                str r14,[sp,#228]
                
                ldr r5,[sp,#220]
                add r8,r8,#128
                cmp r8,r5
                blo .Lstreamloop2
                
                ldrd r0,[sp,#200]
                ldrd r2,[sp,#208]
                ldrd r4,[sp,#216]
                ldr r7,[sp,#224]
                
                fldd d30,[sp,#192]
                
                add r3,r3,#64
                add r6,r6,#64
                cmp r6,r4
                blo .Lmainloop2
                
                b .Ltoresthandle2
                
                
                .align 3
.Lfbase:
.Lzero:
                .double 0.0
.Linf:
                .quad 0x7ff0000000000000
.Llog2:
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

.Lminexp:
                .int -1022
.Lmaxexp:
                .int 1022
.Ltoresthandle2:
                
                add r4,r4,#56
                
                cmp r6,r4
                beq .Lafterrestloop2
                
.Lrestloop2:
                mov r8,r1
                fldd d0,[r3]            // rpoint
                
                fldd d2,[sp,#232+48+st_vfp_size]        // lbt
                fldd d3,[sp,#232+56+st_vfp_size]
                fldd d4,[sp,#232+64+st_vfp_size]
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
                
                faddd d30,d30,d12       // bg_prop+=h_prop
                
                str r3,[sp,#104]
                str r2,[sp,#108]
                ldr r12,[sp,#232+40+56+st_vfp_size]      // streamTmps
                
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
                fldd d12,.Lzero
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
                fldd d4,.Lpolyexp2
                
                fldd d8,.Lpolyexp2+8
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*2
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*3
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*4
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*5
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*6
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*7
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*8
                fmacd d12,d8,d0
                fldd d4,.Lpolyexp2+8*9
                fmacd d4,d12,d0
                fldd d8,.Lpolyexp2+8*10
                fmacd d8,d4,d0
                fldd d12,.Lpolyexp2+8*11
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
.Lafterrestloop2:
                /* multiply by reff_xr_rp3 */
                fldd d0,[sp,#232+40+48+st_vfp_size]
                fmuld d30,d30,d0
                
                ldr r8,[sp,#232+40+56+st_vfp_size]      // streamTmps
                ldr r9,[r0,#36]        // nstreams
                add r9,r8,r9, lsl #3
.Lfinishloop2:
                fldd d1,[r8]
                fmuld d1,d1,d0
                fstd d1,[r8]
                add r8,#8
                cmp r8,r9
                blo .Lfinishloop2
                
                fmrrd r0,r1,d30
                add sp,sp,#232
#ifndef SKIP_VFP_SAVE
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
#endif
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
.Lexpexception:
                push {r8}
                adr r8,.Lfbase
                
                cmp r0,r10
                bgt .Lnolow1
                fldd d12,[r8]      // zero
                b .Ltoexp2
.Lnolow1:
                cmp r0,r11
                blt .Lcomputeexp1
                fldd d12,[r8,#8]      // inf
                b .Ltoexp2
.Lcomputeexp1:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d12,s16
                fnmacd d0,d20,d12           // fractional
                fnmacd d0,d21,d12           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d12,[r8,#40+8]      // polyexp
                fmacd d12,d20,d0
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d12,d0
                fldd d12,[r8,#40+8*3]      // polyexp
                fmacd d12,d20,d0
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d12,d0
                fldd d12,[r8,#40+8*5]      // polyexp
                fmacd d12,d20,d0
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d12,d0
                fldd d12,[r8,#40+8*7]      // polyexp
                fmacd d12,d20,d0
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d12,d0
                fldd d12,[r8,#40+8*9]      // polyexp
                fmacd d12,d20,d0
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d12,d0
                fldd d12,[r8,#40+8*11]      // polyexp
                fmacd d12,d20,d0
                
                // apply exponent
                fmrdh r12,d12
                add r12,r12,r0,lsl #20
                fmdhr d12,r12
.Ltoexp2:
                cmp r1,r10
                bgt .Lnolow2
                fldd d13,[r8]      // zero
                b .Ltoexp3
.Lnolow2:
                cmp r1,r11
                blt .Lcomputeexp2
                fldd d13,[r8,#8]      // inf
                b .Ltoexp3
.Lcomputeexp2:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d13,s17
                fnmacd d1,d20,d13           // fractional
                fnmacd d1,d21,d13           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d13,[r8,#40+8]      // polyexp
                fmacd d13,d20,d1
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d13,d1
                fldd d13,[r8,#40+8*3]      // polyexp
                fmacd d13,d20,d1
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d13,d1
                fldd d13,[r8,#40+8*5]      // polyexp
                fmacd d13,d20,d1
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d13,d1
                fldd d13,[r8,#40+8*7]      // polyexp
                fmacd d13,d20,d1
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d13,d1
                fldd d13,[r8,#40+8*9]      // polyexp
                fmacd d13,d20,d1
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d13,d1
                fldd d13,[r8,#40+8*11]      // polyexp
                fmacd d13,d20,d1
                
                // apply exponent
                fmrdh r12,d13
                add r12,r12,r1,lsl #20
                fmdhr d13,r12
.Ltoexp3:
                cmp r2,r10
                bgt .Lnolow3
                fldd d14,[r8]      // zero
                b .Ltoexp4
.Lnolow3:
                cmp r2,r11
                blt .Lcomputeexp3
                fldd d14,[r8,#8]      // inf
                b .Ltoexp4
.Lcomputeexp3:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d14,s18
                fnmacd d2,d20,d14           // fractional
                fnmacd d2,d21,d14           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d14,[r8,#40+8]      // polyexp
                fmacd d14,d20,d2
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d14,d2
                fldd d14,[r8,#40+8*3]      // polyexp
                fmacd d14,d20,d2
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d14,d2
                fldd d14,[r8,#40+8*5]      // polyexp
                fmacd d14,d20,d2
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d14,d2
                fldd d14,[r8,#40+8*7]      // polyexp
                fmacd d14,d20,d2
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d14,d2
                fldd d14,[r8,#40+8*9]      // polyexp
                fmacd d14,d20,d2
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d14,d2
                fldd d14,[r8,#40+8*11]      // polyexp
                fmacd d14,d20,d2
                
                // apply exponent
                fmrdh r12,d14
                add r12,r12,r2,lsl #20
                fmdhr d14,r12
.Ltoexp4:
                cmp r3,r10
                bgt .Lnolow4
                fldd d15,[r8]      // zero
                b .Ltoexp5
.Lnolow4:
                cmp r3,r11
                blt .Lcomputeexp4
                fldd d15,[r8,#8]      // inf
                b .Ltoexp5
.Lcomputeexp4:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d15,s19
                fnmacd d3,d20,d15           // fractional
                fnmacd d3,d21,d15           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d15,[r8,#40+8]      // polyexp
                fmacd d15,d20,d3
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d15,d3
                fldd d15,[r8,#40+8*3]      // polyexp
                fmacd d15,d20,d3
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d15,d3
                fldd d15,[r8,#40+8*5]      // polyexp
                fmacd d15,d20,d3
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d15,d3
                fldd d15,[r8,#40+8*7]      // polyexp
                fmacd d15,d20,d3
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d15,d3
                fldd d15,[r8,#40+8*9]      // polyexp
                fmacd d15,d20,d3
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d15,d3
                fldd d15,[r8,#40+8*11]      // polyexp
                fmacd d15,d20,d3
                
                // apply exponent
                fmrdh r12,d15
                add r12,r12,r3,lsl #20
                fmdhr d15,r12
.Ltoexp5:
                cmp r4,r10
                bgt .Lnolow5
                fldd d16,[r8]      // zero
                b .Ltoexp6
.Lnolow5:
                cmp r4,r11
                blt .Lcomputeexp5
                fldd d16,[r8,#8]      // inf
                b .Ltoexp6
.Lcomputeexp5:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d16,s20
                fnmacd d4,d20,d16           // fractional
                fnmacd d4,d21,d16           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d16,[r8,#40+8]      // polyexp
                fmacd d16,d20,d4
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d16,d4
                fldd d16,[r8,#40+8*3]      // polyexp
                fmacd d16,d20,d4
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d16,d4
                fldd d16,[r8,#40+8*5]      // polyexp
                fmacd d16,d20,d4
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d16,d4
                fldd d16,[r8,#40+8*7]      // polyexp
                fmacd d16,d20,d4
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d16,d4
                fldd d16,[r8,#40+8*9]      // polyexp
                fmacd d16,d20,d4
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d16,d4
                fldd d16,[r8,#40+8*11]      // polyexp
                fmacd d16,d20,d4
                
                // apply exponent
                fmrdh r12,d16
                add r12,r12,r4,lsl #20
                fmdhr d16,r12
.Ltoexp6:
                cmp r5,r10
                bgt .Lnolow6
                fldd d17,[r8]      // zero
                b .Ltoexp7
.Lnolow6:
                cmp r5,r11
                blt .Lcomputeexp6
                fldd d17,[r8,#8]      // inf
                b .Ltoexp7
.Lcomputeexp6:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d17,s21
                fnmacd d5,d20,d17           // fractional
                fnmacd d5,d21,d17           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d17,[r8,#40+8]      // polyexp
                fmacd d17,d20,d5
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d17,d5
                fldd d17,[r8,#40+8*3]      // polyexp
                fmacd d17,d20,d5
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d17,d5
                fldd d17,[r8,#40+8*5]      // polyexp
                fmacd d17,d20,d5
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d17,d5
                fldd d17,[r8,#40+8*7]      // polyexp
                fmacd d17,d20,d5
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d17,d5
                fldd d17,[r8,#40+8*9]      // polyexp
                fmacd d17,d20,d5
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d17,d5
                fldd d17,[r8,#40+8*11]      // polyexp
                fmacd d17,d20,d5
                
                // apply exponent
                fmrdh r12,d17
                add r12,r12,r5,lsl #20
                fmdhr d17,r12
.Ltoexp7:
                cmp r7,r10
                bgt .Lnolow7
                fldd d18,[r8]      // zero
                b .Ltoexp8
.Lnolow7:
                cmp r7,r11
                blt .Lcomputeexp7
                fldd d18,[r8,#8]      // inf
                b .Ltoexp8
.Lcomputeexp7:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#24]      // log2
                fsitod d18,s22
                fnmacd d6,d20,d18           // fractional
                fnmacd d6,d21,d18           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d18,[r8,#40+8]      // polyexp
                fmacd d18,d20,d6
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d18,d6
                fldd d18,[r8,#40+8*3]      // polyexp
                fmacd d18,d20,d6
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d18,d6
                fldd d18,[r8,#40+8*5]      // polyexp
                fmacd d18,d20,d6
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d18,d6
                fldd d18,[r8,#40+8*7]      // polyexp
                fmacd d18,d20,d6
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d18,d6
                fldd d18,[r8,#40+8*9]      // polyexp
                fmacd d18,d20,d6
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d18,d6
                fldd d18,[r8,#40+8*11]      // polyexp
                fmacd d18,d20,d6
                
                // apply exponent
                fmrdh r12,d18
                add r12,r12,r7,lsl #20
                fmdhr d18,r12
.Ltoexp8:
                cmp r9,r10
                bgt .Lnolow8
                fldd d19,[r8]      // zero
                b .Lexpexceptend
.Lnolow8:
                cmp r9,r11
                blt .Lcomputeexp8
                fldd d19,[r8,#8]      // inf
                b .Lexpexceptend
.Lcomputeexp8:
                fldd d20,[r8,#16]      // log2
                fldd d21,[r8,#16]      // log2
                fsitod d19,s23
                fnmacd d7,d20,d19           // fractional
                fnmacd d7,d21,d19           // fractional
                
                fldd d20,[r8,#40]      // polyexp
                // evaluate polynomial
                
                fldd d19,[r8,#40+8]      // polyexp
                fmacd d19,d20,d7
                fldd d20,[r8,#40+8*2]      // polyexp
                fmacd d20,d19,d7
                fldd d19,[r8,#40+8*3]      // polyexp
                fmacd d19,d20,d7
                fldd d20,[r8,#40+8*4]      // polyexp
                fmacd d20,d19,d7
                fldd d19,[r8,#40+8*5]      // polyexp
                fmacd d19,d20,d7
                fldd d20,[r8,#40+8*6]      // polyexp
                fmacd d20,d19,d7
                fldd d19,[r8,#40+8*7]      // polyexp
                fmacd d19,d20,d7
                fldd d20,[r8,#40+8*8]      // polyexp
                fmacd d20,d19,d7
                fldd d19,[r8,#40+8*9]      // polyexp
                fmacd d19,d20,d7
                fldd d20,[r8,#40+8*10]      // polyexp
                fmacd d20,d19,d7
                fldd d19,[r8,#40+8*11]      // polyexp
                fmacd d19,d20,d7
                
                // apply exponent
                fmrdh r12,d19
                add r12,r12,r9,lsl #20
                fmdhr d19,r12
.Lexpexceptend:
                fcpyd d8,d12
                fcpyd d9,d13
                fcpyd d10,d14
                fcpyd d11,d15
                fcpyd d12,d16
                fcpyd d13,d17
                fcpyd d14,d18
                fcpyd d15,d19
                pop {r8}
                bx lr
