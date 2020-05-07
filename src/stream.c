#include "stream.h"

#include <stdarg.h>
#include <stdio.h>

static int default_printf(STREAM *s, const char *format, ...)
{
    // We do not try to be completely general -- if the formatted string will
    // not fit in a BUFSIZ, we reserve the right to consider that an error.
    // Failing this way is safer than using a variable-length-array and faster
    // than allocating from the free store, which means it is better for our
    // purposes.
    char buf[BUFSIZ];

    va_list vl;
    va_start(vl, format);

    int need = vsnprintf(buf, sizeof buf, format, vl);
    va_end(vl);

    if (need > 0 && (size_t)need >= sizeof buf)
        return -1; // we ran out of space

    size_t wrote = s->op.fwrite(buf, 1, (size_t)need, s);
    return wrote == (size_t)need ? need : -1;
}

static int default_scanf(STREAM *s, const char *format, int *word)
{
    // It is not feasible to defer to op.fread here. We might try to keep a
    // buffer of our own to read "enough" to satisfy a vsscanf call, but we
    // would not know how many bytes of that buffer were successfully consumed
    // unless we could append an "n" specifier to the format and a pointer to
    // int to the va_list -- and there is no portable way to append to a
    // va_list.
    //
    // Instead, we simply defer to the system fscanf. If a library user wants
    // to read the text-based formats (which are the only ones that want
    // fscanf), it could, at worst, convert them to raw format first, perhaps
    // by using `tas -d -ftext - | tas -fraw -`. Since the text formats are
    // meant basically for demonstration and are of increasingly limited use,
    // this is not perceived to be a great limitation.
    return fscanf(s->ud, format, word);
}

static size_t default_read(void *ptr, size_t size, size_t nitems, STREAM *s)
{
    return fread(ptr, size, nitems, s->ud);
}

static size_t default_write(const void *ptr, size_t size, size_t nitems, STREAM *s)
{
    return fwrite(ptr, size, nitems, s->ud);
}

static int default_eof(STREAM *s)
{
    return feof(s->ud);
}

static int default_flush(STREAM *s)
{
    return fflush(s->ud);
}

static int default_seek(STREAM *s, long offset, int whence)
{
    return fseek(s->ud, offset, whence);
}

static long default_tell(STREAM *s)
{
    return ftell(s->ud);
}

struct stream_ops stream_get_default_ops(void)
{
    return (struct stream_ops){
        .fprintf = default_printf,
        .fscanf = default_scanf,
        .fread = default_read,
        .fwrite = default_write,
        .feof = default_eof,
        .fflush = default_flush,
        .fseek = default_seek,
        .ftell = default_tell,
    };
}

struct stream stream_make_from_file(FILE *f)
{
	return (struct stream){
        .ud = f,
        .op = stream_get_default_ops(),
    };
}

