#include "sim.h"
#include "ffi.h"
#include "obj.h"

int main(int argc, char *argv[])
{
    struct obj _o, *o = &_o;;
    struct state _s, *s = &_s;
    size_t size;

    if (argc < 2)
        return -1;

    FILE *f = fopen(argv[1], "rb");
    if (!f)
        return -1;

    obj_read(o, &size, f);
    fclose(f);

    tf_load_obj(s, o);
    // TODO should state take ownership of obj ?
    obj_free(o);

    return 0;
}

