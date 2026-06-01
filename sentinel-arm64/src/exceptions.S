.global el2_vector_table

/* 
 * MACRO: save_guest_context
 * Saves the guest OS general-purpose registers (x0-x30) onto the EL2 stack.
 * We must allocate 256 bytes (32 registers * 8 bytes).
 */
.macro save_guest_context
    sub sp, sp, #256
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
.endm

/* 
 * MACRO: restore_guest_context
 * Restores the guest OS general-purpose registers (x0-x30) from the EL2 stack.
 */
.macro restore_guest_context
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    add sp, sp, #256
.endm

/* ====================================================================
 * EL2 EXCEPTION VECTOR TABLE
 * Must be 2KB aligned (2^11 = 2048)
 * ==================================================================== */
.section .vectors, "ax"
.align 11
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
    /* 1. Safely save the full guest OS execution context */
    save_guest_context

    /* 2. Read the hardware syndrome and fault address */
    mrs x0, esr_el2    /* Arg 0: Exception Syndrome Register */
    mrs x1, far_el2    /* Arg 1: Fault Address Register */
    mov x2, sp         /* Arg 2: Pointer to guest context (registers) */
    
    /* 3. Branch into the C surgical decoder */
    bl handle_sync_lower

    /* 4. Restore the guest OS execution context cleanly */
    restore_guest_context
    
    eret

default_handler:
    b .
