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

/* VMI: Stage-1 Software Page Table Walker Macros (4KB Granule, 48-bit VA) */
#define PGD_SHIFT       39
#define PUD_SHIFT       30
#define PMD_SHIFT       21
#define PTE_SHIFT       12
#define PT_INDEX_MASK   0x1FF

/* VMI: Hardware Stage-1 Translation Table Descriptor (EL1) */
typedef union {
    uint64_t val;
    struct {
        uint64_t type    : 2;  // [1:0] 01=Block, 11=Table/Page
        uint64_t indx    : 3;  // [4:2] AttrIndx
        uint64_t ns      : 1;  // [5]   Non-Secure
        uint64_t ap      : 2;  // [7:6] Data Access Permissions
        uint64_t sh      : 2;  // [9:8] Shareability
        uint64_t af      : 1;  // [10]  Access Flag
        uint64_t ng      : 1;  // [11]  Not Global
        uint64_t oa      : 36; // [47:12] Output Address
        uint64_t res0    : 4;  // [51:48] Reserved
        uint64_t contig  : 1;  // [52]  Contiguous
        uint64_t pxn     : 1;  // [53]  Privileged Execute Never
        uint64_t uxn     : 1;  // [54]  Unprivileged Execute Never
        uint64_t ignored : 9;  // [63:55] Ignored/PBHA
    } __attribute__((packed)) fields;
} stage1_desc_t;

/* External functions */
extern void uart_puts(const char *str);
extern void mmu_init_stage2(void);
extern void configure_stage2_mmu(void);
extern void jump_to_el1(void);

#endif // HYPERVISOR_H
