C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
D <- D & 63
B <- C << D

loop:
N <- D == 0
P <- (@done - (. + 1)) & N + P
C <- C + C
D <- D - 1
P <- (@loop - (. + 1)) + P

done:
B <- B == C
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
