/* eApp images run at a fixed address: the OS maps a 1 MB window at 0x18000000
 * and reads the executable straight into it. */
OUTPUT_FORMAT("elf32-littlearm")
ENTRY(eapp_header)

MEMORY
{
    APPWIN (rwx) : ORIGIN = 0x18000000, LENGTH = 1M
}

SECTIONS
{
    .header  : { KEEP(*(.eapp_header)) } > APPWIN
    .imports : { KEEP(*(.eapp_imports)) } > APPWIN
    .text    : { *(.text .text.*) } > APPWIN
    .rodata  : { *(.rodata .rodata.*) } > APPWIN
    .data    : { *(.data .data.*) } > APPWIN
    . = ALIGN(4);
    __image_end = .;
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > APPWIN
    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) }
}
