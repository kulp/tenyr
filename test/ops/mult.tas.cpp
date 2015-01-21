C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
H <- 0
B <- C * D

// 31 unrolled repetitions of this block
E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

E <- D &  1
F <- E == 1
G <- F &  C
H <- H +  G
D <- D >> 1
C <- C << 1

B <- B == H
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
