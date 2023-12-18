// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Google LLC
 */

#include <cpu_func.h>
#include "dice/ops.h"

void DiceClearMemory(void *context, size_t size, void *address) {
	(void)context;
	memset(address, 0, size);
	barrier_data(address); /* prevent dead-store elimination */
	flush_dcache_range((unsigned long)address, size);
}
