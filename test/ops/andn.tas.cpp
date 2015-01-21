C <- [(@args + (0 + LOAD_ADDRESS))]
D <- [(@args + (1 + LOAD_ADDRESS))]
B <- C &~ D

// TODO rewrite without reference to ~, as this may be implemented in hardware
// common between |~ and &~
D <- ~ D
E <- C ^ D
F <- ~ E
E <- F ^ C
F <- F ^ D
H <- E | F
H <- ~ H

B <- B == H
illegal

args: .word RAND0 ; .word RAND1 ; .word RAND2
