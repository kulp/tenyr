#ifndef PARSER_GLOBAL_H_
#define PARSER_GLOBAL_H_

#include "ops.h"

#define SMALL_IMMEDIATE_BITWIDTH    12
#define WORD_BITWIDTH               32

#define SMALL_IMMEDIATE_MASK    ((1 << SMALL_IMMEDIATE_BITWIDTH) - 1)
#define WORD_MASK               ((1 << WORD_BITWIDTH) - 1)

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

int tenyr_parse(struct parse_data *);

// flag to mark flipping value of relocations after a '-'
#define CE_RHS_FLIP 1
#define CE_NO_NAMED_RELOC 2
// If the flag CE_IS_SIMPLE is when evaluating constant expressions,
// expressions can consist only of OP2 and IMM operations (i.e., no references
// to labels).
#define CE_SIMPLE 4

struct const_expr {
    enum const_expr_type { CE_OP2, CE_SYM, CE_EXT, CE_IMM, CE_ICI } type;
    int32_t i;
    char symbolname[SYMBOL_LEN];
    int op;
    #define IMM_IS_BITS 1 ///< treat an immediate as a bitstring instead of as an integer
    int flags;  ///< flags are automatically inherited by parents in DAG
    struct element *insn; // for '.'-resolving // TODO rename
    struct element_list **deferred; // for an instruction context not available to me yet
    struct symbol *symbol; // for referencing a specific version of a symbol
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
    int mult;   ///< multiplier from addsub
    struct const_expr *ce;
};

struct cstr {
    int len;
    char *str;
    struct cstr *right;
};

struct directive {
    enum directive_type {
        D_NULL, D_GLOBAL, D_SET, D_ZERO
    } type;
    void *data;
};

#endif

