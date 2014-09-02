#ifndef ASM_H_
#define ASM_H_

#include <stdio.h>
#include <stdint.h>

enum { ASM_ASSEMBLE = 1, ASM_DISASSEMBLE = 2 };

struct element;
struct symbol;
struct reloc_node;

struct format {
    const char *name;
    int (*init )(FILE *, int flags, void **ud);
    int (*in   )(FILE *, struct element *, void *ud);
    int (*out  )(FILE *, struct element *, void *ud);

    int (*sym  )(FILE *, struct symbol *, void *ud);
    int (*reloc)(FILE *, struct reloc_node *, void *ud);
    int (*fini )(FILE *, void **ud);
};

int find_format_by_name(const void *_a, const void *_b);

#define ASM_AS_INSN     1
#define ASM_AS_DATA     2
#define ASM_AS_CHAR     4
#define ASM_NO_SUGAR    8
#define ASM_VERBOSE    16
#define ASM_QUIET      32

// returns number of characters printed
int print_disassembly(FILE *out, struct element *i, int flags);
int print_registers(FILE *out, int32_t regs[16]);

extern const struct format tenyr_asm_formats[];
extern const size_t tenyr_asm_formats_count;

int make_format_list(int (*pred)(const struct format *), size_t flen,
        const struct format fmts[flen], size_t len, char buf[len],
        const char *sep);
#endif

/* vi: set ts=4 sw=4 et: */
