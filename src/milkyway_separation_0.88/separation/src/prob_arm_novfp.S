/*
 * prob_arm_novfp.S
 * Author: Mateusz Szpakowski
 * License: LGPLv2
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
                
                .global prob_arm_novfp
                .type   prob_arm_novfp,%function
                /* bg_probability_fast_hprob for ARM architecture */
prob_arm_novfp:
                mov r0,#0
                ldr r1,[r0]     // sigsegv
                bx lr
