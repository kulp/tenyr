#define CAT(X,Y)    CAT_(X,Y)
#define CAT_(X,Y)   X ## Y

#define COUNTSTRING(Stem,Val)                                                  \
    Stem:                                                                      \
    CAT(.L_,Stem):                                                             \
        .word (CAT(CAT(.L_,Stem),_end) - CAT(.L_,Stem) - 1) ;                  \
        .chars Val                                          ;                  \
    .global Stem ;                                                             \
    CAT(CAT(.L_,Stem),_end):                                                   \
    //

COUNTSTRING(undefined_word, "undefined word")
COUNTSTRING(ok            , "ok"            )

.word 0

