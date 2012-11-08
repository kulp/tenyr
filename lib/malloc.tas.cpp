#include "common.th"
#include "errno.th"

#define RANK_0_LOG 4
#define RANK_0_SIZE (1 << RANK_0_LOG)
#define RANKS 3

// NOTE there are some assumptions in the below code that NPER * BPER = 32,
// notably in DIFF()
#define NPERBITS 4
#define BPERBITS 1

#define NPER (1 << NPERBITS)
#define BPER (1 << BPERBITS)

#define TN (@nodes)

// TODO size `nodes' appropriately
nodes:
    .word 0

// TODO initialise counts appropriately
counts:
    .word 0, 0, 1

// Type `SZ' is a word-wide unsigned integer (some places used as a boolean).
// Type `NP' is a word-wide struct, where the word is split into a 28-bit word
// pointer and a 4-bit index into the word pointed to. The word pointer must
// have bit 27-31 the same (i.e. we must be able to sign-extend the 28-pointer
// to get the original 32-bit pointer).

// NODE gets SZ index in C, returns NP node in B, clobbers C
#define NODE(B,C)               \
    B   <- C << BPERBITS      ; \
    B   <- B + @nodes         ; \
    C   <- C & (BPER - 1)     ; \
    B   <- B | C              ; \
    //

// IRANK gets SZ rank in C, returns SZ inverted-rank in B
#define IRANK(B,C)              \
    B   <- - C + (RANKS - 1)  ; \
    //

// DIFF gets NP a in C, NP b in D, returns SZ distance in B
#define DIFF(B,C,D)             \
    B   <- C - D              ; \
    //

// INDEX gets NP n in C, returns SZ distance from TB in B
#define INDEX(B,C)              \
    B   <- TN                 ; \
    B   <- B << BPERBITS      ; \
    DIFF(B,C,B)               ; \
    //

// LLINK gets NP n in C, returns NP left-child in B, clobbers C
#define LLINK(B,C)              \
    INDEX(B,C)                ; \
    C   <- B << 1             ; \
    C   <- C +  1             ; \
    NODE(B,C)                 ; \
    //

// RLINK gets NP n in C, returns NP right-child in B, clobbers C
#define RLINK(B,C)              \
    INDEX(B,C)                ; \
    C   <- B << 1             ; \
    C   <- C +  2             ; \
    NODE(B,C)                 ; \
    //

// ISLEFT gets NP n in C, returns SZ leftness in B
#define ISLEFT(B,C)             \
    INDEX(B,C)                ; \
    B   <- B & 1              ; \
    B   <- B == 1             ; \
    //

// ISRIGHT gets NP n in C, returns SZ rightness in B
#define ISRIGHT(B,C)            \
    INDEX(B,C)                ; \
    B   <- B & 1              ; \
    B   <- B == 0             ; \
    //

// PARENT gets NP n in C, returns NP parent in B, clobbers C
#define PARENT(B,C)             \
    INDEX(B,C)                ; \
    C   <- B -  1             ; \
    C   <- C >> 1             ; \
    NODE(B,C)                 ; \
    //

// TODO check SIBLING logic, not very certain of it
// SIBLING gets NP n in C, returns NP sibling in B, clobbers C
#define SIBLING(B,C,T0,T1)      \
    INDEX(B,C)                ; \
    T0  <- B                  ; \
    ISLEFT(C,B)               ; \
    T1  <- ~ C                ; \
    B   <- B + T1             ; \
    B   <- B - C              ; \
    //

// TODO NODE2RANK
// TODO SIZE2RANK
#define SIZE2RANK(B,C) \
    //

// RANK2WORDS gets SZ rank in C, returns SZ word-count in B, clobbers C
#define RANK2WORDS(B,C)         \
    C   <- C + RANK_0_LOG     ; \
    B   <- 1                  ; \
    B   <- B << C             ; \
    //

// GET_COUNT gets SZ rank in C, returns SZ free-count in B
#define GET_COUNT(B,C)          \
    B   <- [C + rel(counts)]  ; \
    //

