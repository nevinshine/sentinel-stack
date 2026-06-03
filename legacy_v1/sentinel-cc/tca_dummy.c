#include <unistd.h>

void execute_intent() {
  // A simple syscall wrapper that the pass will detect
  write(1, "TCA capability emitted\\n", 23);
}
