SECTIONS
{
    .rodata :
    {
        __rodata_start = .;
        *(.rodata*)
        __rodata_end = .;
    }
}
INSERT AFTER .text
