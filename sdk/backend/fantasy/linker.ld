SECTIONS
{
    .rodata :
    {
        __start_rodata = .;
        *(.rodata*)
        __stop_rodata = .;
    }
}
INSERT AFTER .text
