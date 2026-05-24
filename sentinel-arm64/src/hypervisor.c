#include <stdint.h>

// QEMU virt machine PL011 UART physical address
volatile uint32_t * const UART0_DR = (uint32_t *)0x09000000;

void uart_putchar(char c) {
    *UART0_DR = c;
}

void uart_puts(const char *str) {
    while (*str != '\0') {
        uart_putchar(*str);
        str++;
    }
}

extern void mmu_init_stage2(void);
extern void jump_to_el1(void);

void hypervisor_main(void) {
    uart_puts("\n========================================\n");
    uart_puts("[Sentinel-VMI] EL2 Hypervisor Initialized\n");
    
    mmu_init_stage2();

    uart_puts("[Sentinel-VMI] Transferring control to EL1 Guest...\n");
    uart_puts("========================================\n\n");

    jump_to_el1();

    // We should never reach here
    while(1) { __asm__ volatile("wfe"); }
}

void handle_sync_lower(uint64_t *regs) {
    uint64_t esr;
    __asm__ volatile("mrs %0, esr_el2" : "=r" (esr));

    uint32_t ec = (esr >> 26) & 0x3F;
    if (ec == 0x24) {
        // Stage 2 Data Abort from lower EL
        uint64_t magic = regs[0];
        if (magic == 0x48454B49) {
            uart_puts("[Sentinel-VMI] Zero-Trust Drawbridge Invoked!\n");
            uart_puts("[Sentinel-VMI] Magic Word Authenticated: HEKI\n");
            
            // Increment ELR_EL2 by 4 to skip the faulting instruction
            uint64_t elr;
            __asm__ volatile("mrs %0, elr_el2" : "=r" (elr));
            elr += 4;
            __asm__ volatile("msr elr_el2, %0" : : "r" (elr));
        } else {
            uart_puts("[Sentinel-VMI] WARNING: Unauthorized Drawbridge Access Attempt!\n");
            while(1) { __asm__ volatile("wfe"); }
        }
    } else {
        uart_puts("[Sentinel-VMI] Unhandled Sync Exception from EL1.\n");
        while(1) { __asm__ volatile("wfe"); }
    }
}
