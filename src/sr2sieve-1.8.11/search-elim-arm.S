/*
 * search-elim-arm.S
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
#define htable_st 0
#define olist_st 4
#define clzbm1_st 8
#define elimfun_st 12
#define i_st 16
#define p_st 68

                .align  2
                .global search_elim_arm
                .type   search_elim_arm, %function
search_elim_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#24
                
                /* r0 - D64
                 * r1 - cc
                 * r2 - i
                 * r3 - elimfun */
                str r3,[sp,#elimfun_st]
                str r2,[sp,#i_st]  // save regs
                
                ldr r4,.LGOT0
.LPIC0:
                add r4,pc,r4
                ldr r5,.LGOT0+8
                ldr r6,.LGOT0+12
                ldr r7,.LGOT0+16
                ldr r10,.LGOT0+20
                ldr r5,[r4,r5]
                ldr r6,[r4,r6]
                ldr r7,[r4,r7]
                ldr r10,[r4,r10]
                ldr r9,[r5]
                ldr r3,[r6]
                lsl r3,r3,#1
                ldr r8,[r7]
                ldr r11,[r10]
                str r8,[sp,#htable_st]
                str r11,[sp,#olist_st]
                
                ldr r12,.LGOT0+4         // mod_init_data
                ldr r12,[r4,r12]
                ldr lr,[r12,#32]
                sub r2,lr,#3
                
                add r1,r0,r1,lsl #3
                
                ///////////////////
                // searching hashtable
                mov r4,r0
                
                ldr r5,[sp,#htable_st]
.Lloop1:
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
                ldr r0,[sp,#i_st+16]
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
                blo .Lloop1
                
                add sp,sp,#24
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                .align  2
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word mod64_init_data(GOT)
                .word BJ64(GOT)
                .word hsize_minus1(GOT)
                .word htable(GOT)
                .word olist(GOT)
