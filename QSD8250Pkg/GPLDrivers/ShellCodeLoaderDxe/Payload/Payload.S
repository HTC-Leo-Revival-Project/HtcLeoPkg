    .text
    .global PayloadEntry
    .type PayloadEntry, %function

PayloadEntry:
    push    {r4-r6, lr}        @ preserve registers + return address

    ldr     r4, =0xa9000864    @ bank6_in (phys)
    ldr     r5, =0xa9000814    @ bank6_out (phys)

    orr     r6, r4, #0x200000  @ set bit 21 (flash enable)
    str     r6, [r5]           @ write to output register

    @ small delay loop
delay:
    mov     r0, #0x10000
delay_loop:
    subs    r0, r0, #1
    bne     delay_loop

    pop     {r4-r6, pc}        @ restore + return