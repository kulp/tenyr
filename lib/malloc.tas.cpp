#include "common.th"
#include "errno.th"

#define RANK_0_LOG 4
#define RANK_0_SIZE (1 << RANK_0_LOG)
#define RANKS 4

// NOTE it is assumed in the below code that NPER * BPER = 32
#define NPERBITS    4
#define BPERBITS    1
#define NPER        (1 << NPERBITS)
#define BPER        (1 << BPERBITS)
#define TN          0
#define POOL        @+pool

// DATA ------------------------------------------------------------------------
counts: .zero (RANKS - 1) ; .word 1
nodes:  .zero (((1 << RANKS) / NPER) - (1 / NPER))
pool:   .zero (RANK_0_SIZE * (1 << (RANKS - 1)))

// Type `SZ' is a word-wide unsigned integer (some places used as a boolean).
// Type `NP' is a word-wide struct, where the word is split into a 28-bit word
// index into `nodes` and a 4-bit index into the word indexed.

ilog2:
    B   <- 0
    push(D)
L_ilog2_top:
    C   <- C >> 1
    D   <- C == 0
    P <- @+L_ilog2_done & D + P
    B   <- B + 1
    P <- P + @+L_ilog2_top
L_ilog2_done:
    O <- O + 2
    D <- [O - (1 + 0)]
    P <- [O]

// IRANK gets SZ rank in C, returns SZ inverted-rank in B
#define IRANK(B,C)              \
    B   <- - C + (RANKS - 1)  ; \
    //

// LLINK gets NP n in C, returns NP left-child in B
#define LLINK(B,C)              \
    B   <- C << 1             ; \
    B   <- B +  1             ; \
    //

// RLINK gets NP n in C, returns NP right-child in B
#define RLINK(B,C)              \
    B   <- C << 1             ; \
    B   <- B +  2             ; \
    //

// ISLEFT gets NP n in C, returns SZ leftness in B
#define ISLEFT(B,C)             \
    B   <- C &  1             ; \
    B   <- B == 1             ; \
    //

// ISRIGHT gets NP n in C, returns SZ rightness in B
#define ISRIGHT(B,C)            \
    B   <- C &  1             ; \
    B   <- B == 0             ; \
    //

// PARENT gets NP n in C, returns NP parent in B
#define PARENT(B,C)             \
    B   <- C -  1             ; \
    B   <- B >> 1             ; \
    //

// SIBLING gets NP n in C, returns NP sibling in B, T may be C
#define SIBLING(B,C,T)          \
    ISLEFT(T,C)               ; \
    B   <- B - T              ; \
    T   <- ~ T                ; \
    B   <- B + T              ; \
    //

