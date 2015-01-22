C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
B <- C |~ D

D <- -1 &~ D
E <- C ^ D
F <- C & D
E <- E + F

B <- B == E
illegal
args: .word RAND0 ; .word RAND1 ; .word RAND2
