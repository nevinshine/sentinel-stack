.section .text.boot, "ax", @progbits
.globl _boot
_boot:
    /* Set sp to top of default 128 MiB RAM */
    li sp, 0x84000000

    /* Enable cycle counter access from M-mode and below.
       Set bit 0 (CY) in mcounteren and scounteren CSRs. */
    li   t0, 0x7           /* CY | TM | IR */
    csrw mcounteren, t0
    csrw scounteren, t0

    /* Jump to the C _start symbol */
    j  _start
