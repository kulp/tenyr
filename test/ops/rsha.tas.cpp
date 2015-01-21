C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
D <- D & 31	// doesn't support overshifting yet
B <- C >> D

E <- 0
M <- 0
loop:
F <- (@masks - (. + 1)) + E + P
G <- [F]		// G is output mask
H <- [F + D]	// H is input mask
I <- C & H
I <- I == 0
I <- G &~ I
M <- M | I
O <- 32 - D
E <- E + 1
N <- E < O
P <- (@loop - (. + 1)) & N + P

// fill in sign bits
loop2:
N <- E < 32
P <- (@done - (. + 1)) &~ N + P
O <- [(@masks - (. + 1 - 31)) + P]
G <- [(@masks - (. + 1)) + E + P]
I <- C & O
I <- I == 0
I <- G &~ I
M <- M | I
E <- E + 1
P <- (@loop2 - (. + 1)) + P

done:
B <- B == M
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
masks:
.word (1 << 0),
      (1 << 1),
      (1 << 2),
      (1 << 3),
      (1 << 4),
      (1 << 5),
      (1 << 6),
      (1 << 7),
      (1 << 8),
      (1 << 9),
      (1 << 10),
      (1 << 11),
      (1 << 12),
      (1 << 13),
      (1 << 14),
      (1 << 15),
      (1 << 16),
      (1 << 17),
      (1 << 18),
      (1 << 19),
      (1 << 20),
      (1 << 21),
      (1 << 22),
      (1 << 23),
      (1 << 24),
      (1 << 25),
      (1 << 26),
      (1 << 27),
      (1 << 28),
      (1 << 29),
      (1 << 30),
      (1 << 31)

