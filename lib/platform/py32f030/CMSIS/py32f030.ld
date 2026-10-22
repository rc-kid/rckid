/* Minimal linker script for PY32F030 (Cortex-M0+).
 
   The Puya PY32F030 chips differ in x3, x4, x6 and x8, which also are different CMSIS packs available from ARM. They differ in flash and RAM size which is exactly what the lines below configure. 

   Details in datasheets & part descriptions. 
 */

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K   /* PY32F030x8 */
  RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 8K    /* PY32F030x8 */
}

/* Top of stack = end of RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum heap/stack sizes the linker will check (optional) */
_Min_Heap_Size  = 0x200;
_Min_Stack_Size = 0x400;

ENTRY(Reset_Handler)

SECTIONS
{
  /* Vector table goes first in flash - KEEP so --gc-sections can't drop it */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* C++ static constructors (needed by __libc_init_array in the startup file) */
  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* Load address of .data in flash - startup file copies from here.
     Must come after the .data section above so LOADADDR() can resolve it. */
  _sidata = LOADADDR(.data);

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}
