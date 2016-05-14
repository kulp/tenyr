#ifndef COMMON_H_
#define COMMON_H_

#include <setjmp.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "ops.h"

#define countof(X) (sizeof (X) / sizeof (X)[0])
#define STR(X) STR_(X)
#define STR_(X) #X
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MSB32_(Bits,X) ((1ULL << ((Bits) - 1)) & (uint32_t)(X))
#define EXTEND32_(Bits,X) ((-!!MSB32_(Bits,X)) << (Bits))
#define SEXTEND32(Bits,X) ((Bits) >= 32 ? (uint32_t)(X) : (uint32_t)(EXTEND32_(Bits,X) | (((uint32_t)X) & ~(-1UL << (Bits)))))

#define UNUSED   __attribute__((unused))
#define NORETURN __attribute__((noreturn))

#define CONCAT_(X,Y) X ## Y
#define CONCAT(X,Y) CONCAT_(X,Y)
#define LINE(X) CONCAT(X,__LINE__)

#define list_foreach(Tag,Node,Object)                                          \
    for (struct Tag *LINE(Next) = (Object), *Node = LINE(Next);                            \
            (void)(Node && (LINE(Next) = Node->next)), Node;                         \
            Node = LINE(Next))                                                       \
    //

// TODO document fixed lengths or remove the limitations
#define SYMBOL_LEN   32
#define LINE_LEN    512

#define PRINT_ERRNO 0x80

enum errcode { /* 0 impossible, 1 reserved for default */ DISPLAY_USAGE=2 };
extern jmp_buf errbuf;
#define fatal(Code,...) \
    fatal_(Code,__FILE__,__LINE__,__func__,__VA_ARGS__)

#define debug(Level,...) \
    debug_(Level,__FILE__,__LINE__,__func__,__VA_ARGS__)

// use function pointers to support plugin architecture
extern void (* NORETURN fatal_)(int code, const char *file, int line, const char
    *func, const char *fmt, ...);

extern void (*debug_)(int level, const char *file, int line, const char *func,
    const char *fmt, ...);

struct element {
    struct insn_or_data insn;

    struct symbol {
        char *name;
        int column;
        int lineno;
        int32_t reladdr;
        uint32_t size;

        unsigned resolved:1;
        unsigned global:1;
        unsigned unique:1;  ///< if this symbol comes from a label

        struct const_expr *ce;

        struct symbol *next;
    } *symbol;
    struct reloc_node *reloc;
};

typedef int cmp(const void *, const void*);

static inline char *strcopy(char *dest, const char *src, size_t sz)
{
    // Use memcpy() to copy past embedded NUL characters, but force NUL term
    // Explicit cast to (char*) because this function is included in C++ code
    char *result = (char*)memcpy(dest, src, sz);
    dest[sz - 1] = '\0';
    return result;
}

static inline uint32_t swapword(const uint32_t in)
{
    return (((in >> 24) & 0xff) <<  0) |
           (((in >> 16) & 0xff) <<  8) |
           (((in >>  8) & 0xff) << 16) |
           (((in >>  0) & 0xff) << 24);
}

static inline uint32_t fixup_endian(const uint32_t in)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return in;
#else
    return swapword(in);
#endif
}

long long numberise(char *str, int base);

char *build_path(const char *base, const char *fmt, ...);

#define ALIASING_CAST(Type,Expr) \
    *(Type * __attribute__((__may_alias__)) *)&(Expr)

#endif

/* vi: set ts=4 sw=4 et: */
