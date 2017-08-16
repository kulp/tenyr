C <- [P + (@=args + 0)]
D <- [P + (@=args + 1)]

#define Andx(A,B,C) A <-  B &  C
#define Tstx(A,B,C) A <-  B == C
#define Addx(A,B,C) A <-  B +  C
#define Rshx(A,B,C) A <-  B >> C
#define Lshx(A,B,C) A <-  B << C

// in:   F, G
// out:  H
// temp: E
#define block(Add,And,Lsh,Rsh,Tst) \
And(E,G,1) ; \
Tst(E,E,1) ; \
And(E,E,F) ; \
Add(H,H,E) ; \
Rsh(G,G,1) ; \
Lsh(F,F,1) ; \
//

#define _x32(X) X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X

// passing until proven broken
B <- -1

// test type 0
H <- 0
F <- C
G <- D
_x32( block(Addx,Andx,Lshx,Rshx,Tstx) )
I <- C * D
I <- I == H
B <- B & I

// test type 1
H <- 0
F <- C
G <- -123
_x32( block(Addx,Andx,Lshx,Rshx,Tstx) )
I <- C * -123
I <- I == H
B <- B & I

// test type 2
H <- 0
F <- 234
G <- D
_x32( block(Addx,Andx,Lshx,Rshx,Tstx) )
I <- 234 * D
I <- I == H
B <- B & I

illegal

