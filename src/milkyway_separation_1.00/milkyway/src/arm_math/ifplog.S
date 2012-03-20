/*
 * ifplog.S
 * Author: Mateusz Szpakowski
 * License: GPLv3
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
                .align 3
                .align 3
                .global ifplog10
                .type   ifplog10, %function
ifplog10:
                push {r0,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,sp,#4
                
                bl ifplog_general
                
                mov r4,r3
                tst r2,r2
                cmpeq r3,#0x8000
                beq .Llog10_noshift
                
                ldrd r4,.Llog1_10
                ldr r6,.Llog1_10+8
                
                /// multiply by 1/log(10)
                mov r14,#0
                umull r7,r8,r0,r6
                umull r9,r10,r2,r4
                umull r11,r12,r1,r5
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r14,r14,#0
                adds r7,r7,r11
                adcs r0,r8,r12
                adc r14,r14,#0
                mov r11,#0
                umull r7,r8,r1,r6
                umull r9,r10,r2,r5
                adds r0,r0,r7
                adcs r14,r14,r8
                adc r11,r11,#0
                adds r0,r0,r9
                adcs r1,r14,r10
                adc r11,r11,#0
                umlal r1,r11,r2,r6
                mov r2,r11
                
                mov r4,r3
                
                tst r2,r2
                bmi .Llog10_noshift
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r2,r2,r2
                sub r3,r3,#1
                
                tst r2,r2
                bmi .Llog10_noshift
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r2,r2,r2
                sub r3,r3,#1
.Llog10_noshift:
                pkhbt r3,r3,r4
                add sp,sp,#4
                pop {r0,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                str r1,[r0]
                str r2,[r0,#4]
                str r3,[r0,#8]
                bx lr
          
                .align 3
.Llog1_10:
                .int 2595083607,2610182346,1865280596
                
                .global ifplog
                .type   ifplog, %function
ifplog:
                push {r0,r14}
                bl ifplog_general
                pop {r0,r14}
                str r1,[r0]
                str r2,[r0,#4]
                str r3,[r0,#8]
                bx lr
                .align 3 
.Llogpoints_log:
                .int 0,0,0
                .int 4114874352,360280820,252937143
                .int 634920690,2593792428,479197127
                .int 385249126,951944442,683874180
                .int 1830481118,2133891045,870729689
                .int 3104268838,1577965926,1042620095
                .int 2976290767,3238215927,1201765753
                .int 2465401809,432716177,1349926817
                
                .align 3
.Llogpoints:
                .int 0,0,2147483648
                .int 477218588,3340530119,1908874353
                .int 1717986918,1717986918,1717986918
                .int 3514064151,1952257861,1561806289
                .int 1431655765,1431655765,1431655765
                .int 3964585196,3303820996,1321528398
                .int 2454267026,613566756,1227133513
                .int 1145324612,1145324612,1145324612
                
.Llogseries:
                .int 0,0,2147483648
                .int 2863311530,2863311530,715827882
                .int 2576980377,2576980377,429496729
                .int 613566756,1227133513,306783378
                .int 3817748707,954437176,238609294
                .int 3123612578,780903144,195225786
                .int 2643056797,3634203096,165191049
                .int 2290649224,2290649224,143165576
                .int 2273806215,2273806215,126322567
                .int 4068916385,678152730,113025455
                .int 1636178017,409044504,102261126
                
                ///.global ifplog
                ///.type   ifplog, %function
ifplog_general:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                
                sub sp,sp,#24
                
                ldrd r2,[r0]
                ldr r1,[r0,#8]
                tst r1,r1
                bmi .Lifplog_domain_error
                
                tst r1,r1
                tsteq r2,r2
                cmpeq r3,#0x80000000
                beq .Lifone
                
                str r0,[sp,#16]
                
                adr r7,.Llogpoints
                lsr r6,r3,#26
                and r6,r6,#0x1c
                add r6,r6,lsl #1
                str r6,[sp,#20]     // index
                ldrd r4,[r7,r6]!
                ldr r6,[r7,#8]      // logpoints
                
                tst r1,r1
                bne .Lskip_low_xi_test
                and r7,r3,#0x7fffffff
                cmp r7,#(1<<19)
                /*
                 * if 1<x<1+small_number:
                 *   go to handling small results (0<y<small_number)
                 */
                blo .Llow_xi
