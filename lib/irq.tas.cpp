#include "common.th"

#define ISR_ADDR        rel(ISR) //-2
#define IMR_ADDR        rel(IMR) //-3
#define SAVE_ADDR(Idx)  reloff(SAVE_END,Idx) //-4 - Idx

#define save_registers(...)   \
    O   -> [SAVE_ADDR(0)]   ; \
    O   <- SAVE_ADDR(1)     ; \
    pushall_(O,__VA_ARGS__) ; \
    //

.global irq_handler
irq_handler:
    save_registers(B,C,D,I)

    B   <- [ISR_ADDR]

    // get lowest set bit in B
    C   <- B - 1
    D   <- B ^ C
    B   <- B & D

    I   <- 0            // base, for checking 8 bits at a time

check8:
    C   <- B & 0xff     // 0000000011111111
    // TODO this could be rewritten to save a few instructions in the case
    // where the backward branch does not occur, at the expense of speed in the
    // other case
    C   <- C == A
    D   <- C & 8
    I   <- I + D
    B   <- B >> D
    jnzrel(C,check8)

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
    P   <- offsetpc(C)

    illegal

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

jump00: .word  0; illegal
jump01: .word  1; illegal
jump02: .word  2; illegal
jump03: .word  3; illegal
jump04: .word  4; illegal
jump05: .word  5; illegal
jump06: .word  6; illegal
jump07: .word  7; illegal
jump08: .word  8; illegal
jump09: .word  9; illegal
jump10: .word 10; illegal
jump11: .word 11; illegal
jump12: .word 12; illegal
jump13: .word 13; illegal
jump14: .word 14; illegal
jump15: .word 15; illegal
jump16: .word 16; illegal
jump17: .word 17; illegal
jump18: .word 18; illegal
jump19: .word 19; illegal
jump20: .word 20; illegal
jump21: .word 21; illegal
jump22: .word 22; illegal
jump23: .word 23; illegal
jump24: .word 24; illegal
jump25: .word 25; illegal
jump26: .word 26; illegal
jump27: .word 27; illegal
jump28: .word 28; illegal
jump29: .word 29; illegal
jump30: .word 30; illegal
jump31: .word 31; illegal

