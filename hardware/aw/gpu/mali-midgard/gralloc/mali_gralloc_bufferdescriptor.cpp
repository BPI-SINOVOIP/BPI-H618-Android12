/*
 * Copyright (C) 2016-2017 ARM Limited. All rights reserved.
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

#include <hardware/hardware.h>
#include <stdlib.h>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"

#if GRALLOC_USE_GRALLOC1_API == 1
int mali_gralloc_create_descriptor_internal(gralloc1_buffer_descriptor_t *outDescriptor)
{
	buffer_descriptor_t *buffer_descriptor;

	if (NULL == outDescriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor = reinterpret_cast<buffer_descriptor_t *>(malloc(sizeof(buffer_descriptor_t)));

	if (NULL == buffer_descriptor)
	{
		AERR("failed to create buffer descriptor");
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	*outDescriptor = (gralloc1_buffer_descriptor_t)buffer_descriptor;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_destroy_descriptor_internal(gralloc1_buffer_descriptor_t descriptor)
{
	if (!descriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	free(buffer_descriptor);
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_dimensions_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t width, uint32_t height)
{
	if (!descriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	buffer_descriptor->width = width;
	buffer_descriptor->height = height;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_format_internal(gralloc1_buffer_descriptor_t descriptor, int32_t format)
{
	if (!descriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	buffer_descriptor->hal_format = format;
	buffer_descriptor->format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_producerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
	if (!descriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
	buffer_descriptor->producer_usage = usage;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_set_consumerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
	if (!descriptor)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	buffer_descriptor_t *buffer_descriptor = (buffer_descriptor_t *)descriptor;
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

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outWidth = hnd->width;
	*outHeight = hnd->height;
	return GRALLOC1_ERROR_NONE;
}

int mali_gralloc_get_format_internal(buffer_handle_t buffer, int32_t *outFormat)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
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

	private_handle_t *hnd = (private_handle_t *)buffer;
	*outUsage = hnd->producer_usage;
	return GRALLOC1_ERROR_NONE;
}

#endif
int mali_gralloc_query_getstride(buffer_handle_t buffer, int *pixelStride)
{
	int rval = -1;

	if (buffer != NULL && pixelStride != NULL)
	{
		private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(buffer);

		if (hnd)
		{
			*pixelStride = hnd->stride;
			rval = 0;
		}
	}

	return rval;
}
