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

int tenyr_parse(struct parse_data *);

struct const_expr {
    enum const_expr_type { CE_BAD, CE_OP1, CE_OP2, CE_SYM, CE_EXT, CE_IMM, CE_ICI } type;
    int32_t i;
    char symbolname[SYMBOL_LEN];
    int op;
    #define IMM_IS_BITS    (1 << 0) ///< treat an immediate as a bitstring instead of as an integer
    #define IGNORE_WIDTH   (1 << 1) ///< ignore a too-wide constant (for use with ^^)
    #define IS_DEFERRED    (1 << 2) ///< a constant expression's computation is deferred
    #define RHS_NEGATE     (1 << 3)
    #define NO_NAMED_RELOC (1 << 4)
    #define DO_RELOCATION  (1 << 5)
    #define FORBID_LHS     (1 << 6)
    #define IS_EXTERNAL    (1 << 7)
    #define DONE_EVAL      (1 << 8)
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
    int type;   ///< type{0,1,2,3}
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

/* vi: set ts=4 sw=4 et: */
