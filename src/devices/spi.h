#ifndef DEVICES_SPI_H_
#define DEVICES_SPI_H_

typedef int spi_init(void *pcookie);
typedef int spi_select(void *cookie, int _ss);
/// @p in and @c *out must be 0 or 1
typedef int spi_clock(void *cookie, int _ss, int in, int *out);
typedef int spi_fini(void *cookie);

struct spi_ops {
    spi_init   *init;
    spi_select *select;
    spi_clock  *clock;
    spi_fini   *fini;
};

#endif

