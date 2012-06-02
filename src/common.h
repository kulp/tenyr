#ifndef COMMON_H_
#define COMMON_H_

#include <setjmp.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#define countof(X) (sizeof (X) / sizeof (X)[0])
#define STR(X) STR_(X)
#define STR_(X) #X
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define SEXTEND(Bits,X) (struct { signed i:(Bits); }){ .i = (X) }.i

#define PTR_MASK ((1 << 24) - 1)

#define list_foreach(Tag,Node,Object)                                          \
    for (struct Tag *Next = (Object), *Node = Next;                            \
            (void)(Node && (Next = Next->next)), Node;                         \
            Node = Next)

// TODO document fixed lengths or remove the limitations
#define SYMBOL_LEN   32
#define LINE_LEN    512

#define PRINT_ERRNO 0x80

enum errcode { /* 0 impossible, 1 reserved for default */ DISPLAY_USAGE=2 };
extern jmp_buf errbuf;
#define fatal(Code,...) \
    fatal_(Code,__FILE__,__LINE__,__func__,__VA_ARGS__)

void fatal_(int code, const char *file, int line, const char *func,
            const char *fmt, ...);

// represents a most basic linked list, used for collecting nodes with twalk
struct todo_node  {
    void *what;
    struct todo_node *next;
};

typedef int cmp(const void *, const void*);
typedef void traverse(const void *node, VISIT order, int level);

int tree_destroy(struct todo_node **todo, void **tree, traverse *trav, cmp *comp);

static inline char *strcopy(char *dest, const char *src, size_t sz)
{
    char *result = strncpy(dest, src, sz);
    dest[sz - 1] = '\0';
    return result;
}

// defines a function that traverses a tsearch tree, adding todo nodes
// assumes struct Tag has a reference to an aggregate named `state' that has a
// pointer named `userdata' that contains our todo-list. See `struct
// link_state' and `struct defn' in tld.c for an example.
#define TODO_TRAVERSE_(Tag)                                                    \
static void traverse_##Tag(const void *node, VISIT order, int level)           \
{                                                                              \
    const struct Tag * const *element = node;                                  \
    struct todo_node **todo = (*element)->state->userdata;                     \
    (void)level;                                                               \
                                                                               \
    if (order == leaf || order == preorder) {                                  \
        struct todo_node *here = malloc(sizeof *here);                         \
        here->what = *(void**)node;                                            \
        here->next = *todo;                                                    \
        *todo = here;                                                          \
    }                                                                          \
}

#endif

