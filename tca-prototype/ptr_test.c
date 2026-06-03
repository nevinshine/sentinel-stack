// Freestanding TCA Validation Environment
// No stdlib dependencies to allow bare-metal RISC-V cross-compilation.
#include "../sentinel-cc/include/tca_intrinsics.h"

// ---------------------------------------------------------
// _start: Bare-metal entry point
// ---------------------------------------------------------
int main(void);
void _start() {
  main();
  while (1) {
  } // Halt
}

// Mock LLVM Intrinsics for TCA Teleological Lifting
extern void llvm_telos_intent_start(const char *intent_string) asm(
    "llvm.telos.intent.start");
extern void llvm_telos_intent_end(void) asm("llvm.telos.intent.end");

// Define the injected intrinsics using the RISC-V custom macros
// The inline attribute ensures these wrapper functions are removed and the
// .insn is injected directly into the caller if compiled with optimization.
void tca_set_intent(void *func, long intent) asm("tca_set_intent");
void tca_set_intent(void *func, long intent) { TCA_SET_INTENT(func, intent); }

void tca_clear(void) asm("tca_clear");
void tca_clear(void) { TCA_CLEAR(); }

// Cryptographic Signature Payload (Linked via tca_linker.ld)
char __sentinel_signature[64] __attribute__((section(".signature"))) = {0};

int main() {
  // Cryptographic Root (to ensure it is not optimized out)
  volatile char *root_ptr = __sentinel_signature;

  // Traditional CHERI purecap workload scenario (Stack allocation)
  int buffer[10] = {0};

  // Wrap the workload in Semantic Intent
  // This tells the LLVM pass to inject csetintent and load from .tca_got
  llvm_telos_intent_start("purpose: SpatialBufferManipulation");

  // Deliberate out-of-bounds access
  // TCA enforces WHY this is happening; CHERI hardware enforces WHERE it stops.
  for (int i = 0; i <= 10; i++) {
    buffer[i] = i * 2;
  }

  // Revoke Capability
  // This tells the LLVM pass to inject cclear
  llvm_telos_intent_end();

  return 0;
}
