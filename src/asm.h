#ifndef ASM_H_
#define ASM_H_

#include <stdio.h>
#include <stdint.h>

#include "param.h"
#include "stream.h"

struct element;
struct symbol;
struct reloc_node;

struct format {
    const char *name;
    int (*init )(STREAM *, struct param_state *, void **ud);
    int (*in   )(STREAM *, struct element *, void *ud);
    int (*out  )(STREAM *, struct element *, void *ud);

    int (*sym  )(STREAM *, struct symbol *, int, void *ud);
    int (*reloc)(STREAM *, struct reloc_node *, void *ud);

    int (*emit )(STREAM *, void **ud);
    int (*fini )(STREAM *, void **ud);
    int (*err  )(          void  *ud);
};

#define ASM_AS_INSN                 (1 << 0)
#define ASM_AS_DATA                 (1 << 1)
#define ASM_AS_CHAR                 (1 << 2)
#define ASM_VERBOSE                 (1 << 3)
#define ASM_QUIET                   (1 << 4)
#define ASM_DISASSEMBLE             (1 << 5)
#define ASM_FORCE_DECIMAL_CONSTANTS (1 << 6)

// returns number of characters printed
int print_disassembly(STREAM *out, const struct element *i, int flags);
int print_registers(STREAM *out, const int32_t regs[16]);

extern const struct format tenyr_asm_formats[];
extern const size_t tenyr_asm_formats_count;

size_t make_format_list(int (*pred)(const struct format *), size_t flen,
        const struct format *fmts, size_t len, char *buf,
        const char *sep);

int find_format(const char *optarg, const struct format **f);

struct parse_data;
struct const_expr;

int ce_eval_const(struct parse_data *pd, struct const_expr *ce,
        int32_t *result);
void ce_free(struct const_expr *ce);

#endif

/* vi: set ts=4 sw=4 et: */
