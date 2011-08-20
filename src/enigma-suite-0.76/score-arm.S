/*
 * score-arm.S
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
/*
 * icscore function
 */ 
                .align  2
                .global icscore
                .type   icscore, %function
icscore:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,sp,#26*4
                
                mov r5,#0
                mov r4,#0
                
                /* zeroing f's */
                strd r4,[sp]
                strd r4,[sp,#4*2]
                strd r4,[sp,#4*4]
                strd r4,[sp,#4*6]
                strd r4,[sp,#4*8]
                strd r4,[sp,#4*10]
                strd r4,[sp,#4*12]
                strd r4,[sp,#4*14]
                strd r4,[sp,#4*16]
                strd r4,[sp,#4*18]
                strd r4,[sp,#4*20]
                strd r4,[sp,#4*22]
                strd r4,[sp,#4*24]
                
                ldr r9,.LGOT1
.LPIC1:
                add r9,pc,r9
                ldr r4,.LGOT1+4
                ldr r3,[r9,r4]
                
                add r14,r1,r2,lsl #2
                sub r14,r14,#28
                cmp r1,r14
                bpl .Lloop1end
.Lloop1:
                ldr r4,[r1]                     // ciphertext[i]
                ldr r5,[r1,#4]
                ldr r6,[r1,#8]
                ldr r7,[r1,#12]
                ldr r8,[r1,#16]
                ldr r9,[r1,#20]
                ldr r10,[r1,#24]
                ldr r11,[r1,#28]
                ldr r4,[r0,r4,lsl #2]           // stbrett[ciphertext[i]]
                ldr r5,[r0,r5,lsl #2]
                ldr r6,[r0,r6,lsl #2]
                ldr r7,[r0,r7,lsl #2]
                ldr r8,[r0,r8,lsl #2]
                ldr r9,[r0,r9,lsl #2]
                ldr r10,[r0,r10,lsl #2]
                ldr r11,[r0,r11,lsl #2]
                ldr r4,[r3,r4,lsl #2]           // path_lookup[i][c1]
                add r5,r5,#26
                add r6,r6,#26*2
                add r7,r7,#26*3
                add r8,r8,#26*4
                add r9,r9,#26*5
                add r10,r10,#26*6
                add r11,r11,#26*7
                ldr r5,[r3,r5,lsl #2]
                ldr r6,[r3,r6,lsl #2]
                ldr r7,[r3,r7,lsl #2]
                ldr r8,[r3,r8,lsl #2]
                ldr r9,[r3,r9,lsl #2]
                ldr r10,[r3,r10,lsl #2]
                ldr r11,[r3,r11,lsl #2]
                ldr r4,[r0,r4,lsl #2]           //stbrett[c1]
                ldr r5,[r0,r5,lsl #2]
                ldr r6,[r0,r6,lsl #2]
                ldr r7,[r0,r7,lsl #2]
                ldr r8,[r0,r8,lsl #2]
                ldr r9,[r0,r9,lsl #2]
                ldr r10,[r0,r10,lsl #2]
                ldr r11,[r0,r11,lsl #2]
                
                ldr r12,[sp,r4,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r4,lsl #2]
                ldr r12,[sp,r5,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r5,lsl #2]
                ldr r12,[sp,r6,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r6,lsl #2]
                ldr r12,[sp,r7,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r7,lsl #2]
                ldr r12,[sp,r8,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r8,lsl #2]
                ldr r12,[sp,r9,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r9,lsl #2]
                ldr r12,[sp,r10,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r10,lsl #2]
                ldr r12,[sp,r11,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r11,lsl #2]
                
                add r3,r3,#4*26*8
                add r1,r1,#32
                cmp r1,r14
                blo .Lloop1
.Lloop1end:
                add r14,r14,#28
                cmp r1,r14
                bhs .Lloop2end
.Lloop2:
                ldr r4,[r1]                     // ciphertext[i]
                ldr r4,[r0,r4,lsl #2]           // stbrett[ciphertext[i]]
                ldr r4,[r3,r4,lsl #2]           // path_lookup[i][c1]
                ldr r4,[r0,r4,lsl #2]           //stbrett[c1]
                ldr r12,[sp,r4,lsl #2]          // f[c1]++
                add r12,r12,#1
                str r12,[sp,r4,lsl #2]
                
                add r3,r3,#4*26
                add r1,r1,#4
                cmp r1,r14
                blo .Lloop2
.Lloop2end:
                /* compute score */
                add r9,sp,#92
                mov r1,#0
                mov r2,#0
                mov r3,#0
                mov r4,#0
.Lloop3:
                ldr r5,[sp]
                ldr r6,[sp,#4]
                ldr r7,[sp,#8]
                ldr r8,[sp,#12]
                mla r1,r5,r5,r1
                mla r2,r6,r6,r2
                mla r3,r7,r7,r3
                mla r4,r8,r8,r4
                sub r1,r1,r5
                sub r2,r2,r6
                sub r3,r3,r7
                sub r4,r4,r8
                add sp,sp,#16
                cmp sp,r9
                blo .Lloop3
                
                ldr r5,[sp]
                ldr r6,[sp,#4]
                mla r1,r5,r5,r1
                mla r2,r6,r6,r2
                sub r1,r1,r5
                sub r2,r2,r6
                
                add r1,r1,r2
                add r3,r3,r4
                add r0,r1,r3
                
                add sp,sp,#8
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                .Lend1:
                .align  2
.LGOT1:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC1+8)
                .word path_lookup(GOT)
                
/*
 * biscore function
 */ 
                .align  2
                .global biscore
                .type   biscore, %function
biscore:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,sp,#4
                ldr r9,.LGOT2
.LPIC2:
                add r9,pc,r9
                ldr r4,.LGOT2+4         // path_lookup
                ldr r3,[r9,r4]          // path_lookup
                ldr r12,.LGOT2+8
                ldr r12,[r9,r12]        // bidict
                str r12,[sp]
                
                ldr r4,[r1]             // ciphertext[0]
                ldr r4,[r0,r4,lsl #2]   // stbrett[ciphertext[0]]
                ldr r4,[r3,r4,lsl #2]   // path_lookup[0][c1]
                ldr r4,[r0,r4,lsl #2]   // stbrett[c1]
                
                add r1,r1,#4
                add r3,r3,#26*4
                add r14,r1,r2,lsl #2
                sub r14,r14,#28
                
                mov r2,#0
                cmp r1,r14
                bpl .Lloop4end
.Lloop4:
                ldr r5,[r1]             // ciphertext[i]
                ldr r6,[r1,#4]
                ldr r7,[r1,#8]
                ldr r8,[r1,#12]
                ldr r9,[r1,#16]
                ldr r10,[r1,#20]
                ldr r11,[r1,#24]
                ldr r5,[r0,r5, lsl #2]  // stbrett[ciphertext[i]]
                ldr r6,[r0,r6, lsl #2]
                ldr r7,[r0,r7, lsl #2]
                ldr r8,[r0,r8, lsl #2]
                ldr r9,[r0,r9, lsl #2]
                ldr r10,[r0,r10, lsl #2]
                ldr r11,[r0,r11, lsl #2]
                ldr r5,[r3,r5,lsl #2]   // path_lookup[i][c1]
                add r6,r6,#26
                add r7,r7,#26*2
                add r8,r8,#26*3
                add r9,r9,#26*4
                add r10,r10,#26*5
                add r11,r11,#26*6
                ldr r6,[r3,r6,lsl #2]
                ldr r7,[r3,r7,lsl #2]
                ldr r8,[r3,r8,lsl #2]
                ldr r9,[r3,r9,lsl #2]
                ldr r10,[r3,r10,lsl #2]
                ldr r11,[r3,r11,lsl #2]
                mov r12,#26
                ldr r5,[r0,r5,lsl #2]     // stbrett[c1]
                mla r4,r4,r12,r5
                ldr r6,[r0,r6,lsl #2]
                mla r5,r5,r12,r6
                ldr r7,[r0,r7,lsl #2]
                mla r6,r6,r12,r7
                ldr r8,[r0,r8,lsl #2]
                mla r7,r7,r12,r8
                ldr r9,[r0,r9,lsl #2]
                mla r8,r8,r12,r9
                ldr r10,[r0,r10,lsl #2]
                mla r9,r9,r12,r10
                ldr r11,[r0,r11,lsl #2]
                mla r10,r10,r12,r11
                
                ldr r12,[sp]
                ldr r4,[r12,r4,lsl #2]               // bidict[c1][c2]
                ldr r5,[r12,r5,lsl #2]
                ldr r6,[r12,r6,lsl #2]
                ldr r7,[r12,r7,lsl #2]
                ldr r8,[r12,r8,lsl #2]
                ldr r9,[r12,r9,lsl #2]
                ldr r10,[r12,r10,lsl #2]
                add r2,r2,r4
                add r5,r5,r6
                add r7,r7,r8
                add r9,r9,r10
                add r2,r2,r5
                add r7,r7,r9
                add r2,r2,r7
                mov r4,r11              // c1=cX
                
                add r1,r1,#28
                add r3,r3,#26*28
                cmp r1,r14
                blo .Lloop4
.Lloop4end:
                add r14,r14,#24
                cmp r1,r14
                bhs .Lloop5end
                ldr r8,[sp]
                mov r12,#26
.Lloop5:
                ldr r5,[r1]             // ciphertext[i]
                ldr r5,[r0,r5, lsl #2]  // stbrett[ciphertext[i]]
                ldr r5,[r3,r5,lsl #2]   // path_lookup[i][c1]
                ldr r5,[r0,r5,lsl #2]     // stbrett[c1]
                mla r4,r4,r12,r5
                ldr r4,[r8,r4,lsl #2]               // bidict[c1][c2]
                add r2,r2,r4
                mov r4,r5
                
                add r1,r1,#4
                add r3,r3,#26*4
                cmp r1,r14
                blo .Lloop5
.Lloop5end:
                mov r0,r2       // s0
                add sp,sp,#4
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
.LGOT2:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC2+8)
                .word path_lookup(GOT)
                .word bidict(GOT)

/*
 * triscore function
 */ 
                .align  2
                .global triscore
                .type   triscore, %function
triscore:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,sp,#4
                ldr r9,.LGOT3
.LPIC3:
                add r9,pc,r9
                ldr r4,.LGOT3+4         // path_lookup
                ldr r3,[r9,r4]          // path_lookup
                ldr r12,.LGOT3+8
                ldr r12,[r9,r12]        // tridict
                str r12,[sp]
                
                ldr r4,[r1]             // ciphertext[0]
                ldr r5,[r1,#4]             // ciphertext[1]
                ldr r4,[r0,r4,lsl #2]   // stbrett[ciphertext[0]]
                ldr r5,[r0,r5,lsl #2]   // stbrett[ciphertext[1]]
                ldr r4,[r3,r4,lsl #2]   // path_lookup[0][c1]
                add r3,r3,#26*4
                ldr r5,[r3,r5,lsl #2]   // path_lookup[0][c2]
                ldr r4,[r0,r4,lsl #2]   // stbrett[c1]
                ldr r5,[r0,r5,lsl #2]   // stbrett[c2]
                
                add r1,r1,#8
                add r3,r3,#26*4
                add r14,r1,r2, lsl #2
                sub r14,r14,#28
                
                mov r2,#0
                cmp r1,r14
                bpl .Lloop6end
.Lloop6:
                ldr r6,[r1]             // ciphertext[i]
                ldr r7,[r1,#4]
                ldr r8,[r1,#8]
                ldr r9,[r1,#12]
                ldr r10,[r1,#16]
                ldr r11,[r1,#20]
                ldr r6,[r0,r6, lsl #2]  // stbrett[ciphertext[i]]
                ldr r7,[r0,r7, lsl #2]
                ldr r8,[r0,r8, lsl #2]
                ldr r9,[r0,r9, lsl #2]
                ldr r10,[r0,r10, lsl #2]
                ldr r11,[r0,r11, lsl #2]
                ldr r6,[r3,r6,lsl #2]   // path_lookup[i][c1]
                add r7,r7,#26
                add r8,r8,#26*2
                add r9,r9,#26*3
                add r10,r10,#26*4
                add r11,r11,#26*5
                ldr r7,[r3,r7,lsl #2]
                ldr r8,[r3,r8,lsl #2]
                ldr r9,[r3,r9,lsl #2]
                ldr r10,[r3,r10,lsl #2]
                ldr r11,[r3,r11,lsl #2]
                mov r12,#26
                ldr r6,[r0,r6,lsl #2]     // stbrett[c1]
                mla r4,r4,r12,r5
                ldr r7,[r0,r7,lsl #2]
                mla r4,r4,r12,r6
                ldr r8,[r0,r8,lsl #2]
                mla r5,r5,r12,r6
                ldr r9,[r0,r9,lsl #2]
                mla r5,r5,r12,r7
                ldr r10,[r0,r10,lsl #2]
                mla r6,r6,r12,r7
                ldr r11,[r0,r11,lsl #2]
                mla r6,r6,r12,r8
                mla r7,r7,r12,r8
                mla r7,r7,r12,r9
                mla r8,r8,r12,r9
                mla r8,r8,r12,r10
                mla r9,r9,r12,r10
                mla r9,r9,r12,r11
                ldr r12,[sp]
                ldr r4,[r12,r4,lsl #2]               // tridict[c1][c2][c3]
                ldr r5,[r12,r5,lsl #2]
                ldr r6,[r12,r6,lsl #2]
                ldr r7,[r12,r7,lsl #2]
                ldr r8,[r12,r8,lsl #2]
                ldr r9,[r12,r9,lsl #2]
                
                add r2,r2,r4
                add r5,r5,r6
                add r7,r7,r8
                add r2,r2,r9
                add r5,r5,r7
                add r2,r2,r5
                mov r4,r10
                mov r5,r11
                
                add r1,r1,#24
                add r3,r3,#26*24
                cmp r1,r14
                blo .Lloop6
.Lloop6end:
                add r14,r14,#20
                cmp r1,r14
                bhs .Lloop7end
                ldr r8,[sp]
                mov r12,#26
.Lloop7:
                ldr r6,[r1]             // ciphertext[i]
                ldr r6,[r0,r6, lsl #2]  // stbrett[ciphertext[i]]
                ldr r6,[r3,r6,lsl #2]   // path_lookup[i][c1]
                ldr r6,[r0,r6,lsl #2]     // stbrett[c1]
                
                mla r7,r4,r12,r5
                mov r4,r5
                mla r9,r7,r12,r6
                mov r5,r6
                ldr r6,[r8,r9,lsl #2]   // tridict[c1][c2][c3]
                add r2,r2,r6
                
                add r1,r1,#4
                add r3,r3,#26*4
                cmp r1,r14
                blo .Lloop7
.Lloop7end:
                mov r0,r2       // s0
                add sp,sp,#4
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
.LGOT3:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC3+8)
                .word path_lookup(GOT)
                .word tridict(GOT)
