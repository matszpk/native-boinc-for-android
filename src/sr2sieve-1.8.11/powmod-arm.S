/*
 * powmod-arm.S
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

// shifted half of n and mask
#define nshift_st 32
// prepared a
#define newa_st 40
// a*(onep+fac)
#define abyp_st 48
#define nlpart_st 56
#define npartn_st 60
                
                .text
                .align 2
                .globl  powmod64_arm_shifted
                .type   powmod64_arm_shifted, %function
powmod64_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#64
                
                ldr r8,.LGOT1
.LPIC1:
                add r8,pc,r8
                ldr r9,.LGOT1+4
                ldr r8,[r8,r9]          // mod64_init_data
                
                ldr r6,[r8,#clzp_idx]
                sub r6,r6,#2
                str r6,[sp,#24]
                
                cmp r3,#0
                bhi .Lnoend
                cmpeq r2,#1
                movlo r0,#1
                lsllo r0,r0,r6
                movlo r1,#0
                bls .Lend
.Lnoend:
                tst r3,r3               // clz(b)
                beq .Lonlylowpart
                clz r4,r3
                teq r3,#1
                bne .Lnolowlast
                mov r6,#0x80000000
                b .Lsavenparts
.Lnolowlast:
                rsb r4,r4,#30
                mov r6,#1
                str r6,[sp,#npartn_st]
                lsl r6,r6,r4
                
                str r3,[sp,#nshift_st+4]        // n
                str r6,[sp,#nshift_st]
                str r2,[sp,#nlpart_st]
                b .Lskip1
.Lonlylowpart:
                clz r4,r2
                rsb r4,r4,#30
                mov r6,#1
                lsl r6,r6,r4
.Lsavenparts:
                mov r3,#0
                str r2,[sp,#nshift_st+4]        // n
                str r6,[sp,#nshift_st]
                str r3,[sp,#nlpart_st]
                str r3,[sp,#npartn_st]
.Lskip1:
                ldrd r4,[r8,#pshifted_idx]
                ldrd r6,[r8,#ptimes4_idx]
                strd r4,[sp]
                strd r6,[sp,#8]
                ldrd r4,[r8,#onep_idx]
                adds r4,r4,#14
                adc r5,r5,#0
                strd r4,[sp,#16]
                ldr r12,[r8,#pmask_idx]
                str r12,[sp,#28]
                
                /*
                 *first iteration
                 */
                ldrd r2,[sp]            // pshifted
                ldrd r10,[sp,#16]       // onep+fac
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd0
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd0:
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
                ldrd r2,[sp,#8]         // ptimes4
                lsr r9,r9,r4
                strd r8,[sp,#newa_st]        // newa
                strd r6,[sp,#abyp_st]         // abyp
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd1
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd1:
                /* end sqrmod b */
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod1
                
                /* mulmod a*b */
                adds r0,r0,r8           // x+p
                adc r1,r1,r9
                ldrd r2,[sp,#8]         // ptimes4
                ldrd r6,[sp,#abyp_st]        // abyp2
                ldrd r8,[sp,#newa_st]            // newa
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd2
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd2:
                /* end mulmod a*b */
.Lnomulmod1:
                tst r4,r4
                str r4,[sp,#nshift_st]
                beq .Lendloop1
                /**
                 * end of first iteration
                 */

                /* higher part of n */
                /* main loop */
.Lloop1:
                /* sqrmod b */
                ldrd r2,[sp]            // pshifted
                ldrd r10,[sp,#16]       // onep+fac
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd3
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd3:
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
                ldrd r2,[sp,#8]         // ptimes4
                lsr r9,r9,r4
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd4
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd4:
                /* end sqrmod b */
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod2
                
                /* mulmod a*b */
                adds r0,r0,r8           // x+p
                adc r1,r1,r9
                ldrd r2,[sp,#8]         // ptimes4
                ldrd r6,[sp,#abyp_st]       // abyp2
                ldrd r8,[sp,#newa_st]        // newa
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd5
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd5:
                /* end mulmod a*b */
.Lnomulmod2:
                tst r4,r4
                str r4,[sp,#nshift_st]
                bne .Lloop1
.Lendloop1:                
                ldr r8,[sp,#npartn_st]
                subs r8,#1
                blo .Lend
                str r8,[sp,#npartn_st]
                
                mov r10,#0x80000000
                ldr r11,[sp,#nlpart_st]
                strd r10,[sp,#nshift_st]
                b .Lloop1
.Lend:
                add sp,sp,#64
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word mod64_init_data(GOT)
                
                .align 2
                .globl  vec_powmod64_arm_shifted
                .type   vec_powmod64_arm_shifted, %function
vec_powmod64_arm_shifted:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#80
                
                add r1,r0,r1,lsl #3
                strd r0,[sp,#64]
                strd r2,[sp,#72]

                ldr r8,.LGOT2
.LPIC2:
                add r8,pc,r8
                ldr r9,.LGOT2+4
                ldr r8,[r8,r9]          // mod64_init_data
                
                ldr r6,[r8,#clzp_idx]
                sub r6,r6,#2
                str r6,[sp,#24]
                
                ldrd r4,[r8,#pshifted_idx]
                strd r4,[sp]
                ldrd r4,[r8,#ptimes4_idx]
                strd r4,[sp,#8]
                ldrd r4,[r8,#onep_idx]
                adds r4,r4,#14
                adc r5,r5,#0
                strd r4,[sp,#16]
                ldr r12,[r8,#pmask_idx]
                str r12,[sp,#28]
                
                mov r4,r0
                mov r5,r1
                
                cmp r4,r5
                beq .Lendvecloop
.Lvecloop:
                ldrd r2,[sp,#72]
                ldrd r0,[r4]
                
                cmp r3,#0
                bhi .Lnoend2
                cmpeq r2,#1
                movlo r0,#1
                lsllo r0,r0,r6
                movlo r1,#0
                bls .Lend2
.Lnoend2:
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
                ldr r12,[sp,#28]
                /*
                 *first iteration
                 */
                ldrd r2,[sp]            // pshifted
                ldrd r10,[sp,#16]       // onep+fac
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd6
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd6:
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
                ldrd r2,[sp,#8]         // ptimes4
                lsr r9,r9,r4
                strd r8,[sp,#newa_st]        // newa
                strd r6,[sp,#abyp_st]         // abyp
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd7
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd7:
                /* end sqrmod b */
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod3
                
                /* mulmod a*b */
                adds r0,r0,r8           // x+p
                adc r1,r1,r9
                ldrd r2,[sp,#8]         // ptimes4
                ldrd r6,[sp,#abyp_st]        // abyp2
                ldrd r8,[sp,#newa_st]            // newa
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd8
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd8:
                /* end mulmod a*b */
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
                /* sqrmod b */
                ldrd r2,[sp]            // pshifted
                ldrd r10,[sp,#16]       // onep+fac
                
                adds r0,r0,r2
                adc r1,r1,r3
                
                mov r8,r0
                mov r9,r1
                tst r1,#0x80000000        // mybit>=newb
                bmi .Lnoadd9
                adds r8,r8,r2           // newb+=p
                adc r9,r9,r3
.Lnoadd9:
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
                ldrd r2,[sp,#8]         // ptimes4
                lsr r9,r9,r4
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd10
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd10:
                /* end sqrmod b */
                ldrd r4,[sp,#nshift_st]
                tst r4,r5
                lsr r4,r4,#1
                beq .Lnomulmod4
                
                /* mulmod a*b */
                adds r0,r0,r8           // x+p
                adc r1,r1,r9
                ldrd r2,[sp,#8]         // ptimes4
                ldrd r6,[sp,#abyp_st]       // abyp2
                ldrd r8,[sp,#newa_st]        // newa
                
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r0,r1 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r6,r7 - bbyp
                 * r12 - and mask
                 * output -
                 * r0,r1 - shifted modulo
                 */
                // main op
                umull lr,r5,r0,r7     // newa*bbyp
                umull r10,r11,r1,r6
                umull r6,r7,r1,r7
                adds lr,lr,r10
                adcs r5,r5,r11
                adc r7,r7,#0
                adds r6,r6,r5
                adc r7,r7,#0           // result in r10,r11
                and r6,r6,r12           // and masking
                umull lr,r5,r0,r8        // newa*newb
                mla r5,r1,r8,r5
                mla r5,r0,r9,r5          // result in lr,r5
                umull r0,r1,r2,r6       // tmp*p
                mla r1,r2,r7,r1
                mla r1,r3,r6,r1         // result in r0,r1
                ldrd r8,[sp]          // pshifted
                subs r0,lr,r0
                sbcs r1,r5,r1            // newa*newb-tmp*p
                bpl .Lnoadd11
                adds r0,r0,r8
                adc r1,r1,r9
.Lnoadd11:
                /* end mulmod a*b */
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
                ldrd r4,[sp,#64]
                strd r0,[r4]
                add r4,r4,#8
                str r4,[sp,#64]
                cmp r4,r5
                bne .Lvecloop
.Lendvecloop:
                add sp,sp,#80
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.LGOT2:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC2+8)
                .word mod64_init_data(GOT)
