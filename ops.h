// Instructions
// [TTT.............................] TTT = instruction type/class
// all-zero instruction = no-op
//  a <- a | a + 0 (and A has sticky value zero and takes no writes)
// all-one instruction = illegal instruction
// TTT == 111 : reserved
//
// algebraic assembler
//
// load/store/arith/control (TTT = 0xx)
// [0rDDZZZZXXXXYYYYffffIIIIIIIIIIII]
// I = sign-extended 12-bit immediate
// r =   0 : <-
//       1 : ->
// DD = 00 :  Z  r  X f Y + I
//      01 :  Z  r [X f Y + I]
//      10 : [Z] r  X f Y + I
//      11 : [Z] r [X f Y + I]
//  a <- [b * c + 4]
//  [ip + 3] -> a
//  [ip + c] -> ip (jump from table)
//  c <- c <> 0 (nonzero)
//  d <- c > d (no status flags, just bool result)
//  ri <- ri + c
//  rv <- ri % 2
//  d >> e -> e
//  e <- d >> e
//  ip <- ip + -4
// ops
//  0000 = X bitwise or Y
//  0001 = X bitwise and Y
//  0010 = X add Y
//  0011 = X multiply Y
//  0100 = X modulus Y
//  0101 = X shift left Y
//  0110 = X compare <= Y
//  0111 = X compare == Y
//  1000 = X bitwise nor Y
//  1001 = X bitwise nand Y
//  1010 = X bitwise xor Y
//  1011 = X add two's complement Y
//  1100 = X xor ones' complement Y
//  1101 = X shift right logical Y
//  1110 = X compare > Y
//  1111 = X compare != Y

struct instruction {
    union {
        uint32_t word;
        struct {
            // TODO assumes bits are filled in rightmost-first
            signed   imm : 12;  ///< immediate
            unsigned op  :  4;  ///< operation
            unsigned y   :  4;  ///< operand y
            unsigned x   :  4;  ///< operand x
            unsigned z   :  4;  ///< operand z
            unsigned dd  :  2;  ///< dereference
            unsigned r   :  1;  ///< reverse
            unsigned     :  1;  ///< unused
        } p;
    } u;
};

