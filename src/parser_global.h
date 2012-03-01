#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"

#define SMALL_IMMEDIATE_BITWIDTH    12
#define WORD_BITWIDTH               32

struct parse_data {
    void *scanner;
    struct {
        unsigned savecol;
        char saveline[LINE_LEN];
    } lexstate;
    struct instruction_list *top;
    struct deferred_expr {
        struct const_expr *ce;
        uint32_t *dest;     ///< destination word to be updated
        int width;          ///< width in bits of the right-justified immediate
        int mult;           ///< multiplier (1 or -1, according to sign)
        struct deferred_expr *next;
    } *defexprs;
    struct symbol_list {
        struct symbol *symbol;
        struct symbol_list *next;
    } *symbols;
    struct global_list {
        char name[SYMBOL_LEN];
        struct global_list *next;
    } *globals;
    struct reloc_list {
        struct reloc_node {
            char name[SYMBOL_LEN];   ///< can be empty string for non-globals
            struct instruction *insn;
            int width;
        } reloc;
        struct reloc_list *next;
    } *relocs;
};

int tenyr_parse(struct parse_data *);

struct const_expr {
    enum const_expr_type { OP2, SYM, EXT, IMM, ICI } type; // TODO namespace
    int32_t i;
    char symbolname[SYMBOL_LEN];
    int op;
    struct instruction *insn; // for '.'-resolving
    struct const_expr *left, *right;
};

struct const_expr_list {
    struct const_expr *ce;
    struct const_expr_list *right;
};

struct expr {
    int type;   ///< 0=> X op Y + (mult * I) ; 1=> X op I + Y
    int deref;
    int x;
    int op;
    int y;
    int32_t i;
    int width;  ///< width of deferred expression XXX cleanup
    int mult;   ///< multiplier from addsub
    struct const_expr *ce;
};

struct cstr {
    int len;
    char *str;
    struct cstr *right;
};

struct directive {
    enum directive_type { D_NULL, D_GLOBAL, D_SET } type;
    void *data;
};

#endif

