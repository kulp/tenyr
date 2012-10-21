#include "asm.h"
#include "common.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"

#include <stdio.h>

int do_assembly(FILE *in, FILE *out, const struct format *f);

int main(void)
{
    int rc = 0;

    const struct format *f = &tenyr_asm_formats[2]; // text
    FILE *in = fopen("test.tas", "rb");
    rc = do_assembly(in, stdout, f);
    fclose(in);

    return rc;
}

