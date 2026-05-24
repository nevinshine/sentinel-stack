ENTRY(_start)

SECTIONS
{
    /* QEMU virt machine RAM starts here */
    . = 0x40000000;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /* Allocate a 4KB stack */
    . = ALIGN(4096);
    . += 4096;
    _stack_top = .;

    . = 0x40080000;
    .guest_text : {
        *(.guest_text)
    }
}
