// ==========================================================
// TCA Benchmark Harness — Deterministic Instruction-Cycle
// Measurement of Hardware Intent Gate vs. eBPF LSM Baseline
//
// Runs bare-metal on QEMU riscv64 virt with -icount shift=0.
// Output over 16550 UART at 0x10000000.
// ==========================================================
#include <stdint.h>
#include "../sentinel-cc/include/tca_intrinsics.h"

// Mock LLVM Intrinsics for TCA Teleological Lifting
extern void llvm_telos_intent_start(const char* intent_string)
    asm("llvm.telos.intent.start");
extern void llvm_telos_intent_end(void)
    asm("llvm.telos.intent.end");

// RISC-V custom opcode wrappers (lowered by SentinelPass)
void tca_set_intent(void* func, long intent) asm("tca_set_intent");
void tca_set_intent(void* func, long intent) {
    TCA_SET_INTENT(func, intent);
}
void tca_clear(void) asm("tca_clear");
void tca_clear(void) {
    TCA_CLEAR();
}

// Cryptographic Signature Payload (Linked via tca_linker.ld)
char __sentinel_signature[64] __attribute__((section(".signature"))) = {0};

// ---- UART Primitives ----
#define UART_BASE 0x10000000UL

static void uart_putc(char c) {
    *(volatile char*)UART_BASE = c;
}

static void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

static void uart_u64(uint64_t v) {
    char buf[24];
    int n = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v) { buf[n++] = '0' + (v % 10); v /= 10; }
    while (n) uart_putc(buf[--n]);
}

// ---- Cycle Counter ----
static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("csrr %0, mcycle" : "=r"(c));
    return c;
}

// ---- eBPF LSM Simulation (~150 guest instructions) ----
// Models best-case JIT-compiled BPF trampoline:
//   context save -> bpf_map_lookup_elem (hash) -> branch -> context restore
__attribute__((noinline))
void simulate_ebpf_lsm_hook(void) {
    volatile uint64_t ctx_save[4];  // context save
    ctx_save[0] = 0xA0; ctx_save[1] = 0xA1;
    ctx_save[2] = 0xA2; ctx_save[3] = 0xA3;

    // Simulate hash-map lookup + branch evaluation
    volatile uint64_t hash = 0;
    for (int i = 0; i < 20; i++) {
        hash = hash * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    // Simulate verdict branch
    volatile int verdict = (hash & 1) ? 1 : 0;
    (void)verdict;

    // context restore
    volatile uint64_t r0 = ctx_save[0]; (void)r0;
    volatile uint64_t r1 = ctx_save[1]; (void)r1;
    volatile uint64_t r2 = ctx_save[2]; (void)r2;
    volatile uint64_t r3 = ctx_save[3]; (void)r3;
}

// ---- Bare-metal entry ----
int main(void);
void _start(void) {
    main();
    uart_puts("[BENCHMARK] Complete.\n");
    while (1) asm volatile("wfi");
}

// ================================================================
int main(void) {
    // Anchor the signature section so the linker keeps it
    volatile char* root_ptr = __sentinel_signature;
    (void)root_ptr;

    uart_puts("========================================\n");
    uart_puts("[TCA] Teleological Capability Benchmark\n");
    uart_puts("========================================\n");

    uart_puts("[DIAG] Phase 1: Testing mcycle CSR...\n");
    uint64_t test_c = rdcycle();
    uart_puts("[DIAG] mcycle read OK: ");
    uart_u64(test_c);
    uart_puts("\n");

    // Use a valid RAM address for the "sensitive" target.
    // 0x87F00000 is near the top of QEMU virt's default 128 MiB RAM.
    // The TCA hardware gate in op_helper.c checks addr >= 0x87000000.
    volatile uint64_t* target = (volatile uint64_t*)0x87F00000;

    // ----------------------------------------------------------
    // TEST 1: Plain load (baseline measurement)
    // ----------------------------------------------------------
    uart_puts("[DIAG] Phase 2: Baseline load...\n");
    uint64_t c0 = rdcycle();
    volatile uint64_t dummy_load = *target;
    uint64_t c1 = rdcycle();
    (void)dummy_load;
    uint64_t baseline_load = c1 - c0;
    uart_puts("[DIAG] Baseline done.\n");

    // ----------------------------------------------------------
    // TEST 2: Store WITH TCA intent active (hardware inline gate)
    // ----------------------------------------------------------
    uart_puts("[DIAG] Phase 3: Setting TCA intent...\n");
    llvm_telos_intent_start("network_send_socket_data");
    uart_puts("[DIAG] Intent set. Performing TCA store...\n");

    uint64_t c2 = rdcycle();
    *target = 0xDEADBEEFCAFEBABEULL;  // TCA gate fires inline
    uint64_t c3 = rdcycle();
    uint64_t tca_store = c3 - c2;
    uart_puts("[DIAG] TCA store done.\n");

    llvm_telos_intent_end();

    // ----------------------------------------------------------
    // TEST 3: Store WITH simulated eBPF LSM hook (~150 instr)
    // ----------------------------------------------------------
    uart_puts("[DIAG] Phase 4: eBPF simulation...\n");
    uint64_t c4 = rdcycle();
    simulate_ebpf_lsm_hook();
    *target = 0xCAFEBABEDEADBEEFULL;  // actual store after hook
    uint64_t c5 = rdcycle();
    uint64_t ebpf_store = c5 - c4;

    // ----------------------------------------------------------
    // Report
    // ----------------------------------------------------------
    uart_puts("[DATA] BASELINE_LOAD_CYCLES=");
    uart_u64(baseline_load);
    uart_puts("\n");

    uart_puts("[DATA] TCA_NATIVE_STORE_CYCLES=");
    uart_u64(tca_store);
    uart_puts("\n");

    uart_puts("[DATA] EBPF_SIMULATED_STORE_CYCLES=");
    uart_u64(ebpf_store);
    uart_puts("\n");

    uart_puts("[DATA] TCA_OVERHEAD_VS_BASELINE=");
    if (tca_store >= baseline_load)
        uart_u64(tca_store - baseline_load);
    else
        uart_u64(0);
    uart_puts("\n");

    uart_puts("[DATA] EBPF_OVERHEAD_VS_BASELINE=");
    if (ebpf_store >= baseline_load)
        uart_u64(ebpf_store - baseline_load);
    else
        uart_u64(0);
    uart_puts("\n");

    return 0;
}
