/*
 * Copyright (C) 2016-2018 ARM Limited. All rights reserved.
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

#include <stdlib.h>
#include <inttypes.h>

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "format_info.h"

/*
 * Validate descriptor to ensure that it originated from this version
 * of gralloc. Rudimentary signature is simply calculated from size of
 * buffer descriptor structure and initialised immediately after
 * structure is allocated.
 *
 * @param buffer_descriptor    [in]    Buffer descriptor.
 *
 * @return true, for valid buffer descriptor;
 *         false, otherwise
 */
static bool descriptor_is_valid(buffer_descriptor_t *buffer_descriptor)
{
	if (buffer_descriptor && buffer_descriptor->signature == sizeof(*buffer_descriptor))
	{
		return true;
	}

	return false;
}

#if GRALLOC_VERSION_MAJOR == 1
int mali_gralloc_create_descriptor_internal(gralloc1_buffer_descriptor_t *outDescriptor)
{
	buffer_descriptor_t *buffer_descriptor;

	if (NULL == outDescriptor)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	buffer_descriptor = reinterpret_cast<buffer_descriptor_t *>(malloc(sizeof(buffer_descriptor_t)));

	if (NULL == buffer_descriptor)
	{
		AERR("failed to create buffer descriptor");
		return GRALLOC1_ERROR_NO_RESOURCES;
	}

	/*
	 * Initialise the buffer descriptor.
	 *
	 * Layer count is initialised to a single layer in
	 * case clients don't support multi-layer or use
	 * function GRALLOC1_PFN_SET_LAYER_COUNT.
	 */
	memset((void *)buffer_descriptor, 0, sizeof(*buffer_descriptor));
	buffer_descriptor->layer_count = 1;
	buffer_descriptor->signature = sizeof(buffer_descriptor_t);

	*outDescriptor = (gralloc1_buffer_descriptor_t)buffer_descriptor;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_destroy_descriptor_internal(gralloc1_buffer_descriptor_t descriptor)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	free(buffer_descriptor);
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_dimensions_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t width, uint32_t height)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor->width = width;
	buffer_descriptor->height = height;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_format_internal(gralloc1_buffer_descriptor_t descriptor, int32_t format)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	/*
	 * Explicit cast to unsigned 64-bit 'hal_format',
	 * which can also store internal_format provided
	 * through mali_gralloc_private_set_priv_fmt().
	 */
	buffer_descriptor->hal_format = (uint64_t)format;
	buffer_descriptor->format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_producerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor->producer_usage = usage;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_consumerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor->consumer_usage = usage;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_backing_store_internal(buffer_handle_t buffer, gralloc1_backing_store_t *outStore)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outStore == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;

	*outStore = (gralloc1_backing_store_t)hnd->backing_store_id;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_consumer_usage_internal(buffer_handle_t buffer, uint64_t *outUsage)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outUsage == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outUsage = hnd->consumer_usage;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_dimensions_internal(buffer_handle_t buffer, uint32_t *outWidth, uint32_t *outHeight)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outWidth == NULL || outHeight == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outWidth = (uint32_t)hnd->width;
	*outHeight = (uint32_t)hnd->height;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_format_internal(buffer_handle_t buffer, int32_t *outFormat)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outFormat == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outFormat = hnd->req_format;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_producer_usage_internal(buffer_handle_t buffer, uint64_t *outUsage)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outUsage == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outUsage = hnd->producer_usage;
	return GRALLOC1_ERROR_NONE;
}

#if PLATFORM_SDK_VERSION >= 26
int mali_gralloc_set_layer_count_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t layerCount)
{
	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	if (!descriptor_is_valid(buffer_descriptor))
	{
		AERR("Invalid buffer descriptor %p", buffer_descriptor);
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	if (layerCount == 0)
	{
		AERR("Invalid layer count: %" PRIu32, layerCount);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	buffer_descriptor->layer_count = layerCount;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_layer_count_internal(buffer_handle_t buffer, uint32_t *outLayerCount)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outLayerCount == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outLayerCount = hnd->layer_count;
	return GRALLOC1_ERROR_NONE;
}
#endif /* PLATFORM_SDK_VERSION >= 26 */

#endif
int mali_gralloc_query_getstride(buffer_handle_t buffer, int *pixelStride)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_HANDLE;
#else
		return -EINVAL;
#endif
	}

	if (pixelStride == NULL)
	{
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_VALUE;
#else
		return -EINVAL;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	*pixelStride = hnd->stride;

#if GRALLOC_VERSION_MAJOR == 1
	return GRALLOC1_ERROR_NONE;
#else
	return 0;
#endif
}

int mali_gralloc_query_get_byte_stride(buffer_handle_t buffer, int byteStride[])
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_HANDLE;
#else
		return -EINVAL;
#endif
	}

	if (byteStride == NULL)
	{
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_VALUE;
#else
		return -EINVAL;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	byteStride[0] = hnd->plane_info[0].byte_stride;
	byteStride[1] = hnd->plane_info[1].byte_stride;
	byteStride[2] = hnd->plane_info[2].byte_stride;

#if GRALLOC_VERSION_MAJOR == 1
	return GRALLOC1_ERROR_NONE;
#else
	return 0;
#endif
}

int mali_gralloc_query_get_bytes_per_pixel(buffer_handle_t buffer, int bytesPerPixel[])
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_HANDLE;
#else
		return -EINVAL;
#endif
	}

	if (bytesPerPixel == NULL)
	{
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_VALUE;
#else
		return -EINVAL;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	const int32_t format_idx = get_format_index(hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
	if (format_idx == -1)
	{
		AERR("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
#if GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_BAD_VALUE;
#else
		return -EINVAL;
#endif
	}

	bytesPerPixel[0] = formats[format_idx].bpp[0] / 8;
	bytesPerPixel[1] = formats[format_idx].bpp[1] / 8;
	bytesPerPixel[2] = formats[format_idx].bpp[2] / 8;

#if GRALLOC_VERSION_MAJOR == 1
	return GRALLOC1_ERROR_NONE;
#else
	return 0;
#endif
}
