/*
 * milkyway_util_arm.S
 * Mateusz Szpakowski
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
                .global setupARMFPU
                .type   setupARMFPU,%function
setupARMFPU:
                fmrx r0,fpscr
                and r0,#0xff3fffff
                fmxr fpscr,r0
                bx lr
