.global _start
.extern hypervisor_main

.section .text
_start:
    /* Read CurrentEL */
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #2
    beq el2_entry

not_el2:
    wfe
    b not_el2

el2_entry:
    /* 1. Set up the stack pointer */
    ldr x1, =_stack_top
    mov sp, x1

    /* 2. Load the Vector Table Address into VBAR_EL2 */
    ldr x0, =el2_vector_table
    msr vbar_el2, x0
    isb  /* Instruction Synchronization Barrier to ensure VBAR is set */

    /* 3. Branch to C hypervisor */
    bl hypervisor_main

hang:
    wfe
    b hang


/* ====================================================================
 * EL2 EXCEPTION VECTOR TABLE
 * Must be 2KB aligned (2^11 = 2048)
 * ==================================================================== */
.align 11
.global el2_vector_table
el2_vector_table:

    /* Group 1: Exceptions from Current EL (SP_EL0) - Unused */
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler

    /* Group 2: Exceptions from Current EL (SP_ELx) - Hypervisor bugs trap here! */
    .align 7; b sync_current_el
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler

    /* Group 3: Exceptions from Lower EL (AArch64) - Our Drawbridge traps here! */
    .align 7; b sync_lower_el64
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler

    /* Group 4: Exceptions from Lower EL (AArch32) - Unused */
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler
    .align 7; b default_handler


/* ====================================================================
 * EXCEPTION HANDLERS
 * ==================================================================== */
sync_current_el:
    /* If we crash inside EL2, we land here */
    b .

.extern handle_sync_lower
sync_lower_el64:
    /* Allocate 64 bytes on stack to save x0-x7 */
    sub sp, sp, #64
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]

    /* Pass stack pointer (pointer to regs array) as arg0 to C */
    mov x0, sp
    bl handle_sync_lower

    /* Restore registers */
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    add sp, sp, #64
    
    eret

default_handler:
    b .

.global jump_to_el1
jump_to_el1:
    /* * SPSR_EL2 configuration:
     * D, A, I, F bits set (Exceptions masked)
     * M[4:0] = 0b00101 (Return to EL1h)
     */
    ldr x0, =0x3C5
    msr spsr_el2, x0

    /* Set ELR_EL2 to the guest entry point */
    ldr x0, =0x40080000
    msr elr_el2, x0

    /* * ERET evaluates SPSR_EL2 and ELR_EL2, drops privileges to EL1,
     * and jumps to 0x40080000.
     */
    eret
