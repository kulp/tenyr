#include "common.th"

#define IRR_ADDR        (-1)
#define ISR_ADDR        (-2)
#define IMR_ADDR        (-3)
#define SAVE_ADDR(Idx)  (-4 - (Idx))

#define TRAP_ADDR       0xffffffff
#define VECTOR_ADDR     0xffffffc0
#define ISTACK_TOP      0xffffffbf
#define TRAPJUMP        0xfffff800

// get_interrupt_stack takes a parameter of an initial number of slots to
// reserve
#define get_interrupt_stack(N) \
    O   -> [SAVE_ADDR(0)]    ; \
    O   <- SAVE_ADDR(1 + N)  ; \
    //

#define restore_user_stack()  \
    O   <- [SAVE_ADDR(0)]   ; \
    //

trampoline:
    get_interrupt_stack(3)
    writeall_(O, C,B,D)

    B   <- [ISR_ADDR]

    // get lowest set bit in B
    C   <- B - 1
    C   <- B ^ C
    B   <- B & C

    D   <- 0            // base, for checking 8 bits at a time
    goto(check8)

top8:
    D   <- C & 1 + D
    B   <- B >> 8
check8:
    C   <- B & 0xff     // 0000000011111111
    C   <- C == A
    jnzrel(C,top8)

ready8:
    C   <- B & 0x0f     // 00001111
    C   <- C <> A + 1
    C   <- D << 1 + C

    D   <- B & 0x33     // 00110011
    D   <- D <> A + 1
    D   <- C << 1 + D

    C   <- B & 0x55     // 01010101
    C   <- C <> A + 1
    C   <- D << 1 + C

    popall(B,D)
    push(rel(after))
    // now C has the bit index of the lowest set bit in B
    // At vector dispatch, only O and C are saved. O is usable as the vector's
    // stack pointer ; C is the vector number and can be used as scratch.
    P   <- [VECTOR_ADDR + C]

after:
    popall(C)
    restore_user_stack()
    P   <- [IRR_ADDR]

