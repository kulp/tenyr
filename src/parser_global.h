#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"

struct parse_data {
    void *scanner;
    int errored;
    struct {
        unsigned savecol;
        unsigned swap:1;
        char saveline[2][LINE_LEN], *savep[2];
    } lexstate;
    struct element_list *top;
    struct deferred_expr {
        struct const_expr *ce;
        uint32_t *dest;     ///< destination word to be updated
        int width;          ///< width in bits of the right-justified immediate
        int mult;           ///< multiplier (1 or -1, according to sign)
        int flags;          ///< flags used by ce_eval
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
            char name[SYMBOL_LEN];  ///< can be empty string for non-globals
            struct element *insn;   ///< TODO rename
            int width;
            long flags;
        } reloc;
        struct reloc_list *next;
    } *relocs;
};

struct cstr {
    int size;
    char *head, *tail;
    struct cstr *last, *right;
};

struct directive {
    enum directive_type {
        D_NULL, D_GLOBAL, D_SET, D_ZERO
    } type;
    void *data;
};

int tenyr_parse(struct parse_data *);

#endif

/* vi: set ts=4 sw=4 et: */
