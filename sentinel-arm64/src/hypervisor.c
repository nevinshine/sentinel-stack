#include "hypervisor.h"

extern void dummy_guest_entry(void);

/* Simple UART driver for bare-metal printing */
#define UART0_BASE 0x09000000
static volatile uint32_t * const UART0_DR = (uint32_t *)UART0_BASE;

void uart_puts(const char *str) {
    while (*str) {
        *UART0_DR = (uint32_t)(*str++);
    }
}

void uart_puthex(uint64_t val) {
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        if (nibble < 10) {
            *UART0_DR = '0' + nibble;
        } else {
            *UART0_DR = 'A' + (nibble - 10);
        }
    }
}

/* Macros for ESR_EL2 Decoding */
#define ESR_EC_MASK          (0x3FUL << 26)
#define ESR_EC_SHIFT         26
#define ESR_EC_DATA_ABORT_L  0x24  /* Data Abort from a lower Exception Level */
#define ESR_EC_HVC_EL1       0x16  /* HVC instruction execution in EL1 */

#define ESR_ISS_MASK         0x1FFFFFFUL
#define ESR_ISS_WnR          (1UL << 6)
#define ESR_ISS_DFSC_MASK    0x3FUL

/* Data Fault Status Codes for Stage-2 Translation Faults */
#define DFSC_S2_FAULT_L0     0x04
#define DFSC_S2_FAULT_L1     0x05
#define DFSC_S2_FAULT_L2     0x06
#define DFSC_S2_FAULT_L3     0x07

/* Data Fault Status Codes for Stage-2 Permission Faults */
#define DFSC_S2_PERM_L0      0x0C
#define DFSC_S2_PERM_L1      0x0D
#define DFSC_S2_PERM_L2      0x0E
#define DFSC_S2_PERM_L3      0x0F

/**
 * handle_sync_lower - C hook called from exceptions.S
 * @esr: Value of ESR_EL2 at the time of the exception
 * @far: Value of FAR_EL2 (Fault Address Register) containing the violating IPA
 * @regs: Pointer to the guest OS saved registers (x0-x30)
 */
