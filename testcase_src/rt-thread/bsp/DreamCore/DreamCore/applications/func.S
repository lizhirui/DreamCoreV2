    .globl set_finish
set_finish:
    fence.i
    csrw 0x804, a0
    ret