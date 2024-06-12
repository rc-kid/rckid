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


@ 5 + 5 + 2 + 5 + 5 + 2 + 3 + 2 = 27
.global rckid_color256_to_rgb
.section .time_critical.rckid_color256_to_rgb, "ax"
.thumb_func
rckid_color256_to_rgb:
    @r0 = input buffer (uint8_t)
    @r1 = output buffer (uint16_t)
    @r2 = num pixels
    @r3 = palette (uint16_t*)
    @r4 = first two pixels
    @r5 = second two pixels
    @r6 - third two pixels? / tmp
    @r7 - second two pixels
    push    {r4-r6}
loop256:
    @ load first pixel index, multiply by two and load color from palette to r4
    ldrb r4, [r0]       
    lsls r4, r4, 1
    ldrh r4, [r3, r4]
    @ load second pixel index, multiply by wo and load color from palette to r5
    ldrb r5, [r0, 1]
    lsls r5, r5, 1
    ldrh r5, [r3, r5]
    @ join r4 and r5 into r4 (first 2 pixels (shift r4 by 16))
    lsls r5, r5, 16
    orrs r4, r4, r5
    @ load third pixel index, multiply by wo and load color from palette to r5
    ldrb r5, [r0, 2]
    lsls r5, r5, 1
    ldrh r5, [r3, r5]
    @ load fourth pixel index, multiply by wo and load color from palette to r6
    ldrb r6, [r0, 3]
    lsls r6, r6, 1
    ldrh r6, [r3, r6]
    @ join r5 and r6 into r5 (second 2 pixels (shift r5 by 16))
    lsls r6, r6, 16
    orrs r5, r5, r6
    # store the 4 pixels in r4 and r5
    stm r1!,  { r4, r5 }
    # move input index and num pixels to go
    adds r0, r0, 4
    subs r2, r2, 4
    bne loop256
    pop {r4-r6}
    bx lr

@ Blits given picture
@
@ Blits 8 bytes at a time
.global rckid_blit_x8
.thumb_func
rckid_blit_bitmap:
    @r0 - source data, expected in column right to left formt
    @r1 - dest start
    @r2 - source height
    @r3 - source width
    @r4 - dest advance
    
    @ actually writing the glyph, can read 4 at once, test by 0, all and skip, otherwise 

@ Combines three tile layers where.
@
@ start with foreground, if empty go directly to lower layer, etc. 
@ but what to do when the tiles start at different offsets? 
.global rckid_tile_3_layers
.thumb_func
rckid_tile_3_layers:
    @r0 - address for layer 0 (background) (uint8_t *)
    @r1 - address for layer 1 (middle) (uint8_t *)
    @r2 - address for layer 2 (foreground) (uint8_t *)
    @r3 - address for output (uint16_t *)

    ldr r4, [r0]
    ldr r5, [r1]
    ldr r6, [r2]



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
