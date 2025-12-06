/* Simple linker script for a flat kernel image for qemu versatilepb */
ENTRY(_start)

SECTIONS
{
  . = 0x00008000;

  .text : {
    *(.text*)
  }

  .rodata : { *(.rodata*) }

  .data : {
    _edata = .;
    *(.data*)
    _end_data = .;
  }

  .bss (NOLOAD) : {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }

  _stack_top = 0x00010000; /* 64 KB stack top - small for qemu test */
}
