.syntax unified
.cpu cortex-m0plus
.thumb
.text

.global rckid_mem_fill_32x8
.thumb_func
rckid_mem_fill_32:
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


