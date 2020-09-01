#define elem(Dest, Base, Index) \
    Dest <- E * Index         ; \
    Dest <- Dest + Base       ; \
    //

#define swap(i0, i1)         \
    o <- o - 4             ; \
    c -> [o + (4 - 0)]     ; \
    d -> [o + (4 - 1)]     ; \
    e -> [o + (4 - 2)]     ; \
    f -> [o + (4 - 3)]     ; \
    f <- i0                ; \
    g <- i1                ; \
    [o] <- p + 2 ; o <- o - 1 ; p <- @+do_swap + p ; \
    o <- o + 4             ; \
    f <- [o - 3]           ; \
    e <- [o - 2]           ; \
    d <- [o - 1]           ; \
    c <- [o - 0]             \
    //

do_swap:
    l <- c
    k <- e
    o <- o - e
    c <- o + 1
    elem(d,l,f)
    /* E is already width */
    [o] <- p + 2 ; o <- o - 1 ; p <- @+memcpy + p
    elem(c,l,f)
    elem(d,l,g)
    e <- k
    [o] <- p + 2 ; o <- o - 1 ; p <- @+memcpy + p
    elem(c,l,g)
    d <- o + 1
    e <- k
    [o] <- p + 2 ; o <- o - 1 ; p <- @+memcpy + p
    o <- o + k
    o <- o + 1
    p <- [o]

// c <- base
// d <- number of elements
// e <- size of element
// f <- comparator
#define SI M
#define II J
#define PI H
#define LI I
#define BASE C

    .global qsort
qsort:
    o <- o - 7
    g -> [o + (7 - 0)]
    h -> [o + (7 - 1)]
    i -> [o + (7 - 2)]
    j -> [o + (7 - 3)]
    k -> [o + (7 - 4)]
    l -> [o + (7 - 5)]
    m -> [o + (7 - 6)]
    h <- d < 2                  // test for base case
    p <- @+L_qsort_done & h + p
    PI <- d >> 1                // partition index
    LI <- d - 1                 // last index
    // partitioning
    swap(h,i)
    SI <- 0                     // store index
    II <- 0                     // i index

L_qsort_partition:
    k <- II < LI
    p <- @+L_qsort_partition_done &~ k + p
    o <- o - 1
    c -> [o + (1 - 0)]
    elem(k, BASE, II)
    elem(d, BASE, LI)
    c <- k
    [o] <- p + 2 ; o <- o - 1
    p <- f                      // call comparator
    o <- o + 1
    c <- [o - 0]
    k <- b < 0
    p <- @+L_qsort_noswap &~ k + p
    swap(II,SI)
    SI <- SI + 1
L_qsort_noswap:
    II <- II + 1
    p <- p + @+L_qsort_partition

L_qsort_partition_done:
    swap(SI,LI)
    PI <- SI

    // recursive cases --------------------
    // save argument registers
    m <- c
    j <- e
    k <- f
    // C is already elem(c,c,0)
    d <- PI
    // E is already width
    // F is already callback
    [o] <- p + 2 ; o <- o - 1 ; p <- @+qsort + p
    // restore argument registers
    c <- m
    d <- LI - PI
    e <- j
    f <- k
    PI <- PI + 1
    elem(k,BASE,PI)
    c <- k
    // TODO tail call
    [o] <- p + 2 ; o <- o - 1 ; p <- @+qsort + p

L_qsort_done:
    o <- o + 8
    m <- [o - (1 + 6)]
    l <- [o - (1 + 5)]
    k <- [o - (1 + 4)]
    j <- [o - (1 + 3)]
    i <- [o - (1 + 2)]
    h <- [o - (1 + 1)]
    g <- [o - (1 + 0)]
    p <- [o]

