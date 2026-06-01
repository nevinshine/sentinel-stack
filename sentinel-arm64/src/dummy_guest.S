.section .data
.align 12
.global guest_pgd
guest_pgd:
    /* PGD[0]: Identity mapping base (0x40000000 >> 39 = 0) */
    .quad guest_pud_id + 3
    .fill 35, 8, 0
    /* PGD[36] (0x24): Target mapping base (0x123456789000 >> 39 = 36) */
    .quad guest_pud + 3
    .fill 475, 8, 0

.align 12
guest_pud_id:
    /* PUD[0]: Unused */
    .quad 0
    /* PUD[1]: Identity map 0x40000000 to 0x7FFFFFFF (1GB Block) */
    /* 0x40000000 | AF(1<<10) | SH(3<<8) | Block(1) = 0x40000701 */
    .quad 0x40000701
    .fill 510, 8, 0

.align 12
guest_pud:
    /* PUD[209] (0xD1): (0x123456789000 >> 30) & 0x1FF = 209 */
    .fill 209, 8, 0
    .quad guest_pmd + 3
    .fill 302, 8, 0

.align 12
guest_pmd:
    /* PMD[179] (0xB3): (0x123456789000 >> 21) & 0x1FF = 179 */
    .fill 179, 8, 0
    .quad guest_pte + 3
    .fill 332, 8, 0

.align 12
guest_pte:
    /* PTE[393] (0x189): (0x123456789000 >> 12) & 0x1FF = 393 */
    .fill 393, 8, 0
    /* Maps 0x123456789000 to physical 0x40200000 (4KB Page) */
    /* 0x40200000 | AF(1<<10) | SH(3<<8) | Page(3) = 0x40200703 */
    .quad 0x40200703
    .fill 118, 8, 0

.section .text
.global dummy_guest_entry

dummy_guest_entry:
    /* 1. Set TCR_EL1 */
    /* 48-bit VA, T0SZ=16, 4KB Granule, Inner Shareable, Normal WB */
    /* Disable TTBR1 (EPD1=1), PA Size = 40-bit */
    ldr x0, =0x200803510
    msr tcr_el1, x0

    /* 2. Set MAIR_EL1 */
    /* Attr0: 0xFF (Normal Memory WB) */
    ldr x0, =0xFF
    msr mair_el1, x0

    /* 3. Set TTBR0_EL1 */
    ldr x0, =guest_pgd
    msr ttbr0_el1, x0

    /* 4. Enable Stage-1 MMU */
    mrs x0, sctlr_el1
    orr x0, x0, #1  /* Enable M bit */
    msr sctlr_el1, x0
    isb

    /* 5. Trigger Virtual Machine Introspection (VMI) */
    /* Load the target Virtual Address into x0 */
    ldr x0, =0x123456789000
    hvc #0

    /* 6. Success loop (hypervisor returns here after introspection) */
1:  b 1b
