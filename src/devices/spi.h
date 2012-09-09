#ifndef DEVICES_SPI_H_
#define DEVICES_SPI_H_

#include "plugin.h"

typedef int EXPORT_CALLING spi_init(void *pcookie, const struct plugin_cookie *plugcookie);
typedef int EXPORT_CALLING spi_select(void *cookie, int _ss);
typedef int EXPORT_CALLING spi_clock(void *cookie, int _ss, int in, int *out); ///< @p in and @c *out must be 0 or 1
typedef int EXPORT_CALLING spi_fini(void *cookie);

struct spi_ops {
    spi_init   *init;
    spi_select *select;
    spi_clock  *clock;
    spi_fini   *fini;
};

#endif

