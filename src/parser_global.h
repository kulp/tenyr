#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"

struct parse_data {
    void *scanner;
    int errored;
    struct {
        unsigned savecol;
        char saveline[LINE_LEN];
    } lexstate;
    struct element_list *top;
    struct deferred_expr {
        struct const_expr *ce;
        uint32_t *dest;     ///< destination word to be updated
        struct deferred_expr *next;
        int width;          ///< width in bits of the right-justified immediate
        int mult;           ///< multiplier (1 or -1, according to sign)
        int flags;          ///< flags used by ce_eval
    } *defexprs;
    struct symbol_list {
        struct symbol *symbol;
        struct symbol_list *next;
    } *symbols;
    struct global_list {
        char *name;
        struct global_list *next;
    } *globals;
    struct reloc_list {
        struct reloc_node {
            char *name;             ///< can be NULL
            struct element *insn;   ///< TODO rename
            int width;              ///< number of bits wide, LSB-justified
            int shift;              ///< number of bits shifted down
            long flags;
        } reloc;
        struct reloc_list *next;
    } *relocs;
};

struct cstr {
    char *head, *tail;
    struct cstr *last, *right;
    int size;
};

struct directive {
    void *data;
    enum directive_type {
        D_NULL, D_GLOBAL, D_SET, D_ZERO
    } type;
};

int tenyr_parse(struct parse_data *);
struct symbol *symbol_find(struct symbol_list *list, const char *name);
void tenyr_push_state(int st, void *yyscanner);
void tenyr_pop_state(void *yyscanner);

#endif

/* vi: set ts=4 sw=4 et: */
