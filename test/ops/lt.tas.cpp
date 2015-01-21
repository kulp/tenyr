C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
E <- [(@args + (2 + LOAD_ADDRESS))]

// precondition numbers to be equal to each other with about 1/16 probability
F <- E & 31
F <- 0xf << F
F <- D & F
D <- C ^ F
B <- C < D

// compute inequality
// TODO compute inequality without using subtraction
F <- C >> 31    // get sign bit
G <- D >> 31    // get sign bit
H <- F ^ G      // check sign bit inequality
P <- (@signs_differ - (. + 1)) & H + P

// signs are the same, subtraction will be not overflow
E <- C - D
F <- E >> 31

signs_differ:
B <- B ^ F
B <- ~ B
// B is -1 (matches) or 0 (mismatches)
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
