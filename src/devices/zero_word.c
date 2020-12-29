#include <stdlib.h>

#include "device.h"
#include "sim.h"

device_adder zero_word_add_device;

struct zero_word_state {
    int32_t word;
};

static int zero_word_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    // Assume that zero_word_init will not be called more than once per zero_word_fini
    struct zero_word_state *s = *(void**)cookie = malloc(sizeof *s);
    return s == NULL;
}

static int zero_word_fini(void *cookie)
{
    free(cookie);
    return 0;
}

static int zero_word_op(void *cookie, int op, int32_t addr, int32_t *data)
{
    struct zero_word_state *s = cookie;

    if (op == OP_WRITE) {
        s->word = *data;
    } else if (op == OP_DATA_READ) {
        *data = s->word;
    }

    return 0;
}

int zero_word_add_device(struct device *device)
{
    *device = (struct device){
        .bounds = { 0, 0 },
        .ops = {
            .op = zero_word_op,
            .init = zero_word_init,
            .fini = zero_word_fini,
        },
    };

    return 0;
}

/* vi: set ts=4 sw=4 et: */
