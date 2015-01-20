C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
B <- C ^^ D

E <- 12
F <- 1 << E
F <- F - 1
E <- C << 12
F <- D & F
H <- E | F

B <- B == H
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
