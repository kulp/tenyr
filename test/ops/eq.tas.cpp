C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
E <- [(@args + (2 + LOAD_ADDRESS))]

// precondition numbers to be equal to each other with about 25% probability
F <- E & 31
F <- 3 << F
F <- D & F
D <- C ^ F
B <- C == D

// compute equality
H <- C ^ D

// OR reduction to find bits that differed
G <- 0xffff
I <- H >>> 16
H <- H & G
H <- H | I
I <- H >>> 8
H <- H & 0xff
H <- H | I
I <- H >>> 4
H <- H & 0xf
H <- H | I
I <- H >>> 2
H <- H & 0x3
H <- H | I
I <- H >>> 1
H <- H & 0x1
H <- H | I

// H is 1 (unequal) or 0 (equal)
H <- - H
// H is -1 (unequal) or 0 (equal)
B <- B ^ H
// B is -1 (matches) or 0 (mismatches)
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
