MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .text :
  {
    . = ALIGN(4);

    *(.text*)
    *(.rodata*)
    *(.constdata*)

    /* Keep init/fini in FLASH text region (prevents LMA overlap) */
    KEEP(*(.init))
    KEEP(*(.fini))
    *(.init*)
    *(.fini*)

    /* Common runtime/exception sections */
    *(.eh_frame*)
    *(.ARM.exidx*)
    *(.ARM.extab*)

    . = ALIGN(4);
    _etext = .;
  } > FLASH

  /* Source address in FLASH for copying .data */
  _sidata = _etext;

  .data : AT (_sidata)
  {
    . = ALIGN(8);
    _sdata = .;
    *(.data*)
    . = ALIGN(8);
    _edata = .;
  } > RAM

  .bss :
  {
    . = ALIGN(8);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(8);
    _ebss = .;
  } > RAM

  . = ALIGN(8);
  PROVIDE(end = .);
  PROVIDE(_end = .);
}
