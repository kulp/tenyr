#ifndef EXPR_H_
#define EXPR_H_

#include "ops.h"
#include "parser.h"

struct const_expr {
    enum const_expr_type {
        CE_BAD,

        CE_OP1,
        CE_OP2,
        CE_SYM,
        CE_EXT,
        CE_IMM,
        CE_ICI,

        CE_max
    } type;
    int32_t i;
    char **symbol_name;     ///< points to either `deferred_name` or `symbol->name`
    char *deferred_name;    ///< possibly pointed to by `symbol_name`
    int op;
    #define IMM_IS_BITS     (1 << 0)    ///< immediate is a bitstring instead of an integer
    #define IGNORE_WIDTH    (1 << 1)    ///< ignore a too-wide constant (for use with ^^)
    #define IS_DEFERRED     (1 << 2)    ///< a constant expression's computation is deferred
    #define IS_EXTERNAL     (1 << 3)
    #define RHS_NEGATE      (1 << 4)
    #define NO_NAMED_RELOC  (1 << 5)
    #define DO_RELOCATION   (1 << 6)
    #define FORBID_LHS      (1 << 7)
    #define SPECIAL_LHS     (1 << 8)
    int flags;  ///< flags are automatically inherited by parents in DAG
    struct element *insn; // for '.'-resolving // TODO rename
    struct element_list **deferred; // for an instruction context not available to me yet
    struct symbol *symbol; // for referencing a specific version of a symbol
    YYLTYPE srcloc; // XXX stop depending on generated parser.h because of YYLTYPE
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

#endif

/* vi: set ts=4 sw=4 et: */
