#ifndef COMMON_H_
#define COMMON_H_

#define countof(X) (sizeof (X) / sizeof (X)[0])
#define STR(X) STR_(X)
#define STR_(X) #X
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define SEXTEND(Bits,X) (struct { signed i:(Bits); }){ .i = (X) }.i

#define PTR_MASK ~(-1 << 24)

#endif

