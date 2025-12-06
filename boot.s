    .section .text
    .global _start

_start:
    /* Set up a simple stack. Place it in .bss area by using an address.
       For versatilepb we can use an address in RAM, e.g. 0x8000 (small test).
       In real use we'd rely on the linker to place .bss and stack properly. */
    ldr r0, =_stack_top
    mov sp, r0

    /* Call C entry */
    bl kernel_main

hang:
    b hang

    .align 4
