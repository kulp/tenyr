C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
D <- ~ D + 1

// 16 unrolled repetitions of this block
E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

E <- C ^ D
D <- C & D
D <- D << 1
C <- E ^ D
D <- E & D
D <- D << 1

B <- C - D
B <- B == C
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
