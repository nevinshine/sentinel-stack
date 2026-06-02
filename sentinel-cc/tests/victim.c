#include <unistd.h>

void llvm_telos_intent_start(void) asm("llvm.telos.intent.start");
void llvm_telos_intent_end(void) asm("llvm.telos.intent.end");

// Mock definitions for x86 linker
void llvm_riscv_tca_cap_setintent(void* func, long intent) asm("llvm.riscv.tca.cap.setintent");
void llvm_riscv_tca_cap_setintent(void* func, long intent) {}

void llvm_riscv_tca_cap_clear(void) asm("llvm.riscv.tca.cap.clear");
void llvm_riscv_tca_cap_clear(void) {}


void safe_logger() {
  const char *msg = "SAFE\n";
  llvm_telos_intent_start();
  // Simulated hypervisor / intent-bound operation
  write(1, msg, 5);
  llvm_telos_intent_end();
}

void unsafe() {
  const char *msg = "UNSAFE\n";
  write(1, msg, 7);
}

// Provide actual array for the signature so sign_tool can overwrite it
char __sentinel_signature[64] __attribute__((section(".signature"))) = {0};

int main() {
  __asm__ volatile("" : : "r"(__sentinel_signature));

  safe_logger();
  unsafe();
  return 0;
}
