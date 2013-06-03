/*
 * vfp_ChirpData.S
 * Author: Mateusz Szpakowski
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
                .align  2
.Lhalfd:
                .double 0.5
.LroundVal:
                .double 4503599627370496.0
.Linc8d:
                .double 0,1,2,3,4,5,6,7
.Lsincosapprox:
                .float 1.5707963268,-0.6466386396,0.0679105987,-0.0011573807   // sine
                .float 1.0,-1.2341299769,0.2465220241,-0.0123926179     // cosine
                
                .align  2
                .global vfp_ChirpData
                .type   vfp_ChirpData, %function
vfp_ChirpData:
                push {r4,r5,r6,r7,r8,lr}
                vpush {d8,d9,d10,d11,d12,d13,d14,d15}
#define stargidx (24+64)
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
                sub r3,r3,#8*7
                
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
                
                sub sp,sp,#24+32+8
                add r7,sp,#24
                fstmiad sp,{d9,d10}
                
                mov r4,#0       // i
                
                adr r5,.Linc8d
                adr r6,.Lsincosapprox
                adr r8,.Lsincosapprox+4*4
                
                cmp r0,r3
                bhs .Lendmainloop
.Lmainloop:
                fmsr s24,r4
                fuitod d8,s24
                fldmiad r5,{d0,d1,d2,d3,d4,d5,d6,d7}
                fldmiad sp,{d9,d10}
                faddd d0,d0,d8
                faddd d1,d1,d8
                faddd d2,d2,d8
                faddd d3,d3,d8
                faddd d4,d4,d8
                faddd d5,d5,d8
                faddd d6,d6,d8
                faddd d7,d7,d8
                // square of i
                fmuld d0,d0,d0
                fmuld d1,d1,d1
                fmuld d2,d2,d2
                fmuld d3,d3,d3
                fmuld d4,d4,d4
                fmuld d5,d5,d5
                fmuld d6,d6,d6
                fmuld d7,d7,d7
                // multiply by srate
                fmuld d0,d0,d9
                fmuld d1,d1,d9
                fmuld d2,d2,d9
                fmuld d3,d3,d9
                fmuld d4,d4,d9
                fmuld d5,d5,d9
                fmuld d6,d6,d9
                fmuld d7,d7,d9
                
                // rounding to -0.5/+0.5
                faddd d12,d0,d10
                faddd d13,d1,d10
                faddd d14,d2,d10
                faddd d15,d3,d10
                fsubd d12,d12,d10
                fsubd d13,d13,d10
                fsubd d14,d14,d10
                fsubd d15,d15,d10
                fsubd d0,d0,d12
                fsubd d1,d1,d13
                fsubd d2,d2,d14
                fsubd d3,d3,d15
                // second half of xxxx
                faddd d12,d4,d10
                faddd d13,d5,d10
                faddd d14,d6,d10
                faddd d15,d7,d10
                fsubd d12,d12,d10
                fsubd d13,d13,d10
                fsubd d14,d14,d10
                fsubd d15,d15,d10
                fsubd d4,d4,d12
                fsubd d5,d5,d13
                fsubd d6,d6,d14
                fsubd d7,d7,d15
                // to single precision
                fcvtsd s24,d0
                fcvtsd s25,d1
                fcvtsd s26,d2
                fcvtsd s27,d3
                fcvtsd s28,d4
                fcvtsd s29,d5
                fcvtsd s30,d6
                fcvtsd s31,d7
                
                fldmias r6,{s16,s17,s18,s19}
                // square of y
                fmuls s0,s24,s24
                fmuls s1,s25,s25
                fmuls s2,s26,s26
                fmuls s3,s27,s27
                fmuls s4,s28,s28
                fmuls s5,s29,s29
                fmuls s6,s30,s30
                fmuls s7,s31,s31
                // sine
                fmuls s8,s0,s19
                fmuls s9,s1,s19
                fmuls s10,s2,s19
                fmuls s11,s3,s19
                fmuls s12,s4,s19
                fmuls s13,s5,s19
                fmuls s14,s6,s19
                fmuls s15,s7,s19
                fadds s8,s8,s18
                fadds s9,s9,s18
                fadds s10,s10,s18
                fadds s11,s11,s18
                fadds s12,s12,s18
                fadds s13,s13,s18
                fadds s14,s14,s18
                fadds s15,s15,s18
                fmuls s8,s8,s0
                fmuls s9,s9,s1
                fmuls s10,s10,s2
                fmuls s11,s11,s3
                fmuls s12,s12,s4
                fmuls s13,s13,s5
                fmuls s14,s14,s6
                fmuls s15,s15,s7
                fadds s8,s8,s17
                fadds s9,s9,s17
                fadds s10,s10,s17
                fadds s11,s11,s17
                fadds s12,s12,s17
                fadds s13,s13,s17
                fadds s14,s14,s17
                fadds s15,s15,s17
                fmuls s8,s8,s0
                fmuls s9,s9,s1
                fmuls s10,s10,s2
                fmuls s11,s11,s3
                fmuls s12,s12,s4
                fmuls s13,s13,s5
                fmuls s14,s14,s6
                fmuls s15,s15,s7
                fadds s8,s8,s16
                fadds s9,s9,s16
                fadds s10,s10,s16
                fadds s11,s11,s16
                fadds s12,s12,s16
                fadds s13,s13,s16
                fadds s14,s14,s16
                fadds s15,s15,s16
                fmuls s8,s8,s24
                fmuls s9,s9,s25
                fmuls s10,s10,s26
                fmuls s11,s11,s27
                fmuls s12,s12,s28
                fmuls s13,s13,s29
                fmuls s14,s14,s30
                fmuls s15,s15,s31
                fldmias r8,{s24,s25,s26,s27}
                // cosine
                fmuls s16,s0,s27
                fmuls s17,s1,s27
                fmuls s18,s2,s27
                fmuls s19,s3,s27
                fmuls s20,s4,s27
                fmuls s21,s5,s27
                fmuls s22,s6,s27
                fmuls s23,s7,s27
                fadds s16,s16,s26
                fadds s17,s17,s26
                fadds s18,s18,s26
                fadds s19,s19,s26
                fadds s20,s20,s26
                fadds s21,s21,s26
                fadds s22,s22,s26
                fadds s23,s23,s26
                fmuls s16,s16,s0
                fmuls s17,s17,s1
                fmuls s18,s18,s2
                fmuls s19,s19,s3
                fmuls s20,s20,s4
                fmuls s21,s21,s5
                fmuls s22,s22,s6
                fmuls s23,s23,s7
                fadds s16,s16,s25
                fadds s17,s17,s25
                fadds s18,s18,s25
                fadds s19,s19,s25
                fadds s20,s20,s25
                fadds s21,s21,s25
                fadds s22,s22,s25
                fadds s23,s23,s25
                fmuls s16,s16,s0
                fmuls s17,s17,s1
                fmuls s18,s18,s2
                fmuls s19,s19,s3
                fmuls s20,s20,s4
                fmuls s21,s21,s5
                fmuls s22,s22,s6
                fmuls s23,s23,s7
                fadds s16,s16,s24
                fadds s17,s17,s24
                fadds s18,s18,s24
                fadds s19,s19,s24
                fadds s20,s20,s24
                fadds s21,s21,s24
                fadds s22,s22,s24
                fadds s23,s23,s24
                // load sine
                // doubling cosine/sine
                fmuls s0,s8,s16 // c*s
                fmuls s1,s9,s17
                fmuls s2,s10,s18
                fmuls s3,s11,s19
                fmuls s4,s12,s20
                fmuls s5,s13,s21
                fmuls s6,s14,s22
                fmuls s7,s15,s23
                fmuls s16,s16,s16 // c*c
                fmuls s17,s17,s17
                fmuls s18,s18,s18
                fmuls s19,s19,s19
                fmuls s20,s20,s20
                fmuls s21,s21,s21
                fmuls s22,s22,s22
                fmuls s23,s23,s23
                fnmacs s16,s8,s8  // c*c-s*s = x
                fnmacs s17,s9,s9
                fnmacs s18,s10,s10
                fnmacs s19,s11,s11
                fnmacs s20,s12,s12
                fnmacs s21,s13,s13
                fnmacs s22,s14,s14
                fnmacs s23,s15,s15
                fadds s0,s0,s0  // 2*s*c = y
                fadds s1,s1,s1
                fadds s2,s2,s2
                fadds s3,s3,s3
                fadds s4,s4,s4
                fadds s5,s5,s5
                fadds s6,s6,s6
                fadds s7,s7,s7
                fmuls s8,s0,s16 // cd1 = x*y
                fmuls s9,s1,s17
                fmuls s10,s2,s18
                fmuls s11,s3,s19
                fmuls s12,s4,s20
                fmuls s13,s5,s21
                fmuls s14,s6,s22
                fmuls s15,s7,s23
                fmuls s0,s0,s0 // cd3 = y*y
                fmuls s1,s1,s1
                fmuls s2,s2,s2
                fmuls s3,s3,s3
                fmuls s4,s4,s4
                fmuls s5,s5,s5
                fmuls s6,s6,s6
                fmuls s7,s7,s7
                fmuls s24,s16,s16 // cd2 = x*x
                fmuls s25,s17,s17
                fmuls s26,s18,s18
                fmuls s27,s19,s19
                fmuls s28,s20,s20
                fmuls s29,s21,s21
                fmuls s30,s22,s22
                fmuls s31,s23,s23
                fadds s24,s24,s0  // norm = x*x+y*y
                fadds s25,s25,s1
                fadds s26,s26,s2
                fadds s27,s27,s3
                fadds s28,s28,s4
                fadds s29,s29,s5
                fadds s30,s30,s6
                fadds s31,s31,s7
                fmscs s0,s16,s16   // c = x*x-y*y
                fmscs s1,s17,s17
                fmscs s2,s18,s18
                fmscs s3,s19,s19
                fmscs s4,s20,s20
                fmscs s5,s21,s21
                fmscs s6,s22,s22
                fmscs s7,s23,s23
                fadds s16,s8,s8     // s = 2*x*y
                fadds s17,s9,s9
                fadds s18,s10,s10
                fadds s19,s11,s11
                fadds s20,s12,s12
                fadds s21,s13,s13
                fadds s22,s14,s14
                fadds s23,s15,s15
                fsts s7,[sp,#24+64]
                flds s7,.Ltwos
                fstmias r7,{s16,s17,s18,s19,s20,s21,s22,s23}
                // reciprocal of magnitude
                // iter 1: invmag = 2.0-mag
                fsubs s8,s7,s24
                fsubs s9,s7,s25
                fsubs s10,s7,s26
                fsubs s11,s7,s27
                fsubs s12,s7,s28
                fsubs s13,s7,s29
                fsubs s14,s7,s30
                fsubs s15,s7,s31
                // iter 2: invmag = invmag*(2.0-mag*invmag)
                fmuls s16,s8,s24
                fmuls s17,s9,s25
                fmuls s18,s10,s26
                fmuls s19,s11,s27
                fmuls s20,s12,s28
                fmuls s21,s13,s29
                fmuls s22,s14,s30
                fmuls s23,s15,s31
                fsubs s16,s7,s16
                fsubs s17,s7,s17
                fsubs s18,s7,s18
                fsubs s19,s7,s19
                fsubs s20,s7,s20
                fsubs s21,s7,s21
                fsubs s22,s7,s22
                fsubs s23,s7,s23
                fmuls s8,s16,s8
                fmuls s9,s17,s9
                fmuls s10,s18,s10
                fmuls s11,s19,s11
                fmuls s12,s20,s12
                fmuls s13,s21,s13
                fmuls s14,s22,s14
                fmuls s15,s23,s15
                // iter 3: invmag = invmag*(2.0-mag*invmag)
                fmuls s16,s8,s24
                fmuls s17,s9,s25
                fmuls s18,s10,s26
                fmuls s19,s11,s27
                fmuls s20,s12,s28
                fmuls s21,s13,s29
                fmuls s22,s14,s30
                fmuls s23,s15,s31
                fsubs s16,s7,s16
                fsubs s17,s7,s17
                fsubs s18,s7,s18
                fsubs s19,s7,s19
                fsubs s20,s7,s20
                fsubs s21,s7,s21
                fsubs s22,s7,s22
                fsubs s23,s7,s23
                fmuls s8,s16,s8
                fmuls s9,s17,s9
                fmuls s10,s18,s10
                fmuls s11,s19,s11
                fmuls s12,s20,s12
                fmuls s13,s21,s13
                fmuls s14,s22,s14
                fmuls s15,s23,s15
                // restore sine values
                fldmias r7,{s16,s17,s18,s19,s20,s21,s22,s23}
                // correct cosine/sine
                flds s7,[sp,#24+64]
                fmuls s0,s0,s8
                fmuls s1,s1,s9
                fmuls s2,s2,s10
                fmuls s3,s3,s11
                fmuls s4,s4,s12
                fmuls s5,s5,s13
                fmuls s6,s6,s14
                fmuls s7,s7,s15
                fmuls s16,s16,s8
                fmuls s17,s17,s9
                fmuls s18,s18,s10
                fmuls s19,s19,s11
                fmuls s20,s20,s12
                fmuls s21,s21,s13
                fmuls s22,s22,s14
                fmuls s23,s23,s15
                pld [r0,#128]
                // multiply by data
                fldmias r0!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fmuls s24,s8,s0
                fmuls s25,s9,s0
                fmuls s26,s10,s1
                fmuls s27,s11,s1
                fmuls s28,s12,s2
                fmuls s29,s13,s2
                fmuls s30,s14,s3
                fmuls s31,s15,s3
                fnmacs s24,s9,s16
                fmacs s25,s8,s16
                fnmacs s26,s11,s17
                fmacs s27,s10,s17
                fnmacs s28,s13,s18
                fmacs s29,s12,s18
                fnmacs s30,s15,s19
                fmacs s31,s14,s19
                fstmias r1!,{s24,s25,s26,s27,s28,s29,s30,s31}
                pld [r0,#128]
                fldmias r0!,{s8,s9,s10,s11,s12,s13,s14,s15}
                fmuls s24,s8,s4
                fmuls s25,s9,s4
                fmuls s26,s10,s5
                fmuls s27,s11,s5
                fmuls s28,s12,s6
                fmuls s29,s13,s6
                fmuls s30,s14,s7
                fmuls s31,s15,s7
                fnmacs s24,s9,s20
                fmacs s25,s8,s20
                fnmacs s26,s11,s21
                fmacs s27,s10,s21
                fnmacs s28,s13,s22
                fmacs s29,s12,s22
                fnmacs s30,s15,s23
                fmacs s31,s14,s23
                fstmias r1!,{s24,s25,s26,s27,s28,s29,s30,s31}
                
                add r4,r4,#8
                cmp r0,r3
                blo .Lmainloop
.Lendmainloop:
                add r3,r3,#8*7
                cmp r0,r3
                bhs .Lendsmallloop
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
                add sp,sp,#24+32+8
.Lendoffunc:
                mov r0,#0
                vpop {d8,d9,d10,d11,d12,d13,d14,d15}
                pop {r4,r5,r6,r7,r8,lr}
                bx lr
                
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word analysis_state(GOT)
.Ltwos:
                .float 2.0
