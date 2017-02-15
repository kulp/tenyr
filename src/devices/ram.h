#ifndef RAM_H_
#define RAM_H_

#include <stdint.h>

#define RAM_BASE  (1ULL << 12)
#define RAM_END  ((1ULL << 14) - 1)

struct device;

int ram_add_device(struct device *device);
int ram_add_device_sized(struct device *device, uint32_t base, uint32_t len);

#endif

/* vi: set ts=4 sw=4 et: */
