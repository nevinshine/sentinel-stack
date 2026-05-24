.global guest_start
.section .guest_text, "ax"

guest_start:
    /* Trigger the Drawbridge */
    ldr x0, =0x48454B49       /* HEKI magic word */
    ldr x1, =0x40200000       /* Locked Drawbridge Address */
    str w0, [x1]              /* Attempt write -> triggers Stage 2 Data Abort */

    /* QEMU UART Address */
    ldr x1, =0x09000000
    /* ASCII 'G' for Guest */
    mov w2, #71 
print_loop:
    str w2, [x1]
    /* Arbitrary delay */
    ldr x3, =0x1000000 
delay:
    subs x3, x3, #1
    bne delay
    b print_loop
