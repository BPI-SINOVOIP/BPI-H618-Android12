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

#ifndef MALI_GRALLOC_BUFFERDESCRIPTOR_H_
#define MALI_GRALLOC_BUFFERDESCRIPTOR_H_

#include <hardware/hardware.h>
#include "gralloc_priv.h"
#include "mali_gralloc_module.h"
#include "mali_gralloc_formats.h"

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

typedef uint64_t gralloc_buffer_descriptor_t;

typedef struct buffer_descriptor
{
	uint32_t signature;

	uint32_t width;
	uint32_t height;
	uint64_t producer_usage;
	uint64_t consumer_usage;
	uint64_t hal_format;
	uint32_t layer_count;

	mali_gralloc_format_type format_type;
	size_t size;
	int pixel_stride;
	int old_byte_stride;
	int old_alloc_width;
	int old_alloc_height;
	uint64_t internal_format;
	uint64_t alloc_format;
	plane_info_t plane_info[MAX_PLANES];

#if NEW_AW_ALIGN != 1
	int aw_byte_align[MAX_PLANES];
#endif
	int aw_format;
	unsigned int ion_metadata_flag;

#ifdef __cplusplus
	buffer_descriptor() :
	    signature(0),
	    width(0),
	    height(0),
	    producer_usage(0),
	    consumer_usage(0),
	    hal_format(0),
	    layer_count(0),
	    format_type(MALI_GRALLOC_FORMAT_TYPE_USAGE),
	    size(0),
	    pixel_stride(0),
	    old_byte_stride(0),
	    old_alloc_width(0),
	    old_alloc_height(0),
	    internal_format(0),
	    alloc_format(0),
	    aw_format(0),
	    ion_metadata_flag(0)
	{
		memset(plane_info, 0, sizeof(plane_info_t) * MAX_PLANES);
#if NEW_AW_ALIGN != 1
		memset(aw_byte_align, 0, sizeof(int) * MAX_PLANES);
#endif

	}
#endif
} buffer_descriptor_t;

#if GRALLOC_VERSION_MAJOR == 1
int mali_gralloc_create_descriptor_internal(gralloc1_buffer_descriptor_t *outDescriptor);
int mali_gralloc_destroy_descriptor_internal(gralloc1_buffer_descriptor_t descriptor);
int mali_gralloc_set_dimensions_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t width, uint32_t height);
int mali_gralloc_set_format_internal(gralloc1_buffer_descriptor_t descriptor, int32_t format);
int mali_gralloc_set_producerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage);
int mali_gralloc_set_consumerusage_internal(gralloc1_buffer_descriptor_t descriptor, uint64_t usage);

int mali_gralloc_get_backing_store_internal(buffer_handle_t buffer, gralloc1_backing_store_t *outStore);
int mali_gralloc_get_consumer_usage_internal(buffer_handle_t buffer, uint64_t *outUsage);
int mali_gralloc_get_dimensions_internal(buffer_handle_t buffer, uint32_t *outWidth, uint32_t *outHeight);
int mali_gralloc_get_format_internal(buffer_handle_t buffer, int32_t *outFormat);
int mali_gralloc_get_producer_usage_internal(buffer_handle_t buffer, uint64_t *outUsage);
#if PLATFORM_SDK_VERSION >= 26
int mali_gralloc_set_layer_count_internal(gralloc1_buffer_descriptor_t descriptor, uint32_t layerCount);
int mali_gralloc_get_layer_count_internal(buffer_handle_t buffer, uint32_t *outLayerCount);
#endif
#endif
int mali_gralloc_query_getstride(buffer_handle_t handle, int *pixelStride);
int mali_gralloc_query_get_bytes_per_pixel(buffer_handle_t buffer, int bytesPerPixel[]);
int mali_gralloc_query_get_byte_stride(buffer_handle_t buffer, int byteStride[]);

#endif /* MALI_GRALLOC_BUFFERDESCRIPTOR_H_ */
