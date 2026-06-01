#include "hypervisor.h"

/* Simple UART driver for bare-metal printing */
#define UART0_BASE 0x09000000
static volatile uint32_t * const UART0_DR = (uint32_t *)UART0_BASE;

void uart_puts(const char *str) {
    while (*str) {
        *UART0_DR = (uint32_t)(*str++);
    }
}

/* Macros for ESR_EL2 Decoding */
#define ESR_EC_MASK          (0x3FUL << 26)
#define ESR_EC_SHIFT         26
#define ESR_EC_DATA_ABORT_L  0x24  /* Data Abort from a lower Exception Level */

#define ESR_ISS_MASK         0x1FFFFFFUL
#define ESR_ISS_WnR          (1UL << 6)
#define ESR_ISS_DFSC_MASK    0x3FUL

/* Data Fault Status Codes for Stage-2 Translation Faults */
#define DFSC_S2_FAULT_L0     0x04
#define DFSC_S2_FAULT_L1     0x05
#define DFSC_S2_FAULT_L2     0x06
#define DFSC_S2_FAULT_L3     0x07

/**
 * handle_sync_lower - C hook called from exceptions.S
 * @esr: Value of ESR_EL2 at the time of the exception
 * @far: Value of FAR_EL2 (Fault Address Register) containing the violating IPA
 */
void handle_sync_lower(uint64_t esr, uint64_t far) {
    uint32_t ec = (esr & ESR_EC_MASK) >> ESR_EC_SHIFT;
    
    if (ec == ESR_EC_DATA_ABORT_L) {
        uint32_t iss = esr & ESR_ISS_MASK;
        uint32_t dfsc = iss & ESR_ISS_DFSC_MASK;
        
        /* Check if the fault is explicitly a Stage-2 Translation Fault */
        if (dfsc >= DFSC_S2_FAULT_L0 && dfsc <= DFSC_S2_FAULT_L3) {
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
