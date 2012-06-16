#include "common.th"

// arguments in C and D
// result in B ; 0 for match, -1 for mismatch

// TODO permit compared strings to differ after the terminating \0

	.global strcmp
strcmp:
    b <- 0              // start with matching
_loop:
    j <- [c]            // load word from string
    g <- [d]            // load word from string
    h <- j == a         // if it is zero, we are done
    i <- g == a         // if it is zero, we are done
    jnzrel(h,_done)
    jnzrel(i,_done)
    c <- c + 1          // increment index for next time
    d <- d + 1          // increment index for next time
    e <- j <> g         // check for mismatch
    b <- b | e          // accumulate mismatches
    goto(_loop)
_done:
    e <- j <> g         // check for mismatch
    b <- b | e          // accumulate mismatches
    e <- h <> i         // check for length mismatch
    b <- b | e          // accumulate mismatches
    ret

