/* STM32G431RB linker script with explicit FreeRTOS heap section
* Flash:    128 KB @ 0x08000000
* RAM:      32 KB @ 0x20000000
*/

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

/* Entry point */
ENTRY(Reset_Handler)

/* Initial stack pointer = top of RAM*/
_estack = ORIGIN(RAM) + LENGTH(RAM);


SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    /* Program code and read-only data */
    .text :
    {
        *(.text*)
        *(.rodata*)
    } > FLASH

    /* Initialized data (copied from Flash to RAM) */
    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM

    /* Zero-initialized data */
    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    /* Explicit FreeRTOS heap section (optimal) */
    .heap (NOLOAD) :
    {
        __heap_start__ = .;
        . = . +8K;      /* Reserve 8 KB for FreeRTOS heap*/
        __heap_end__ = .;
    } > RAM

    /* End of RAM marker */
    _end = .;

    /* Copy helper for .data */
    _sidata = LOADADDR(.data);
}
