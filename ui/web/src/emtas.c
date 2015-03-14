#include "asm.h"
#include "common.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"

#include <stdio.h>

int do_assembly(FILE *in, FILE *out, const struct format *f, void *ud);

int main(void)
{
    int rc = 0;

    const struct format *f = &tenyr_asm_formats[2]; // text

    void *ud = NULL;
    if (f->init)
        if (f->init(stdin, NULL, &ud))
            fatal(0, "Error during initialisation for format '%s'", f->name);

    rc = do_assembly(stdin, stdout, f, ud);

    if (f->fini)
        f->fini(stdin, &ud);

    return rc;
}