.Lskip_low_xi_test:
                add r7,r1,#1
                cmp r7,#0x10000     // mantisa0xffff
                bne .Ltonormal
                cmp r3,#0xfff00000
                /*
                 * if 1small_number<x<1:
                 *   go to handling small results (0>y>-small_number)
                 */
                bhs .Lhigh_xi
.Ltonormal:
                // multiply: logpoints*x
                mov r11,#0
                umull r7,r8,r2,r5
                umull r9,r10,r3,r4
                adds r7,r7,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                mov r12,#0
                umull r9,r10,r3,r5
                adds r8,r8,r9
                adcs r11,r11,r10
                adc r12,r12,#0
                umull r9,r10,r2,r6
                adds r7,r8,r9
                adcs r8,r11,r10
                adc r9,r12,#0
                umlal r8,r9,r3,r6       // {r7,r8,r9}
                
                and r9,r9,#0x3fffffff
                
                lsrs r2,r9,#1
                movs r1,r8,rrx
                mov r0,r7,rrx   // shift right
                
                orr r2,r2,#0x40000000   // (1+xi)/4
                
                adds r7,r7,r7       // shift xi by 1 (final result)
                adcs r8,r8,r8
                adc r9,r9,r9
                
                bl ifprecip_96      // reciprocal
                
                lsrs r2,r2,#1
                movs r1,r1,rrx
                mov r0,r0,rrx   // shift right (recip)
                
                /// multiply recip by xai
                /// result: zi
                mov r10,#0
                umull r3,r4,r0,r9
                umull r5,r6,r2,r7
                umull r11,r12,r1,r8
                adds r3,r3,r5
                adcs r4,r4,r6
                adc r10,r10,#0
                adds r3,r3,r11
                adcs r4,r4,r12
                adc r10,r10,#0
                mov r12,#0
                umull r5,r6,r1,r9
                umull r0,r7,r2,r8
                adds r4,r4,r5
                adcs r10,r10,r6
                adc r12,r12,#0
                adds r0,r4,r0
                adcs r1,r10,r7
                adc r12,r12,#0
                umlal r1,r12,r2,r9  // zi={r0,r1,r12}
                
                /* standard routine */
                /* compute zi^2 */
                mov r7,#0
                umull r3,r4,r0,r12
                umull r5,r6,r1,r1
                adds r3,r3,r3
                adcs r4,r4,r4
                adc r7,r7,#0
                adds r3,r3,r5
                adcs r4,r4,r6
                adc r7,r7,#0
                mov r8,#0
                umull r5,r6,r1,r12
                adds r5,r5,r5
                adcs r6,r6,r6
                adc r8,r8,#0
                adds r4,r4,r5
                adcs r7,r7,r6
                adc r8,r8,#0
                umlal r7,r8,r12,r12     // {r4,r7,r8}
                
                adds r4,r4,r4
                adcs r7,r7,r7
                adc r8,r8,r8        // shift left 1
                
                adr r6,.Llogseries+12
                str r6,[sp]
                
                strd r0,[sp,#8]
                str r12,[sp,#4]
.Llogloop:
                /* zi*zi2 */
                mov r14,#0
                umull r6,r9,r0,r8
                umull r10,r11,r12,r4
                adds r6,r6,r10
                adcs r9,r9,r11
                adc r14,r14,#0
                umull r10,r11,r1,r7
                adds r6,r6,r10
                adcs r9,r9,r11
                adc r14,r14,#0
                mov r6,#0
                umull r10,r11,r12,r7
                adds r9,r9,r10
                adcs r14,r14,r11
                adc r6,r6,#0
                umull r10,r11,r1,r8
                adds r0,r9,r10
                adcs r1,r14,r11
                adc r6,r6,#0
                umlal r1,r6,r12,r8     // {r0,r1,r6}
                
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r12,r6,r6     // shift left 1
                
                // log series
                ldr r9,[sp]
                ldrd r10,[r9]
                ldr r6,[r9,#8]
                add r9,r9,#12
                str r9,[sp]
                
                // logseries*zi
                mov r14,#0
                umull r2,r3,r0,r6
                umull r5,r9,r12,r10
                adds r2,r2,r5
                adcs r3,r3,r9
                adc r14,r14,#0
                umull r5,r9,r1,r11
                adds r2,r2,r5
                adcs r3,r3,r9
                adc r14,r14,#0
                mov r2,#0
                umull r5,r9,r1,r6
                adds r3,r3,r5
                adcs r14,r14,r9
                adc r2,r2,#0
                umull r5,r9,r12,r11
                adds r10,r3,r5
                adcs r11,r14,r9
                adc r9,r2,#0
                umlal r11,r9,r12,r6  // {r10,r11,r9}
                
                adds r10,r10,r10
                adcs r11,r11,r11
                adc r9,r9,r9
                
                ldrd r2,[sp,#8]
                ldr r5,[sp,#4]
                
                adds r2,r2,r10
                adcs r3,r3,r11
                adc r5,r5,r9
                
                strd r2,[sp,#8]
                str r5,[sp,#4]
                
                tst r9,r9
                tsteq r11,r11
                bne .Llogloop
                
                cmp r10,#0x100
                
                // if is higher (1<<8)-1
                bhs .Llogloop
                
                // after loop
                ldrd r0,[sp,#8]
                ldr r2,[sp,#4]
                
                ldr r3,[sp,#20]
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r2,r2,r2
                
                adr r7,.Llogpoints_log
                ldrd r4,[r7,r3]!
                ldr r6,[r7,#8]
                
                adds r0,r0,r4       // logpoints_log+yi
                adcs r1,r1,r5
                adc r2,r2,r6
                
                b .Llog_after
.Lhigh_xi:
                /*
                 * if 1<x<1+small_number:
                 *   go to handling small results (0<y<small_number)
                 */
                sub sp,sp,#32
                
                mov r11,r2
                mov r12,r3
                
                mov r10,#0
                lsrs r12,r12,#1
                movs r11,r11,rrx
                mov r10,r10,rrx
                
                lsrs r2,r12,#1
                movs r1,r11,rrx
                mov r0,r10,rrx
                
                orr r2,r2,#0x40000000   // (1+xi)/4
                
                rsbs r10,r10,#0
                rscs r11,r11,#0
                rsc r12,r12,#0x80000000
                
                bl ifprecip_96      // reciprocal
                
                /* multiply recip*xi */
                mov r9,#0
                umull r3,r4,r1,r10
                umull r5,r6,r0,r11
                adds r3,r3,r5
                adcs r4,r4,r6
                adc r5,r9,#0
                mov r8,#0
                umull r3,r6,r2,r10
                umull r7,r9,r1,r11
                adds r4,r4,r3
                adcs r5,r5,r6
                adc r8,r8,#0
                adds r4,r4,r7
                adcs r5,r5,r9
                adc r6,r8,#0
                mov r10,#0
                umull r3,r7,r0,r12
                umull r8,r9,r2,r11
                adds r4,r4,r3
                adcs r5,r5,r7
                adc r6,r6,#0
                adds r5,r5,r8
                adcs r6,r6,r9
                adc r10,r10,#0
                umull r3,r8,r1,r12
                adds r5,r5,r3
                adcs r6,r6,r8
                adc r7,r10,#0
                umlal r6,r7,r2,r12       /// zi {r4,r5,r6,r7 }
                
                b .Lxi_exception_common
.Llow_xi:
                /*
                 * if 1<x<1+small_number:
                 *   go to handling small results (0<y<small_number)
                 */
                sub sp,sp,#32
                
                mov r11,r2
                mov r12,r3
                
                and r12,r12,#0x7fffffff
                
                lsl r0,r11,#30
                lsr r1,r11,#2
                orr r1,r1,r12,lsl #30
                lsr r2,r12,#2
                
                orr r2,r2,#0x40000000   // (1+xi)/4
                
                bl ifprecip_96      // reciprocal
                
                lsrs r2,r2,#1
                movs r1,r1,rrx
                mov r0,r0,rrx   // shift right (recip)
                
                /* recip*xi */
                umull r3,r4,r0,r11
                mov r8,#0
                umull r7,r5,r1,r11
                adds r4,r4,r7
                adcs r5,r5,#0
                adc r6,r8,#0
                mov r10,#0
                umull r3,r7,r0,r12
                umull r8,r9,r2,r11
                adds r4,r4,r3
                adcs r5,r5,r7
                adc r6,r6,#0
                adds r5,r5,r8
                adcs r6,r6,r9
                adc r10,r10,#0
                umull r3,r8,r1,r12
                adds r5,r5,r3
                adcs r6,r6,r8
                adc r7,r10,#0
                umlal r6,r7,r2,r12       /// zi {r4,r5,r6,r7 }

.Lxi_exception_common:
                strd r4,[sp]
                strd r6,[sp,#8]         // yi
                
                /* compute zi^2 */
                mov r8,#0
                mov r9,#0
                umull r0,r1,r4,r7
                umull r2,r3,r5,r6
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r8,r8,#0
                adds r2,r2,r2
                adcs r3,r3,r3
                adc r9,r9,#0
                adds r0,r0,r2
                adcs r0,r1,r3
                adc r1,r8,r9
                mov r3,#0
                umull r2,r8,r5,r7
                umull r9,r10,r6,r6
                adds r2,r2,r2
                adcs r8,r8,r8
                adc r3,r3,#0
                adds r0,r0,r2
                adcs r1,r1,r8
                adc r3,r3,#0
                adds r0,r0,r9
                adcs r1,r1,r10
                adc r2,r3,#0
                mov r3,#0
                umull r8,r9,r6,r7
                adds r8,r8,r8
                adcs r9,r9,r9
                adc r3,r3,#0
                adds r1,r1,r8
                adcs r2,r2,r9
                adc r3,r3,#0
                umlal r2,r3,r7,r7     // zi*zi={ r0,r1,r2,r3 }
                
                adds r0,r0,r0       // shift one
                adcs r1,r1,r1
                adcs r2,r2,r2
                adc r3,r3,r3
                
                adr r8,.Llogseries_1_2
                
                /// first iteration
                bl .Llog_highprec_iter
                
                ldrd r8,[sp,#24]
                tst r9,r9
                bne .Ldoseconditer
                cmpeq r8,#1<<12
                blo .Lskipseconditer
.Ldoseconditer:
                
                ldrd r0,[sp,#16]
                mov r2,r8
                mov r3,r9
                
                adr r8,.Llogseries_1_2+12
                /// second iteration
                bl .Llog_highprec_iter
.Lskipseconditer:
                
                adds r0,r0,r0   // final shift
                adcs r1,r1,r1
                adcs r2,r2,r2
                adc r3,r3,r3
                
                tst r3,r3
                beq .Lshiftover32_128
                
                clz r4,r3
                rsb r5,r4,#32
                
                lsl r3,r3,r4
                orr r3,r3,r2,lsr r5
                lsl r2,r2,r4
                orr r2,r2,r1,lsr r5
                lsl r1,r1,r4
                orr r1,r1,r0,lsr r5
                
                b .Lshiftfinish_128
.Lshiftover32_128:
                tst r2,r2
                beq .Lshiftover64_128
                
                clz r4,r2
                rsb r5,r4,#32
                
                lsl r3,r2,r4
                orr r3,r3,r1,lsr r5
                lsl r2,r1,r4
                orr r2,r2,r0,lsr r5
                lsl r1,r0,r4
                 
                add r4,r4,#32
                
                b .Lshiftfinish_128
.Lshiftover64_128:
                tst r1,r1
                beq .Lshiftover96_128
                
                clz r4,r1
                rsb r5,r4,#32
                
                lsl r3,r1,r4
                orr r3,r3,r0,lsr r5
                lsl r2,r0,r4
                mov r1,#0
                
                add r4,r4,#64
                
                b .Lshiftfinish_128
.Lshiftover96_128:
                clz r4,r0
                
                lsl r3,r0,r4
                mov r2,#0
                mov r1,#0
                
                add r4,r4,#96
.Lshiftfinish_128:
                // convert to ifp96
                mov r0,r1
                mov r1,r2
                mov r2,r3
                
                rsb r4,r4,#0
                ldr r6,[sp,#48]     // output address
                //strd r2,[r6]
                ldrsh r5,[r6,#8]
                pkhbt r3,r4,r5
                //str r4,[r6,#8]
                
                add sp,sp,#56
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
.Lifone:
                mov r1,#0
                mov r2,#0
                mov r3,#0x8000
                
                add sp,sp,#24
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                
                bx lr
                
                
.Llogseries_1_2:
                .int 2863311530,2863311530,715827882
                .int 2576980377,2576980377,429496729

                
.Llog_after:    /* after main routine:
                    generate final result number
                   */
                ldr r3,[sp,#16]     // output
                ldrsh r4,[r3,#8]    // mantisa
                
                tst r4,r4
                beq .Lifmantisa_zero
                cmp r4,#-1
                beq .Lifmantisa_minus1
                
                adr r9,.Llog2
                ldrd r6,[r9]      // log2
                ldr r8,[r9,#8]
                
                tst r4,r4
                rsbmi r4,r4,#0    // negate
                
                clz r5,r4
                lsl r4,r5
                
                umull r14,r10,r4,r6
                umull r11,r12,r4,r7
                adds r10,r10,r11
                adc r12,r12,#0
                umull r14,r11,r4,r8
                adds r12,r12,r14
                adcs r11,r11,#0      // { r10,r12,r11 }
                bmi .Lskipleftshift
                adds r10,r10,r10
                adcs r12,r12,r12
                adc r11,r11,r11
                add r5,r5,#1
.Lskipleftshift:
                add r5,r5,#1
                rsbs r6,r5,#32
                bmi .Ldofixresult
                beq .Lafterfixresult
                
                lsr r0,r0,r6
                orr r0,r0,r1,lsl r5
                lsr r1,r1,r6
                orr r1,r1,r2,lsl r5
                lsr r2,r2,r6
                
                b .Lafterfixresult
.Ldofixresult:
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r2,r2,r2
.Lafterfixresult:
                ldrsh r7,[r3,#8]    // mantisa
                tst r7,r7
                bmi .Ldonegate
                
                adds r0,r0,r10
                adcs r1,r1,r12
                adcs r2,r2,r11
                
                bcc .Lskipshift2
                movs r2,r2,rrx
                movs r1,r1,rrx
                mov r0,r0,rrx
                add r6,r6,#1
.Lskipshift2:
                b .Llogfinish
.Ldonegate:
                subs r0,r10,r0
                sbcs r1,r12,r1
                sbcs r2,r11,r2
                
                bmi .Lskipshift3
                adds r0,r0,r0
                adcs r1,r1,r1
                adc r2,r2,r2
                sub r6,r6,#1
.Lskipshift3:
.Llogfinish:
                tst r7,r7
                mov r4,#0
                submi r4,r4,#1
                
                //str r1,[r3]
                //str r2,[r3,#4]
                pkhbt r3,r6,r4
                //str r6,[r3,#8]
                
                add sp,sp,#24
                
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr               
                
                /* (128-bit) higher precision logarithm iteration */
.Llog_highprec_iter:
                /* used  by .Llow_xi and .Lhigh_xi */
                push {r8,lr}
                /////////////////////////////////
                //// zi*(zi2)
                mov r12,#0
                umull r8,r9,r0,r7
                umull r10,r11,r3,r4
                adds r8,r8,r10
                adcs r9,r9,r11
                adc r12,r12,#0
                umull r10,r11,r1,r6
                umull r4,r14,r2,r5
                adds r8,r8,r10
                adcs r9,r9,r11
                adc r12,r12,#0
                adds r8,r8,r4
                adcs r4,r9,r14
                adc r12,r12,#0
                mov r9,#0
                umull r8,r10,r3,r5
                umull r11,r14,r1,r7
                adds r4,r4,r8
                adcs r12,r12,r10
                adc r9,r9,#0
                adds r4,r4,r11
                adcs r12,r12,r14
                adc r9,r9,#0
                mov r11,#0
                umull r8,r10,r2,r6
                adds r4,r4,r8
                adcs r5,r12,r10
                adc r9,r9,#0
                umull r8,r10,r2,r7
                umull r12,r14,r3,r6
                adds r5,r5,r8
                adcs r9,r9,r10
                adc r11,r11,#0
                adds r5,r5,r12
                adcs r6,r9,r14
                adc r11,r11,#0
                umlal r6,r11,r3,r7     // new zi: {r4,r5,r6,r7 }
                
                adds r4,r4,r4
                adcs r5,r5,r5
                adcs r6,r6,r6
                adc r7,r11,r11
                
                strd r0,[sp,#24]
                strd r2,[sp,#32]
                
                //// toadd = logseries * zi
                ldr r11,[sp]
                ldrd r8,[r11]  // logseries[i]
                ldr r10,[r11,#8]
                
                mov r1,#0
                umull r2,r0,r8,r6
                umull r11,r12,r10,r4
                umull r3,r14,r9,r5
                adds r2,r2,r11
                adcs r0,r0,r12
                adc r1,r1,#0
                adds r2,r2,r3
                adcs r0,r0,r14
                adc r1,r1,#0
                mov r2,#0
                umull r3,r11,r8,r7
                umull r12,r14,r10,r5
                adds r0,r0,r3
                adcs r1,r1,r11
                adc r2,r2,#0
                adds r0,r0,r12
                adcs r1,r1,r14
                adc r2,r2,#0
                mov r3,#0
                umull r8,r11,r9,r6
                umull r12,r14,r10,r6
                adds r0,r0,r8
                adcs r1,r1,r11
                adc r2,r2,#0
                adds r1,r1,r12
                adcs r2,r2,r14
                adc r3,r3,#0
                umull r8,r11,r9,r7
                adds r1,r1,r8
                adcs r2,r2,r11
                adc r3,r3,#0
                umlal r2,r3,r10,r7
                
                adds r0,r0,r0
                adcs r1,r1,r1
                adcs r2,r2,r2
                adc r3,r3,r3
                
                ldrd r8,[sp,#8]
                ldrd r10,[sp,#16]
                
                adds r0,r8,r0
                adcs r1,r9,r1
                adcs r2,r10,r2
                adc r3,r11,r3
                
                strd r0,[sp,#8]
                strd r2,[sp,#16]
                /////////////////////////
                
                pop {r12,lr}
                bx lr
                
                .align 3
.Llog2:
                .quad 15118436252839555992
                .int 2977044471
                .int -1
.Llog10:
                .quad 12297267996955956779
                .int 2472381917
                .int 1
                .align 3
.Lifmantisa_minus1:
                /* if 1/exp(1)<=x<1: */
                ldrd r4,.Llog2
                ldr r6,.Llog2+8
                
                adds r0,r0,r0       // shift left one
                adcs r1,r1,r1
                adc r2,r2,r2
                
                subs r0,r4,r0
                sbcs r1,r5,r1
                sbc r2,r6,r2
                
                mov r8,#0xffffffff
                mov r9,#0xffffffff
                
                b .Lmantisa_common
                
                .align 3
.Lifmantisa_zero:
                /* if 0<=x<exp(1) */
                mov r8,#0
                mov r9,#0
.Lmantisa_common:
                tst r2,r2
                beq .Lshiftfracover32
                
                clz r5,r2
                rsb r4,r5,#32
                
                lsl r2,r2,r5
                orr r2,r2,r1,lsr r4
                lsl r1,r1,r5
                orr r1,r1,r0,lsr r4
                lsl r0,r0,r5
                
                b .Llogfinish2
.Lshiftfracover32:
                tst r1,r1
                beq .Lshiftfracover64
                
                clz r5,r1
                rsb r4,r5,#32
                
                lsl r2,r1,r5
                orr r2,r2,r0,lsr r4
                lsl r1,r0,r5
                mov r0,#0
                add r5,r5,#32
                
                b .Llogfinish2
.Lshiftfracover64:
                clz r5,r0
                rsb r4,r5,#32
                
                lsl r2,r0,r5
                mov r1,#0
                mov r0,#0
                add r5,r5,#64
.Llogfinish2:
                //str r1,[r3]
                //str r2,[r3,#4]
                rsb r6,r5,#0
                add r6,r6,r9
                pkhbt r3,r6,r8
                //str r6,[r3,#8]

                add sp,sp,#24
                
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
.Lifplog_domain_error:
                add sp,sp,#24
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr
                
                
                /* returns reciprocal (96-bit) */
                .global ifprecip_wrap_96
                .type   ifprecip_wrap_96, %function
ifprecip_wrap_96:
                /* ONLY FOR TESTING */
                push {r3,r4,r12,r14}
                
                bl ifprecip_96
                
                pop {r3,r4,r12,r14}
                strd r0,[r3]
                str r2,[r3,#8]
                bx lr
                
                /* returns reciprocal (96-bit), 0.5<x<1.0 */
                .global ifprecip_96
                .type   ifprecip_96, %function
ifprecip_96:
                push {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                sub sp,sp,#8
                
                lsr r3,r2,#26
                sub r3,r3,#16       // divpoint index
                
                adr r7,.Ldivpoints
                add r7,r7,r3,lsl #3
                add r7,r7,r3,lsl #2
                ldrd r4,[r7]
                ldr r6,[r7,#8]
                
                lsl r2,r2,#2        // shift by 2
                orr r2,r2,r1,lsr #30
                lsl r1,r1,#2
                orr r1,r1,r0,lsr #30
                lsl r0,r0,#2
                
                mov r8,#5
                str r8,[sp]
                // main iterations
.Lifprecip_loop:
                // iteration 0:
                mov r12,#0      // xi*ai
                umull r3,r7,r1,r5
                umull r8,r9,r0,r6
                umull r10,r11,r2,r4
                adds r3,r3,r8
                adcs r7,r7,r9
                adc r12,r12,#0
                adds r3,r3,r10
                adcs r7,r7,r11
                adc r12,r12,#0
                mov r14,#0
                umull r3,r8,r1,r6
                umull r9,r10,r2,r5
                adds r7,r7,r3
                adcs r12,r12,r8
                adc r14,r14,#0
                adds r7,r7,r9
                adcs r12,r12,r10
                adc r14,r14,#0
                umull r3,r8,r2,r6
                adds r12,r12,r3
                adc r14,r14,r8  // {r7,r12,r14}
                adds r7,r7,r4
                adcs r12,r12,r5
                adc r14,r14,r6  // ai*xi+ai
                rsbs r7,r7,#0
                rscs r12,r12,#0
                rscs r14,r14,#0  // -tmp
                bpl .Lskipdivfix
                mov r7,#0
                mov r12,#0
                mov r14,#0
.Lskipdivfix:
                // tmp*xi
                mov r11,#0
                umull r3,r8,r12,r5
                umull r9,r10,r7,r6
                adds r3,r3,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                umull r9,r10,r14,r4
                adds r3,r3,r9
                adcs r8,r8,r10
                adc r11,r11,#0
                mov r7,#0
                umull r9,r10,r12,r6
                adds r8,r8,r9
                adcs r11,r11,r10
                adc r7,r7,#0
                umull r9,r10,r14,r5
                adds r8,r8,r9
                adcs r11,r11,r10
                adc r7,r7,#0
                umull r9,r10,r14,r6
                adds r11,r11,r9
                adc r7,r7,r10       // {r8,r11,r7}
                adds r4,r8,r4
                adcs r5,r11,r5
                adc r6,r7,r6    // tmp*xi+xi
                
                ldr r8,[sp]
                subs r8,r8,#1
                str r8,[sp]
                bne .Lifprecip_loop
                
                mov r0,r4
                mov r1,r5
                mov r2,r6
                
                add sp,sp,#8
                pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
                bx lr

.Ldivpoints:
                .int 4042322160,4042322160,4042322160
                .int 954437176,2386092942,3817748707
                .int 1356305461,226050910,3616814565
                .int 3435973836,3435973836,3435973836
                .int 818089008,204522252,3272356035
                .int 2733161006,3904515723,3123612578
                .int 746950834,1493901668,2987803336
                .int 2863311530,2863311530,2863311530
                .int 1030792151,1889785610,2748779069
                .int 3634203096,2312674697,2643056797
                .int 3499602981,159072862,2545165805
                .int 613566756,1227133513,2454267026
                .int 1036716243,3554455693,2369637128
                .int 2290649224,2290649224,2290649224
                .int 1108378657,277094664,2216757314
                .int 0,0,2147483648
                .int 3253763103,4034666247,2082408385
