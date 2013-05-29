#include "common.th"

#define IRR_ADDR        rel(IRR) //-1
#define ISR_ADDR        rel(ISR) //-2
#define IMR_ADDR        rel(IMR) //-3
#define SAVE_ADDR(Idx)  reloff(SAVE_END,Idx) //-4 - Idx

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
    pushall(C,B,D,E)

    B   <- [ISR_ADDR]

    // get lowest set bit in B
    C   <- B - 1
    C   <- B ^ C
    B   <- B & C

    E   <- 0            // base, for checking 8 bits at a time
    goto(check8)

top8:
    E   <- C & 8 + E
    B   <- B >> 8
check8:
    C   <- B & 0xff     // 0000000011111111
    C   <- C == A
    jnzrel(C,top8)

ready8:
    C   <- B & 0x0f     // 00001111
    C   <- C <> A + 1

    D   <- B & 0x33     // 00110011
    D   <- D <> A + 1
    D   <- C << 1 + D

    C   <- B & 0x55     // 01010101
    C   <- C <> A + 1
    D   <- D << 1 + C

    // now D + E has the bit index of the lowest set bit in B
    C   <- D + E
    C   <- [C + rel(irqtable)]

    popall(B,D,E)
    push(rel(after))
    // At vector dispatch, only O and C are saved. O is usable as the vector's
    // stack pointer ; C is the vector number and can be used as scratch.
    P   <- offsetpc(C)

after:
    popall(C)
    restore_user_stack()
    P   <- [IRR_ADDR]

// The IRR is written by the CPU, before irq_handler is called, with the address
// of the next userspace PC to be executed
IRR: .word @abort + 0x1000 // XXX hand-relocated address
ISR: .word -1 << 7
IMR: .word -1

SAVE: .word 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
SAVE_END:

irq_unhandled: .word 0x00ffdead ; illegal

.set irq00, @irq_unhandled ; .set irq01, @irq_unhandled ;
.set irq02, @irq_unhandled ; .set irq03, @irq_unhandled ;
.set irq04, @irq_unhandled ; .set irq05, @irq_unhandled ;
.set irq06, @irq_unhandled ; .set irq07, @irq_unhandled ;
.set irq08, @irq_unhandled ; .set irq09, @irq_unhandled ;
.set irq10, @irq_unhandled ; .set irq11, @irq_unhandled ;
.set irq12, @irq_unhandled ; .set irq13, @irq_unhandled ;
.set irq14, @irq_unhandled ; .set irq15, @irq_unhandled ;
.set irq16, @irq_unhandled ; .set irq17, @irq_unhandled ;
.set irq18, @irq_unhandled ; .set irq19, @irq_unhandled ;
.set irq20, @irq_unhandled ; .set irq21, @irq_unhandled ;
.set irq22, @irq_unhandled ; .set irq23, @irq_unhandled ;
.set irq24, @irq_unhandled ; .set irq25, @irq_unhandled ;
.set irq26, @irq_unhandled ; .set irq27, @irq_unhandled ;
.set irq28, @irq_unhandled ; .set irq29, @irq_unhandled ;
.set irq30, @irq_unhandled ; .set irq31, @irq_unhandled ;

.global irq00 ; .global irq01 ; .global irq02 ; .global irq03 ;
.global irq04 ; .global irq05 ; .global irq06 ; .global irq07 ;
.global irq08 ; .global irq09 ; .global irq10 ; .global irq11 ;
.global irq12 ; .global irq13 ; .global irq14 ; .global irq15 ;
.global irq16 ; .global irq17 ; .global irq18 ; .global irq19 ;
.global irq20 ; .global irq21 ; .global irq22 ; .global irq23 ;
.global irq24 ; .global irq25 ; .global irq26 ; .global irq27 ;
.global irq28 ; .global irq29 ; .global irq30 ; .global irq31 ;

irq07: .word  7; ret

irqtable:
    .word @irq00, @irq01, @irq02, @irq03, @irq04, @irq05, @irq06, @irq07,
          @irq08, @irq09, @irq10, @irq11, @irq12, @irq13, @irq14, @irq15,
          @irq16, @irq17, @irq18, @irq19, @irq20, @irq21, @irq22, @irq23,
          @irq24, @irq25, @irq26, @irq27, @irq28, @irq29, @irq30, @irq31

abort: .word 0x00aadead; illegal
