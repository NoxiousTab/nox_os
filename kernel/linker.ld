ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
  . = 0x00010000; /* load kernel at 64 KiB */

  .text : {
    KEEP(*(.text.start))
    *(.text*)
  }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : {
    __bss_start = .;
    *(.bss*) *(COMMON)
    __bss_end = .;
  }
}
