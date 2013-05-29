#include "common.th"

#define IRR_ADDR        rel(IRR) //-1
#define ISR_ADDR        rel(ISR) //-2
#define IMR_ADDR        rel(IMR) //-3
#define SAVE_ADDR(Idx)  reloff(SAVE_END,Idx) //-4 - Idx

#define iret            P   <- [IRR_ADDR] 

// TODO rewrite save_registers() to reduce arithmetic on stack pointer (O is
// updated twice consecutively due to the pushall_() macro where if handwritten
// it could be updated only once)
#define get_interrupt_stack() \
    O   -> [SAVE_ADDR(0)]   ; \
    O   <- SAVE_ADDR(1)     ; \
    //

#define restore_user_stack()  \
    O   <- [SAVE_ADDR(0)]   ; \
    //

.global irq_handler
irq_handler:
    get_interrupt_stack()
    pushall(C,B,D,I)

    B   <- [ISR_ADDR]

    // get lowest set bit in B
    C   <- B - 1
    D   <- B ^ C
    B   <- B & D

    I   <- 0            // base, for checking 8 bits at a time

check8:
    C   <- B & 0xff     // 0000000011111111
    C   <- C == A
    jzrel(C,ready8)
    D   <- C & 8
    I   <- I + D
    B   <- B >> D
    goto(check8)

ready8:
    C   <- B & 0x0f     // 00001111
    C   <- C <> A + 1

    D   <- B & 0x33     // 00110011
    D   <- D <> A + 1
    D   <- C << 1 + D

    C   <- B & 0x55     // 01010101
    C   <- C <> A + 1
    D   <- D << 1 + C

    // now D + I has the bit index of the lowest set bit in B
    C   <- D + I
    C   <- [C + rel(jumptable)]

    push(rel(after))
    P   <- offsetpc(C)

after:
// TODO if we are very constrained for space we could popall(B,D,I) before
// calling the interrupt vector, in case the vector needs stack space more than
// free registers ; or we could just do it so that we don't have to guarantee
// any free registers except C (which contains the interrupt number)
    popall(C,B,D,I)
    restore_user_stack()
    iret

// The IRR is written by the CPU, before irq_handler is called, with the address
// of the next userspace PC to be executed
IRR: .word @abort + 0x1000 // XXX hand-relocated address
ISR: .word -1 << 7
IMR: .word -1

SAVE: .word 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
SAVE_END:

jumptable:
    .word @jump00
    .word @jump01
    .word @jump02
    .word @jump03
    .word @jump04
    .word @jump05
    .word @jump06
    .word @jump07
    .word @jump08
    .word @jump09
    .word @jump10
    .word @jump11
    .word @jump12
    .word @jump13
    .word @jump14
    .word @jump15
    .word @jump16
    .word @jump17
    .word @jump18
    .word @jump19
    .word @jump20
    .word @jump21
    .word @jump22
    .word @jump23
    .word @jump24
    .word @jump25
    .word @jump26
    .word @jump27
    .word @jump28
    .word @jump29
    .word @jump30
    .word @jump31

jump00: .word  0; ret
jump01: .word  1; ret
jump02: .word  2; ret
jump03: .word  3; ret
jump04: .word  4; ret
jump05: .word  5; ret
jump06: .word  6; ret
jump07: .word  7; ret
jump08: .word  8; ret
jump09: .word  9; ret
jump10: .word 10; ret
jump11: .word 11; ret
jump12: .word 12; ret
jump13: .word 13; ret
jump14: .word 14; ret
jump15: .word 15; ret
jump16: .word 16; ret
jump17: .word 17; ret
jump18: .word 18; ret
jump19: .word 19; ret
jump20: .word 20; ret
jump21: .word 21; ret
jump22: .word 22; ret
jump23: .word 23; ret
jump24: .word 24; ret
jump25: .word 25; ret
jump26: .word 26; ret
jump27: .word 27; ret
jump28: .word 28; ret
jump29: .word 29; ret
jump30: .word 30; ret
jump31: .word 31; ret

abort: .word 0x00aadead; illegal
