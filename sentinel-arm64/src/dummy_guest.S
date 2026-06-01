.section .text
.global dummy_guest_entry

dummy_guest_entry:
    /* Step 1: Establish a known baseline by writing to an unprotected address */
    /* Assuming 0x40100000 is mapped as Read/Write in Stage-2 */
    ldr     x0, =0x40100000
    mov     x1, #0xAA
    str     x1, [x0]

    /* Step 2: Target the Drawbridge-locked memory region */
    /* This address must match the physical block configured as Read-Only (s2ap = 1) */
    ldr     x2, =0x40200000
    
    /* Step 3: Execute the violating write transaction */
    /* The hardware MMU will combine EL1 permissions with Stage-2 Read-Only */
    /* This exact instruction triggers the Stage-2 Data Abort trap to EL2 */
    mov     x3, #0xFF
    str     x3, [x2]

    /* Step 4: Fallback safeguard loop */
    /* If execution reaches here, the Stage-2 memory protection failed to trap */
1:  b       1b
