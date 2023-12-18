/*
 * Copyright (C) 2016-2019 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <log/log.h>
#include <assert.h>
#include <vector>

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "gralloc_priv.h"
#include "mali_gralloc_bufferallocation.h"
#include "format_info.h"

#if GRALLOC_USE_LEGACY_CALCS == 1
#include "legacy/buffer_alloc.h"
#endif
#include <hardware/graphics-sunxi.h>

static mali_gralloc_format_caps cpu_runtime_caps;
static mali_gralloc_format_caps dpu_runtime_caps;
static mali_gralloc_format_caps dpu_aeu_runtime_caps;
static mali_gralloc_format_caps vpu_runtime_caps;
static mali_gralloc_format_caps gpu_runtime_caps;
static mali_gralloc_format_caps cam_runtime_caps;
/* Writing to runtime_caps_read is guarded by mutex caps_init_mutex. */
static pthread_mutex_t caps_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool runtime_caps_read = false;

#define MALI_GRALLOC_GPU_LIB_NAME "libGLES_mali.so"
#define MALI_GRALLOC_VPU_LIB_NAME "libstagefrighthw.so"
#define MALI_GRALLOC_DPU_LIB_NAME "hwcomposer.default.so"
#define MALI_GRALLOC_DPU_AEU_LIB_NAME "dpu_aeu_fake_caps.so"

/* Producer/consumer definitions.
 * CPU: Software access
 * GPU: Graphics processor
 * DPU: Display processor
 * DPU_AEU: AFBC encoder (input to DPU)
 * VPU: Video processor
 * CAM: Camera ISP
 */
#define MALI_GRALLOC_PRODUCER_CPU     ((uint16_t)1 << 0)
#define MALI_GRALLOC_PRODUCER_GPU     ((uint16_t)1 << 1)
#define MALI_GRALLOC_PRODUCER_DPU     ((uint16_t)1 << 2)
#define MALI_GRALLOC_PRODUCER_DPU_AEU ((uint16_t)1 << 3)
#define MALI_GRALLOC_PRODUCER_VPU     ((uint16_t)1 << 4)
#define MALI_GRALLOC_PRODUCER_CAM     ((uint16_t)1 << 5)

#define MALI_GRALLOC_CONSUMER_CPU     ((uint16_t)1 << 0)
#define MALI_GRALLOC_CONSUMER_GPU     ((uint16_t)1 << 1)
#define MALI_GRALLOC_CONSUMER_DPU     ((uint16_t)1 << 2)
#define MALI_GRALLOC_CONSUMER_VPU     ((uint16_t)1 << 3)

typedef struct
{
	uint32_t base_format;
	uint64_t format_ext;
	uint16_t f_flags;
} fmt_props;



static bool get_block_capabilities(const char *name, mali_gralloc_format_caps *block_caps)
{
	void *dso_handle = NULL;
	bool rval = false;

	/* Clear any error conditions */
	dlerror();

	/* Look for MALI_GRALLOC_FORMATCAPS_SYM_NAME_STR symbol in user-space drivers
	 * to determine hw format capabilities.
	 */
	dso_handle = dlopen(name, RTLD_LAZY);

	if (dso_handle)
	{
		void *sym = dlsym(dso_handle, MALI_GRALLOC_FORMATCAPS_SYM_NAME_STR);

		if (sym)
		{
			memcpy((void *)block_caps, sym, sizeof(mali_gralloc_format_caps));
			rval = true;
		}
		else
		{
			AERR("Symbol %s is not found in %s shared object",
			      MALI_GRALLOC_FORMATCAPS_SYM_NAME_STR, name);
		}
		dlclose(dso_handle);
	}
	else
	{
		AWAR("Unable to dlopen %s shared object, error = %s", name, dlerror());
	}

	return rval;
}


/*
 * Obtains the capabilities of each media system IP that form the producers
 * and consumers. Default capabilities are assigned (within this function) for
 * each IP, based on CFLAGS which specify the version of each IP (or, for GPU,
 * explicit features):
 * - GPU: MALI_GPU_SUPPORT_*
 * - DPU: MALI_DISPLAY_VERSION
 * - VPU: MALI_VIDEO_VERSION
 *
 * See src/Android.mk for default values.
 *
 * These defaults can be overridden by runtime capabilities defined in the
 * userspace drivers (*.so) loaded for each IP. The drivers should define a
 * symbol named MALI_GRALLOC_FORMATCAPS_SYM_NAME, which contains all
 * capabilities from set MALI_GRALLOC_FORMAT_CAPABILITY_*
 *
 * @return none.
 *
 * NOTE: although no capabilities are returned, global variables global variables
 * named '*_runtime_caps' are updated.
 */
