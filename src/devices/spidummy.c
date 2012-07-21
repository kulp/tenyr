#include "spi.h"

#include <stdio.h>

int spidummy_spi_init(void *pcookie)
{
	puts(__func__);
	return -1;
}

int spidummy_spi_select(void *pcookie, int _ss)
{
	puts(__func__);
	return -1;
}

int spidummy_spi_clock(void *cookie, int _ss, int in, int *out)
{
	puts(__func__);
	return -1;
}

int spidummy_spi_fini(void *pcookie)
{
	puts(__func__);
	return -1;
}

