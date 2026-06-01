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

/* ====================================================================
 * STAGE-2 TRANSLATION CONTROL
 * ==================================================================== */
.global configure_stage2_mmu
configure_stage2_mmu:
    /* Step 1: Load the base address of your C-allocated Level-1 table */
    ldr     x0, =stage2_l1_table
    
    /* Step 2: Bind the table pointer to the Virtualization Translation Table Base Register */
    /* Note: VMID [55:48] can be set to 1 for our primary isolated guest cluster */
    mov     x1, #1
    lsl     x1, x1, #48
    orr     x0, x0, x1
    msr     vttbr_el2, x0

    /* Step 3: Populate VTCR_EL2 with our 40-bit, 4KB granule configuration mask */
    ldr     x0, =0x80023518
    msr     vtcr_el2, x0

    /* Step 3.5: Enable Stage 2 Translation in HCR_EL2 */
    mrs     x0, hcr_el2
    orr     x0, x0, #1       /* VM = 1 (Enable Stage 2) */
    mov     x1, #1
    lsl     x1, x1, #31      /* RW = 1 (EL1 is 64-bit) */
    orr     x0, x0, x1
    msr     hcr_el2, x0

    /* Step 4: Ensure all register writes are fully completed before proceeding */
    dsb     sy
    isb

    ret

