/*
 * hash-arm.S
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
.macro POST_INSERT_CORE_2_S INDEX
                and lr,r3,r8,lsr r0
                mov r11,r1
                ldrh r10,[r4,lr]
                add r1,r1,#1
                cmp r10,r12
                ldrd r8,[r5,#\INDEX*8+8]
                
                eorne r10,r10,#0x8000
                strneh r10,[r2,#\INDEX*2]
                orrne r11,r11,#0x8000
                cmp r6,r8
                cmpeq r7,r9
                strh r11,[r4,lr]
.endm

.macro POST_INSERT_CORE_2 INDEX
                and lr,r3,r8,lsr r0
                beq .Lloop2end
                ldrh r10,[r4,lr]
                mov r11,r1
                cmp r10,r12
                add r1,r1,#1
                
                eorne r10,r10,#0x8000
                ldrd r8,[r5,#\INDEX*8+8]
                strneh r10,[r2,#\INDEX*2]
                orrne r11,r11,#0x8000
                cmp r6,r8
                strh r11,[r4,lr]
                cmpeq r7,r9
.endm

.macro POST_INSERT_CORE_2_E INDEX
                and lr,r3,r8,lsr r0
                beq .Lloop2end
                ldrh r10,[r4,lr]
                mov r11,r1
                cmp r10,r12
                add r1,r1,#1
                
                eorne r10,r10,#0x8000
                ldrd r8,[r5,#\INDEX*8+8]
                strneh r10,[r2,#\INDEX*2]
                orrne r11,r11,#0x8000
                cmp r6,r8
                strh r11,[r4,lr]
                cmpeq r7,r9
                
                beq .Lloop2end
.endm

                .align  2
                .global build_hashtable_arm
                .type   build_hashtable_arm, %function
build_hashtable_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#4
                str r0,[sp]     // m
                
                ldr r1,.LGOT1
.LPIC1:
                add r1,pc,r1
                ldr r2,.LGOT1+12
                ldr r3,[r1,r2]          // hsize
                ldr r3,[r3]
                ldr r2,.LGOT1+16
                ldr r4,[r1,r2]          // htable
                ldr r4,[r4]
                ldr r2,.LGOT1+4
                ldr r6,[r1,r2]         // empty slot
                ldrh r6,[r6]
                
                orr r6,r6,r6, lsl #16
                mov r7,r6
                
                add r3,r4,r3, lsl #1
                mov r5,r4
                
                // clear_hashtable
.Lloop1:
                strd r6,[r5]
                strd r6,[r5,#8]
                strd r6,[r5,#16]
                strd r6,[r5,#24]
                strd r6,[r5,#32]
                strd r6,[r5,#40]
                strd r6,[r5,#48]
                strd r6,[r5,#56]
                
                add r5,#64
                cmp r5,r3
                blo .Lloop1
                
                // put insert .....
                ldr r2,.LGOT1+28
                ldr r2,[r1,r2]          // mod64_init_data.clzp
                ldr r10,[r2,#32]
                sub r10,r10,#3
                ldr r2,.LGOT1+20
                ldr r3,[r1,r2]          // hsize_minus1
                ldr r3,[r3]
                ldr r2,.LGOT1+8
                ldr r5,[r1,r2]          // BJ64
                ldr r5,[r5]
                ldr r2,.LGOT1+24
                ldr r2,[r1,r2]          // olist
                ldr r2,[r2]
                lsl r3,r3,#1            // for slot
                
                
                uxth r12,r6
                add r6,r5,r12,lsl #3
                mov r8,#-1
                mov r9,#-1
                strd r8,[r6]    // BJ64[empty_slot]
                
                add r8,r5,r0,lsl #3
                
                mov r0,r10
                
                ldrd r6,[r5]            // BJ64[0]
                strd r6,[r8]            // BJ64[m]
                
                /* r0 - clzpm1
                 * r1 - j = 0
                 * r2 - olist
                 * r3 - hsize_minus1
                 * r4 - htable
                 * r5 - BJ64
                 * r12 - empty_slot */ 
                mov r1,#0
                
                ldrd r8,[r5]
.Lloop2:
                POST_INSERT_CORE_2_S 0
                POST_INSERT_CORE_2 1
                POST_INSERT_CORE_2 2
                POST_INSERT_CORE_2 3
                POST_INSERT_CORE_2 4
                POST_INSERT_CORE_2 5
                POST_INSERT_CORE_2 6
                POST_INSERT_CORE_2_E 7
                
                add r2,r2,#16
                add r5,r5,#64
                b .Lloop2
.Lloop2end:
                ldr r0,[sp]
                cmp r1,r0
                movlo r0,r1
                movhs r0,#0
                
                add sp,sp,#4
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
.Lend1:
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word   empty_slot(GOT)
                .word   BJ64(GOT)
                .word   hsize(GOT)
                .word   htable(GOT)
                .word   hsize_minus1(GOT)
                .word   olist(GOT)
                .word   mod64_init_data(GOT)
                