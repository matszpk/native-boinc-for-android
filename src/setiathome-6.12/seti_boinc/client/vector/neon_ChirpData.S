/*
 * vfp_ChirpData.S
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
                .align  2
.Lhalfd:
                .double 0.5
.LroundVal:
                .double 4503599627370496.0
                .align 4
.Linc8d:
                .double 0,1,2,3,4,5,6,7
.Lsinapprox:
                .float 1.5707963268,1.5707963268,1.5707963268,1.5707963268
                .float -0.6466386396,-0.6466386396,-0.6466386396,-0.6466386396
                .float 0.0679105987,0.0679105987,0.0679105987,0.0679105987
                .float -0.0011573807,-0.0011573807,-0.0011573807,-0.0011573807
.Lcosapprox:
                .float 1.0,1.0,1.0,1.0
                .float -1.2341299769,-1.2341299769,-1.2341299769,-1.2341299769
                .float 0.2465220241,0.2465220241,0.2465220241,0.2465220241
                .float -0.0123926179,-0.0123926179,-0.0123926179,-0.0123926179
.Ltwosq:
                .float 2.0,2.0,2.0,2.0
                .float 2.0,2.0,2.0,2.0
                .float 2.0,2.0,2.0,2.0
                .float 2.0,2.0,2.0,2.0
                
                .align  2
                .global neon_ChirpData
                .type   neon_ChirpData, %function
neon_ChirpData:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#define stargidx (40+64)
                /* r0 - input data
                 * r1 - output data
                 * r2 - chirprateind
                 * sp[0-1] - chirp_rate
                 * sp[2] - numDataPoints
                 * sp[4-5] - sample_rate
                 */
                tst r2,r2
                bne .Lrealfunc
                mov r3,r0   // swap addresses
                mov r0,r1
                mov r1,r3
                ldr r2,[sp,#stargidx+8]     // numDataPoints
                lsl r2,r2,#3
                bl memcpy(PLT)
                b .Lendoffunc
                /*
                 * real functions
                 */
.Lrealfunc:
                ldr r4,.LGOT1
.LPIC1:
                add r4,pc,r4
                ldr r5,.LGOT1+4
                ldr r5,[r4,r5]
                
                ldr r3,[sp,#stargidx+8]     // numDataPoints
                
                add r6,r3,r3,lsl #1
                lsl r6,r6,#2
                fldd d0,[r5,#32]
                fmsr s4,r6
                fuitod d1,s4
                faddd d0,d0,d1
                fstd d0,[r5,#32]
                
                add r3,r0,r3,lsl #3
                sub r3,r3,#8*15
                
                fldd d11,.Lhalfd
                fldd d9,[sp,#stargidx]        // chirp_rate
                fldd d10,[sp,#stargidx+16]   // sample_rate
                fmuld d10,d10,d10
                fmuld d9,d9,d11
                fdivd d9,d9,d10
                
                fldd d10,.LroundVal
                fsubd d11,d11,d11   // zero
                fcmpd d9,d11
                fmstat
                fnegdmi d10,d10     // negate is negative srate
                
                sub sp,sp,#16+64
                add r7,sp,#16
                add r11,sp,#16+32
                fstmiad sp,{d9,d10}
                
                mov r4,#0       // i
                
                adr r5,.Linc8d
                adr r6,.Lsinapprox
                adr r10,.Lcosapprox
                adr r9,.Ltwosq
                
                cmp r0,r3
                bhs .Lendmainloop
.Lmainloop:
                add r8,r4,#8
                fmsr s8,r4
                fmsr s9,r8
                fuitod d0,s8
                fuitod d1,s9
                fldmiad r5,{d8,d9,d10,d11,d12,d13,d14,d15}
                fldmiad sp,{d2,d3}
                faddd d16,d8,d0
                faddd d17,d9,d0
                faddd d18,d10,d0
                faddd d19,d11,d0
                faddd d20,d12,d0
                faddd d21,d13,d0
                faddd d22,d14,d0
                faddd d23,d15,d0
                faddd d24,d8,d1
                faddd d25,d9,d1
                faddd d26,d10,d1
                faddd d27,d11,d1
                faddd d28,d12,d1
                faddd d29,d13,d1
                faddd d30,d14,d1
                faddd d31,d15,d1
                // square of i
                fmuld d16,d16,d16
                fmuld d17,d17,d17
                fmuld d18,d18,d18
                fmuld d19,d19,d19
                fmuld d20,d20,d20
                fmuld d21,d21,d21
                fmuld d22,d22,d22
                fmuld d23,d23,d23
                fmuld d24,d24,d24
                fmuld d25,d25,d25
                fmuld d26,d26,d26
                fmuld d27,d27,d27
                fmuld d28,d28,d28
                fmuld d29,d29,d29
                fmuld d30,d30,d30
                fmuld d31,d31,d31
                // multiply by srate
                fmuld d16,d16,d2
                fmuld d17,d17,d2
                fmuld d18,d18,d2
                fmuld d19,d19,d2
                fmuld d20,d20,d2
                fmuld d21,d21,d2
                fmuld d22,d22,d2
                fmuld d23,d23,d2
                fmuld d24,d24,d2
                fmuld d25,d25,d2
                fmuld d26,d26,d2
                fmuld d27,d27,d2
                fmuld d28,d28,d2
                fmuld d29,d29,d2
                fmuld d30,d30,d2
                fmuld d31,d31,d2
                
                // rounding to -0.5/+0.5
                faddd d8,d16,d3
                faddd d9,d17,d3
                faddd d10,d18,d3
                faddd d11,d19,d3
                faddd d12,d20,d3
                faddd d13,d21,d3
                faddd d14,d22,d3
                faddd d15,d23,d3
                fsubd d8,d8,d3
                fsubd d9,d9,d3
                fsubd d10,d10,d3
                fsubd d11,d11,d3
                fsubd d12,d12,d3
                fsubd d13,d13,d3
                fsubd d14,d14,d3
                fsubd d15,d15,d3
                fsubd d16,d16,d8
                fsubd d17,d17,d9
                fsubd d18,d18,d10
                fsubd d19,d19,d11
                fsubd d20,d20,d12
                fsubd d21,d21,d13
                fsubd d22,d22,d14
                fsubd d23,d23,d15
                
                faddd d8,d24,d3
                faddd d9,d25,d3
                faddd d10,d26,d3
                faddd d11,d27,d3
                faddd d12,d28,d3
                faddd d13,d29,d3
                faddd d14,d30,d3
                faddd d15,d31,d3
                fsubd d8,d8,d3
                fsubd d9,d9,d3
                fsubd d10,d10,d3
                fsubd d11,d11,d3
                fsubd d12,d12,d3
                fsubd d13,d13,d3
                fsubd d14,d14,d3
                fsubd d15,d15,d3
                fsubd d24,d24,d8
                fsubd d25,d25,d9
                fsubd d26,d26,d10
                fsubd d27,d27,d11
                fsubd d28,d28,d12
                fsubd d29,d29,d13
                fsubd d30,d30,d14
                fsubd d31,d31,d15
                // to single precision
                fcvtsd s0,d16
                fcvtsd s1,d17
                fcvtsd s2,d18
                fcvtsd s3,d19
                fcvtsd s4,d20
                fcvtsd s5,d21
                fcvtsd s6,d22
                fcvtsd s7,d23
                fcvtsd s8,d24
                fcvtsd s9,d25
                fcvtsd s10,d26
                fcvtsd s11,d27
                fcvtsd s12,d28
                fcvtsd s13,d29
                fcvtsd s14,d30
                fcvtsd s15,d31
                
                //fldmias r6,{s8,s9,s10,s11,s12,s13,s14,s15}
                vldmia r6,{q8,q9,q10,q11}
                // square of y
                vmul.f32 q4,q0,q0
                vmul.f32 q5,q1,q1
                vmul.f32 q6,q2,q2
                vmul.f32 q7,q3,q3
                // sine
                vmul.f32 q12,q4,q11
                vmul.f32 q13,q5,q11
                vmul.f32 q14,q6,q11
                vmul.f32 q15,q7,q11
                vadd.f32 q12,q12,q10
                vadd.f32 q13,q13,q10
                vadd.f32 q14,q14,q10
                vadd.f32 q15,q15,q10
                vmul.f32 q12,q12,q4
                vmul.f32 q13,q13,q5
                vmul.f32 q14,q14,q6
                vmul.f32 q15,q15,q7
                vadd.f32 q12,q12,q9
                vadd.f32 q13,q13,q9
                vadd.f32 q14,q14,q9
                vadd.f32 q15,q15,q9
                vmul.f32 q12,q12,q4
                vmul.f32 q13,q13,q5
                vmul.f32 q14,q14,q6
                vmul.f32 q15,q15,q7
                vadd.f32 q12,q12,q8
                vadd.f32 q13,q13,q8
                vadd.f32 q14,q14,q8
                vadd.f32 q15,q15,q8
                vmul.f32 q12,q12,q0
                vmul.f32 q13,q13,q1
                vmul.f32 q14,q14,q2
                vmul.f32 q15,q15,q3
                vldmia r10,{q0,q1,q2,q3}
                // cosine
                vmul.f32 q8,q4,q3
                vmul.f32 q9,q5,q3
                vmul.f32 q10,q6,q3
                vmul.f32 q11,q7,q3
                vadd.f32 q8,q8,q2
                vadd.f32 q9,q9,q2
                vadd.f32 q10,q10,q2
                vadd.f32 q11,q11,q2
                vmul.f32 q8,q8,q4
                vmul.f32 q9,q9,q5
                vmul.f32 q10,q10,q6
                vmul.f32 q11,q11,q7
                vadd.f32 q8,q8,q1
                vadd.f32 q9,q9,q1
                vadd.f32 q10,q10,q1
                vadd.f32 q11,q11,q1
                vmul.f32 q8,q8,q4
                vmul.f32 q9,q9,q5
                vmul.f32 q10,q10,q6
                vmul.f32 q11,q11,q7
                vadd.f32 q8,q8,q0
                vadd.f32 q9,q9,q0
                vadd.f32 q10,q10,q0
                vadd.f32 q11,q11,q0
                // q8-q11 - cosine, q12-q15 - sine
                // doubling cosine/sine
                vmul.f32 q4,q8,q12      // c*s
                vmul.f32 q5,q9,q13
                vmul.f32 q6,q10,q14
                vmul.f32 q7,q11,q15
                vmul.f32 q0,q8,q8       // c*c
                vmul.f32 q1,q9,q9
                vmul.f32 q2,q10,q10
                vmul.f32 q3,q11,q11
                vmls.f32 q0,q12,q12    // c*c-s*s
                vmls.f32 q1,q13,q13
                vmls.f32 q2,q14,q14
                vmls.f32 q3,q15,q15
                vadd.f32 q4,q4,q4   // 2*c*s
                vadd.f32 q5,q5,q5
                vadd.f32 q6,q6,q6
                vadd.f32 q7,q7,q7
                // next doubling cosine/sine
                vmul.f32 q8,q0,q4      // cd1 = x*y
                vmul.f32 q9,q1,q5
                vmul.f32 q10,q2,q6
                vmul.f32 q11,q3,q7
                vmul.f32 q4,q4,q4    // cd3 = y*y
                vmul.f32 q5,q5,q5
                vmul.f32 q6,q6,q6
                vmul.f32 q7,q7,q7
                vmul.f32 q0,q0,q0    // cd2 = x*x
                vmul.f32 q1,q1,q1
                vmul.f32 q2,q2,q2
                vmul.f32 q3,q3,q3
                vadd.f32 q12,q0,q4  // norm = cd2+cd3
                vadd.f32 q13,q1,q5
                vadd.f32 q14,q2,q6
                vadd.f32 q15,q3,q7
                vadd.f32 q8,q8,q8   // s = cd1*2
                vadd.f32 q9,q9,q9
                vadd.f32 q10,q10,q10
                vadd.f32 q11,q11,q11
                vsub.f32 q0,q0,q4     // c = cd2-cd3
                vsub.f32 q1,q1,q5
                vsub.f32 q2,q2,q6
                vsub.f32 q3,q3,q7
                // c - q0-q3, s - q8-q11, norm - q12-q15
                vstmia r7,{q8,q9,q10,q11}
                vldmia r9,{q4,q5,q6,q7}
                // reciprocal of magnitude
                // iter 1: invmag = 2.0-mag
                vsub.f32 q4,q4,q12
                vsub.f32 q5,q5,q13
                vsub.f32 q6,q6,q14
                vsub.f32 q7,q7,q15
                // iter 2: invmag = invmag*(2.0-mag*invmag)
                vrecps.f32 q8,q4,q12
                vrecps.f32 q9,q5,q13
                vrecps.f32 q10,q6,q14
                vrecps.f32 q11,q7,q15
                vmul.f32 q4,q4,q8
                vmul.f32 q5,q5,q9
                vmul.f32 q6,q6,q10
                vmul.f32 q7,q7,q11
                vldmia r7,{q8,q9,q10,q11}
                // invnorm - q4-q7
                // correct cosine/sine
                vmul.f32 q0,q0,q4
                vmul.f32 q1,q1,q5
                vmul.f32 q2,q2,q6
                vmul.f32 q3,q3,q7
                vmul.f32 q8,q8,q4
                vmul.f32 q9,q9,q5
                vmul.f32 q10,q10,q6
                vmul.f32 q11,q11,q7
                vzip.32 q0,q8
                vzip.32 q1,q9
                vzip.32 q2,q10
                vzip.32 q3,q11
                vstmia r7,{q2,q3}
                vstmia r11,{q10,q11}
                // multiply by data
                pld [r0,#256]
                vldmia r0!,{q12,q13,q14,q15}
                vmul.f32 q4,q12,q0
                vmul.f32 q5,q13,q8
                vmul.f32 q6,q14,q1
                vmul.f32 q7,q15,q9
                vrev64.32 q2,q0
                vrev64.32 q10,q8
                vrev64.32 q3,q1
                vrev64.32 q11,q9
                vmul.f32 q2,q2,q12
                vmul.f32 q10,q10,q13
                vmul.f32 q3,q3,q14
                vmul.f32 q11,q11,q15
                fnegs s17,s17
                fnegs s19,s19
                fnegs s21,s21
                fnegs s23,s23
                fnegs s25,s25
                fnegs s27,s27
                fnegs s29,s29
                fnegs s31,s31
                vtrn.32 q4,q2
                vtrn.32 q5,q10
                vtrn.32 q6,q3
                vtrn.32 q7,q11
                vadd.f32 q4,q4,q2
                vadd.f32 q5,q5,q10
                vadd.f32 q6,q6,q3
                vadd.f32 q7,q7,q11
                vstmia r1!,{q4,q5,q6,q7}
                vldmia r7,{q0,q1}
                vldmia r11,{q8,q9}
                pld [r0,#256]
                vldmia r0!,{q12,q13,q14,q15}
                vmul.f32 q4,q12,q0
                vmul.f32 q5,q13,q8
                vmul.f32 q6,q14,q1
                vmul.f32 q7,q15,q9
                vrev64.32 q2,q0
                vrev64.32 q10,q8
                vrev64.32 q3,q1
                vrev64.32 q11,q9
                vmul.f32 q2,q2,q12
                vmul.f32 q10,q10,q13
                vmul.f32 q3,q3,q14
                vmul.f32 q11,q11,q15
                fnegs s17,s17
                fnegs s19,s19
                fnegs s21,s21
                fnegs s23,s23
                fnegs s25,s25
                fnegs s27,s27
                fnegs s29,s29
                fnegs s31,s31
                vtrn.32 q4,q2
                vtrn.32 q5,q10
                vtrn.32 q6,q3
                vtrn.32 q7,q11
                vadd.f32 q4,q4,q2
                vadd.f32 q5,q5,q10
                vadd.f32 q6,q6,q3
                vadd.f32 q7,q7,q11
                vstmia r1!,{q4,q5,q6,q7}
                
                add r4,r4,#16
                cmp r0,r3
                blo .Lmainloop
.Lendmainloop:
                add r3,r3,#8*15
                cmp r0,r3
                bhs .Lendsmallloop
                adr r6,.Lsincosapprox
.Lsmallloop:
                fmsr s24,r4
                fldmiad sp,{d9,d10}
                fuitod d0,s24
                // square of i
                fmuld d0,d0,d0
                // multiply by srate
                fmuld d0,d0,d9
                // rounding to -0.5/+0.5
                faddd d12,d0,d10
                fsubd d12,d12,d10
                fsubd d0,d0,d12
                fcvtsd s24,d0
                
                fldmias r6,{s8,s9,s10,s11,s12,s13,s14,s15}
                // square of y
                fmuls s0,s24,s24
                // sine/cosine
                fmuls s16,s0,s11
                fmuls s17,s0,s15
                fadds s16,s16,s10
                fadds s17,s17,s14
                fmuls s16,s16,s0
                fmuls s17,s17,s0
                fadds s16,s16,s9
                fadds s17,s17,s13
                fmuls s16,s16,s0
                fmuls s17,s17,s0
                fadds s16,s16,s8
                fadds s17,s17,s12   // s16 - sine
                fmuls s16,s16,s24   // s17 - cosine
                // doubling cosine/sine
                fmuls s18,s16,s17
                fmuls s19,s16,s16
                fmuls s20,s17,s17
                fadds s18,s18,s18  // y=2*s*c
                fsubs s19,s20,s19  // x=c*c-s*s
                fmuls s21,s18,s19  // cd1
                fmuls s22,s19,s19  // cd2
                fmuls s23,s18,s18  // cd3
                fsubs s8,s22,s23  // c
                fadds s9,s21,s21  // s
                // compute 1.0/norm
                fadds s10,s22,s23 // mag
                // reciprocal
                flds s11,.Ltwos
                // iter1: invmag = 2.0-mag
                fsubs s12,s11,s10
                // iter2: invmag = invmag*(2.0-invmag*mag)
                fmuls s13,s12,s10
                fsubs s13,s11,s13
                fmuls s12,s12,s13
                // iter3: invmag = invmag*(2.0-invmag*mag)
                fmuls s13,s12,s10
                fsubs s13,s11,s13
                fmuls s12,s12,s13
                // correct cosine/sine
                fmuls s8,s8,s12
                fmuls s9,s9,s12
                // multiply data
                fldmias r0!,{s14,s15}
                fmuls s6,s14,s8
                fmuls s7,s14,s9
                fnmacs s6,s15,s9
                fmacs s7,s15,s8
                fstmias r1!,{s6,s7}
                
                add r4,r4,#1
                cmp r0,r3
                blo .Lsmallloop
.Lendsmallloop:
                add sp,sp,#16+64
.Lendoffunc:
                mov r0,#0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word analysis_state(GOT)
                .align 2
.Ltwos:
                .float 2.0
.Lsincosapprox:
                .float 1.5707963268,-0.6466386396,0.0679105987,-0.0011573807   // sine
                .float 1.0,-1.2341299769,0.2465220241,-0.0123926179     // cosine
