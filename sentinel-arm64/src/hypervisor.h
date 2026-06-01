#ifndef HYPERVISOR_H
#define HYPERVISOR_H

#include <stdint.h>

/*
 * ARM64 Stage-2 Translation Descriptor (Block/Page)
 * Uses Attribute Combining to enforce strict physical memory permissions.
 */
typedef union {
    uint64_t val;
    struct {
        uint64_t type    : 2;  // [1:0] 01=Block, 11=Page
        uint64_t memattr : 4;  // [5:2] Stage-2 Memory Attributes
        uint64_t s2ap    : 2;  // [7:6] Stage-2 Access Permissions (01=RO, 11=RW)
        uint64_t sh      : 2;  // [9:8] Shareability
        uint64_t af      : 1;  // [10]  Access Flag (Must be 1 to prevent fault)
        uint64_t res0_1  : 1;  // [11]  Reserved
        uint64_t ipa     : 36; // [47:12] Output Address (Granule specific)
        uint64_t res0_2  : 6;  // [53:48] Reserved
        uint64_t xn      : 1;  // [54] Execute Never
        uint64_t res0_3  : 9;  // [63:55] Reserved
    } __attribute__((packed)) fields;
} stage2_desc_t;

/* External functions */
extern void uart_puts(const char *str);
extern void mmu_init_stage2(void);
extern void configure_stage2_mmu(void);
extern void jump_to_el1(void);

#endif // HYPERVISOR_H
