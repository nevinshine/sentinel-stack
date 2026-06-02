#include <stdint.h>
#include "tca_intrinsics.h"

// External intent markers handled by SentinelPass
extern void llvm_telos_intent_start(const char* intent_string) asm("llvm.telos.intent.start");
extern void llvm_telos_intent_end(void) asm("llvm.telos.intent.end");

// RISC-V custom opcode wrappers (lowered by SentinelPass)
void tca_set_intent(void* func, long intent) asm("tca_set_intent");
void tca_set_intent(void* func, long intent) {
    TCA_SET_INTENT(func, intent);
}
void tca_clear(void) asm("tca_clear");
void tca_clear(void) {
    TCA_CLEAR();
}

// QEMU UART base address for virt machine
#define UART0_BASE 0x10000000
#define UART_THR   0x00

// UART initialization
static void uart_init() {
    // Basic setup if needed, usually works out of box in QEMU bare metal
}

// Write character to UART
static void uart_putc(char c) {
    volatile uint8_t *uart = (uint8_t *)UART0_BASE;
    uart[UART_THR] = c;
}

// Write string to UART
static void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Convert uint64_t to hex and print
static void uart_u64(uint64_t val) {
    const char hex_digits[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex_digits[(val >> i) & 0xF]);
    }
}

// Ensure signature section is preserved for the sign_tool
char __sentinel_signature[64] __attribute__((section(".signature"))) = {0};

// ------------------------------------------------------------------
// TCA Exfiltration Benchmark
// ------------------------------------------------------------------
void _start() {
    uart_init();

    // Prevent compiler from optimizing out the root signature ptr
    extern char __sentinel_signature[];
    volatile char* root_ptr = __sentinel_signature;
    (void)root_ptr;

    uart_puts("========================================\n");
    uart_puts("[TCA] Quantum-State IFC Network Slam\n");
    uart_puts("========================================\n");

    // The sensitive memory region (simulating /etc/shadow)
    // We defined 0x87E00000 in op_helper.c for taint propagation
    volatile uint64_t* tainted_source = (volatile uint64_t*)0x87E00000;
    
    // The Mock TX Trigger MMIO we mapped in hw/riscv/virt.c
    volatile uint64_t* tx_trigger = (volatile uint64_t*)0x87D00000;

    uart_puts("[DIAG] Phase 1: Setting valid network intent...\n");
    llvm_telos_intent_start("network_send_socket_data");
    uart_puts("[DIAG] Valid intent activated. We are authorized to send data.\n");

    uart_puts("[DIAG] Phase 2: Reading from sensitive (tainted) memory...\n");
    // This read will be intercepted by the TCA gate in op_helper.c
    // and flip the tca_taint_flag in the CPU hardware state.
    uint64_t secret_data = *tainted_source;
    (void)secret_data;
    uart_puts("[DIAG] Memory read. Taint should be physically propagated.\n");

    uart_puts("[DIAG] Phase 3: Attempting to flush data to Network Interface...\n");
    // Writing to the Mock TX Trigger will invoke tca_mock_tx_write.
    // The NIC will query the CPU's tca_taint_flag. If 1, it will drop the packet!
    *tx_trigger = 1;

    uart_puts("[DIAG] Exfiltration benchmark complete.\n");

    llvm_telos_intent_end();

    // Loop forever
    while (1) {
        asm volatile("wfi");
    }
}