static void get_ip_capabilities(void)
{
	/* Ensure capability setting is not interrupted by other
	 * allocations during start-up.
	 */
	pthread_mutex_lock(&caps_init_mutex);

	if (runtime_caps_read)
	{
		goto already_init;
	}

	sanitize_formats();

	memset((void *)&cpu_runtime_caps, 0, sizeof(cpu_runtime_caps));
	memset((void *)&dpu_runtime_caps, 0, sizeof(dpu_runtime_caps));
	memset((void *)&dpu_aeu_runtime_caps, 0, sizeof(dpu_aeu_runtime_caps));
	memset((void *)&vpu_runtime_caps, 0, sizeof(vpu_runtime_caps));
	memset((void *)&gpu_runtime_caps, 0, sizeof(gpu_runtime_caps));
	memset((void *)&cam_runtime_caps, 0, sizeof(cam_runtime_caps));

	/* Determine CPU IP capabilities */
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616;

	/* Determine GPU IP capabilities */
	if (access(MALI_GRALLOC_GPU_LIBRARY_PATH1 MALI_GRALLOC_GPU_LIB_NAME, R_OK) == 0)
	{
		get_block_capabilities(MALI_GRALLOC_GPU_LIBRARY_PATH1 MALI_GRALLOC_GPU_LIB_NAME, &gpu_runtime_caps);
	}
	else if (access(MALI_GRALLOC_GPU_LIBRARY_PATH2 MALI_GRALLOC_GPU_LIB_NAME, R_OK) == 0)
	{
		get_block_capabilities(MALI_GRALLOC_GPU_LIBRARY_PATH2 MALI_GRALLOC_GPU_LIB_NAME, &gpu_runtime_caps);
	}

	if ((gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT) == 0)
	{
#if MALI_GPU_SUPPORT_AFBC_BASIC == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_READ;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616;

#if MALI_GPU_SUPPORT_AFBC_YUV_WRITE == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_WRITE;
#endif

#if MALI_GPU_SUPPORT_AFBC_SPLITBLK == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
#endif

#if MALI_GPU_SUPPORT_AFBC_WIDEBLK == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
#endif

#if MALI_GPU_SUPPORT_AFBC_TILED_HEADERS == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS;
#endif
#endif /* MALI_GPU_SUPPORT_AFBC_BASIC == 1 */
	}

/* Build specific capability changes */
#if GRALLOC_ARM_NO_EXTERNAL_AFBC == 1
	{
		dpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		gpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		vpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		cam_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
#endif

	runtime_caps_read = true;

already_init:
	pthread_mutex_unlock(&caps_init_mutex);

	ALOGV("GPU format capabilities 0x%" PRIx64, gpu_runtime_caps.caps_mask);
	ALOGV("DPU format capabilities 0x%" PRIx64, dpu_runtime_caps.caps_mask);
	ALOGV("VPU format capabilities 0x%" PRIx64, vpu_runtime_caps.caps_mask);
	ALOGV("CAM format capabilities 0x%" PRIx64, cam_runtime_caps.caps_mask);
}


/* This is used by the unit tests to get the capabilities for each IP. */
extern "C" {
	void mali_gralloc_get_caps(struct mali_gralloc_format_caps *gpu_caps,
	                           struct mali_gralloc_format_caps *vpu_caps,
	                           struct mali_gralloc_format_caps *dpu_caps,
	                           struct mali_gralloc_format_caps *dpu_aeu_caps,
	                           struct mali_gralloc_format_caps *cam_caps)
	{
		get_ip_capabilities();

		memcpy(gpu_caps, (void *)&gpu_runtime_caps, sizeof(*gpu_caps));
		memcpy(vpu_caps, (void *)&vpu_runtime_caps, sizeof(*vpu_caps));
		memcpy(dpu_caps, (void *)&dpu_runtime_caps, sizeof(*dpu_caps));
		memcpy(dpu_aeu_caps, (void *)&dpu_aeu_runtime_caps, sizeof(*dpu_aeu_caps));
		memcpy(cam_caps, (void *)&cam_runtime_caps, sizeof(*cam_caps));
	}
}


/*
 * Determines all IP consumers included by the requested buffer usage.
 * Capabilities must be defined (OPTIONS_PRESENT) to indicate that an IP is part
 * of the media system (otherwise it will be ignored). Private usage flags are
 * excluded from this process.
 *
 * @param usage   [in]    Buffer usage.
 *
 * @return flags word of all enabled consumers;
 *         0, if no consumers are enabled
 */
static uint16_t get_consumers(uint64_t usage)
{
	uint16_t consumers = 0;

	/* Private usage is not applicable to consumer derivation */
	usage &= ~GRALLOC_USAGE_PRIVATE_MASK;

	get_ip_capabilities();

	if (usage == GRALLOC_USAGE_HW_COMPOSER &&
	    dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		consumers = MALI_GRALLOC_CONSUMER_DPU;
	}
	else
	{
		if (usage & GRALLOC_USAGE_SW_READ_MASK &&
		    cpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			consumers |= MALI_GRALLOC_CONSUMER_CPU;
		}

		/* GRALLOC_USAGE_HW_FB describes a framebuffer which contains a
		 * pre-composited scene that is scanned-out to a display. This buffer
		 * can be consumed by even the most basic display processor which does
		 * not support multi-layer composition.
		 */
		if (usage & GRALLOC_USAGE_HW_FB &&
		    dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			consumers |= MALI_GRALLOC_CONSUMER_DPU;
		}

		if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER &&
		    vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			consumers |= MALI_GRALLOC_CONSUMER_VPU;
		}

		/* GRALLOC_USAGE_HW_COMPOSER does not explicitly define whether the
		 * display processor is producer or consumer. When used in combination
		 * with GRALLOC_USAGE_HW_TEXTURE, it is assumed to be consumer since the
		 * GPU and DPU both act as compositors.
		 */
		if ((usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER)) ==
		    (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER) &&
			 dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			consumers |= MALI_GRALLOC_CONSUMER_DPU;
		}

		if (usage & GRALLOC_USAGE_HW_TEXTURE &&
		    gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			consumers |= MALI_GRALLOC_CONSUMER_GPU;
		}
	}

	return consumers;
}


/*
 * Determines all IP producers included by the requested buffer usage.
 * Capabilities must be defined (OPTIONS_PRESENT) to indicate that an IP is part
 * of the media system (otherwise it will be ignored). Private usage flags are
 * excluded from this process.
 *
 * @param usage   [in]    Buffer usage.
 *
 * @return flags word of all enabled producers;
 *         0, if no producers are enabled
 */
