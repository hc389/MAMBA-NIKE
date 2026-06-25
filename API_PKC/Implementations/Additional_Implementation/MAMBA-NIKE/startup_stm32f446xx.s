/*
 * Minimal startup for STM32F446RE (Cortex-M4).
 * Initializes .data and .bss, calls main().
 *
 * This is a bare-minimum startup sufficient for running KAT tests.
 * For production use, replace with the full STM32Cube startup file
 * that initializes FPU, clocks, and peripherals.
 */

.syntax unified
.cpu cortex-m4
.thumb

.global g_pfnVectors
.global Default_Handler

.word _estack
.word Reset_Handler
.word NMI_Handler
.word HardFault_Handler
.word MemManage_Handler
.word BusFault_Handler
.word UsageFault_Handler
.word 0
.word 0
.word 0
.word 0
.word SVC_Handler
.word DebugMon_Handler
.word 0
.word PendSV_Handler
.word SysTick_Handler

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack

    /* Copy .data from FLASH to SRAM */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    b     .LoopCopyDataInit

.CopyDataInit:
    ldr   r3, [r2], #4
    str   r3, [r0], #4

.LoopCopyDataInit:
    cmp   r0, r1
    bcc   .CopyDataInit

    /* Zero-fill .bss */
    ldr   r0, =_sbss
    ldr   r1, =_ebss
    movs  r2, #0
    b     .LoopFillZerobss

.FillZerobss:
    str   r2, [r0], #4

.LoopFillZerobss:
    cmp   r0, r1
    bcc   .FillZerobss

    bl    main
    b     .

.size Reset_Handler, .-Reset_Handler

/* Default exception handlers — infinite loop */
.macro DEFAULT_IRQ_HANDLER name
.section .text.\name
.weak \name
.type \name, %function
\name:
    b .
.size \name, .-\name
.endm

DEFAULT_IRQ_HANDLER NMI_Handler
DEFAULT_IRQ_HANDLER HardFault_Handler
DEFAULT_IRQ_HANDLER MemManage_Handler
DEFAULT_IRQ_HANDLER BusFault_Handler
DEFAULT_IRQ_HANDLER UsageFault_Handler
DEFAULT_IRQ_HANDLER SVC_Handler
DEFAULT_IRQ_HANDLER DebugMon_Handler
DEFAULT_IRQ_HANDLER PendSV_Handler
DEFAULT_IRQ_HANDLER SysTick_Handler
DEFAULT_IRQ_HANDLER Default_Handler
