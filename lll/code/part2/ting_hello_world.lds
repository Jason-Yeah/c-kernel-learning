ENTRY(nomain)

SECTIONS
{
    . = 0X0804800 + SIZEOF_HEADERS;

    tingtext : { *(.text) *(.data) *(.rodata)}

    /DISCARD/ : { *(.comment) }
}