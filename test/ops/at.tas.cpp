C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
D <- D & 31	// doesn't support overshifting yet
B <- C @ D

E <- C >>> D
E <- E & 1
E <- E == 1

B <- B == E
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
