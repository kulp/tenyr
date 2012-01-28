#ifndef COMMON_H_
#define COMMON_H_

#include <setjmp.h>

#define countof(X) (sizeof (X) / sizeof (X)[0])
#define STR(X) STR_(X)
#define STR_(X) #X
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define SEXTEND(Bits,X) (struct { signed i:(Bits); }){ .i = (X) }.i

#define PTR_MASK ~(-1 << 24)

#define list_foreach(Tag,Node,Object) \
    for (struct Tag *Next = (Object), *Node = Next; \
            (void)(Node && (Next = Next->next)), Node; \
            Node = Next)

// TODO document fixed lengths or remove the limitations
#define LABEL_LEN    32
#define LINE_LEN    512

enum errcode { /* 0 impossible, 1 reserved for default */ DISPLAY_USAGE=2 };
extern jmp_buf errbuf;
void fatal(const char *message, enum errcode code);

#endif