void handle_sync_lower(uint64_t esr, uint64_t far, uint64_t *regs) {
    uint32_t ec = (esr & ESR_EC_MASK) >> ESR_EC_SHIFT;
    
    if (ec == ESR_EC_DATA_ABORT_L) {
        uint32_t iss = esr & ESR_ISS_MASK;
        uint32_t dfsc = iss & ESR_ISS_DFSC_MASK;
        
        /* Check if the fault is explicitly a Stage-2 Translation or Permission Fault */
        if ((dfsc >= DFSC_S2_FAULT_L0 && dfsc <= DFSC_S2_FAULT_L3) ||
            (dfsc >= DFSC_S2_PERM_L0 && dfsc <= DFSC_S2_PERM_L3)) {
            int is_write = (iss & ESR_ISS_WnR) ? 1 : 0;
            // int fault_level = dfsc - DFSC_S2_FAULT_L0; // Unused for now
            
            /* SECURITY VIOLATION TRAPPED
             * This is where your hypervisor enforces policy or triggers alerts.
             */
            if (is_write) {
                uart_puts("[EL2-SECURITY] STAGE-2 FAULT TRAPPED: Malicious write attempt blocked!\n");
                uart_puts("[EL2-SECURITY] HALT: System lockdown initiated.\n");
                // For bare-metal validation, we trap or panic the execution
                while (1) {
                    /* STAGE-2 FAULT TRAPPED */
                    __asm__ volatile("wfe");
                }
            } else {
                uart_puts("[EL2-SECURITY] STAGE-2 FAULT: Unauthorized Read Blocked.\n");
            }
        }
    } else if (ec == ESR_EC_HVC_EL1) {
        /* VMI: Intercept HVC and perform Software Page Table Walk */
        uint64_t guest_va = regs[0]; // x0 contains the target VA
        
        uart_puts("\n[Sentinel-VMI] Intercepted HVC. Introspecting Guest VA: ");
        uart_puthex(guest_va);
        uart_puts("\n");

        uint64_t ttbr1;
        __asm__ volatile("mrs %0, ttbr1_el1" : "=r" (ttbr1));
        uint64_t ttbr0;
        __asm__ volatile("mrs %0, ttbr0_el1" : "=r" (ttbr0));

        // Use TTBR1 for upper half (0xFFFF...), TTBR0 for lower half
        uint64_t pgd_base = (guest_va & (1ULL << 55)) ? (ttbr1 & ~0xFFF) : (ttbr0 & ~0xFFF);

        uart_puts("[Sentinel-VMI] TTBR Base PA: ");
        uart_puthex(pgd_base);
        uart_puts("\n");

        uint64_t pgd_idx = (guest_va >> PGD_SHIFT) & PT_INDEX_MASK;
        stage1_desc_t *pgd = (stage1_desc_t *)pgd_base;
        stage1_desc_t pgd_entry = pgd[pgd_idx];

        uart_puts("[Sentinel-VMI] PGD Index: ");
        uart_puthex(pgd_idx);
        uart_puts(" -> Entry: ");
        uart_puthex(pgd_entry.val);
        uart_puts("\n");

        if ((pgd_entry.fields.type & 1) == 0) {
            uart_puts("[Sentinel-VMI] VMI Failed: Invalid PGD Entry\n");
            return;
        }

        uint64_t pud_base = (pgd_entry.fields.oa << PTE_SHIFT);
        uint64_t pud_idx = (guest_va >> PUD_SHIFT) & PT_INDEX_MASK;
        stage1_desc_t *pud = (stage1_desc_t *)pud_base;
        stage1_desc_t pud_entry = pud[pud_idx];

        uart_puts("[Sentinel-VMI] PUD Index: ");
        uart_puthex(pud_idx);
        uart_puts(" -> Entry: ");
        uart_puthex(pud_entry.val);
        uart_puts("\n");

        if ((pud_entry.fields.type & 1) == 0) {
            uart_puts("[Sentinel-VMI] VMI Failed: Invalid PUD Entry\n");
            return;
        }

        uint64_t pmd_base = (pud_entry.fields.oa << PTE_SHIFT);
        uint64_t pmd_idx = (guest_va >> PMD_SHIFT) & PT_INDEX_MASK;
        stage1_desc_t *pmd = (stage1_desc_t *)pmd_base;
        stage1_desc_t pmd_entry = pmd[pmd_idx];

        uart_puts("[Sentinel-VMI] PMD Index: ");
        uart_puthex(pmd_idx);
        uart_puts(" -> Entry: ");
        uart_puthex(pmd_entry.val);
        uart_puts("\n");

        if ((pmd_entry.fields.type & 1) == 0) {
            uart_puts("[Sentinel-VMI] VMI Failed: Invalid PMD Entry\n");
            return;
        }

        uint64_t pte_base = (pmd_entry.fields.oa << PTE_SHIFT);
        uint64_t pte_idx = (guest_va >> PTE_SHIFT) & PT_INDEX_MASK;
        stage1_desc_t *pte = (stage1_desc_t *)pte_base;
        stage1_desc_t pte_entry = pte[pte_idx];

        uart_puts("[Sentinel-VMI] PTE Index: ");
        uart_puthex(pte_idx);
        uart_puts(" -> Entry: ");
        uart_puthex(pte_entry.val);
        uart_puts("\n");

        if ((pte_entry.fields.type & 1) == 0) {
            uart_puts("[Sentinel-VMI] VMI Failed: Invalid PTE Entry\n");
            return;
        }

        uint64_t physical_frame = (pte_entry.fields.oa << PTE_SHIFT);
        uint64_t page_offset = guest_va & 0xFFF;
        uint64_t final_pa = physical_frame | page_offset;

        uart_puts("[Sentinel-VMI] *** VMI SUCCESS: VA ");
        uart_puthex(guest_va);
        uart_puts(" maps to PA ");
        uart_puthex(final_pa);
        uart_puts(" ***\n");

        /* Advance ELR_EL2 to skip the HVC instruction and return */
        uint64_t elr;
        __asm__ volatile("mrs %0, elr_el2" : "=r" (elr));
        elr += 4;
        __asm__ volatile("msr elr_el2, %0" : : "r" (elr));
    }
}

/*
 * hypervisor_main: Primary entry point for the bare-metal Sentinel Hypervisor.
 * Executing in Ring -1 (EL2).
 */
void hypervisor_main(void) {
    uart_puts("[Sentinel-VMI] Booting Type-1 Hypervisor at EL2...\n");

    /* Initialize Stage-2 MMU tables */
    mmu_init_stage2();

    /* Orchestrate translation control registers in bare-metal assembly */
    configure_stage2_mmu();

    uart_puts("[Sentinel-VMI] Initialization complete. Dropping to EL1 Guest...\n");

    /* Drop privileges and hand execution to the Guest OS at EL1 */
    jump_to_el1();
}