static uint16_t get_producers(uint64_t usage)
{
	uint16_t producers = 0;

	/* Private usage is not applicable to producer derivation */
	usage &= ~GRALLOC_USAGE_PRIVATE_MASK;

	get_ip_capabilities();

	if (usage == GRALLOC_USAGE_HW_COMPOSER &&
	    dpu_aeu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producers = MALI_GRALLOC_PRODUCER_DPU_AEU;
	}
	else
	{
		if (usage & GRALLOC_USAGE_SW_WRITE_MASK &&
		    cpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			producers |= MALI_GRALLOC_PRODUCER_CPU;
		}

		/* DPU is normally consumer however, when there is an alternative
		 * consumer (VPU) and no other producer (e.g. VPU), it acts as a producer.
		 */
		if ((usage & GRALLOC_USAGE_DECODER) != GRALLOC_USAGE_DECODER &&
		    (usage & (GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER)) ==
		    (GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER) &&
		    dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			producers |= MALI_GRALLOC_PRODUCER_DPU;
		}

		if (usage & GRALLOC_USAGE_HW_RENDER &&
		    gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			producers |= MALI_GRALLOC_PRODUCER_GPU;
		}

		if (usage & GRALLOC_USAGE_HW_CAMERA_WRITE &&
		    cam_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			producers |= MALI_GRALLOC_PRODUCER_CAM;
		}

		/* Video decoder producer is signalled by a combination of usage flags
		 * (see definition of GRALLOC_USAGE_DECODER).
		 */
		if ((usage & GRALLOC_USAGE_DECODER) == GRALLOC_USAGE_DECODER &&
		    vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
		{
			producers |= MALI_GRALLOC_PRODUCER_VPU;
		}
	}

	return producers;
}


/*
 * Determines the intersection of all IP consumers capability sets. Since all
 * capabiltiies are positive, the intersection can be expressed via a logical
 * AND operation. Capabilities must be defined (OPTIONS_PRESENT) to indicate
 * that an IP is part of the media system (otherwise it will be ignored).
 * See definition of MALI_GRALLOC_FORMAT_CAPABILITY_* for more information.
 *
 * @param consumers   [in]    Buffer consumers.
 *
 * @return flags word of common capabilities shared by *all* consumers;
 *         0, if no capabilities are shared
 */
static uint64_t get_consumer_caps(const uint16_t consumers)
{
	uint64_t consumer_caps = ~0;

	get_ip_capabilities();

	/* Consumers can't write */
	consumer_caps &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_WRITE;

	if (consumers & MALI_GRALLOC_CONSUMER_CPU)
	{
		consumer_caps &= cpu_runtime_caps.caps_mask;
	}

	if (consumers & MALI_GRALLOC_CONSUMER_GPU &&
	    gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		consumer_caps &= gpu_runtime_caps.caps_mask;
	}

	if (consumers & MALI_GRALLOC_CONSUMER_DPU &&
	    dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		consumer_caps &= dpu_runtime_caps.caps_mask;
	}

	if (consumers & MALI_GRALLOC_CONSUMER_VPU &&
	    vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		consumer_caps &= vpu_runtime_caps.caps_mask;
	}

	return consumer_caps;
}


/*
 * Determines the intersection of all IP producers capability sets. Since all
 * capabiltiies are positive, the intersection can be expressed via a logical
 * AND operation. Capabilities must be defined (OPTIONS_PRESENT) to indicate
 * that an IP is part of the media system (otherwise it will be ignored).
 * See definition of MALI_GRALLOC_FORMAT_CAPABILITY_* for more information.
 *
 * @param producers   [in]    Buffer producers.
 *
 * @return flags word of common capabilities shared by *all* producers;
 *         0, if no capabilities are shared
 */
static uint64_t get_producer_caps(const uint16_t producers)
{
	uint64_t producer_caps = ~0;

	get_ip_capabilities();

	/* Producers can't read */
	producer_caps &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_READ;

	if (producers & MALI_GRALLOC_PRODUCER_CPU)
	{
		producer_caps &= cpu_runtime_caps.caps_mask;
	}

	if (producers & MALI_GRALLOC_PRODUCER_GPU &&
	    gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producer_caps &= gpu_runtime_caps.caps_mask;
	}

	if (producers & MALI_GRALLOC_PRODUCER_DPU &&
	    dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producer_caps &= dpu_runtime_caps.caps_mask;
	}

	if (producers & MALI_GRALLOC_PRODUCER_DPU_AEU &&
	    dpu_aeu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producer_caps &= dpu_aeu_runtime_caps.caps_mask;
	}

	if (producers & MALI_GRALLOC_PRODUCER_CAM &&
	    cam_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producer_caps &= cam_runtime_caps.caps_mask;
	}

	if (producers & MALI_GRALLOC_PRODUCER_VPU &&
	    vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		producer_caps &= vpu_runtime_caps.caps_mask;
	}

	return producer_caps;
}


#if GRALLOC_USE_LEGACY_CALCS == 1
namespace legacy
{

void mali_gralloc_adjust_dimensions(const uint64_t internal_format,
                                    const uint64_t usage,
                                    const alloc_type_t type,
                                    const uint32_t width,
                                    const uint32_t height,
                                    int * const internal_width,
                                    int * const internal_height)
{
	/* Determine producers and consumers. */
	const uint16_t producers = get_producers(usage);
	const uint16_t consumers = get_consumers(usage);

	/*
	 * Default: define internal dimensions the same as public.
	 */
	*internal_width = width;
	*internal_height = height;

	/*
	 * Video producer requires additional height padding of AFBC buffers (whole
	 * rows of 16x16 superblocks). Cropping will be applied to internal
	 * dimensions to fit the public size.
	 */
	if (producers & MALI_GRALLOC_PRODUCER_VPU)
	{
		if (internal_format & MALI_GRALLOC_INTFMT_AFBC_BASIC)
		{
			switch (internal_format & MALI_GRALLOC_INTFMT_FMT_MASK)
			{
			/* 8-bit/10-bit YUV420 formats. */
			case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
			case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
			case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
			case MALI_GRALLOC_FORMAT_INTERNAL_Y0L2:
				*internal_height += (internal_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS) ? 16 : 32;
				break;

			default:
				break;
			}
		}
	}

	get_afbc_alignment(*internal_width, *internal_height, type,
	                   internal_width, internal_height);

out:
	ALOGV("%s: internal_format=0x%" PRIx64 " usage=0x%" PRIx64
	      " width=%u, height=%u, internal_width=%d, internal_height=%d",
	      __FUNCTION__, internal_format, usage, width, height, *internal_width, *internal_height);
}

}
#endif /* end of legacy */


/*
 * Update buffer dimensions for producer/consumer constraints. This process is
 * not valid with CPU producer/consumer since the new resolution cannot be
 * communicated to generic clients through the public APIs. Adjustments are
 * likely to be related to AFBC.
 *
 * @param alloc_format   [in]    Format (inc. modifiers) to be allocated.
 * @param usage          [in]    Buffer usage.
 * @param width          [inout] Buffer width (in pixels).
 * @param height         [inout] Buffer height (in pixels).
 *
 * @return none.
 */
void mali_gralloc_adjust_dimensions(const uint64_t alloc_format,
                                    const uint64_t usage,
                                    int* const width,
                                    int* const height)
{
	/* Determine producers and consumers. */
	const uint16_t producers = get_producers(usage);
	const uint16_t consumers = get_consumers(usage);

	/*
	 * Video producer requires additional height padding of AFBC buffers (whole
	 * rows of 16x16 superblocks). Cropping will be applied to internal
	 * dimensions to fit the public size.
	 */
	if ((producers & MALI_GRALLOC_PRODUCER_VPU) &&
		(alloc_format & MALI_GRALLOC_INTFMT_AFBC_BASIC))
	{
		const int32_t idx = get_format_index(alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
		if (idx != -1)
		{
			/* 8-bit/10-bit YUV420 formats. */
			if (formats[idx].is_yuv && formats[idx].hsub == 2 && formats[idx].vsub == 2)
			{
				*height += (alloc_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS) ? 16 : 32;
			}
		}
	}

	if (producers & MALI_GRALLOC_PRODUCER_GPU)
	{
		/* Pad all AFBC allocations to multiple of GPU tile size. */
		if (alloc_format & MALI_GRALLOC_INTFMT_AFBC_BASIC)
		{
			*width = GRALLOC_ALIGN(*width, 16);
			*height = GRALLOC_ALIGN(*height, 16);
		}
	}

out:
	ALOGV("%s: alloc_format=0x%" PRIx64 " usage=0x%" PRIx64
	      " alloc_width=%u, alloc_height=%u",
	      __FUNCTION__, alloc_format, usage, *width, *height);
}


/*
 * Obtain level of support for base format across all producers and consumers as
 * defined by IP support table. This support is defined for the most capable IP -
 * specific IP might have reduced support based on specific capabilities.
 *
 * @param producers      [in]    Producers (flags).
 * @param consumers      [in]    Consumers (flags).
 * @param format         [in]    Format entry in IP support table.
 *
 * @return format support (flags) comprising:
 *         F_NONE: None
 *         F_LIN: Uncompressed/linear
 *         F_AFBC: AFBC (via modifiers)
 */
static uint16_t ip_supports_base_format(const uint16_t producers,
                                        const uint16_t consumers,
                                        const format_ip_support_t * const format)
{
	uint32_t support = ~0;

	/* Determine producer support for base format. */
	if (producers & MALI_GRALLOC_PRODUCER_CPU)
	{
		support &= format->cpu_wr;
	}
	if (producers & MALI_GRALLOC_PRODUCER_GPU)
	{
		support &= format->gpu_wr;
	}
	if (producers & MALI_GRALLOC_PRODUCER_DPU)
	{
		support &= format->dpu_wr;
	}
	if (producers & MALI_GRALLOC_PRODUCER_DPU_AEU)
	{
		support &= format->dpu_aeu_wr;
	}
	if (producers & MALI_GRALLOC_PRODUCER_CAM)
	{
		/* There is no implementation for camera */
        //support &= format->cam_wr;
	}
	if (producers & MALI_GRALLOC_PRODUCER_VPU)
	{
		support &= format->vpu_wr;
	}

	/* Determine producer support for base format. */
	if (consumers & MALI_GRALLOC_CONSUMER_CPU)
	{
		support &= format->cpu_rd;
	}
	if (consumers & MALI_GRALLOC_CONSUMER_GPU)
	{
		support &= format->gpu_rd;
	}
	if (consumers & MALI_GRALLOC_CONSUMER_DPU)
	{
		support &= format->dpu_rd;
	}
	if (consumers & MALI_GRALLOC_CONSUMER_VPU)
	{
		support &= format->vpu_rd;
	}

	return support;
}


/*
 * Determines whether a base format is subsampled YUV, where each
 * chroma channel has fewer samples than the luma channel. The
 * sub-sampling is always a power of 2.
 *
 * @param base_format   [in]    Base format (internal).
 *
 * @return 1, where format is subsampled YUV;
 *         0, otherwise
 */
bool is_subsampled_yuv(const uint32_t base_format)
{
	unsigned long i;

	for (i = 0; i < num_formats; i++)
	{
		if (formats[i].id == (base_format & MALI_GRALLOC_INTFMT_FMT_MASK))
		{
			if (formats[i].is_yuv == true &&
			    (formats[i].hsub > 1 || formats[i].vsub > 1))
			{
				return true;
			}
		}
	}
	return false;
}


/*
 * Determines whether multi-plane AFBC (requires specific IP capabiltiies) is
 * supported across all producers and consumers.
 *
 * @param producers      [in]    Producers (flags).
 * @param consumers      [in]    Consumers (flags).
 * @param producer_caps  [in]    Producer capabilities (flags).
 * @param consumer_caps  [in]    Consumer capabilities (flags).
 *
 * @return 1, multiplane AFBC is supported
 *         0, otherwise
 */
static inline bool is_afbc_multiplane_supported(const uint16_t producers,
                                                const uint16_t consumers,
                                                const uint64_t producer_caps,
                                                const uint64_t consumer_caps)
{
	GRALLOC_UNUSED(consumers);

	return (producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
	        producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
	        producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_EXTRAWIDEBLK &&
	        /*no_producer*/ consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_MULTIPLANE_READ &&
	        producers == 0) ? true : false;
}


/*
 * Determines whether a given base format is supported by all producers and
 * consumers. After checking broad support across producer/consumer IP, this
 * function uses capabilities to disable features (base formats and AFBC
 * modifiers) that are not supported by specific versions of each IP.
 *
 * @param fmt_idx        [in]    Index into format properties table (base format).
 * @param ip_fmt_idx     [in]    Index into format IP support table (base format).
 * @param usage          [in]    Buffer usage.
 * @param producers      [in]    Producers (flags).
 * @param consumers      [in]    Consumers (flags).
 * @param producer_caps  [in]    Producer capabilities (flags).
 * @param consumer_caps  [in]    Consumer capabilities (flags).
 *
 * @return format support (flags) comprising:
 *         F_NONE: None
 *         F_LIN: Uncompressed/linear
 *         F_AFBC: AFBC (via modifiers)
 */
static uint8_t is_format_supported(const int32_t fmt_idx,
                                   const int32_t ip_fmt_idx,
                                   const uint64_t usage,
                                   const uint16_t producers,
                                   const uint16_t consumers,
                                   const uint64_t producer_caps,
                                   const uint64_t consumer_caps)
{
	/* Determine format support from table. */
	uint8_t f_flags = ip_supports_base_format(producers, consumers,
	                                          &formats_ip_support[ip_fmt_idx]);

	/* Determine whether producers/consumers support required AFBC features. */
	if (f_flags & F_AFBC)
	{
		if (!formats[fmt_idx].afbc ||
		    (producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC) == 0)
		{
			f_flags &= ~F_AFBC;
		}

		/* Check that multi-plane format supported by producers/consumers. */
		if (formats[fmt_idx].npln > 1 &&
		    !is_afbc_multiplane_supported(producers, consumers, producer_caps, consumer_caps))
		{
			f_flags &= ~F_AFBC;
		}

		/* Apply some additional restrictions from producer_caps and consumer_caps */
		/* Some modifiers affect base format support */
		if (formats[fmt_idx].is_yuv)
		{
			if ((producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_WRITE) == 0)
			{
				f_flags &= ~F_AFBC;
			}

			if ((consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_READ) == 0)
			{
				f_flags &= ~F_AFBC;
			}
		}

		if (usage & MALI_GRALLOC_USAGE_FRONTBUFFER)
		{
			if ((producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_DOUBLE_BODY) == 0)
			{
				f_flags &= ~F_AFBC;
			}
		}
	}

	if (f_flags != F_NONE)
	{
#if PLATFORM_SDK_VERSION >= 26
		if (formats[fmt_idx].id == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
		    (producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0)
		{
			f_flags = F_NONE;
		}

		if (formats[fmt_idx].id == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
		    (producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0)
		{
			f_flags = F_NONE;
		}
#endif
	}

	return f_flags;
}


/*
 * Ensures that the allocation format conforms to the AFBC specification and is
 * supported by producers and consumers. Format modifiers are (in most cases)
 * disabled as required to make valid. It is important to first resolve invalid
 * combinations which are not dependent upon others to reduce the possibility of
 * circular dependency.
 *
 * @param alloc_format          [in]    Allocation format (base + modifiers).
 * @param producer_active_caps  [in]    Producer capabilities (flags).
 * @param consumer_active_caps  [in]    Consumer capabilities (flags).
 *
 * @return valid alloc_format with AFBC possibly disabled (if required)
 */
static uint64_t validate_afbc_format(uint64_t alloc_format,
                                     const uint64_t producer_active_caps,
                                     const uint64_t consumer_active_caps)
{
	const uint32_t base_format = alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/*
	 * AFBC with tiled-headers must be enabled for AFBC front-buffer-safe allocations.
	 * NOTE: format selection algorithm will always try and enable AFBC with
	 * tiled-headers where supported by producer(s) and consumer(s).
	 */
	if (alloc_format & MALI_GRALLOC_INTFMT_AFBC_DOUBLE_BODY)
	{
		/*
		 * Disable (extra-) wide-block which is unsupported with front-buffer safe AFBC.
		 */
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_WIDEBLK;
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK;
	}

	/*
	 * AFBC specification: Split-block is not supported for
	 * subsampled formats (YUV) when wide-block is enabled.
	 */
	if (alloc_format & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK &&
	    alloc_format & MALI_GRALLOC_INTFMT_AFBC_SPLITBLK &&
	    is_subsampled_yuv(base_format))
	{
		/* Disable split-block instead of wide-block because because
		 * wide-block has greater impact on display performance.
		 */
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
	}

	/* AFBC specification: Split-block must be enabled for
	 * non-subsampled formats > 16 bpp, where wide-block is enabled.
	 */
	if (alloc_format & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK &&
	   (alloc_format & MALI_GRALLOC_INTFMT_AFBC_SPLITBLK) == 0 &&
	   !is_subsampled_yuv(base_format) &&
	   base_format != MALI_GRALLOC_FORMAT_INTERNAL_RGB_565)
	{
		/* Enable split-block if supported by producer(s) & consumer(s),
		 * otherwise disable wide-block.
		 */
		if (producer_active_caps & consumer_active_caps &
			MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK)
		{
			alloc_format |= MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
		}
		else
		{
			alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_WIDEBLK;
		}
	}

	/* Some RGB formats don't support split block. */
	if (base_format == MALI_GRALLOC_FORMAT_INTERNAL_RGB_565)
	{
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
	}

	/* Ensure that AFBC features are supported by producers/consumers. */
	if ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_BASIC) &&
	    (producer_active_caps & consumer_active_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC) == 0)
	{
		ALOGE("AFBC basic selected but not supported by producer/consumer. Disabling "
		       "MALI_GRALLOC_INTFMT_AFBC_BASIC");
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_BASIC;
	}

	if ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_SPLITBLK) &&
	    (producer_active_caps & consumer_active_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK) == 0)
	{
		ALOGE("AFBC split-block selected but not supported by producer/consumer. Disabling "
		       "MALI_GRALLOC_INTFMT_AFBC_SPLITBLK");
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
	}

	if ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK) &&
	    (producer_active_caps & consumer_active_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK) == 0)
	{
		ALOGE("AFBC wide-block selected but not supported by producer/consumer. Disabling "
		       "MALI_GRALLOC_INTFMT_AFBC_WIDEBLK");
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_WIDEBLK;
	}

	if ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS) &&
	    (producer_active_caps & consumer_active_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS) == 0)
	{
		ALOGE("AFBC tiled-headers selected but not supported by producer/consumer. Disabling "
		       "MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS");
		alloc_format &= ~MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
	}

	return alloc_format;
}


