    .text
    .global UartWriteCharPayload

UartWriteCharPayload:
    push    {r4-r7, lr}          @ save registers + return addr

    ldr     r4, =0xA9A00000      @ UART base
    add     r5, r4, #0x08        @ SR
    add     r6, r4, #0x0C        @ TF

wait_ready:
    ldr     r7, [r5]             @ read status register
    tst     r7, #0x4             @ TX_READY bit
    beq     wait_ready

    mov     r7, #0x41            @ 'A'
    str     r7, [r6]             @ write to TX FIFO

    pop     {r4-r7, pc}          @ restore + return