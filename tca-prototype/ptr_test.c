#include <stdio.h>

int main() {
    // Allocate a fixed-size array on the stack
    int buffer[1] = {42};
    
    // Assign a pointer to that array
    int *ptr = buffer;
    
    printf("Valid access: buffer[0] = %d\\n", *ptr);
    
    // Intentional pointer arithmetic to go out-of-bounds
    // In a standard C environment, this just calculates a new integer address.
    // In CHERI, this modifies the pointer capability.
    int *out_of_bounds_ptr = ptr + 2; 
    
    // Attempting to write to the out-of-bounds capability.
    // When compiled with purecap, this will trigger a hardware trap (SIGSEGV/SIGPROT).
    *out_of_bounds_ptr = 99;
    
    printf("Out-of-bounds access succeeded? Value: %d\\n", *out_of_bounds_ptr);
    
    return 0;
}
