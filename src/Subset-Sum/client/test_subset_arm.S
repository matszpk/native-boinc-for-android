/*
 * test_subset_arm.S
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
                .align  2
                .global test_subset_main_arm
                .type   test_subset_main_arm, %function
test_subset_main_arm:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                sub sp,sp,#16
                str r2,[sp]
                /* r0 - subset array ptr
                 * r1 - subset_size
                 * r2 - checksum
                 * r3 - max_sums_length
                 */
                ldr r4,.LGOT0
.LPIC0:
                add r4,pc,r4
                ldr r5,.LGOT0+4
                ldr r6,[r4,r5]
                ldr r6,[r6]     // sums
                str r6,[sp,#4]
                
                add r1,r0,r1,lsl #2
                mov lr,r0       // previous subset
                mov r12, #0
                
                // subset should be zeroed at end
.Lsumloop:
                ldrd r8,[lr]
                ldrd r10,[lr,#8]
                
                add r12,r12,r8
                add r12,r12,r9
                add r12,r12,r10
                add r12,r12,r11
                
                add lr,lr,#16
                cmp lr,r1
                blo .Lsumloop
                str r12,[sp,#12]        // subsetsum
                
                // first iteration
                mov lr,r6
                add r7,r6,r3,lsl #2
                mov r10,#0
                mov r11,#0
.Lzeroloop0:
                strd r10,[lr]
                strd r10,[lr,#8]
                add lr,lr,#16
                cmp lr,r7
                blo .Lzeroloop0
                
                sub lr,r7,#4    // sums+length-1
                
                ldr r5,[r0]     // subset[0]
                sub r5,r5,#1
                mov r4,#1
                and r9,r5,#0x1f         // tmp=(number%ELEM_SIZE)
                lsr r10,r5,#5   // pos
                ldr r8,[lr,-r10,lsl #2]  // sums[length-pos-1]
                orr r8,r8,r4,lsl r9
                str r8,[lr,-r10,lsl #2]  // sums[length-pos-1]
                
                add r0,r0,#4
                cmp r0,r1
                bhs .Lendloop1
                
                // next iterations
                /**
                 * register usage
                 * r6 - sums
                 * r7 - end of sums
                 * r0 - subset
                 * r1 - subset end
                 * r2 - current
                 * lr - sums+length-1
                 * r4 - 1
                 */
                
                /* subshift != 0 */
.Lloop1:
                ldr r6,[sp,#4]  // sums
                ldr r2,[r0]
                ands r4,r2,#0x1f
                beq .Lsubshift0
                
                bic r8,r2,#0x1f
                rsb r5,r4,#32
                add r3,r6,r8,lsr #3     // sums+full_elem_shifts
                ldr r12,[r3]
                add r3,r3,#4
                /* r4 - sub_shift
                 * r5 - 32-sub_shift
                 * r3 - sums+full_elem_shifts
                 */
.Loeloop1:
                ldrd r8,[r6]
                ldrd r10,[r3]
                orr r8,r8,r12,lsl r4
                orr r8,r8,r10,lsr r5
                orr r9,r9,r10,lsl r4
                orr r9,r9,r11,lsr r5
                strd r8,[r6]
                mov r12,r11
                
                add r3,r3,#8
                add r6,r6,#8
                cmp r3,r7
                blo .Loeloop1
                
                // last and or single
                ldr r8,[r6]
                sub r2,r2,#1    // or_single
                orr r8,r8,r12,lsl r4
                and r9,r2,#0x1f // or_single
                str r8,[r6]
                lsr r2,r2,#5    // or_single
                
                mov r4,#1
                // or_single
                ldr r11,[lr,-r2, lsl #2]        // or_single
                orr r11,r11,r4,lsl r9
                str r11,[lr,-r2, lsl #2]
                
.Lenditer:
                add r0,r0,#4
                cmp r0,r1
                blo .Lloop1
.Lendloop1:
                // BOINC code
                // load checksum
                ldr r2,[sp]
                ldr r3,[r2]
                ldr r6,[sp,#4]
                sub r7,r7,#4
                
                cmp r6,r7
                bhs .Lendcheckloop
.Lcheckloop:
                ldrd r8,[r6]
                // perform: if(UINT32_MAX-checksum<=sums[i]):
                //            checksum+=sums[i]
                //          else checksums+=sums[i]-(UINT32_MAX-checksum)
                cmp r0,r0 // set c flags
                adcs r3,r3,r8
                subcs r3,r3,#1
                
                cmp r0,r0 // set c flags
                adcs r3,r3,r9
                subcs r3,r3,#1
                
                add r6,r6,#8
                cmp r6,r7
                blo .Lcheckloop
.Lendcheckloop:
                add r7,r7,#4
                
                cmp r6,r7
                bhs .Lendcheck
                
                ldr r8,[r6]
                
                cmp r0,r0  // set c flags
                adcs r3,r3,r8
                subcs r3,r3,#1
.Lendcheck:
                
                str r3,[r2]     // save checksum
                ldr r0,[sp,#12]
                add sp,sp,#16
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
                bx lr
                
.Lsubshift0:
                add r3,r6,r2,lsr #3     // sums+full_elem_shifts
                /* r4 - sub_shift
                 * r5 - 32-sub_shift
                 * r3 - sums+full_elem_shifts
                 */
.Loeloop2:
                ldrd r8,[r6]
                ldrd r10,[r3]
                orr r8,r8,r10
                orr r9,r9,r11
                strd r8,[r6]
                
                add r6,r6,#8
                add r3,r3,#8
                cmp r3,r7
                blo .Loeloop2
                
                // last and or single
                ldr r8,[r6]
                sub r2,r2,#32    // or_single
                orr r8,r8,r10
                ldr r11,[lr,-r2, lsr #3]        // or_single
                str r8,[r6]
                
                // or_single
                orr r11,r11,#0x80000000
                str r11,[lr,-r2, lsr #3]
                
                b .Lenditer
.LGOT0:
                .word _GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
                .word sums(GOT)
