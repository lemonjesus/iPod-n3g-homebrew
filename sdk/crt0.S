@ eApp image header + startup for iPod nano 3G firmware.
@ The OS loads the flat image at 0x18000000. It then calls:
@   entry[0]()               once, after linking       -> _eapp_init
@   entry[4](os, game)       every frame               -> eapp_frame
@   entry[1]()               on shutdown               -> eapp_shutdown (optional)
@ I don't think entry[2] and entry[3] are used.

    .arm
    .syntax unified

    .section .eapp_header, "a", %progbits
    .global eapp_header
eapp_header:
    .word 0x70706165            @ 'eapp'
    .word 0x10001000            @ version
    .word 5                     @ entry count
    .word 0                     @ unused?
    .word eapp_imports          @ first import record (tools/mkimports.py)
    .word _eapp_init            @ entry[0]
    .word eapp_shutdown         @ entry[1]
    .word _eapp_nop             @ entry[2]
    .word _eapp_nop             @ entry[3]
    .word eapp_frame            @ entry[4]

    .text
    .align 2
    .global _eapp_init
    .type _eapp_init, %function
_eapp_init:
    @ The 1 MB window is not zeroed by the OS: clear .bss ourselves.
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b
    b       eapp_init

    .global _eapp_nop
    .type _eapp_nop, %function
_eapp_nop:
    bx      lr

    .weak eapp_init
    .set  eapp_init, _eapp_nop
    .weak eapp_shutdown
    .set  eapp_shutdown, _eapp_nop

    .section .note.GNU-stack, "", %progbits
