#include <stdint.h>

extern void uart_puts(const char *str);

/* * AArch64 requires tables to be aligned to the granule size (4KB).
 * We need one Level 1 table, and one Level 2 table to map 1GB.
 */
uint64_t stage2_l1_table[512] __attribute__((aligned(4096)));
uint64_t stage2_l2_table[512] __attribute__((aligned(4096)));

void mmu_init_stage2(void) {
    uart_puts("[Sentinel-VMI] Initializing Stage 2 MMU...\n");

    // 1. Link Level 1 to Level 2
    // Descriptor type: Table (0b11 at bits 1:0)
    uint64_t l2_phys_addr = (uint64_t)&stage2_l2_table;
    stage2_l1_table[1] = l2_phys_addr | 0x3; // Index 1 covers 0x40000000 to 0x7FFFFFFF

    // 2. Populate Level 2 with 2MB Block Descriptors
    // Map the 1GB of RAM starting at 0x40000000
    for (int i = 0; i < 512; i++) {
        uint64_t phys_base = 0x40000000 + ((uint64_t)i * 0x200000);
        
        if (phys_base == 0x40200000) {
            // THE DRAWBRIDGE: Lock this 2MB block to Read-Only (S2AP = 0b01)
            stage2_l2_table[i] = phys_base | (0x1 << 6) | (0xF << 2) | (1 << 10) | 0x1;
        } else {
            // S2AP = 0b11 (Read/Write), MemAttr = Normal, AF = 1, Type = Block (0b01)
            stage2_l2_table[i] = phys_base | (0x3 << 6) | (0xF << 2) | (1 << 10) | 0x1;
        }
    }

    // Map the UART (0x09000000) so the guest can print
    // Index 0 of L1 covers 0x0 to 0x3FFFFFFF. We need another L2 table for MMIO realistically, 
    // but for this lab, we will directly hack a 1GB block descriptor into L1 index 0 for MMIO.
    // S2AP = 0b11, MemAttr = Device, AF = 1, Type = Block (0b01)
    stage2_l1_table[0] = 0x00000000 | (0x3 << 6) | (1 << 10) | 0x1;

    // 3. Configure VTCR_EL2 (Translation Control)
    // 40-bit IPA, 4KB Granule, Inner Shareable, Normal Memory
    uint64_t vtcr_val = (1ULL << 31) | // RES1
                        (2ULL << 16) | // PS = 40-bit
                        (0ULL << 14) | // TG0 = 4KB
                        (3ULL << 12) | // SH0 = Inner Shareable
                        (1ULL << 8)  | // IRGN0 = Normal WB
                        (1ULL << 10) | // ORGN0 = Normal WB
                        (1ULL << 6)  | // SL0 = 1 (Level 1)
                        (25ULL << 0);  // T0SZ = 25 (39-bit space) (64 - 24 = 40-bit space)
    
    __asm__ volatile("msr vtcr_el2, %0" : : "r" (vtcr_val));

    // 4. Set VTTBR_EL2 to our L1 table
    uint64_t vttbr_val = (uint64_t)&stage2_l1_table;
    __asm__ volatile("msr vttbr_el2, %0" : : "r" (vttbr_val));

    // 5. Enable Stage 2 Translation in HCR_EL2
    uint64_t hcr_val;
    __asm__ volatile("mrs %0, hcr_el2" : "=r" (hcr_val));
    hcr_val |= (1ULL << 0); // VM = 1 (Enable Stage 2)
    hcr_val |= (1ULL << 31); // RW = 1 (EL1 is 64-bit)
    __asm__ volatile("msr hcr_el2, %0" : : "r" (hcr_val));

    // 6. Synchronize
    __asm__ volatile("isb");
    
    uart_puts("[Sentinel-VMI] Stage 2 MMU Active. Identity map locked.\n");
}
