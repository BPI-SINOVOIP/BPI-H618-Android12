#include <stdlib.h>
#include <linux/ion.h>
#include <sys/sysinfo.h>
#include <cutils/properties.h>
#include "gralloc_sunxi.h"
#include "gralloc_priv.h"
#include <string.h>

char str_heap_type[5][14] =
{
	{"SYSTEM"},
	{"SYSTEM_CONTIG"},
	{"DMA"},
	{"SECURE"},
	{"CARVEOUT"}
};

struct sunxi_data sunxi_data =
{
	.dram_size       = 0,
	.secure_level    = 3,
	.iommu_enabled   = false,
};

static void get_dram_size(void)
{
	int err;
	struct sysinfo s_info;

	err = sysinfo(&s_info);
	if(err)
		AERR("%s(%d): Failed to get dram size, err = %d!\n", __func__, __LINE__, err);
	else
		sunxi_data.dram_size = s_info.totalram/1024/1024;
}

static void get_secure_level(void)
{
	char secure_level[PROPERTY_VALUE_MAX] = {0};
	property_get("ro.sys.widevine_oemcrypto_level", secure_level, "3");
	if(atoi(secure_level))
		sunxi_data.secure_level = atoi(secure_level);
}

static void get_iommu_type(void)
{

#ifdef USE_IOMMU
		sunxi_data.iommu_enabled = true;
#else
		sunxi_data.iommu_enabled = false;
#endif
}

char *get_heap_type_name(int heap_mask)
{
	int index;

	switch(heap_mask)
	{
		case ION_HEAP_SYSTEM_MASK:
			index = 0;
			break;

		case ION_HEAP_SYSTEM_CONTIG_MASK:
			index = 1;
			break;

		case ION_HEAP_TYPE_DMA_MASK:
			index = 2;
			break;

		case ION_HEAP_SECURE_MASK:
			index = 3;
			break;

		case ION_HEAP_CARVEOUT_MASK:
			index = 4;
			break;

		default:
			AERR("Invalid heap mask %d!\n", heap_mask);
			index = -1;
			break;
        }

	if (index >= 0)
	{
		return str_heap_type[index];
	}
	else
	{
		return NULL;
	}
}

void sunxi_init(void)
{
	get_dram_size();
	get_secure_level();
	get_iommu_type();
	if (get_gralloc_debug())
	{
		ALOGD("Dram size is %d MB\n", sunxi_data.dram_size);
		ALOGD("Secure level is %d\n", sunxi_data.secure_level);
		ALOGD("SUNXI_YUV_ALIGN is %d\n", SUNXI_YUV_ALIGN);
		ALOGD("IOMMU is %s\n", sunxi_data.iommu_enabled ? "enabled" : "disabled");
	}
}
