#include "hypervisor.h"

/*
 * AArch64 requires tables to be aligned to the granule size (4KB).
 * We need one Level 1 table, and one Level 2 table to map 1GB.
 */
stage2_desc_t stage2_l1_table[512] __attribute__((aligned(4096)));
stage2_desc_t stage2_l2_table[512] __attribute__((aligned(4096)));

void mmu_init_stage2(void) {
  uart_puts("[Sentinel-VMI] Initializing Stage 2 MMU...\n");

  // 1. Link Level 1 to Level 2
  // Descriptor type: Table (0b11 at bits 1:0)
  uint64_t l2_phys_addr = (uint64_t)&stage2_l2_table;
  stage2_desc_t l1_link = {0};
  l1_link.fields.type = 3;                 // Table descriptor
  l1_link.fields.ipa = l2_phys_addr >> 12; // Shift by granule bits
  stage2_l1_table[1] = l1_link;

  // 2. Populate Level 2 with 2MB Block Descriptors
  // Map the 1GB of RAM starting at 0x40000000
  for (int i = 0; i < 512; i++) {
    uint64_t phys_base = 0x40000000 + ((uint64_t)i * 0x200000);
    stage2_desc_t desc = {0};

    desc.fields.type = 1;      // Block descriptor
    desc.fields.af = 1;        // Access Flag must be 1
    desc.fields.memattr = 0xF; // Normal Memory
    desc.fields.ipa = phys_base >> 12;

    if (phys_base == 0x40000000) {
      // Guest Text: Read-Only, Executable (xn=0)
      desc.fields.s2ap = 1; // 0b01 = Read-Only
      desc.fields.xn = 0;   // 0b0 = Executable
    } else if (phys_base == 0x40200000) {
      // THE DRAWBRIDGE: Lock this 2MB block to Read-Only, Execute-Never
      desc.fields.s2ap = 1; // 0b01 = Read-Only
      desc.fields.xn = 1;   // 0b1 = Execute-Never
    } else {
      // Standard Data mapping: Read/Write, Execute-Never
      desc.fields.s2ap = 3; // 0b11 = Read/Write
      desc.fields.xn = 1;   // 0b1 = Execute-Never
    }

    stage2_l2_table[i] = desc;
  }

  // Map the UART (0x09000000) so the guest can print
  // S2AP = 0b11, MemAttr = Device (0), AF = 1, Type = Block (0b01)
  stage2_desc_t uart_desc = {0};
  uart_desc.fields.type = 1;
  uart_desc.fields.af = 1;
  uart_desc.fields.memattr = 0; // Device Memory
  uart_desc.fields.s2ap = 3;    // Read/Write
  uart_desc.fields.ipa = 0x00000000 >> 12;
  stage2_l1_table[0] = uart_desc;

  uart_puts("[Sentinel-VMI] Stage 2 MMU Tables Populated in Memory.\n");
}
