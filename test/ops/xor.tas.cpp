C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
B <- C ^ D

E <- C | D
F <- C & D
H <- E &~ F

B <- B == H
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
