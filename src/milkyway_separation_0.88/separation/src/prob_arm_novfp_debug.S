/*
 * prob_arm_novfp.S
 * Mateusz Szpakowski
 * License: GPLv3
 */
                .arch armv6
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
//// PROBNOVFP_DEBUG
#ifdef PROB_DEBUG
.Lprint_ifp_dbl:
                push {r0,r1,r2,r3,r12,lr}
                sub sp,sp,#20
                
                str r0,[sp,#8]
                strd r2,[sp]
                
                mov r0,sp
                add r1,sp,#12
                bl intfp_to_fp(PLT)
                
                ldr r1,[sp,#12]
                ldr r2,[sp,#16]
                adr r0,.Lprint_ifp_dbl_str
                
                bl printf(PLT)
                
                add sp,sp,#20
                pop {r0,r1,r2,r3,r12,lr}
                bx lr
                
.Lprint_ifp_dbl_str:
                .string "%1.18e,  #\n"
                .align 3
.Ltest1:
                .word 0xbff00000
                .word 0
                
                .align 3
.Lprint_dbl:
                push {r0,r1,r2,r3,r12,lr}
                
                mov r3,r1
                mov r2,r0
                
                adr r0,.Lprint_dbl_str
                bl printf(PLT)
                
                pop {r0,r1,r2,r3,r12,lr}
                bx lr

.Lprint_dbl_str:
                .string "%1.18e\n"
                .align 3
.Lputs_sep:
                push {r0,r1,r2,r3,r12,lr}
                
                tst r11,r11
                adreq r0,.Lconvolve_str
                adrne r0,.Lstream_str
                bl puts(PLT)
                
                pop {r0,r1,r2,r3,r12,lr}
                bx lr
.Lconvolve_str:
                .string "############# convolve"
                .align 2
.Lstream_str:
                .string "############# stream"
                
                .align 3
.Lprint_word:
                push {r0,r1,r2,r3,r12,lr}
                
                mov r1,r0
                adr r0,.Lprint_word_str
                bl printf(PLT)
                
                pop {r0,r1,r2,r3,r12,lr}
                bx lr
.Lprint_word_str:
                .string "word:%08x\n"
#endif
//// PROBNOVFP_DEBUG
                .align 4
                //  a={ r2,r3,r0 }, b={ r4,r5,r1 }
.Lifpadd_int:
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale
.Ltoadd:
                cmp r7,#64
                orrhs r7,r7,#64
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                teq r0,r1       // test sign equality
                bmi .Ldiffsigns
                // all positive
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
                b .Laddend
.Ldiffsigns:
                subs r2,r2,r4
                sbcs r3,r3,r5
                movcs r1,r0     // dont change sign
                bcs .Lnoborrow
                rsbs r2,r2,#0    // negate
                rsc r3,r3,#0
.Lnoborrow:
                orrs r9,r2,r3
                bne .Lnozero
                mov r0,#0x8000    // positive,smallest mantisa
                b .Lsavefirst
.Lnozero:
                // do shift out
                tst r3,r3
                beq .Lshiftover32
                clz r9,r3
                rsbs r10,r9,#32
                lsl r3,r3,r9
                orr r3,r3,r2,lsr r10
                lsl r2,r2,r9
                sub r0,r0,r9    // subtract from mantisa
                b .Laddend
.Lshiftover32:
                clz r9,r2
                lsl r3,r2,r9
                mov r2,#0
                sub r0,r0,r9    // subtract from mantisa
                sub r0,r0,#32
.Laddend:
                pkhbt r0,r0,r1
.Lsavefirst:
                /* result: { r2,r3,r0} */
                bx lr

.Lmantisale:
                rsb r7,r7,#0    // negate
                
                cmp r7,#64
                orrhs r7,r7,#64
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                teq r1,r0       // test sign equality
                bmi .Ldiffsigns2
                // all positive
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
                b .Laddend2
.Ldiffsigns2:
                subs r4,r4,r2
                sbcs r5,r5,r3
                movcs r0,r1     // dont change sign
                bcs .Lnoborrow2
                rsbs r4,r4,#0    // negate
                rsc r5,r5,#0
.Lnoborrow2:
                orrs r9,r4,r5
                bne .Lnozero2
                mov r1,#0x8000    // positive,smallest mantisa
                mov r2,r4
                mov r3,r5
                b .Lsavefirst2
.Lnozero2:
                // do shift out
                tst r5,r5
                beq .Lshiftover32_2
                clz r9,r5
                rsbs r10,r9,#32
                lsl r3,r5,r9
                orr r3,r3,r4,lsr r10
                lsl r2,r4,r9
                sub r1,r1,r9    // subtract from mantisa
                b .Laddend2
.Lshiftover32_2:
                clz r9,r4
                lsl r3,r4,r9
                mov r2,#0
                sub r1,r1,r9    // subtract from mantisa
                sub r1,r1,#32
.Laddend2:
                pkhbt r0,r1,r0
.Lsavefirst2:
                /* result: { r2,r3,r0} */
                bx lr
                
.Lmask:
                .word 0x7ff00000
.Lsignmask:
                .word 0xffff0000
.Linf:
                .word 0xffff
                
#define S_reg 0
#define S_m_sun_r0_F 16
#define S_m_sun_r0_E 24
#define S_q_inv_r0_F 32
#define S_q_inv_r0_E 28
#define S_r0_F 40
#define S_r0_E 48
#define S_r4_loop 52
#define S_bg_a_F 56
#define S_bg_a_E 64
#define S_bg_b_F 72
#define S_bg_b_E 68
#define S_bg_c_F 80
#define S_bg_c_E 88

#define S_xyz_x_F 96
#define S_xyz_x_E 104
#define S_xyz_y_F 112
#define S_xyz_y_E 108
#define S_xyz_z_F 120
#define S_xyz_z_E 128
#define S_r10_loop 132
#define S_tmpreg   136
#define S_xyzs_x_F 136+8
#define S_xyzs_x_E 144+8
#define S_xyzs_y_F 152+8
#define S_xyzs_y_E 148+8
#define S_xyzs_z_F 160+8
#define S_xyzs_z_E 168+8
#define S_auxbg 172+8
#define S_oldrg_F 136+8
#define S_oldrg_E 144+8
#define S_bgprob_F 176+8
#define S_bgprob_E 184+8
#define S_tmpx_F 192+8
#define S_tmpx_E 188+8
#define S_tmpy_F 200+8
#define S_tmpy_E 208+8
#define S_hprob_F 136+8
#define S_hprob_E 144+8
#define S_tmpg_F 152+8
#define S_tmpg_E 148+8
/// streamTmps
#define S_sti 212+8
#define S_stend 216+8
#define S_invlog2 240+8
/// sc
#define S_sci 220+8
#define S_gPrime_F 228+8
#define S_gPrime_E 236+8

#define S_size 248+8

#define ap_m_sun_r0_intfp 152
#define ap_q_inv_sqr_intfp 164
#define ap_r0_intfp 176
#define ap_bg_a_intfp 188
#define ap_bg_b_intfp 200
#define ap_bg_c_intfp 212

#define sc_a_x 0
#define sc_a_y 12
#define sc_a_z 24
#define sc_c_x 48
#define sc_c_y 60
#define sc_c_z 72
#define sc_sigma2_sq_inv 96

                .align 3
.Linvlog2:
                .quad 13306513097844322491
                
                .global prob_arm_novfp_debug
                .type   prob_arm_novfp_debug,%function
prob_arm_novfp_debug:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                
                /* r0 - ap,
                   r1 - sc,
                   r2 - sg_dx,
                   r3 - r_point
                   sp - qw_r3_n 
                   sp+4 - lbt
                   sp+8 - gPrime
                   sp+16 - reff_xr_rp3
                   sp+24 - streamTmps
                   sp+28 - streamTmpsIntFp */
                sub sp,sp,#S_size
                
                ldr r4,[r0,#32]        // convolve
                ldr r5,[r0,#36]        // nstreams
                ldr r9,[sp,#S_size+40+28]      // streamTmps
                
                add r8,r9,r5, lsl #3
                add r8,r8,r5, lsl #2
                mov r6,#0
                mov r7,#0
                mov r10,#0x8000
.Lzeroloop1:
                strd r6,[r9]
                str r10,[r9,#8]
                add r9,r9,#12
                cmp r9,r8
                bne .Lzeroloop1
                
                strd r6,[sp,#S_bgprob_F]
                mov r9,#0x8000
                str r9,[sp,#S_bgprob_E]
                
                strd r0,[sp]
                strd r2,[sp,#8]
                
                /* converts gPrime to intfp */
                ldr r7,[r0,#44]
                str r7,[sp,#S_auxbg]
                tst r7,r7
                beq .Lskipgrpimeconv
                /*
                 */
                ldr r8,[sp,#S_size+40+8]
                ldr r9,[sp,#S_size+40+12]
                
                lsl r11,r9,#11
                orr r11,r11,r8,lsr #21
                lsl r10,r8,#11
                orr r11,r11,#0x80000000
                
                bic r5,r9,#0x80000000
                tst r8,r8
                tsteq r5,r5
                bne .Lnozero3
                mov r10,#0
                mov r11,#0
                mov r5,#0x8000
                b .Lconv_end
.Lnozero3:
                ldr r8,.Lmask
                cmp r9,r8
                beq .Linf_or_nan
                lsr r5,r9,#20
                bic r5,r5,#0x800
                sub r5,r5,#0x3f0
                sub r5,r5,#0x0f
                ldr r8,.Lsignmask
                bic r5,r5,r8
                b .Lconv_end
.Linf_or_nan:
                ldr r5,.Linf
.Lconv_end:
                tst r9,r9
                ldrmi r8,.Lsignmask
                orrmi r5,r5,r8
                
                strd r10,[sp,#S_gPrime_F]
                str r5,[sp,#S_gPrime_E]
                
                /* end converts gPrime to intfp */
                ldrd r8,[r0,#ap_bg_a_intfp]
                ldr r10,[r0,#ap_bg_a_intfp+8]
                strd r8,[sp,#S_bg_a_F]
                str r10,[sp,#S_bg_a_E]
                ldrd r8,[r0,#ap_bg_b_intfp]
                ldr r10,[r0,#ap_bg_b_intfp+8]
                strd r8,[sp,#S_bg_b_F]
                str r10,[sp,#S_bg_b_E]
                ldrd r8,[r0,#ap_bg_c_intfp]
                ldr r10,[r0,#ap_bg_c_intfp+8]
                strd r8,[sp,#S_bg_c_F]
                str r10,[sp,#S_bg_c_E]
.Lskipgrpimeconv:
                ldr r6,[sp,#S_size+40]         // qw_r3_N
                add r4,r4,r4,lsl #1
                add r4,r6,r4,lsl #2     // qw_r3_N+convolve
                str r4,[sp,#S_r4_loop]        // save reg
                
                ldr r8,[r0,#36]  // nstreams
                ldr r7,[sp,#S_size+40+28]
                add r7,r7,r8,lsl #3
                add r7,r7,r8,lsl #2
                str r7,[sp,#S_stend]
                
                ldr r8,.Linvlog2
                ldr r9,.Linvlog2+4
                
                strd r8,[sp,#S_invlog2]
                
                /* prepare main loop */
                ldrd r8,[r0,#ap_m_sun_r0_intfp]   // m_sun_r0
                ldr r10,[r0,#ap_m_sun_r0_intfp+8]
                strd r8, [sp,#S_m_sun_r0_F]
                str r10,[sp,#S_m_sun_r0_E]
                ldrd r8,[r0,#ap_q_inv_sqr_intfp]   // q_inv_sqr
                ldr r10,[r0,#ap_q_inv_sqr_intfp+8]
                strd r8, [sp,#S_q_inv_r0_F]
                str r10,[sp,#S_q_inv_r0_E]    // q_inv_sqr
                ldrd r8,[r0,#ap_r0_intfp]
                ldr r10,[r0,#ap_r0_intfp+8]
                strd r8,[sp,#S_r0_F]
                str r10,[sp,#S_r0_E]    // r0
                
.Lmainloop:
#ifdef PROB_DEBUG
                // convolve
                push {r10,r11}
                mov r11,#0
                bl .Lputs_sep
                pop {r10,r11}
#endif
                ///////////////////////
                //// START TO PROBNOVFP_DEBUG!!!
                ///////////////////////
                ldr r12,[sp,#S_size+44]  // lbt
                ldr r7,[sp,#12]
                
                ldrd r2,[r7]            // r_point[i]
                ldr r0,[r7,#8]

                ldrd r4,[r12,#24]           // lbt.z
                ldr r1,[r12,#32]
                
                /* r_point[i]*lbt.z
                   input: r_point[i]: {r2,r3,r0},
                          lbt.z:  {r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift1
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift1:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r8,r9,r1}, save to */
                
                strd r8,[sp,#S_xyz_z_F]        // { xyz.z }
                str r1,[sp,#S_xyz_z_E]
                
                ldrd r4,[r12,#12]           // lbt.y
                ldr r1,[r12,#20]
                /* r_point[i]*lbt.y
                   input: r_point[i]: {r2,r3,r0},
                          lbt.y:  {r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift2
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift2:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                
                /* output: {r8,r9,r1}, save to */
                
                strd r8,[sp,#S_xyz_y_F]        // { xyz.y }
                str r1,[sp,#S_xyz_y_E]
                
                ldrd r8,[r12]           // lbt.x
                ldr r1,[r12,#8]
                
                /* r_point[i]*lbt.x
                   input: r_point[i]: {r2,r3,r0},
                          lbt.x:  {r8,r9,r1} */
                umull r7,r4,r2,r9
                umull r5,r10,r3,r8
                adds r7,r7,r5
                adcs r4,r4,r10
                
                mov r5,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r5,r5,#0
                umlals r4,r5,r3,r9
                
                addmi r7,r7,#1
                bmi .Lnooneshift3
                adds r4,r4,r4
                adcs r5,r5,r5
.Lnooneshift3:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r4,r5,r1}, use */
                
                ldrd r2,[sp,#S_m_sun_r0_F]
                ldr r0,[sp,#S_m_sun_r0_E]
                
                /* r_point[i]*lbt.x {+m_sun_r0}
                   m_sun_r0: {r2,r3,r0} */
                bl .Lifpadd_int
                
                strd r2,[sp,#S_xyz_x_F]   // { xyz.x }
                str r0,[sp,#S_xyz_x_E]
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                ldrd r2,[sp,#S_xyz_x_F+24]
                ldr r0,[sp,#S_xyz_x_E+24]
                bl .Lprint_ifp_dbl
                
                ldrd r2,[sp,#S_xyz_y_F+24]
                ldr r0,[sp,#S_xyz_y_E+24]
                bl .Lprint_ifp_dbl
                
                ldrd r2,[sp,#S_xyz_z_F+24]
                ldr r0,[sp,#S_xyz_z_E+24]
                bl .Lprint_ifp_dbl
                
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /************
                 * compuate sqr(xyz.x)+sqr(xyz.y)+q_inv_sqr*sqr(xyz.z)
                 *************/
                /* sqr(xys.x):
                   input: x:{r2,r3,r0} */
                umull r1,r4,r2,r3
                umull r5,r7,r3,r3
                adds r1,r1,r1
                adcs r4,r4,r4
                adc r7,r7,#0
                adds r2,r5,r4
                adcs r3,r7,#0
                
                add r0,r0,r0
                addmi r0,r0,#1
                bmi .Lnooneshift4
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift4:
                movpl r0,#0x8000
                
                mov r5,#0
                pkhbt r0,r0,r5
                /* output: {r2,r3,r0} */
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /* sqr(xys.y):
                   input: y:{r4,r5,r1} */
                ldrd r4,[sp,#S_xyz_y_F]
                ldr r1,[sp,#S_xyz_y_E]
                
                umull r7,r8,r4,r5
                umull r9,r10,r5,r5
                adds r7,r7,r7
                adcs r8,r8,r8
                adc r10,r10,#0
                adds r4,r9,r8
                adcs r5,r10,#0
                
                add r1,r1,r1
                addmi r1,r1,#1
                bmi .Lnooneshift5
                adds r4,r4,r4
                adcs r5,r5,r5
.Lnooneshift5:
                movpl r1,#0x8000
                
                mov r7,#0
                pkhbt r1,r1,r7
                /* output: {r4,r5,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /* sqr(x)+sqr(y):
                   input: sqr(x): {r2,r3,r0}, sqr(y):{r4,r5,r1} */
                // ifpaddp2-sqrxyzx-sqrxyzy.S
                // main routine
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale_p1
                # do right shift b
                cmp r7,#64
                orrhs r7,r7,#64
                
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend_p1
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
.Laddend_p1:
                pkhbt r0,r0,r1
                b .Lsavefirst_p1
.Lmantisale_p1:
                rsb r7,r7,#0    // negate
                cmp r7,#64
                orrhs r7,r7,#64
                
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2_p1
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
.Laddend2_p1:
                pkhbt r0,r1,r0
.Lsavefirst_p1:
                /* output: {r2,r3,r0} */
                /// end of add
                
                /* sqr(xys.z):
                   input: z:{r4,r5,r1} */
                ldrd r4,[sp,#S_xyz_z_F]
                ldr r1,[sp,#S_xyz_z_E]
                
                umull r7,r8,r4,r5
                umull r9,r10,r5,r5
                adds r7,r7,r7
                adcs r8,r8,r8
                adc r10,r10,#0
                adds r4,r9,r8
                adcs r5,r10,#0
                
                add r1,r1,r1
                addmi r1,r1,#1
                bmi .Lnooneshift6
                adds r4,r4,r4
                adcs r5,r5,r5
.Lnooneshift6:
                movpl r1,#0x8000
                
                mov r7,#0
                pkhbt r1,r1,r7     // square(z): {r4,r5,r1}
                /* output: {r4,r5,r1} */
                
                /* q_inv_sqr */
                ldrd r8,[sp,#S_q_inv_r0_F]
                ldr r11,[sp,#S_q_inv_r0_E]
                /* sqr(x)*q_inv_sqr:
                  input: sqr(x): {r4,r5,r1}, q_inv_sqr:{r8,r9,r11}*/
                // ifpmul-sqrx-qinvsqr.S
                eor r10,r11,r1
                str r10,[sp,#S_tmpreg]
                sxth r1,r1
                sxtah r1,r1,r11
                
                mov r14,#0
                umull r7,r10,r4,r9
                umull r11,r12,r5,r8
                adds r7,r7,r11
                adcs r4,r10,r12
                adc r14,r14,#0
                umlals r4,r14,r5,r9
                mov r5,r14
                
                addmi r1,r1,#1
                bmi .Lnooneshift7
                adds r4,r4,r4
                adcs r5,r5,r5
.Lnooneshift7:
                movpl r1,#0x8000
                ldr r9,[sp,#S_tmpreg]     // apply sign
                pkhbt r1,r1,r9     // sqr(z)*q_inv_sqr
                /* output: {r4,r5,r1} */
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /// add to rest
                /* rest+{sqr(x)*q_inv_sqr}:
                   input: rest:{r2,r3,r0}, sqr(x)*q_inv_sqr: {r4,r5,r1} */
                // main routine
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale_p2
                # do right shift b
                cmp r7,#64
                orrhs r7,r7,#64
                
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend_p2
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
.Laddend_p2:
                pkhbt r0,r0,r1
                b .Lsavefirst_p2
.Lmantisale_p2:
                rsb r7,r7,#0    // negate
                cmp r7,#64
                orrhs r7,r7,#64
                
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2_p2
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
.Laddend2_p2:
                pkhbt r0,r1,r0
.Lsavefirst_p2:
                // end of add to rest
                /* output: {r2,r3,r0} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif

                ////////////////////////////////
                //// square root: input {r2,r3,r0}
                ////////////////////////////////
                sxth r0,r0
                asrs r0,r0,#1
                
                and r3,r3,#0x7fffffff
                
                adr r12,.Lsqrtpdata1
                addcs r12,r12,#88*4
                
                ldr r7,.Lsqrtranges
                cmp r3,r7
                addhs r12,r12,#88
                ldr r7,.Lsqrtranges+4
                cmp r3,r7
                addhs r12,r12,#88
                ldr r7,.Lsqrtranges+8
                cmp r3,r7
                addhs r12,r12,#88
                
                add r14,r12,#80
                // [-1, 1, -1, 1, -1, 1, -1, 1, -1, 1, 1]
                /* evaluate polynomial */
                ldrd r4,[r12]           // yi<0
                add r12,r12,#8
.Lsqrtloop:
                // polydata[1]
                mov r11,#0
                umull r1,r7,r2,r5       // yi*xi
                umull r8,r9,r3,r4
                adds r1,r1,r8
                adcs r10,r7,r9
                adc r11,r11,#0
                umlal r10,r11,r3,r5
                adds r10,r10,r10        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12]           // polydata[i]
                subs r4,r4,r10
                sbc r5,r5,r11           // yi>0
                
                add r12,r12,#8
                cmp r12,r14
                bne .Lsqrtloop
                
                // polydata[10]
                mov r11,#0
                umull r1,r7,r2,r5       // yi*xi
                umull r8,r9,r3,r4
                adds r1,r1,r8
                adcs r10,r7,r9
                adc r11,r11,#0
                umlal r10,r11,r3,r5
                adds r10,r10,r10        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12]           // polydata[i]
                adds r4,r4,r10
                adc r5,r5,r11           // yi>0
                
                mov r7,#0
                pkhbt r1,r0,r7
                /* result: {r4,r5,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                // h_prob=qw_r3_N[i] / (rg * cube(rg+ap->r0))
                /// ap->r0
                
                strd r4,[sp,#S_oldrg_F]
                str r1,[sp,#S_oldrg_E]    // old rg
                ldrd r2,[sp,#S_r0_F]
                ldr r0,[sp,#S_r0_E]
                
                bl .Lifpadd_int
                
                /* do cube: (rg+r0)^3
                   input: {r2,r3,r0} */
                sxth r1,r0
                adds r1,r1,r1,lsl #1
                
                umull r4,r5,r2,r3
                umull r7,r8,r3,r3
                adds r4,r4,r4
                adcs r5,r5,r5
                adc r8,r8,#0
                adds r5,r5,r7
                adcs r8,r8,#0
                
                addmi r1,r1,#1
                bmi .Lnooneshift8
                adds r5,r5,r5
                adc r8,r8,r8
.Lnooneshift8:
                mov r11,#0
                // next step
                umull r4,r7,r2,r8
                umull r9,r10,r3,r5
                adds r4,r4,r9
                adcs r7,r7,r10
                adc r11,r11,#0
                umlals r7,r11,r3,r8
                
                addmi r1,r1,#1
                bmi .Lnooneshift9
                adds r7,r7,r7
                adcs r11,r11,r11
.Lnooneshift9:
                movpl r1,#0x8000
                pkhbt r0,r1,r0
                /* output: {r7,r11,r0} */
                
                ldrd r4,[sp,#S_oldrg_F]
                ldr r1,[sp,#S_oldrg_E]    // old rg
                
                /* (rg+r0)^3*rg
                   input: (rg+r0)^3: {r7,r11,r0}, rg: {r4,r5,r1} */
                sxth r12,r0
                sxtah r12,r12,r1
                
                mov r10,#0
                umull r2,r3,r4,r11
                umull r8,r9,r5,r7
                adds r2,r2,r8
                adcs r2,r3,r9
                adc r3,r10,#0
                umlals r2,r3,r5,r11
                
                addmi r12,r12,#1
                bmi .Lnooneshift9_5
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift9_5:
                movpl r12,#0x8000
                eor r8,r0,r1
                pkhbt r0,r12,r8
                /* output: {r2,r3,r0} */
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                //ldrd r4,[r6]        // qw_r3_N
                ldr r1,[r6,#8]
                
                ///////////////////////////
                ////// Division (a/b)
                /// reciprocal: input: b:{r2,r3,r0}
                orrs r4,r2,r3
                beq .Ldivbyzero
                /* compute reciprocal */
                /* get nrdiv start point */
                lsr r10,r3,#24
                adr r11,.Lnrdiv_points-16*8
                and r10,r10,#0xfffffff8
                ldrd r4,[r11,r10]
                
                adds r2,r2,r2
                adc r3,r3,r3
                
                mov r14,#3
                /* newton raphson method */
                /* iteration 1 */
.Ldivloop:
                mov r11,#0
                umull r12,r7,r2,r5       // mulh(xi,ai)
                umull r8,r9,r3,r4
                adds r12,r12,r8
                adcs r10,r7,r9
                adc r11,r11,#0
                umlal r10,r11,r3,r5
                adds r10,r10,r4         // -mulh(xi,ai)-xi
                adc r11,r11,r5
                rsbs r10,r10,#0         //
                rscs r11,r11,#0
                movmi r10,#0            // fix if negative
                movmi r11,#0
                umull r12,r7,r4,r11      // mulh(xi,tmp)
                umull r8,r9,r5,r10
                umull r10,r11,r5,r11
                adds r12,r12,r8
                adcs r7,r7,r9
                adc r11,r11,#0
                adds r10,r10,r7
                adc r11,r11,#0
                adds r4,r4,r10          // xi+mulh(xi,tmp)
                adc r5,r5,r11
                
                subs r14,r14,#1
                bpl .Ldivloop
                
                sxth r8,r0
                mvn r7,r8               // negate mantisa
                ldrd r2,[r6]        // qw_r3_N
                sxtah r7,r7,r1
                
                // multiply by A
                umull r8,r9,r2,r5
                umull r10,r11,r3,r4
                umull r4,r5,r3,r5
                adds r8,r8,r10
                adcs r9,r9,r11
                adc r5,r5,#0
                adds r4,r4,r9
                adcs r5,r5,#0
                
                addmi r7,r7,#1
                bmi .Lnooneshift10
                adds r4,r4,r4    // shift 1
                adc r5,r5,r5
.Lnooneshift10:
                eor r8,r0,r1
                pkhbt r1,r7,r8  /// {r4,r5,r1}
                /* output: {r4,r5,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                ///
                ldr r2,[sp,#S_auxbg]
                tst r2,r2
                beq .Lnoauxbg
                //b .Lnoauxbg
                
                /****************************
                 * handling aux_bg
                 *******************************/
                
                strd r4,[sp,#S_hprob_F]
                str r1,[sp,#S_hprob_E]
                
                ldr r7,[sp,#S_reg+8]    // sg_dx
                ldrd r4,[r7]
                ldr r1,[r7,#8]
                add r7,r7,#12
                
                ldrd r2,[sp,#S_gPrime_F]
                ldr r0,[sp,#S_gPrime_E]
                
                str r7,[sp,#S_reg+8]
                /* add: gPrime+sg_dx[i] */
                bl .Lifpadd_int
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r2,[sp,#S_tmpg_F]
                str r0,[sp,#S_tmpg_E]
                
                /* computing polynomial:
                   bg_a*sqr(g)+bg_b*g+bg_c */
                ldrd r4,[sp,#S_bg_a_F]
                ldr r1,[sp,#S_bg_a_E]
                
                /* bg_a*g:
                  input: g:{r2,r3,r0}, bg_a:{r4,r5,r1} */
                sxth r12,r0
                sxtah r12,r12,r1
                
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r2,r8,r10
                adc r11,r11,#0
                umlals r2,r11,r3,r5
                mov r3,r11
                
                addmi r12,r12,#1
                bmi .Lnooneshift_bg_1
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift_bg_1:
                movpl r12,#0x8000
                eor r7,r0,r1
                pkhbt r0,r12,r7
                /* output: {r2,r3,r0} */
                
                ldrd r4,[sp,#S_bg_b_F]
                ldr r1,[sp,#S_bg_b_E]
                
                /* add: bg_a*g+bg_b */
                bl .Lifpadd_int
                
                ldrd r4,[sp,#S_tmpg_F]
                ldr r1,[sp,#S_tmpg_E]
                
                /* multiply: (bg_a*g+bg_b)*g
                   input: value:{r2,r3,r0}, g: {r4,r5,r1} */
                sxth r12,r0
                sxtah r12,r12,r1
                
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r2,r8,r10
                adc r11,r11,#0
                umlals r2,r11,r3,r5
                mov r3,r11
                
                addmi r12,r12,#1
                bmi .Lnooneshift_bg_2
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift_bg_2:
                movpl r12,#0x8000
                eor r7,r0,r1
                pkhbt r0,r12,r7
                /* output: {r2,r3,r0} */
                
                ldrd r4,[sp,#S_bg_c_F]
                ldr r1,[sp,#S_bg_c_E]
                
                /* polyval+bg_c */
                bl .Lifpadd_int
                
                ldrd r4,[r6]
                ldr r1,[r6,#8]
                /* multiply by polyval*qw_r3_N:
                   input: polyval: {r2,r3,r0}, qw_r3_N: {r4,r5,r1} */
                sxth r12,r0
                sxtah r12,r12,r1
                
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r2,r8,r10
                adc r11,r11,#0
                umlals r2,r11,r3,r5
                mov r3,r11
                
                addmi r12,r12,#1
                bmi .Lnooneshift_bg_aux
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift_bg_aux:
                movpl r12,#0x8000
                eor r7,r0,r1
                pkhbt r0,r12,r7
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                ldrd r4,[sp,#S_hprob_F]
                ldr r1,[sp,#S_hprob_E]
                
                /* add to h_prob */
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                ldrd r4,[sp,#S_bgprob_F]
                ldr r1,[sp,#S_bgprob_E]
                
                /* summate: bg_prob+=h_prob */
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                b .Lfromauxbg
.Lnoauxbg:
                /// summate bg_prob
                ldrd r2,[sp,#S_bgprob_F]
                ldr r0,[sp,#S_bgprob_E]
                
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
.Lfromauxbg:
                strd r2,[sp,#S_bgprob_F]
                str r0,[sp,#S_bgprob_E]
                
                /* preparing streamloop */
                ldr r8,[sp,#S_size+40+28]   // streamTmps
                str r8,[sp,#S_sti]
                ldr r11,[sp,#4]     // sc
                str r11,[sp,#S_sci]
                
                /******************
                 * StreamLoop
                 **************************/
.Lstreamloop:
#ifdef PROBNOVFP_DEBUG
                mov r11,#1
                bl .Lputs_sep
                nop
                nop
#endif
                ldr r11,[sp,#S_sci]
                ldr r12,.Lsignmask
                
                ldrd r4,[r11,#sc_c_x]
                ldr r1,[r11,#sc_c_x+8]
                eor r1,r1,r12
                ldrd r2,[sp,#S_xyz_x_F]
                ldr r0,[sp,#S_xyz_x_E]
                
                /* xyzs.x=xyz.x-sc[j].c.x; */
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r2,[sp,#S_xyzs_x_F]
                str r0,[sp,#S_xyzs_x_E]
                
                ldrd r4,[r11,#sc_c_y]
                ldr r1,[r11,#sc_c_y+8]
                eor r1,r1,r12
                ldrd r2,[sp,#S_xyz_y_F]
                ldr r0,[sp,#S_xyz_y_E]
                
                /* xyzs.y=xyz.y-sc[j].c.y; */
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r2,[sp,#S_xyzs_y_F]
                str r0,[sp,#S_xyzs_y_E]
                
                ldrd r4,[r11,#sc_c_z]
                ldr r1,[r11,#sc_c_z+8]
                eor r1,r1,r12
                ldrd r2,[sp,#S_xyz_z_F]
                ldr r0,[sp,#S_xyz_z_E]
                
                /* xyzs.z=xyz.z-sc[j].c.z; */
                bl .Lifpadd_int
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r2,[sp,#S_xyzs_z_F]
                str r0,[sp,#S_xyzs_z_E]
                
                ldrd r4,[r11,#sc_a_z]
                ldr r1,[r11,#sc_a_z+8]
                
                /// compute dot product: xyzs dot sc.a
                
                /* multiply: xyzs.z*sc[j].a.z
                   input: xyzs.z:{r2,r3,r0}, sc.a.z:{r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift11
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift11:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r8,r9,r1}, save to */
                
                strd r8,[sp,#S_tmpx_F]
                str r1,[sp,#S_tmpx_E]
                
                /// next elem of dot product
                ldrd r4,[r11,#sc_a_y]
                ldr r1,[r11,#sc_a_y+8]
                ldrd r2,[sp,#S_xyzs_y_F]
                ldr r0,[sp,#S_xyzs_y_E]
                
                /* multiply: xyzs.y*sc[j].a.y
                   input: xyzs.y:{r2,r3,r0}, sc.a.y:{r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift12
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift12:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r8,r9,r1} */
                
                strd r8,[sp,#S_tmpy_F]
                str r1,[sp,#S_tmpy_E]
                
                /// last elem of dot product
                ldrd r4,[r11,#sc_a_x]
                ldr r1,[r11,#sc_a_x+8]
                ldrd r2,[sp,#S_xyzs_x_F]
                ldr r0,[sp,#S_xyzs_x_E]
                
                /* multiply: xyzs.x*sc[j].a.x
                   input: xyzs.x:{r2,r3,r0}, sc.a.x:{r4,r5,r1} */
                /// ifpmul-xyzsx-scax.S
                mov r12,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r4,r8,r10
                
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r12,r12,#0
                umlals r4,r12,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift22
                adds r4,r4,r4
                adcs r12,r12,r12
.Lnooneshift22:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r4,r12,r1} */
                
                /// sumating
                mov r5,r12
                
                ldrd r2,[sp,#S_tmpy_F]
                ldr r0,[sp,#S_tmpy_E]
                
                /* xyzs.y*sc[j].a.y+xyzs.z*sc[j].a.z */
                bl .Lifpadd_int
                
                ldrd r4,[sp,#S_tmpx_F]
                ldr r1,[sp,#S_tmpx_E]
                
                /* xyzs.x*sc[j].a.x+xyzs.y*sc[j].a.y+xyzs.z*sc[j].a.z */
                bl .Lifpadd_int
                /* dotted: {r2,r3,r0} */
                
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                //////////////////////
                /// multiply by sc.a
                ldrd r4,[r11,#sc_a_x]
                ldr r1,[r11,#sc_a_x+8]
                
                /* dotted*sc.a.x:
                   input: dotted:{r2,r3,r0}, sc.a.x:{r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift23
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift23:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r8,r9.r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r8
                mov r3,r9
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r8,[sp,#S_tmpx_F]
                str r1,[sp,#S_tmpx_E]
                
                ldrd r4,[r11,#sc_a_y]
                ldr r1,[r11,#sc_a_y+8]
                
                /* dotted*sc.a.y:
                   input: dotted:{r2,r3,r0}, sc.a.y:{r4,r5,r1} */
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                
                mov r9,#0
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r9,r9,#0
                umlals r8,r9,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift24
                adds r8,r8,r8
                adcs r9,r9,r9
.Lnooneshift24:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                /* output: {r8,r9,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r8
                mov r3,r9
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                strd r8,[sp,#S_tmpy_F]
                str r1,[sp,#S_tmpy_E]
                
                ldrd r4,[r11,#sc_a_z]
                ldr r1,[r11,#sc_a_z+8]
                
                /* dotted*sc.a.z:
                   input: dotted:{r2,r3,r0}, sc.a.z:{r4,r5,r1} */
                mov r12,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r4,r8,r10
                
                sxth r7,r0
                sxtah r7,r7,r1
                
                adc r12,r12,#0
                umlals r4,r12,r3,r5
                
                addmi r7,r7,#1
                bmi .Lnooneshift25
                adds r4,r4,r4
                adcs r12,r12,r12
.Lnooneshift25:
                movpl r7,#0x8000
                eor r1,r0,r1
                pkhbt r1,r7,r1
                
                mov r5,r12
                /* output: {r4,r5,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /* subtracting  */
                ldr r12,.Lsignmask
                ldrd r2,[sp,#S_xyzs_z_F] 
                ldr r0,[sp,#S_xyzs_z_E]
                eor r1,r12,r1
                
                /* xyzs.z-=sc[j].a.z*dotted; */
                bl .Lifpadd_int
                
                /* xyzs.z^2:
                   input: {r2,r3,r0} */
                umull r1,r4,r2,r3
                umull r5,r7,r3,r3
                adds r1,r1,r1
                adcs r4,r4,r4
                adc r7,r7,#0
                adds r2,r5,r4
                adcs r3,r7,#0
                
                add r0,r0,r0
                addmi r0,r0,#1
                bmi .Lnooneshift26
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift26:
                movpl r0,#0x8000
                
                mov r5,#0
                pkhbt r0,r0,r5
                /* output: {r2,r3,r0} */
                
                ldrd r4,[sp,#S_tmpy_F]
                ldr r1,[sp,#S_tmpy_E]
                
                strd r2,[sp,#S_tmpy_F]
                str r0,[sp,#S_tmpy_E]
                
                ldrd r2,[sp,#S_xyzs_y_F] 
                ldr r0,[sp,#S_xyzs_y_E]
                eor r1,r12,r1
                
                /* xyzs.y-=sc[j].a.y*dotted; */
                bl .Lifpadd_int
                
                /* xyzs.y^2:
                   input: {r2,r3,r0} */
                umull r1,r4,r2,r3
                umull r5,r7,r3,r3
                adds r1,r1,r1
                adcs r4,r4,r4
                adc r7,r7,#0
                adds r2,r5,r4
                adcs r3,r7,#0
                
                add r0,r0,r0
                addmi r0,r0,#1
                bmi .Lnooneshift27
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift27:
                movpl r0,#0x8000
                
                mov r5,#0
                pkhbt r0,r0,r5
                /* output: {r2,r3,r0} */
                
                ldrd r4,[sp,#S_tmpx_F]
                ldr r1,[sp,#S_tmpx_E]
                
                strd r2,[sp,#S_tmpx_F]
                str r0,[sp,#S_tmpx_E]
                
                ldrd r2,[sp,#S_xyzs_x_F]
                ldr r0,[sp,#S_xyzs_x_E]
                eor r1,r12,r1
                
                /* xyzs.x-=sc[j].a.x*dotted; */
                bl .Lifpadd_int
                
                /* xyzs.x^2:
                   input: {r2,r3,r0} */
                umull r1,r4,r2,r3
                umull r5,r7,r3,r3
                adds r1,r1,r1
                adcs r4,r4,r4
                adc r7,r7,#0
                adds r2,r5,r4
                adcs r3,r7,#0
                
                add r0,r0,r0
                addmi r0,r0,#1
                bmi .Lnooneshift28
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift28:
                movpl r0,#0x8000
                
                mov r5,#0
                pkhbt r0,r0,r5
                /* output: {r2,r3,r0} */
                
                /* summate norm */
                ldrd r4,[sp,#S_tmpx_F]
                ldr r1,[sp,#S_tmpx_E]
                
                /* xyzs.x^2+xyzs.y^2:
                   input: xyzs.x^2: {r2,r3,r0}, xyzs.y^2: {r4,r5,r1} */
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale_p3
                # do right shift b
                cmp r7,#64
                orrhs r7,r7,#64
                
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend_p3
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
.Laddend_p3:
                pkhbt r0,r0,r1
                b .Lsavefirst_p3
.Lmantisale_p3:
                rsb r7,r7,#0    // negate
                cmp r7,#64
                orrhs r7,r7,#64
                
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2_p3
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
.Laddend2_p3:
                pkhbt r0,r1,r0
.Lsavefirst_p3:
                /* output: {r2,r3,r0} */
                
                /* rest+xyzs.z^2:
                   input: rest: {r2,r3,r0}, xyzs.z^2: {r4,r5,r1} */
                ldrd r4,[sp,#S_tmpy_F]
                ldr r1,[sp,#S_tmpy_E]
                
                // main routine
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale_p4
                # do right shift b
                cmp r7,#64
                orrhs r7,r7,#64
                
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend_p4
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
.Laddend_p4:
                pkhbt r0,r0,r1
                b .Lsavefirst_p4
.Lmantisale_p4:
                rsb r7,r7,#0    // negate
                cmp r7,#64
                orrhs r7,r7,#64
                
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2_p4
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
.Laddend2_p4:
                pkhbt r0,r1,r0
.Lsavefirst_p4:
                /* xyz_norm: {r2,r3,r0} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                ///////////////////////
                /// multply by sigma2_sq_inv
                ldrd r4,[r11,#sc_sigma2_sq_inv]
                ldr r1,[r11,#sc_sigma2_sq_inv+8]
                
                /* increment and save sc_i */
                add r11,r11,#128
                str r11,[sp,#S_sci]
                
                /* input: xyz_norm:{r2,r3,r0}, sigma2_sq_inv:{r4,r5,r1} */
                sxth r12,r0
                sxtah r0,r12,r1
                
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r2,r8,r10
                adc r11,r11,#0
                umlals r2,r11,r3,r5
                mov r3,r11
                
                addmi r0,r0,#1
                bmi .Lnooneshift29
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift29:
                movpl r0,#0x8000
                /* output: {r2,r3,r0} (without sign) */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                lsl r0,r0,#16
                lsr r0,r0,#16
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
                nop
                nop
#endif
#ifdef PROB_DEBUG
                // test
                push {r10,r11}
                mov r11,#1
                bl .Lputs_sep
                pop {r10,r11}
#endif
                //////////////////////////////
                /// exponent
                //ldrd r2,[r0]    // 1.fraction
                ldrd r8,[sp,#S_invlog2]
                sxth r4,r0
                // multiply invlog2
                umull r5,r7,r2,r9
                umull r10,r12,r3,r8
                umull r2,r3,r3,r9
                adds r5,r5,r10
                adcs r7,r7,r12
                adc r3,r3,#0
                adds r2,r2,r7
                adcs r3,r3,#0
                
                addmi r4,r4,#1
                bmi .Lnooneshift
                adds r2,r2,r2    // shift 1
                adc r3,r3,r3
.Lnooneshift:
                // integer part
                cmp r4,#10      // 1024
                bge .Loutofrange
                //ldr r8,[r0,#8]  // 
                rsbs r2,r2,#0   // must be negative
                rsc r3,r3,#0
                
                tst r4,r4
                bmi .Lifexplessthan1
                
                rsb r10,r4,#30
                //adds r9,r8,#0x10000
                cmp r4,r4
                mov r1,r3,rrx   // get sign
                asr r1,r1,r10    // integer part
                pkhbt r1,r1,r4 // zeroing higher 16-bits
                
                rsb r5,r4,#32
                lsl r3,r3,r4
                orr r3,r3,r2,lsr r5
                lsl r2,r2,r4
                
                b .Lafterextract
.Lifexplessthan1:
                //lsr r7,r8,#16       // integer part (0 or -1)
                mov r1,#0xff00
                orr r1,#0xff
                
                cmp r4,r4       // get sign: C is 1
                movs r3,r3,rrx   // shift right 1 with sign
                mov r2,r2,rrx
                
                mvn r4,r4       // negate-1
                
                rsbs r5,r4,#32
                submi r5,r4,#32
                asrmi r2,r3,r5
                lsrpl r2,r2,r4
                orrpl r2,r2,r3,lsl r5
                asr r3,r3,r4
.Lafterextract:
                //mov r1,r7
                
                adr r12,.Lexppdata1
                lsr r7,r3,#26
                and r3,r3,#0x1fffffff
                and r7,r7,#0x18
                add r7,r7,r7,lsl #3
                add r12,r12,r7
                ldrd r4,[r12]
                /* evaluate polynomial */
                // polydata[1]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#8]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[2]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#16]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[3]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#24]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[4]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#32]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[5]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#40]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[6]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#48]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[7]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#56]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                // polydata[8]
                mov r11,#0
                umull r7,r8,r2,r5       // yi*xi
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umlal r8,r11,r3,r5
                adds r8,r8,r8        // tmp<<1
                adc r11,r11,r11
                ldrd r4,[r12,#64]           // polydata[i]
                adds r4,r8,r4
                adc r5,r11,r5
                
                //strd r4,[r0]    // save to r0
                b .Lexpend
.Loutofrange:
                mov r4,#0
                mov r5,#0
                mov r1,#0x8000
.Lexpend:
                /* output: {r4,r5,r1} */
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                mov r0,r1
                mov r2,r4
                mov r3,r5
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
#endif
                
                /////////////////////////////
                /// exp(x)*qw_r3_N
                ldrd r2,[r6]
                ldr r0,[r6,#8]
                // input: exp(x):{r4,r5,r1}, qw_r3_N: {r2,r3,r0}
                sxth r12,r0
                sxtah r12,r12,r1
                
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r2,r8,r10
                adc r11,r11,#0
                umlals r2,r11,r3,r5
                mov r3,r11
                
                addmi r12,r12,#1
                bmi .Lnooneshift30
                adds r2,r2,r2
                adcs r3,r3,r3
.Lnooneshift30:
                movpl r12,#0x8000
                eor r7,r0,r1
                pkhbt r0,r12,r7
                /* output: {r2,r3,r0} */

                /// final adding
                ldr r12,[sp,#S_sti]
                ldrd r4,[r12]
                ldr r1,[r12,#8]
                
                /* streamTmps[j]+=result */
                // main routine
                sxth r7,r0
                sxth r8,r1
                
                subs r7,r7,r8
                blt .Lmantisale_p5
                # do right shift b
                cmp r7,#64
                orrhs r7,r7,#64
                
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r4,r5,r9
                lsrpl r4,r4,r7
                orrpl r4,r4,r5,lsl r9
                lsr r5,r5,r7
                
                // main addition
                adds r2,r2,r4
                adcs r3,r3,r5
                bcc .Laddend_p5
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r0,r0,#1    // add to mantisa
.Laddend_p5:
                //pkhbt r0,r0,r0
                b .Lsavefirst_p5
.Lmantisale_p5:
                rsb r7,r7,#0    // negate
                cmp r7,#64
                orrhs r7,r7,#64
                
                # do right shift b
                rsbs r9,r7,#32
                submi r9,r7,#32
                lsrmi r2,r3,r9
                lsrpl r2,r2,r7
                orrpl r2,r2,r3,lsl r9
                lsr r3,r3,r7

                // main addition
                adds r2,r4,r2
                adcs r3,r5,r3
                bcc .Laddend2_p5
                movs r3,r3, rrx
                mov r2,r2, rrx
                add r1,r1,#1    // add to mantisa
.Laddend2_p5:
                pkhbt r0,r1,r0
.Lsavefirst_p5:
                /// end of final adding
                /* save to streamTmpsIntFp[j] */
#ifdef PROB_DEBUG
                // test
                push {r10,r11}
                mov r11,#1
                bl .Lputs_sep
                pop {r10,r11}
#endif

                strd r2,[r12]
                str r0,[r12,#8]
#ifdef PROBNOVFP_DEBUG
                push {r0,r1,r2,r3,r12,r14}
                bl .Lprint_ifp_dbl
                pop {r0,r1,r2,r3,r12,r14}
                
                /*add sp,sp,#S_size
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                nop
                nop*/
#endif
                
                /// add to streamTmps
                ldr r8,[sp,#S_stend]
                add r12,r12,#12
                str r12,[sp,#S_sti]
#ifdef PROB_DEBUG
                // test
                push {r10,r11}
                mov r11,#1
                bl .Lputs_sep
                pop {r10,r11}
#endif
                cmp r12,r8
                bne .Lstreamloop
                
                ldr r3,[sp,#12]
                ldr r4,[sp,#S_r4_loop]
                
                add r3,r3,#12
                add r6,r6,#12
                str r3,[sp,#12]
                cmp r6,r4
                bne .Lmainloop
                /************************
                 * end of mainloop
                 **********************/
                
                /* multiply by reff_xr_rp3 */
                ldr r8,[sp,#S_size+40+28]   // streamTmpsIntFp
                ldr r9,[sp,#S_stend]
                ldr r10,[sp,#S_size+40+24]   // streamTmpsFp
                
.Lmulloop:
                ldrd r2,[r8]
                ldr r0,[r8,#8]
                bl .Lintfp_to_fp_int
                ldr r0,[sp,#S_size+40+16]     // reff_xr_rp3
                ldr r1,[sp,#S_size+40+20]     // reff_xr_rp3
                bl __aeabi_dmul(PLT)
                strd r0,[r10]
                
                add r10,r10,#8
                add r8,r8,#12
                cmp r8,r9
                bne .Lmulloop
#ifdef PROBNOVFP_DEBUG
                mov r11,#0
                bl .Lputs_sep
                nop
                nop
#endif
                
                ldrd r2,[sp,#S_bgprob_F]
                ldr r0,[sp,#S_bgprob_E]
                bl .Lintfp_to_fp_int
                ldr r0,[sp,#S_size+40+16]     // reff_xr_rp3
                ldr r1,[sp,#S_size+40+20]     // reff_xr_rp3
                bl __aeabi_dmul(PLT)
                
                add sp,sp,#S_size
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
.Lintfp_to_fp_int:
                sxth r4,r0
                
                adds r2,r2,#0x400   // rounding
                adcs r3,r3,#0
                bcc .Lifnocarry
                movs r3,r3,rrx
                mov r2,r2,rrx
                add r4,r4,#1
.Lifnocarry:
                ldr r5,.Lmaxmantisa
                cmp r4,r5
                blt .Lnotinf
                mov r2,#0
                ldr r3,.Lmask2
                tst r0,r0
                orrmi r3,r3,#0x80000000
                b .Lconv_end2
.Lnotinf:
                ldr r5,.Lminmantisa
                cmp r4,r5
                bgt .Lisnormal
                mov r2,#0
                mov r3,#0
                tst r0,r0
                orrmi r3,r3,#0x80000000
                b .Lconv_end2
.Lisnormal:
                sub r4,r4,r5
                lsl r7,r4,#20
                bic r7,r7,#0x80000000
                
                bic r3,r3,#0x80000000
                
                lsr r2,r2,#11
                orr r2,r2,r3,lsl #21
                orr r3,r7,r3,lsr #11
                
                tst r0,r0
                orrmi r3,r3,#0x80000000
.Lconv_end2:
                bx lr
.Ldivbyzero:    // segfault
                mov r0,#0
                ldr r1,[r0]
                
                .align 2
.Lmask2:
                .word 0x7ff00000
.Lminmantisa:
                .word -0x3ff
.Lmaxmantisa:
                .word 0x3ff
                
.Lsqrtranges:
                .int 406319185, 889516851, 1464138954
                .align 4
                .word 0,0
.Lsqrtpdata1:
                .quad 36808955663348648
                .quad 82202948024323612
                .quad 116462026303225353
                .quad 147975539213210115
                .quad 189088095071733597
                .quad 252197563200987347
                .quad 360287809749108158
                .quad 576460748546640184
                .quad 1152921504561573197
                .quad 4611686018427174481
                .quad 9223372036854775974
.Lsqrtpdata2:
                .quad 7095896236997152
                .quad 32271049479021006
                .quad 75272618939144893
                .quad 126089577195759835
                .quad 180902483773113294
                .quad 249983455221658014
                .quad 359855863629174491
                .quad 576401445209649615
                .quad 1152916067455389408
                .quad 4611685719618290273
                .quad 9223372044298598494
.Lsqrtpdata3:
                .quad 1367920999083559
                .quad 9986374570562467
                .quad 35323101716819877
                .quad 82588217249810979
                .quad 149037996802949644
                .quad 233593129895041344
                .quad 353870711792181904
                .quad 574872928861506879
                .quad 1152655467250862464
                .quad 4611659005488945988
                .quad 9223373291554207669
.Lsqrtpdata4:
                .quad 263702821636188
                .quad 2788330965539128
                .quad 13953360086828187
                .quad 44517325401181861
                .quad 103951892267617034
                .quad 196504623244512813
                .quad 332413304163665068
                .quad 566255968771237336
                .quad 1150358275601356412
                .quad 4611292207672715748
                .quad 9223399904218698030
.Lsqrtpdata5:
                .quad 52055724315897604
                .quad 116252523963049071
                .quad 164702177099473424
                .quad 209269014454793496
                .quad 267410948533738849
                .quad 356661214276282101
                .quad 509523906904886173
                .quad 815238608770404989
                .quad 1630477228102570984
                .quad 6521908912666089281
                .quad 13043817825332782448
.Lsqrtpdata6:
                .quad 10035112695553582
                .quad 45638155845244710
                .quad 106451558579080603
                .quad 178317590144132885
                .quad 255834746018915573
                .quad 353529992743356070
                .quad 508913042843861545
                .quad 815154741186938944
                .quad 1630469538873265807
                .quad 6521908490086512676
                .quad 13043817835859937211
.Lsqrtpdata7:
                .quad 1934532429158924
                .quad 14122866356627234
                .quad 49954409513011027
                .quad 116797376926898284
                .quad 210771556387649368
                .quad 330350572374747545
                .quad 500448759943124385
                .quad 812993092637086492
                .quad 1630100994529666601
                .quad 6521870710602486999
                .quad 13043819599745735452
.Lsqrtpdata8:
                .quad 372932106793951
                .quad 3943295467850302
                .quad 19733031075467850
                .quad 62957005342927672
                .quad 147010175879210880
                .quad 277899503261405364
                .quad 470103403061507965
                .quad 800806870810999615
                .quad 1626852274943564921
                .quad 6521351980156125360
                .quad 13043857235736788606
                
                //.word 0
.Lnrdiv_points: /* start point table for Newton-Raphson method */
                .quad 17361641481138401520
                .quad 16397105843297379214
                .quad 15534100272597517150
                .quad 14757395258967641292
                .quad 14054662151397753612
                .quad 13415813871788764811
                .quad 12832517616493601124
                .quad 12297829382473034410
                .quad 11805916207174113034
                .quad 11351842506898185609
                .quad 10931403895531586142
                .quad 10540996613548315209
                .quad 10177513971701821581
                .quad 9838263505978427528
                .quad 9520900167075897608
                .quad 9223372036854775808
                .quad 8943875914525843207
.Linfmantisa:
                .int 0x7fff
                .align 3
                //.word 0
.Lexppdata1:
                .quad 13294840995170
                .quad 140155567705833
                .quad 1420858861872864
                .quad 12298016920048126
                .quad 88711584777478112
                .quad 511935043705946563
                .quad 2215698446799909394
                .quad 6393154322601308688
                .quad 9223372036854775837
.Lexppdata2:
                .quad 15810319504286
                .quad 166673998323022
                .quad 1689695467953879
                .quad 14624889221745082
                .quad 105496447800544053
                .quad 608796796394340630
                .quad 2634924357634930296
                .quad 7602784607747877799
                .quad 10968499650544839058
.Lexppdata3:
                .quad 18801744444964
                .quad 198209904691689
                .quad 2009397872678605
                .quad 17392022318625859
                .quad 125457126331920151
                .quad 723985481863012800
                .quad 3133470793593433486
                .quad 9041285549366948089
                .quad 13043817825332782254
.Lexppdata4:
                .quad 22359168268414
                .quad 235712628923368
                .quad 2389590247060729
                .quad 20682716685595994
                .quad 149194507261714673
                .quad 860968686190168290
                .quad 3726345762394533902
                .quad 10751961104078460355
                .quad 15511800964685064997