/*
 * Derives a valid AFBC format (via modifiers) for all producers and consumers.
 * Formats are validated after enabling the largest feature set supported (and
 * desirable) for the IP usage. Some format modifier combinations are not
 * compatible. See MALI_GRALLOC_INTFMT_* modifiers for more information.
 *
 * @param base_format    [in]    Base format (internal).
 * @param usage          [in]    Buffer usage.
 * @param producer       [in]    Buffer producers (write).
 * @param consumer       [in]    Buffer consumers (read).
 * @param producer_caps  [in]    Buffer producer capabilities (intersection).
 * @param consumer_caps  [in]    Buffer consumer capabilities (intersection).
 *
 * @return valid AFBC format, where modifiers are enabled (supported/preferred);
 *         base format without modifers, otherwise
 */
static uint64_t get_afbc_format(const uint32_t base_format,
                                const uint64_t usage,
                                const uint16_t producer,
                                const uint16_t consumer,
                                const uint64_t producer_caps,
                                const uint64_t consumer_caps)
{
	uint64_t alloc_format = base_format;

	/*
	 * Determine AFBC modifiers where capabilities are defined for all producers
	 * and consumers. NOTE: AFBC is not supported for video transcode (VPU --> VPU).
	 */
	if (producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT &&
	    ((producer & MALI_GRALLOC_PRODUCER_VPU) == 0 || (consumer & MALI_GRALLOC_CONSUMER_VPU) == 0))
	{
		if (producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
		    consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
		{
			alloc_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;

			const int format_idx = get_format_index(base_format);
			if (format_idx != -1)
			{
				if (formats[format_idx].yuv_transform == true)
				{
					alloc_format |= MALI_GRALLOC_INTFMT_AFBC_YUV_TRANSFORM;
				}
			}

			if (producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
			    consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
			{
				alloc_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;

				if (usage & MALI_GRALLOC_USAGE_FRONTBUFFER &&
				    producer_caps & consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_DOUBLE_BODY)
				{
					alloc_format |= MALI_GRALLOC_INTFMT_AFBC_DOUBLE_BODY;
				}
			}

			/*
			 * Specific producer/consumer combinations benefit from additional
			 * AFBC features (e.g. GPU --> DPU).
			 */
			if (producer & MALI_GRALLOC_PRODUCER_GPU && consumer & MALI_GRALLOC_CONSUMER_DPU)
			{
				if (producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK &&
				    consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK)
				{
					alloc_format |= MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
				}

				/*
				 * NOTE: assume that all AFBC layers are pre-rotated. 16x16 SB
				 * must be used with DPU consumer when rotation is required.
				 */
				if (producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK &&
				    consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK)
				{
					alloc_format |= MALI_GRALLOC_INTFMT_AFBC_WIDEBLK;
				}
			}
		}
	}

	alloc_format = validate_afbc_format(alloc_format, producer_caps, consumer_caps);

	return alloc_format;
}

/*
 * Obtains the 'active' capabilities (for producers/consumers) by applying
 * additional constraints to the capabilities declared for each IP. Some rules
 * are based on format, others specific to producer/consumer. This function must
 * be careful not to make any assumptions about the base format properties since
 * fallback might still occur. It is safe to use any properties which are common
 * across all compatible formats as defined by is_format_compatible().
 *
 * @param format                 [in]    Base format requested.
 * @param producers              [in]    Producers (flags).
 * @param consumers              [in]    Consumers (flags).
 * @param producer_active_caps   [out]   Active producer capabilities (flags).
 * @param consumer_active_caps   [out]   Active consumer capabilities (flags).
 * @param buffer_size            [in]    Buffer resolution (w x h, in pixels).
 *
 * @return none.
 */
static void get_active_caps(const format_info_t format,
                            const uint16_t producers,
                            const uint16_t consumers,
                            uint64_t * const producer_active_caps,
                            uint64_t * const consumer_active_caps,
                            const int buffer_size)
{
	const uint64_t producer_caps = (producer_active_caps) ? *producer_active_caps : 0;
	const uint64_t consumer_caps = (consumer_active_caps) ? *consumer_active_caps : 0;
	uint64_t producer_mask = ~0;
	uint64_t consumer_mask = ~0;

	if (format.is_yuv)
	{
		/* AFBC wide-block is not supported across IP for YUV formats. */
		producer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
		consumer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;

		if ((producer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_WRITE) == 0)
		{
			producer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		}
		else if (producers & MALI_GRALLOC_PRODUCER_GPU)
		{
			/* All GPUs that can write YUV AFBC can only do it in 16x16,
			 * optionally with tiled headers.
			 */
			producer_mask &=
				~(MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK |
				  MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK);
		}

		if ((consumer_caps & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_READ) == 0)
		{
			consumer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		}
	}

	if (producers & MALI_GRALLOC_PRODUCER_DPU_AEU ||
	    consumers & MALI_GRALLOC_CONSUMER_DPU)
	{
		/* DPU does not support split-block other than RGB(A) 24/32-bit */
		if (!format.is_rgb || format.bpp[0] < 24)
		{
			if (producers & MALI_GRALLOC_PRODUCER_DPU_AEU)
			{
				producer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
			}
			if (consumers & MALI_GRALLOC_CONSUMER_DPU)
			{
				consumer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
			}
		}
	}

	if (consumers & MALI_GRALLOC_CONSUMER_DPU)
	{
		bool afbc_allowed = false;

        GRALLOC_UNUSED(buffer_size);

#if MALI_DISPLAY_VERSION == 550 || MALI_DISPLAY_VERSION == 650
#if GRALLOC_DISP_W != 0 && GRALLOC_DISP_H != 0
#define GRALLOC_AFBC_MIN_SIZE 75
		/* Disable AFBC based on buffer dimensions */
		afbc_allowed = ((buffer_size * 100) / (GRALLOC_DISP_W * GRALLOC_DISP_H)) >= GRALLOC_AFBC_MIN_SIZE;
#else /* If display size is not valid then always allow AFBC */
		afbc_allowed = true;
#endif
#else /* For cetus, always allow AFBC */
		afbc_allowed = true;
#endif
		if (!afbc_allowed)
		{
			consumer_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		}
	}

	if (producer_active_caps)
	{
		*producer_active_caps &= producer_mask;
	}
	if (consumer_active_caps)
	{
		*consumer_active_caps &= consumer_mask;
	}
}


/*
 * Obtains support flags and modifiers for base format. Support flags are:
 * - F_NONE: None
 * - F_LIN: Uncompressed/linear
 * - F_AFBC: AFBC (via modifiers)
 *
 * @param base_format           [in]    Base format for which to deduce support.
 * @param usage                 [in]    Buffer usage.
 * @param producers             [in]    Producers (flags).
 * @param consumers             [in]    Consumers (flags).
 * @param producer_active_caps  [in]    Producer capabilities (flags).
 * @param consumer_active_caps  [in]    Consumer capabilities (flags).
 * @param fmt_supported         [out]   Format (base + modifiers) and support (flags).
 *
 * @return 1, base format supported
 *         0, otherwise
 */
bool get_supported_format(const uint32_t base_format,
                          const uint64_t usage,
                          const uint16_t producers,
                          const uint16_t consumers,
                          const uint64_t producer_active_caps,
                          const uint64_t consumer_active_caps,
                          fmt_props * const fmt_supported)
{
	const int32_t fmt_idx = get_format_index(base_format);
	const int32_t ip_fmt_idx = get_ip_format_index(base_format);
	assert(fmt_idx >= 0);
	if (ip_fmt_idx == -1)
	{
		/* Return undefined base format. */
		AERR("Failed to find IP support info for format id: 0x%" PRIx32,
		      base_format);
		return false;
	}

	fmt_supported->f_flags = is_format_supported(fmt_idx,
	                                             ip_fmt_idx,
	                                             usage,
	                                             producers,
	                                             consumers,
	                                             producer_active_caps,
	                                             consumer_active_caps);
	ALOGV("IP support: 0x%" PRIx16, fmt_supported->f_flags);
	if (fmt_supported->f_flags == F_NONE &&
	    consumers & MALI_GRALLOC_CONSUMER_GPU &&
	    consumers & MALI_GRALLOC_CONSUMER_DPU)
	{
		/* Determine alternative caps for formats when GPU/DPU consumer.
		 * Although we normally combine capabilities for multiple consumers with "AND",
		 * in some situations (e.g. formats) we make best effort and say that fallback
		 * to GPU is acceptable and perferred over rejecting allocation. GPU composition
		 * must always be supported in case of fallback from DPU.
		 */
		const uint16_t consumers_nodpu = consumers & ~MALI_GRALLOC_CONSUMER_DPU;
		uint64_t consumer_nodpu_caps = consumer_active_caps;

		/* Set consumer caps to GPU-only (assume superset of DPU). */
		consumer_nodpu_caps = get_consumer_caps(consumers_nodpu);
		get_active_caps(formats[fmt_idx],
		                producers, consumers_nodpu,
		                NULL, &consumer_nodpu_caps,
		                0 /* N/A without DPU consumer */);

		fmt_supported->f_flags = is_format_supported(fmt_idx,
	                                                 ip_fmt_idx,
		                                             usage,
	                                                 producers,
	                                                 consumers_nodpu,
	                                                 producer_active_caps,
	                                                 consumer_nodpu_caps);
	}

	fmt_supported->base_format = base_format;

	if (fmt_supported->f_flags & F_AFBC)
	{
		const uint64_t afbc_format = get_afbc_format(base_format,
	                                                 usage,
	                                                 producers,
	                                                 consumers,
	                                                 producer_active_caps,
	                                                 consumer_active_caps);

		ALOGV("AFBC format: 0x%" PRIx64, afbc_format);

		/* Disable AFBC when forced by usage or no format modifiers selected. */
		if ((usage & MALI_GRALLOC_USAGE_NO_AFBC) == MALI_GRALLOC_USAGE_NO_AFBC ||
		    afbc_format == fmt_supported->base_format)
		{
			fmt_supported->f_flags &= ~F_AFBC;
		}

		/* Check that AFBC features are correct for multiplane format. */
		alloc_type_t alloc_type;
		get_alloc_type(afbc_format & MALI_GRALLOC_INTFMT_EXT_MASK,
					   fmt_idx,
					   usage,
					   &alloc_type);
		if (formats[fmt_idx].npln > 1 && alloc_type.is_multi_plane == false)
		{
			fmt_supported->f_flags &= ~F_AFBC;
		}

		/* Store any format modifiers */
		fmt_supported->format_ext = afbc_format & MALI_GRALLOC_INTFMT_EXT_MASK;
	}

	if ((fmt_supported->f_flags & F_AFBC) == 0)
	{
		fmt_supported->format_ext = 0;
	}

	ALOGV("AFBC ext format: 0x%" PRIx64, fmt_supported->format_ext);

	return (fmt_supported->f_flags == F_NONE) ? false : true;
}


/*
 * Determines whether two base formats have comparable 'color' components. Alpha
 * is considered unimportant for YUV formats.
 *
 * @param f_old     [in]    Format properties (old format).
 * @param f_new     [in]    Format properties (new format).
 *
 * @return 1, format components are equivalent
 *         0, otherwise
 */
static bool comparable_components(const format_info_t * const f_old,
                                  const format_info_t * const f_new)
{
	if (f_old->is_yuv)
	{
		/* Formats have the same number of components. */
		if (f_new->ncmp == f_old->ncmp)
		{
			return true;
		}

		/* Alpha component can be dropped for yuv formats.
		 * This assumption is required for mapping Y0L2 to
		 * single plane 10-bit YUV420 AFBC.
		 */
		if (f_old->has_alpha)
		{
			if (f_new->ncmp == 3 &&
			    f_new->is_yuv &&
			    !f_new->has_alpha)
			{
				return true;
			}
		}
	}
	else if (f_old->is_rgb)
	{
		if (f_new->ncmp == f_old->ncmp &&
		    f_new->bpp[0] == f_old->bpp[0])
		{
			return true;
		}
	}
	else
	{
		if (f_new->id == f_old->id)
		{
			return true;
		}
	}

	return false;
}


/*
 * Determines whether two base formats are compatible such that data from one
 * format could be accurately represented/interpreted in the other format.
 *
 * @param f_old     [in]    Format properties (old format).
 * @param f_new     [in]    Format properties (new format).
 *
 * @return 1, formats are equivalent
 *         0, otherwise
 */
static bool is_format_compatible(const format_info_t * const f_old,
                                 const format_info_t * const f_new)
{
	if (f_new->bps == f_old->bps &&
	    f_new->hsub == f_old->hsub &&
	    f_new->vsub == f_old->vsub &&
	    f_new->is_rgb == f_old->is_rgb &&
	    f_new->is_yuv == f_old->is_yuv &&
	    comparable_components(f_old, f_new))
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*
 * Obtains the 'best' allocation format for requested format and usage:
 * 1. Find compatible base formats (based on format properties alone)
 * 2. Find base formats supported by producers/consumers
 * 3. Find best modifiers from supported base formats
 * 4. Select allocation format from "best" base format with "best" modifiers
 *
 * NOTE: Base format re-mapping should not take place when CPU usage is
 * requested.
 *
 * @param req_base_format       [in]    Base format requested by client.
 * @param usage                 [in]    Buffer usage.
 * @param producers             [in]    Producers (flags).
 * @param consumers             [in]    Consumers (flags).
 * @param producer_active_caps  [in]    Producer capabilities (flags).
 * @param consumer_active_caps  [in]    Consumer capabilities (flags).
 *
 * @return alloc_format, supported for usage;
 *         MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED, otherwise
 */
static uint64_t get_best_format(const uint32_t req_base_format,
                                const uint64_t usage,
                                const uint16_t producers,
                                const uint16_t consumers,
                                const uint64_t producer_active_caps,
                                const uint64_t consumer_active_caps)
{
	uint64_t alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;

	assert(req_base_format != MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED);
	const int32_t req_fmt_idx = get_format_index(req_base_format);
	ALOGV("req_base_format: 0x%" PRIx32, req_base_format);
	assert(req_fmt_idx >= 0);

	/* 1. Find compatible base formats. */
	std::vector<fmt_props> f_compat;
	for (uint16_t i = 0; i < num_formats; i++)
	{
		if (is_format_compatible(&formats[req_fmt_idx], &formats[i]))
		{
			fmt_props fmt = {0, 0, 0};
			fmt.base_format = formats[i].id;
			ALOGV("Compatible: Base-format: 0x%" PRIx32, fmt.base_format);
			f_compat.push_back(fmt);
		}
	}
	assert(f_compat.size() > 0);

	/* 2. Find base formats supported by IP and among them, find the highest
	 * number of modifier enabled format and check if requested format is present
	 */

	int32_t num_supported_formats = 0;
	int32_t req_format_num_extn_bits = -1;
	int32_t best_fmt_num_extn_bits = -1;
	uint64_t first_of_best_formats = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;
	uint64_t req_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;

	for (uint16_t i = 0; i < f_compat.size(); i++)
	{
		fmt_props fmt = {0, 0, 0};
		bool supported = get_supported_format(f_compat[i].base_format,
		                                      usage,
		                                      producers,
		                                      consumers,
		                                      producer_active_caps,
		                                      consumer_active_caps,
		                                      &fmt);
		if (supported)
		{
			num_supported_formats++;
			ALOGV("Supported: Base-format: 0x%" PRIx32 ", Modifiers: 0x%" PRIx64 ", Flags: 0x%" PRIx16,
			      fmt.base_format, fmt.format_ext, fmt.f_flags);

			/* Calculate the number of modifier bits enabled for a supported format
			 * so as to identify best (highest modifier bits enabled) format among them
			 */
			int32_t sup_fmt_num_extn_bits = 0;
			uint64_t format_ext = fmt.format_ext;
			for (; format_ext; ++sup_fmt_num_extn_bits)
			{
				format_ext &= format_ext - 1;
			}

			/* 3. Find best modifiers from supported base formats */
			if (sup_fmt_num_extn_bits > best_fmt_num_extn_bits)
			{
				best_fmt_num_extn_bits = sup_fmt_num_extn_bits;
				first_of_best_formats = fmt.base_format | fmt.format_ext;
			}

			/* Check if current supported format is same as requested format */
			if (fmt.base_format == req_base_format)
			{
				req_format_num_extn_bits = sup_fmt_num_extn_bits;
				req_format = fmt.base_format | fmt.format_ext;
			}
		}
	}

	/* 4. Select allocation format from "best" base format with "best" modifiers */
	if (num_supported_formats > 0)
	{
		/* Select first/one of best format when requested format is either not
		* supported or requested format is not the best format.
		*/
		if ((req_format_num_extn_bits != best_fmt_num_extn_bits) &&
			(((producers & MALI_GRALLOC_PRODUCER_CPU) == 0) &&
			((consumers & MALI_GRALLOC_CONSUMER_CPU) == 0)))
		{
			alloc_format = first_of_best_formats;
		}
		else if (req_format_num_extn_bits != -1)
		{
			alloc_format = req_format;
		}
	}

	ALOGV("Selected format: 0x%" PRIx64, alloc_format);
	return alloc_format;
}

/*
 * Determines whether multi-plane AFBC is supported with a set of format
 * modifiers. This function does not consider producers and consumers.
 *
 * @param format_ext     [in]    Format modifiers (extension bits).
 *
 * @return 1, multiplane AFBC is supported
 *         0, otherwise
 */
static inline bool is_multiplane_enabled(const uint64_t format_ext)
{
	const uint64_t modifiers = format_ext & MALI_GRALLOC_INTFMT_EXT_MASK;

	return ((modifiers & MALI_GRALLOC_INTFMT_AFBC_BASIC) &&
	        (modifiers & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS) &&
	        (modifiers & MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK)) ? true : false;
}

/*
 * Determines the base format suitable for requested allocation format (base +
 * modifiers). Going forward, the base format requested MUST be compatible with
 * the format modifiers. In legacy mode, more leeway is given such that fallback
 * to a supported base format for multi-plane AFBC formats is handled here
 * within the gralloc implementation.
 *
 * @param fmt_idx        [in]    Index into format properties table (base format).
 * @param format_ext     [in]    Format modifiers (extension bits).
 *
 * @return base_format, suitable for modifiers;
 *         MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED, otherwise
 */
static uint32_t get_base_format_for_modifiers(const int32_t fmt_idx,
                                              const uint64_t format_ext)
{
	uint32_t base_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;

	if (format_ext == 0)
	{
		/* Uncompressed formats have no forced fallback. */
		base_format = formats[fmt_idx].id;
	}
	else if (formats[fmt_idx].afbc &&
	         (formats[fmt_idx].npln == 1 || is_multiplane_enabled(format_ext)))
	{
		/* Requested format modifiers are suitable for base format. */
		base_format = formats[fmt_idx].id;
	}
#if GRALLOC_USE_LEGACY_CALCS == 1
	else
	{
		/* For legacy clients *only*, allow fall-back to 'compatible' base format.
		 * Multi-plane AFBC format requeset would not be intentional and therefore
		 * fallback to single-plane should happen automatically internally.
		 */
		for (uint16_t i = 0; i < num_formats; i++)
		{
			if (is_format_compatible(&formats[fmt_idx], &formats[i]))
			{
				if (formats[i].afbc &&
				    (formats[i].npln == 1 || is_multiplane_enabled(format_ext)))
				{
					base_format = formats[i].id;
					break;
				}
			}
		}
	}
#endif

	return base_format;
}


/*
 * Obtain format modifiers from requested format.
 *
 * @param req_format       [in]    Requested format (base + optional modifiers).
 * @param usage            [in]    Buffer usage.
 *
 * @return format modifiers, where extracted from requested format;
 *         0, otherwise
 */
uint64_t get_format_ext(const uint64_t req_format, const uint64_t usage)
{
	uint64_t format_ext = 0;

	if (usage & MALI_GRALLOC_USAGE_PRIVATE_FORMAT)
	{
		format_ext = (req_format & MALI_GRALLOC_INTFMT_EXT_WRAP_MASK) << MALI_GRALLOC_INTFMT_EXT_WRAP_SHIFT;
	}
	else
	{
		format_ext = req_format & MALI_GRALLOC_INTFMT_EXT_MASK;
	}

	return format_ext;
}


/*
 * Obtain base format from requested format. There are two primary ways in which
 * the client can specify requested format:
 * - Public API:
 *   - Normal usage, with HAL_PIXEL_FORMAT_* / MALI_GRALLOC_FORMAT_INTERNAL_*
 *   - Private usage, (as normal usage) with additional format modifiers (MALI_GRALLOC_INTFMT_*)
 * - Private API: allows private usage to be provided explicitly
 *                (type == MALI_GRALLOC_FORMAT_TYPE_INTERNAL)
 *
 * @param req_format       [in]    Requested format (base + optional modifiers).
 * @param usage            [in]    Buffer usage.
 * @param type             [in]    Format type (public usage or internal).
 * @param map_to_internal  [in]    Base format (if public HAL_PIXEL_FORMAT_*)
 *                                 should be mapped to internal representation.
 *
 * @return base format, where identified/translated from requested format;
 *         MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED, otherwise
 */
uint32_t get_base_format(const uint64_t req_format,
                         const uint64_t usage,
                         const mali_gralloc_format_type type,
                         const bool map_to_internal)
{
	GRALLOC_UNUSED(type);

	uint32_t base_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;

	if (usage & MALI_GRALLOC_USAGE_PRIVATE_FORMAT)
	{
		base_format = req_format & MALI_GRALLOC_INTFMT_FMT_WRAP_MASK;

		if (base_format == MALI_GRALLOC_FORMAT_INTERNAL_YV12_WRAP)
		{
			base_format = MALI_GRALLOC_FORMAT_INTERNAL_YV12;
		}
		else if (base_format == MALI_GRALLOC_FORMAT_INTERNAL_Y8_WRAP)
		{
			base_format = MALI_GRALLOC_FORMAT_INTERNAL_Y8;
		}
		else if (base_format == MALI_GRALLOC_FORMAT_INTERNAL_Y16_WRAP)
		{
			base_format = MALI_GRALLOC_FORMAT_INTERNAL_Y16;
		}
	}
	else
	{
		/*
		 * Internal format (NV12) overlaps with HAL format (JPEG). To disambiguate,
		 * reject HAL_PIXEL_FORMAT_JPEG when provided through the public interface.
		 * All formats requested through private interface (type ==
		 * MALI_GRALLOC_FORMAT_TYPE_INTERNAL) should be accepted, including
		 * MALI_GRALLOC_FORMAT_INTERNAL_NV12 (same value as HAL_PIXEL_FORMAT_JPEG).
		 */
#if PLATFORM_SDK_VERSION >= 26 && PLATFORM_SDK_VERSION < 28
		if (type == MALI_GRALLOC_FORMAT_TYPE_INTERNAL || req_format != HAL_PIXEL_FORMAT_JPEG)
#endif
		{
			/* Mask out extension bits which could be present with type 'internal'. */
			base_format = req_format & MALI_GRALLOC_INTFMT_FMT_MASK;
		}
	}

	if ((usage & GRALLOC_USAGE_AFBC_MODE)
            && ((usage & MALI_GRALLOC_USAGE_NO_AFBC) != MALI_GRALLOC_USAGE_NO_AFBC)) {
		/* sunxi afbc gpu */
		return MALI_GRALLOC_FORMAT_INTERNAL_AW_WRAP;
	}
	/* Obtain a valid base format, optionally mapped to internal. Flex formats
	 * are always mapped to internal base format.
	 * NOTE: Overlap between HAL_PIXEL_FORMAT_* and MALI_GRALLOC_FORMAT_INTERNAL_*
	 * is intentional. See enumerations for more information.
	 */
	return get_internal_format(base_format, map_to_internal);
}


/*
 * Select pixel format (base + modifier) for allocation.
 *
 * @param req_format       [in]   Format (base + optional modifiers) requested by client.
 * @param type             [in]   Format type (public usage or internal).
 * @param usage            [in]   Buffer usage.
 * @param buffer_size      [in]   Buffer resolution (w x h, in pixels).
 * @param internal_format  [out]  Legacy format (base format as requested).
 *
 * @return alloc_format, format to be used in allocation;
 *         MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED, where no suitable
 *         format could be found.
 */
uint64_t mali_gralloc_select_format(const uint64_t req_format,
                                    const mali_gralloc_format_type type,
                                    const uint64_t usage,
                                    const int buffer_size,
                                    uint64_t * const internal_format)
{
	uint64_t alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;

	/*
	 * Obtain base_format (no extension bits) and indexes into format tables.
	 */
	const uint32_t req_base_format = get_base_format(req_format, usage, type, true);
	const int32_t req_fmt_idx = get_format_index(req_base_format);
	if (req_base_format == MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED ||
	    req_fmt_idx == -1)
	{
		ALOGW("Invalid base format! req_base_format = 0x%" PRIx32
		      ", req_format = 0x%" PRIx64 ", type = 0x%" PRIx32,
		      req_base_format, req_format, type);
		goto out;
	}
	if (req_base_format == MALI_GRALLOC_FORMAT_INTERNAL_AW_WRAP) {
		ALOGV("gralloc alloc buffer for sunxi format...");
		alloc_format = req_base_format;
		goto out;
	}
	/* Reject if usage specified is outside white list of valid usages. */
	if (type != MALI_GRALLOC_FORMAT_TYPE_INTERNAL && (usage & (~VALID_USAGE)) != 0)
	{
		ALOGE("Invalid usage specified: 0x%" PRIx64, usage);
		goto out;
	}

	/*
	 * Construct format as requested (using AFBC modifiers) ensuring that
	 * base format is compatible with modifiers. Otherwise, reject allocation with UNDEFINED.
	 * NOTE: IP support is not considered and modifiers are not adjusted.
	 */
	if (usage & MALI_GRALLOC_USAGE_PRIVATE_FORMAT || type == MALI_GRALLOC_FORMAT_TYPE_INTERNAL)
	{
		const uint64_t format_ext = get_format_ext(req_format, usage);
		const uint32_t base_format = get_base_format_for_modifiers(req_fmt_idx, format_ext);
		if (base_format != MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED)
		{
			alloc_format = format_ext | base_format;
		}
	}
	else
	{
		/* Determine producers and consumers. */
		const uint16_t producers = get_producers(usage);
		const uint16_t consumers = get_consumers(usage);

		ALOGV("Producers: 0x%" PRIx16 ", Consumers: 0x%" PRIx16,
		      producers, consumers);

		/* Obtain producer and consumer capabilities. */
		const uint64_t producer_caps = get_producer_caps(producers);
		const uint64_t consumer_caps = get_consumer_caps(consumers);

		ALOGV("Producer caps: 0x%" PRIx64 ", Consumer caps: 0x%" PRIx64,
		      producer_caps, consumer_caps);

		if (producers == 0 && consumers == 0)
		{
			ALOGD("Producer and consumer not identified.");
			goto out;
		}
		else if (producers == 0 || consumers == 0)
		{
			ALOGV("Producer or consumer not identified.");
		}

		uint64_t producer_active_caps = producer_caps;
		uint64_t consumer_active_caps = consumer_caps;
		uint64_t consumer_caps_mask;

		get_active_caps(formats[req_fmt_idx],
		                producers, consumers,
		                &producer_active_caps, &consumer_active_caps,
		                buffer_size);

		ALOGV("Producer caps (active): 0x%" PRIx64 ", Consumer caps (active): 0x%" PRIx64,
		      producer_active_caps, consumer_active_caps);

		alloc_format = get_best_format(formats[req_fmt_idx].id,
		                               usage,
		                               producers,
		                               consumers,
		                               producer_active_caps,
		                               consumer_active_caps);
	}

out:
	/*
	 * Reconstruct internal format (legacy).
	 * In order to retain backwards-compatiblity, private_handle_t member,
	 * 'internal_format' will *not* be updated with single-plane format. Clients with
	 * support for multi-plane AFBC should use a combination of 'internal_format' and
	 * 'is_multi_plane()'' to determine whether the allocated format is multi-plane.
	 */
	if (alloc_format == MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED)
	{
		*internal_format = MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED;
	}
	else
	{
		*internal_format = get_base_format(req_format, usage, type, false);
		*internal_format |= (alloc_format & MALI_GRALLOC_INTFMT_EXT_MASK);
	}

	ALOGV("mali_gralloc_select_format: req_format=0x%08" PRIx64 ", usage=0x%" PRIx64
	      ", req_base_format=0x%" PRIx32 ", alloc_format=0x%" PRIx64 ", internal_format=0x%" PRIx64,
	      req_format, usage, req_base_format, alloc_format, *internal_format);

	return alloc_format;
}

