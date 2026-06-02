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

/* Thread Control Block */
typedef struct {
    uint64_t sp; /* Stack Pointer */
} ThreadContext;

ThreadContext thread_a;
ThreadContext thread_b;

uint8_t stack_b[4096] __attribute__((aligned(16)));

/* 
 * The TCA-Aware Context Switcher
 * Saves standard Callee-Saved registers (ra, s0-s11) PLUS the TCA hardware state.
 */
__attribute__((naked)) void switch_context(ThreadContext *prev, ThreadContext *next) {
    __asm__ volatile(
        /* 1. Allocate stack space for 14 standard registers + 2 TCA CSRs = 16 * 8 = 128 bytes */
        "addi sp, sp, -128\n"

        /* 2. Save standard callee-saved registers and return address */
        "sd ra,   0(sp)\n"
        "sd s0,   8(sp)\n"
        "sd s1,  16(sp)\n"
        "sd s2,  24(sp)\n"
        "sd s3,  32(sp)\n"
        "sd s4,  40(sp)\n"
        "sd s5,  48(sp)\n"
        "sd s6,  56(sp)\n"
        "sd s7,  64(sp)\n"
        "sd s8,  72(sp)\n"
        "sd s9,  80(sp)\n"
        "sd s10, 88(sp)\n"
        "sd s11, 96(sp)\n"

        /* 3. [TCA HOOK] Save Hardware Capability State */
        "csrr t0, 0x800\n"        /* Read CSR_TCA_INTENT */
        "sd   t0, 104(sp)\n"      
        "csrr t1, 0x801\n"        /* Read CSR_TCA_TAINT */
        "sd   t1, 112(sp)\n"

        /* 4. Swap the Stack Pointers */
        "sd sp, 0(a0)\n"          /* prev->sp = sp */
        "ld sp, 0(a1)\n"          /* sp = next->sp */

        /* 5. [TCA HOOK] Restore Hardware Capability State */
        "ld   t0, 104(sp)\n"
        "csrw 0x800, t0\n"        /* Write CSR_TCA_INTENT */
        "ld   t1, 112(sp)\n"
        "csrw 0x801, t1\n"        /* Write CSR_TCA_TAINT */

        /* 6. Restore standard callee-saved registers */
        "ld ra,   0(sp)\n"
        "ld s0,   8(sp)\n"
        "ld s1,  16(sp)\n"
        "ld s2,  24(sp)\n"
        "ld s3,  32(sp)\n"
        "ld s4,  40(sp)\n"
        "ld s5,  48(sp)\n"
        "ld s6,  56(sp)\n"
        "ld s7,  64(sp)\n"
        "ld s8,  72(sp)\n"
        "ld s9,  80(sp)\n"
        "ld s10, 88(sp)\n"
        "ld s11, 96(sp)\n"

        /* 7. Deallocate stack space and return */
        "addi sp, sp, 128\n"
        "ret\n"
    );
}

// Ensure signature section is preserved for the sign_tool
char __sentinel_signature[64] __attribute__((section(".signature"))) = {0};

void thread_b_func() {
    uart_puts("[THREAD B] Woke up. Executing in expected clean context...\n");

    llvm_telos_intent_start("network_send_socket_data");

    volatile uint64_t* tx_trigger = (volatile uint64_t*)0x87D00000;
    
    uart_puts("[THREAD B] Attempting to transmit on TX Trigger...\n");
    // Thread B writes to the TX Trigger (0x87D00000). It succeeds.
    *tx_trigger = 1;
    uart_puts("[THREAD B] TX Write finished. Yielding back to Thread A.\n");
    
    // Thread B calls switch_context(&thread_b, &thread_a).
    switch_context(&thread_b, &thread_a);
    
    while(1) {
        asm volatile("wfi");
    }
}

void _start() {
    uart_puts("========================================\n");
    uart_puts("[TCA] OS Isolation Test\n");
    uart_puts("========================================\n");

    // Prevent compiler from optimizing out the root signature ptr
    extern char __sentinel_signature[];
    volatile char* root_ptr = __sentinel_signature;
    (void)root_ptr;

    llvm_telos_intent_start("network_send_socket_data");

    // Setup thread B stack
    uint64_t *sp_b = (uint64_t *)(stack_b + sizeof(stack_b));
    sp_b -= 16; // 128 bytes / 8 = 16
    
    sp_b[0] = (uint64_t)thread_b_func; // ra
    sp_b[13] = 0; // t0 (INTENT, saved at offset 104 = 13*8)
    sp_b[14] = 0; // t1 (TAINT, saved at offset 112 = 14*8)
    
    thread_b.sp = (uint64_t)sp_b;
    
    volatile uint64_t* tainted_source = (volatile uint64_t*)0x87E00000;
    volatile uint64_t* tx_trigger = (volatile uint64_t*)0x87D00000;

    uart_puts("[THREAD A] Reading from sensitive (tainted) memory...\n");
    // 1. Thread A (Malicious) reads 0x87E00000. Hardware 0x801 (Taint) flips to 1.
    uint64_t secret_data = *tainted_source;
    (void)secret_data;
    
    uart_puts("[THREAD A] Yielding to Thread B...\n");
    // 2. Thread A calls switch_context(&thread_a, &thread_b).
    switch_context(&thread_a, &thread_b);
    
    uart_puts("[THREAD A] Woke back up. Context restored.\n");
    uart_puts("[THREAD A] Attempting to transmit on TX Trigger...\n");
    // 6. Thread A wakes up. The assembly restores 0x801 back to 1.
    // 7. Thread A attempts to write to the TX Trigger. The Network Slam engages.
    *tx_trigger = 1;
    
    uart_puts("[THREAD A] Run complete (Should not be reached if NIC drops the transmission).\n");
    
    llvm_telos_intent_end();

    while(1) {
        asm volatile("wfi");
    }
}