#define FUNCIFY2(Stem,B,C)      \
    push(c)                   ; \
    c   <- C                  ; \
    call(Stem##_func)         ; \
    pop(c)                    ; \
    B   <- b                  ; \
    //

// NODE2RANK gets NP n in C, returns SZ rank in B
#define NODE2RANK(B,C)  FUNCIFY2(NODE2RANK,B,C)
NODE2RANK_func:
    push(D)
    D   <- 0
L_NODE2RANK_top:
    B   <- C == 0
    P <- @+L_NODE2RANK_done & B + P
    D   <- D + 1
    PARENT(C,C)
    P <- P + @+L_NODE2RANK_top
L_NODE2RANK_done:
    B   <- - D + (RANKS - 1)
    O <- O + 2
    D <- [O - (1 + 0)]
    P <- [O]

#define SIZE2RANK(B,C)  FUNCIFY2(SIZE2RANK,B,C)
SIZE2RANK_func:
    B   <- C == 0
    P <- @+L_SIZE2RANK_zero & B + P
    C   <- C - 1
    C   <- C >> RANK_0_LOG
    call(ilog2)
    B   <- B + 1
L_SIZE2RANK_zero:
    O <- O + 1
    P <- [O]

// RANK2WORDS gets SZ rank in C, returns SZ word-count in B, T may be C
// TODO update grammar to permit expressions without parens to save insns
#define RANK2WORDS(B,X,T)       \
    T   <- X                  ; \
    T   <- T + RANK_0_LOG     ; \
    B   <- 1                  ; \
    B   <- B << T             ; \
    //

// GET_COUNT gets SZ rank in C, returns SZ free-count in B
#define GET_COUNT(B,C)          \
    B   <- [C + @+counts + P] ; \
    //

// SET_COUNT gets SZ rank in C, SZ free-count in D, returns nothing
#define SET_COUNT(C,D)          \
    D   -> [C + @+counts + P] ; \
    //

// GET_NODE gets NP n in C, returns SZ shifted-word in B, T may be C
#define GET_NODE(B,C,T)         \
    B   <- C >> BPERBITS      ; \
    T   <- C & (BPER - 1)     ; \
    T   <- T << BPERBITS      ; \
    B   <- [B + @+counts + P] ; \
    B   <- B >> T             ; \
    //

// SET_NODE gets NP n in C, SZ pre-shift-word in D, V can be D
#define SET_NODE(C,D,T,U,V)                                              \
    T   <- C & (BPER - 1)       /* T is shift count in nodes        */ ; \
    T   <- T << BPERBITS        /* T is shift count in bits         */ ; \
    U   <- (BPER - 1)           /* U is mask                        */ ; \
    U   <- U << T               /* U is shifted mask                */ ; \
    V   <- C >> BPERBITS        /* V is index into node array       */ ; \
    C   <- [V + @+counts + P]   /* C is current word                */ ; \
    C   <- C &~ U               /* C is word with node masked out   */ ; \
    U   <- D << T               /* U is now D shifted over          */ ; \
    C   <- C | U                /* C is word with new node included */ ; \
    C   -> [V + @+counts + P]   /* C gets written back to node      */ ; \
    //

// GET_LEAF gets NP n in C, returns SZ leafiness in B, T may be C
#define GET_LEAF(B,C,T)         \
    GET_NODE(B,C,T)           ; \
    B   <- B &  1             ; \
    B   <- B == 0             ; \
    //

// GET_FULL gets NP n in C, returns SZ fullness in B, T may be C
#define GET_FULL(B,C,T)         \
    GET_NODE(B,C,T)           ; \
    B   <- B &  2             ; \
    B   <- B == 2             ; \
    //

// GET_VALID gets NP n in C, returns SZ validity in B
#define GET_VALID(B,C,T,U)     \
    PARENT(C,C)              ; \
    GET_LEAF(T,C,U)          ; \
    B   <- C &~ T            ; \
    //

// SET_LEAF gets NP n in C, SZ truth in D, returns nothing
// T,W can be D, clobbers C
// We take a shortcut ; either we are setting the leaf true from false, and it
// is therefore not full, or we are setting it false from true, and fullness
// has no meaning. This allows us to avoid reading and rewriting the fullness
// value.
#define SET_LEAF(C,D,T,U,V,W)   \
    T   <- D & 1              ; \
    SET_NODE(C,T,U,V,W)       ; \
    //

// SET_FULL gets NP n in C, SZ truth in X, returns nothing, clobbers C
#define SET_FULL(C,X,T,U,V,W) \
    GET_NODE(T,C,U)         ; \
    U   <- X                ; \
    U   <- U &  2           ; \
    T   <- T &~ 2           ; \
    U   <- U | T            ; \
    SET_NODE(C,U,T,V,W)     ; \
    //

// NODE2ADDR gets NP n in C, returns address in B
#define NODE2ADDR(B,C)  FUNCIFY2(NODE2ADDR,B,C)
NODE2ADDR_func:
    O <- O - 2
    D -> [O + (2 - 0)]
    E -> [O + (2 - 1)]
    B   <- 0
    NODE2RANK(E,C)
    E   <- RANK_0_LOG + E   // i = RANK_0_LOG + NODE2RANK(n)
NODE2ADDR_looptop:
    D   <- C == TN          // while (n != TN) {
    P <- @+NODE2ADDR_loopbottom & D + P
    ISRIGHT(D,C)            // ISRIGHT(n)
    D   <- - D              // -1 -> 1, 0 -> 0
    D   <- D << E           // ISRIGHT(n) << i
    B   <- B | D            // base |= ISRIGHT(n) << i
    PARENT(C,C)             // n = PARENT(n)
    E   <- E + 1            // i++
    P <- P + @+NODE2ADDR_looptop // }
NODE2ADDR_loopbottom:
    B   <- B + @+counts + P
    O <- O + 1
    P <- [O]

// ADDR2NODE gets SZ addr in C, returns NP node in B
#define ADDR2NODE(B,C)  FUNCIFY2(ADDR2NODE,B,C)
ADDR2NODE_func:
    O <- O - 4
    D -> [O + (4 - 0)]
    E -> [O + (4 - 1)]
    F -> [O + (4 - 2)]
    G -> [O + (4 - 3)]    // F, G are scratch
    RANK2WORDS(D,(RANKS - 2),F)
    E   <- POOL         // E is base being built
    B   <- TN           // B is the node being checked
L_ADDR2NODE_looptop:
    GET_LEAF(G,B,F)
    P <- @+L_ADDR2NODE_loopdone & G + P
    F   <- C - E        // F is (key - base)
    F   <- F < D        // if true, then left-link
    P <- @+L_ADDR2NODE_left & F + P
    RLINK(B,B)          // n = RLINK(n)
    E   <- E + D        // base += RANK2WORDS(rank)
    P <- P + @+L_ADDR2NODE_loopbottom
L_ADDR2NODE_left:
    LLINK(B,B)          // n = LLINK(n)
    // fallthrough

L_ADDR2NODE_loopbottom:
    D   <- D >> 1       // drop to next smaller rank
    P <- P + @+L_ADDR2NODE_looptop

L_ADDR2NODE_loopdone:
#if DEBUG
    D   <- C == E
    P <- @+L_ADDR2NODE_done & D + P
    call(abort)

L_ADDR2NODE_done:
#endif
    O <- O + 5
    G <- [O - (1 + 3)]
    F <- [O - (1 + 2)]
    E <- [O - (1 + 1)]
    D <- [O - (1 + 0)]
    P <- [O]

.global buddy_malloc
buddy_malloc:
    SIZE2RANK(C,C)
    P <- P + @+buddy_alloc

.global buddy_calloc
buddy_calloc:
    push(E)             // save E which we will use below
    C   <- C * D
    push(C)
    call(buddy_alloc)
    C   <- B
    D   <- 0
    pop(E)              // pop size from C into third memset param slot
    call(memset)
    O <- O + 2
    E <- [O - (1 + 0)]
    P <- [O]

.global buddy_free
buddy_free:
    // TODO
    O <- O + 1
    P <- [O]

.global buddy_realloc
buddy_realloc:
    // TODO
    O <- O + 1
    P <- [O]

// -----------------------------------------------------------------------------

// buddy_splitnode gets NP n in C, SZ rank in D, returns SZ success in B
buddy_splitnode:
    O <- O - 2
    E -> [O + (2 - 0)]
    F -> [O + (2 - 1)]
    B   <- 0
    E   <- D == 0
    P <- @+L_buddy_splitnode_done & E + P
    B   <- 1
    GET_FULL(E,C,F)
    E   <- E == 0
    P <- @+L_buddy_splitnode_notempty &~ E + P

    GET_COUNT(E,D)
    E   <- E - 1
    SET_COUNT(D,E)

L_buddy_splitnode_notempty:
    D   <- D - 1
    GET_COUNT(E,D)
    E   <- E + 2
    SET_COUNT(D,E)

    D   <- 0
    SET_LEAF(C,D,D,E,F,D)

L_buddy_splitnode_done:
    O <- O + 3
    F <- [O - (1 + 1)]
    E <- [O - (1 + 0)]
    P <- [O]

// XXX buddy_nosplit has been modified without sufficient care and appears to
// be internally inconsistent
// TODO move buddy_nosplit into buddy_alloc as in the C version
// buddy_nosplit gets SZ rank in C, returns address or 0 in B
buddy_nosplit:
    O <- O - 4
    D -> [O + (4 - 0)]
    E -> [O + (4 - 1)]
    F -> [O + (4 - 2)]
    G -> [O + (4 - 3)]
    B   <- C
    IRANK(E,C)
    F   <- 1
    F   <- F << E       // F is loop bound
    D   <- F - 1        // D is first index to check
    F   <- F + D        // F is loop bound adjust for `first'
L_buddy_nosplit_loop_top:
    // here is the main body, where we have a nonzero count of the needed
    // rank. B is rank, D is scratch, E is scratch and currently count
    E   <- D
    GET_VALID(C,E,F,G)  // C is validity
    P <- @+L_buddy_nosplit_loop_bottom &~ C + P
    GET_LEAF(C,E,G)     // C is leafiness
    P <- @+L_buddy_nosplit_loop_bottom &~ C + P
    GET_FULL(C,E,G)     // C is fullness
    P <- @+L_buddy_nosplit_loop_bottom & C + P
    SET_FULL(E,-1,D,G,C,D)
    GET_COUNT(C,B)
    C   <- C - 1
    SET_COUNT(E,C)
    C   <- E
    ADDR2NODE(B,C)

L_buddy_nosplit_loop_bottom:
    D   <- D + 1    // D is loop index
    P <- P + @+L_buddy_nosplit_loop_top

    O <- O + 5
    G <- [O - (1 + 3)]
    F <- [O - (1 + 2)]
    E <- [O - (1 + 1)]
    D <- [O - (1 + 0)]
    P <- [O]

// buddy_alloc gets SZ rank in C, returns address or 0 in B
buddy_alloc:
    O <- O - 3
    D -> [O + (3 - 0)]
    E -> [O + (3 - 1)]
    F -> [O + (3 - 2)]                  // D,E are scratch, F is rank
    D   <- C < RANKS
    P <- @+L_buddy_alloc_error &~ D + P    // bail if need is too large
L_buddy_alloc_rankplus:
    D   <- C < RANKS
    P <- @+L_buddy_alloc_rankdone &~ D + P
    GET_COUNT(E,C)
    E   <- E == 0
    P <- @+L_buddy_alloc_rankdone &~ E + P
    F   <- F + 1
    P <- P + @+L_buddy_alloc_rankplus

L_buddy_alloc_rankdone:
    call(buddy_nosplit)
    P <- P + @+L_buddy_alloc_done

L_buddy_alloc_do_split:
    D   <- E
    call(buddy_autosplit)

L_buddy_alloc_done:
    O <- O + 4
    F <- [O - (1 + 2)]
    E <- [O - (1 + 1)]
    D <- [O - (1 + 0)]
    P <- [O]

L_buddy_alloc_error:
    D   <- ENOMEM
    D   -> errno
    B   <- 0
    P <- P + @+L_buddy_alloc_done

