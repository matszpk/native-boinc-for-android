/*
 * giant-arm.S
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

                .text
                
#define mod64_init_data_IDX 4
#define BJ64_IDX 8
#define hsize_IDX 12
#define htable_IDX 16
#define hsize_minus1_IDX 20
#define olist_IDX 24

#define BJ64_st 0
#define htable_st 4
#define clzbm1_st 8
#define hsize_minus1_st 12
#define olist_st 16
#define bbyp2_st 24
#define pshifted_st 32
#define ptimes4_st 40
#define elimfun_st 48
#define pmask_st 52

#define b_st 112
#define p_st 120
#define tmpdat_st 56

                .align 2
                .global giant4_arm
                .type   giant4_arm, %function
giant4_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#72
                /* r0 - D64
                 * r1 - cc
                 * r2 - M
                 * r3 - fun
                 * sp+112 - b
                 * sp+120 - p
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
                ldrd r0,[sp,#b_st+16]        // b
                bl premulmod64_arm_init_shifted(PLT)
                pop {r0,r1,r2,r3}
                
                ldr r5,.LGOT0+mod64_init_data_IDX
                ldr r5,[r4,r5]
                ldr r6,[r5,#32]
                sub r6,r6,#3
                ldr r7,[r5,#64]
                ldrd r8,[r5,#40]
                ldrd r10,[r5,#48]
                str r6,[sp,#clzbm1_st]
                str r7,[sp,#pmask_st]
                strd r8,[sp,#ptimes4_st]
                strd r10,[sp,#pshifted_st]
                ldrd r6,[r5,#56]
                ldrd r8,[r5,#72]
                strd r6,[sp,#bbyp2_st]
                strd r8,[sp,#b_st]
                
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
.Lloop1:
                mov r4,r0
                ldrd r8,[sp,#pshifted_st]          // pshifted
                strd r0,[sp,#tmpdat_st+8]
                ldr r0,[sp,#pmask_st]
                ldrd r2,[sp,#ptimes4_st]         // p<<2
                
.Lloop2:
                /* MainCore new interface
                 * r2 - D->R+i
                 * input:
                 * r2,r3 - p<<2
                 * r6,r7 - new A (shifted) D64[i]
                 * r8,r9 - new B; // can be modified
                 * r10,r11 - bbyp
                 * r0 - and mask
                 * output -
                 * r6,r7 - shifted modulo
                 */
                /* iter 1 */
                ldrd r6,[r4]            // load D64[i]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#bbyp2_st]        // bbyp2
                ldrd r8,[sp,#b_st]            // b
                
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r0           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#pshifted_st]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd1l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd1l:
                strd r6,[r4]    // save to D64
                
                /* iter 2 */
                ldrd r6,[r4,#8]            // load D64[i]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#bbyp2_st]        // bbyp2
                ldrd r8,[sp,#b_st]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r0           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#pshifted_st]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd2l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd2l:
                strd r6,[r4,#8]    // save to D64
                
                /* iter 3 */
                ldrd r6,[r4,#16]            // load D64[i]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#bbyp2_st]        // bbyp2
                ldrd r8,[sp,#b_st]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r0           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#pshifted_st]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd3l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd3l:
                strd r6,[r4,#16]    // save to D64
                
                /* iter 4 */
                ldrd r6,[r4,#24]            // load D64[i]
                adds r6,r6,r8           // a+p
                adc r7,r7,r9
                ldrd r10,[sp,#bbyp2_st]        // bbyp2
                ldrd r8,[sp,#b_st]            // b
                // main op
                umull r1,r5,r6,r11     // newa*bbyp
                umull r12,r14,r7,r10
                umull r10,r11,r7,r11
                adds r1,r1,r12
                adcs r5,r5,r14
                adc r11,r11,#0
                adds r10,r10,r5
                adc r11,r11,#0           // result in r10,r11
                and r10,r10,r0           // and masking
                umull r1,r5,r6,r8        // newa*newb
                mla r5,r7,r8,r5
                mla r5,r6,r9,r5          // result in r1,r5
                umull r6,r7,r2,r10       // tmp*p
                mla r7,r2,r11,r7
                mla r7,r3,r10,r7         // result in r6,r7
                ldrd r8,[sp,#pshifted_st]          // pshifted
                subs r6,r1,r6
                sbcs r7,r5,r7            // newa*newb-tmp*p
                bpl .Lnoadd4l
                adds r6,r6,r8
                adc r7,r7,r9
.Lnoadd4l:
                strd r6,[r4,#24]    // save to D64
                
                ldr r1,[sp,#tmpdat_st+12]
                add r4,r4,#32
                cmp r4,r1       // cc
                blo .Lloop2
                
                ldrd r0,[sp,#tmpdat_st+8]
                ///////////////////
                // searching hashtable
                mov r4,r0
                
                ldrd r2,[sp,#clzbm1_st] // and hsize_minus1
                ldr r5,[sp,#htable_st]
                ldr r9,[sp,#BJ64_st]
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
                ldrd r10,[sp,#p_st+16]
                
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
                
                add sp,sp,#72
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word mod64_init_data(GOT)
                .word BJ64(GOT)
                .word hsize(GOT)
                .word htable(GOT)
                .word hsize_minus1(GOT)
                .word olist(GOT)
