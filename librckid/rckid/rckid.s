.syntax unified
.cpu cortex-m0plus
.thumb
.text

.global rckid_mem_fill_32x8
.thumb_func
rckid_mem_fill_32x8:
    @ r0 = buffer
    @ r1 = num transfers
    @ r2 = source
    push    {r4-r5, lr}
    mov r3, r2
    mov r4, r2
    mov r5, r2
loop:
    stm r0!, { r2-r5 }
    stm r0!, { r2-r5 }
    subs r1, r1, #8
    bne loop
    pop     {r4-r5, PC}

@.global rckid_color256_to_rgb
@.thumb_func
@rckid_color256_to_rgb:
@    @r0 = output buffer (uint32, ColorRGB)
@    @r1 = input buffer (uint32, Color256)
@    @r2 = num transfers
@    @r3 = palette (uint32, ColorRGB)
@
@    @r6, r7 = where we store the results
@loop:
@    @ load four pixels from the source and increment 
@    ld r4, [r1]
@    adds r1, r1, 1
@    @ shift the pixels out 8 bits at a time, and determine the color of the pixel 
@    shl 
@
@
@
@    subs r2, 4
@    bne loop
