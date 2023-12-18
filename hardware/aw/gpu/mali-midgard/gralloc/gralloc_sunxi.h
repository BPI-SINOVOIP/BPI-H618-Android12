#ifndef _GRALLOC_SUNXI_H_
#define _GRALLOC_SUNXI_H_

#include <hardware/gralloc.h>
#include <cutils/properties.h>
#include <stdlib.h>

#include "gralloc_helper.h"

#ifndef ION_HEAP_SECURE_MASK
#define ION_HEAP_TYPE_SUNXI_START (ION_HEAP_TYPE_CUSTOM + 1)
#define ION_HEAP_TYPE_SECURE     (ION_HEAP_TYPE_SUNXI_START)
#define ION_HEAP_SECURE_MASK     (1<<ION_HEAP_TYPE_SECURE)
#endif /* ION_HEAP_SECURE_MASK */

struct sunxi_data
{
	int dram_size;  /* MB */

	/*
	 * For allwinner platforms, there are just two secure levels:
	 * 1: the system is with secure os.
	 * 3: the system is without secure os.
	 */
	int secure_level;

	/*
	 * ro.kernel.iomem.type = 0xaf10 -> IOMMU is enabled
	 * ro.kernel.iomem.type = 0xfa01 -> IOMMU is disabled
	 */
	bool iommu_enabled;
};

extern struct sunxi_data sunxi_data;

void sunxi_init(void);

extern char *get_heap_type_name(int heap_mask);

static inline bool sunxi_secure(int usage)
{
	return (usage & GRALLOC_USAGE_PROTECTED) && (sunxi_data.secure_level == 1);
}

static inline bool sunxi_memcontig(int usage)
{
	return usage & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_2D | GRALLOC_USAGE_HW_FB);
}

static inline bool get_gralloc_debug(void)
{
	char gralloc_debug[PROPERTY_VALUE_MAX] = {0};

	property_get("debug.gralloc.showmsg", gralloc_debug, "0");
	if(atoi(gralloc_debug))
		return atoi(gralloc_debug);

	return 0;
}

#endif /* GRALLOC_SUNXI_H_ */