// SET_COUNT gets SZ rank in C, SZ free-count in D, returns nothing
#define SET_COUNT(C,D,T0)       \
    D   -> [C + rel(counts)]  ; \
    //

// GET_NODE gets NP n in C, returns SZ shifted-word in B, clobbers C
#define GET_NODE(B,C)           \
    B   <- C >> BPERBITS      ; \
    C   <- C & (BPER - 1)     ; \
    B   <- [B]                ; \
    B   <- B >> C             ; \
    //

// GET_LEAF gets NP n in C, returns SZ leafiness in B, clobbers C
#define GET_LEAF(B,C)           \
    GET_NODE(B,C)             ; \
    B   <- B &  1             ; \
    B   <- B == 0             ; \
    //

// GET_FULL gets NP n in C, returns SZ fullness in B, clobbers C
#define GET_FULL(B,C)           \
    GET_NODE(B,C)             ; \
    B   <- B &  2             ; \
    B   <- B == 2             ; \
    //

// GET_VALID gets NP n in C, returns SZ validity in B
#define GET_VALID(B,C,T0)       \
    T0  <- C                  ; \
    INDEX(B,C)                ; \
    PARENT(C,T0)              ; \
    GET_LEAF(T0,C)            ; \
    B   <- B &~ T0            ; \
    //

// TODO SET_LEAF
// TODO SET_FULL
#define SET_FULL(C,D,T0) \
    //
// TODO NODE2ADDR
// TODO ADDR2NODE
ADDR2NODE:
    // TODO
    ret

.global buddy_malloc
buddy_malloc:
    goto(buddy_alloc)

.global buddy_calloc
buddy_calloc:
    C   <- C * D
    push(C)
    call(buddy_alloc)
    C   <- B
    D   <- 0
    pop(E)
    call(memset)
    ret

.global buddy_free
buddy_free:
    // TODO
    ret

.global buddy_realloc
buddy_realloc:
    // TODO
    ret

// -----------------------------------------------------------------------------

buddy_splitnode:
    // TODO
    ret

buddy_autosplit:
    // TODO
    ret

// TODO check for clobbering
// buddy_alloc gets SZ size in C, returns address or 0 in B
buddy_alloc:
    pushall(D,E,F,G)
    SIZE2RANK(B,C)
    D   <- B < RANKS
    jzrel(D,L_buddy_alloc_error)
    D   <- B
    GET_COUNT(E,D)
    D   <- E == 0
    jnzrel(D,L_buddy_alloc_do_split)
    // here is the main body, where we have a nonzero count of the needed
    // rank. B is rank, D is scratch, E is scratch and currently count
    D   <- B
    IRANK(E,D)
    F   <- 1
    F   <- F << E       // F is loop bound
    D   <- F - 1        // D is first index to check
    F   <- F + D        // F is loop bound adjust for `first'
L_buddy_alloc_loop_top:
    NODE(E,D)           // E is NODE(D)
    GET_VALID(C,E,G)    // C is validity
    jzrel(C,L_buddy_alloc_loop_bottom)
    GET_LEAF(C,E)       // C is leafiness
    jzrel(C,L_buddy_alloc_loop_bottom)
    GET_FULL(C,E)       // C is fullness
    jnzrel(C,L_buddy_alloc_loop_bottom)
    SET_FULL(E,-1,G)
    GET_COUNT(C,B)
    C   <- C - 1
    SET_COUNT(E,C,G)
    C   <- E
    call(ADDR2NODE)
    goto(L_buddy_alloc_done)

L_buddy_alloc_loop_bottom:
    D   <- D + 1    // D is loop index
    goto(L_buddy_alloc_loop_top)

L_buddy_alloc_do_split:
    D   <- E
    call(buddy_autosplit)
    goto(L_buddy_alloc_done)

L_buddy_alloc_error:
    D   <- ENOMEM
    D   -> errno
    B   <- 0
L_buddy_alloc_done:
    popall(D,E,F,G)
    ret

