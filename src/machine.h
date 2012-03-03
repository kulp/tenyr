#ifndef MACHINE_H_
#define MACHINE_H_

#include <stdint.h>

struct mstate {
	size_t devices_count;   ///< how many device slots are used
	size_t devices_max;     ///< how many device slots are allocated
	struct device **devices;
	int32_t regs[16];
} machine;

#endif

