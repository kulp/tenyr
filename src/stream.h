#ifndef STREAM_H_
#define STREAM_H_

#include <stddef.h>

typedef int stream_printf(void *userdata, const char *format, ...);
typedef size_t stream_read(void *ptr, size_t size, size_t nitems, void *userdata);
typedef size_t stream_write(const void *ptr, size_t size, size_t nitems, void *userdata);

struct stream_ops {
    stream_printf *fprintf;
    stream_read *fread;
    stream_write *fwrite;
};

struct stream {
    void *ud; ///< userdata
    const struct stream_ops op;
};

typedef struct stream STREAM;

#endif

/* vi: set ts=4 sw=4 et: */
