// Provides a thin abstraction over C streams (FILE*) in order to enable
// library interactions with software that does not support FILE* (e.g. FFI
// with non-C languages).
#ifndef STREAM_H_
#define STREAM_H_

#include <stddef.h>
#include <stdio.h>

typedef struct stream STREAM;

typedef int    stream_printf (STREAM *s, const char *format, ...);
typedef int    stream_scanf  (STREAM *s, const char *format, ...);

typedef size_t stream_read   (      void *ptr, size_t size, size_t nitems, STREAM *s);
typedef size_t stream_write  (const void *ptr, size_t size, size_t nitems, STREAM *s);

typedef int    stream_eof    (STREAM *s);
typedef int    stream_flush  (STREAM *s);

typedef int    stream_seek   (STREAM *s, long offset, int whence);
typedef long   stream_tell   (STREAM *s);

struct stream_ops {
    stream_printf *fprintf;
    stream_scanf  *fscanf;

    stream_read   *fread;
    stream_write  *fwrite;

    stream_eof    *feof;
    stream_flush  *fflush;

    stream_seek   *fseek;
    stream_tell   *ftell;
};

struct stream {
    void *ud; ///< userdata
    const struct stream_ops op;
};

struct stream_ops stream_get_default_ops(void);

struct stream stream_make_from_file(FILE *f);

#endif

/* vi: set ts=4 sw=4 et: */
