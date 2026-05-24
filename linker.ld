/* Linker script for AetherOS */
ENTRY(_start)

SECTIONS
{
    /* The kernel starts at 1MB physical address (image base is 0x100000) */
    . = 0x100000;

    .text :
    {
        _text_start = .;
        __text_start = .;
        *(.text.entry)
        *(.text)
        *(.text.*)
        . = ALIGN(4096);
        _text_end = .;
        __text_end = .;
    }

    .rdata :
    {
        _rdata_start = .;
        __rdata_start = .;
        *(.rdata)
        *(.rdata.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4096);
        _rdata_end = .;
        __rdata_end = .;
    }

    .data :
    {
        _data_start = .;
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4096);
        _data_end = .;
        __data_end = .;
    }

    .bss :
    {
        _bss_start = .;
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4096);
        _bss_end = .;
        __bss_end = .;
    }

    _kernel_end = .;
    __kernel_end = .;
}
