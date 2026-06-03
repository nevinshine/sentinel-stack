
void llvm_telos_intent_start(void) asm("llvm.telos.intent.start");
void safe_logger() {
  llvm_telos_intent_start();
  // In actual code, there would be IO here
  return;
}
int main() {
  safe_logger();
  return 0;
}
