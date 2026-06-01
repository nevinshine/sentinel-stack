ENTRY(_start)

SECTIONS
{
    /* QEMU ARM64 'virt' machine physical RAM starts here */
    . = 0x40000000;

    .text : {
        *(.text.boot)  /* Ensure boot.S entry code runs first */
        *(.text .text.*)
    }

    . = ALIGN(2048);
    .vectors : {
        *(.vectors)    /* Vector table entry point */
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* Allocate an 8KB stack space for EL2 execution */
    . = ALIGN(16);
    . = . + 0x2000;
    __el2_stack_top = .;

    _end = .;
}
