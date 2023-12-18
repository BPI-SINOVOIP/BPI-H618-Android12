/*
 * Copyright (C) 2016, 2018-2019 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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
#include <inttypes.h>
#include <hardware/hardware.h>
#include <hardware/gralloc1.h>

#include "mali_gralloc_module.h"

#include "mali_gralloc_private_interface.h"
#include "mali_gralloc_buffer.h"
#include "mali_gralloc_ion.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_reference.h"
#include "mali_gralloc_bufferaccess.h"
#include "framebuffer_device.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_debug.h"
#include "format_info.h"
#include <hardware/graphics-sunxi.h>

typedef struct mali_gralloc_func
{
	gralloc1_function_descriptor_t desc;
	gralloc1_function_pointer_t func;
} mali_gralloc_func;

static void mali_gralloc_dump(gralloc1_device_t *device, uint32_t *outSize, char *outBuffer)
{
	if (NULL == outSize)
	{
		ALOGE("Invalid pointer to outSize and return");
		return;
	}

	mali_gralloc_dump_internal(outSize, outBuffer);
	GRALLOC_UNUSED(device);
}

static int32_t mali_gralloc_create_descriptor(gralloc1_device_t *device, gralloc1_buffer_descriptor_t *outDescriptor)
{
	int ret = 0;
	ret = mali_gralloc_create_descriptor_internal(outDescriptor);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_destroy_descriptor(gralloc1_device_t *device, gralloc1_buffer_descriptor_t descriptor)
{
	int ret = 0;
	ret = mali_gralloc_destroy_descriptor_internal(descriptor);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_consumer_usage(gralloc1_device_t *device, gralloc1_buffer_descriptor_t descriptor,
                                               /*uint64_t */ gralloc1_consumer_usage_t usage)
{
	int ret = 0;
	ret = mali_gralloc_set_consumerusage_internal(descriptor, usage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_dimensions(gralloc1_device_t *device, gralloc1_buffer_descriptor_t descriptor,
                                           uint32_t width, uint32_t height)
{
	int ret = 0;
	ret = mali_gralloc_set_dimensions_internal(descriptor, width, height);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_format(gralloc1_device_t *device, gralloc1_buffer_descriptor_t descriptor,
                                       /*int32_t*/ android_pixel_format_t format)
{
	int ret = 0;
	ret = mali_gralloc_set_format_internal(descriptor, format);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_producer_usage(gralloc1_device_t *device, gralloc1_buffer_descriptor_t descriptor,
                                               /*uint64_t */ gralloc1_producer_usage_t usage)
{
	int ret = 0;
	ret = mali_gralloc_set_producerusage_internal(descriptor, usage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_backing_store(gralloc1_device_t *device, buffer_handle_t buffer,
                                              gralloc1_backing_store_t *outStore)
{
	int ret = 0;
	ret = mali_gralloc_get_backing_store_internal(buffer, outStore);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_consumer_usage(gralloc1_device_t *device, buffer_handle_t buffer,
                                               uint64_t * /*gralloc1_consumer_usage_t*/ outUsage)
{
	int ret = 0;
	ret = mali_gralloc_get_consumer_usage_internal(buffer, outUsage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_dimensions(gralloc1_device_t *device, buffer_handle_t buffer, uint32_t *outWidth,
                                           uint32_t *outHeight)
{
	int ret = 0;
	ret = mali_gralloc_get_dimensions_internal(buffer, outWidth, outHeight);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_format(gralloc1_device_t *device, buffer_handle_t buffer, int32_t *outFormat)
{
	int ret = 0;
	ret = mali_gralloc_get_format_internal(buffer, outFormat);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_producer_usage(gralloc1_device_t *device, buffer_handle_t buffer,
                                               uint64_t * /*gralloc1_producer_usage_t*/ outUsage)
{
	int ret = 0;
	ret = mali_gralloc_get_producer_usage_internal(buffer, outUsage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_stride(gralloc1_device_t *device, buffer_handle_t buffer, uint32_t *outStride)
{
	GRALLOC_UNUSED(device);
	int stride;

	const int ret = mali_gralloc_query_getstride(buffer, &stride);
	if (ret == GRALLOC1_ERROR_NONE)
	{
		*outStride = (uint32_t)stride;
	}

	return ret;
}

int32_t mali_gralloc_allocate(gralloc1_device_t *device, uint32_t numDescriptors,
                              const gralloc1_buffer_descriptor_t *descriptors, buffer_handle_t *outBuffers)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);
	buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(*descriptors);
	uint64_t usage;
	bool shared = false;

	/* Initialise output parameters. */
	for (uint32_t i = 0; i < numDescriptors; i++)
	{
		outBuffers[i] = NULL;
	}

	usage = bufDescriptor->producer_usage | bufDescriptor->consumer_usage;

#if DISABLE_FRAMEBUFFER_HAL != 1

	if (usage & GRALLOC_USAGE_HW_FB)
	{
		if (mali_gralloc_fb_allocate(m, bufDescriptor, outBuffers) < 0)
		{
			return GRALLOC1_ERROR_NO_RESOURCES;
		}
	}
	else
#endif
	{
		if (mali_gralloc_buffer_allocate(m, (gralloc_buffer_descriptor_t *)descriptors, numDescriptors, outBuffers,
		                                 &shared) < 0)
		{
			ALOGE("Failed to allocate buffer.");
			return GRALLOC1_ERROR_NO_RESOURCES;
		}

		if (!shared && 1 != numDescriptors)
		{
			return GRALLOC1_ERROR_NOT_SHARED;
		}
	}

	return GRALLOC1_ERROR_NONE;
}


static int32_t mali_gralloc_retain(gralloc1_device_t *device, buffer_handle_t buffer)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (mali_gralloc_reference_retain(m, buffer) < 0)
	{
		return GRALLOC1_ERROR_NO_RESOURCES;
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_release(gralloc1_device_t *device, buffer_handle_t buffer)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (mali_gralloc_reference_release(m, buffer, true) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	return GRALLOC1_ERROR_NONE;
}

/*
 *  Returns the number of flex layout planes which are needed to represent the
 *  given buffer.
 *
 * @param device       [in]   Gralloc device.
 * @param buffer       [in]   The buffer handle for which the number of planes should be queried
 * @param outNumPlanes [out]  The number of flex planes required to describe the given buffer
 *
 * @return GRALLOC1_ERROR_NONE - The locking is successful;
 *         GRALLOC1_ERROR_BAD_HANDLE - The buffer handle is invalid
 *         GRALLOC1_ERROR_BAD_VALUE - The flex plane pointer is invalid
 *         Appropriate error, otherwise
 */
static int32_t mali_gralloc1_get_num_flex_planes(const gralloc1_device_t * const device,
                                                 const buffer_handle_t buffer,
                                                 uint32_t * const outNumPlanes)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer handle");
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outNumPlanes == NULL)
	{
		AERR("Invalid flex plane pointer");
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	int status = mali_gralloc_get_num_flex_planes(m, buffer, outNumPlanes);
	if (status != 0)
	{
		return status;
	}

	return GRALLOC1_ERROR_NONE;
}

/*
 *  Locks the Gralloc 1.0 buffer for the specified CPU usage.
 *
 * @param device        [in]    Gralloc1 device.
 * @param buffer        [in]    The buffer to lock.
 * @param producerUsage [in]    The producer usage flags to request (gralloc1_producer_usage_t).
 * @param consumerUsage [in]    The consumer usage flags to request (gralloc1_consumer_usage_t).
 * @param accessRegion  [in]    The portion of the buffer that the client
 *                              intends to access.
 * @param outData       [out]   To be filled with a CPU-accessible pointer to
 *                              the buffer data for CPU usage.
 * @param acquireFence  [in]    Refers to an acquire sync fence object.
 *
 * @return GRALLOC1_ERROR_NONE, when locking is successful;
 *         appropriate error, otherwise
 *
 * @Notes: Locking a buffer simultaneously for write or read/write leaves the
 *         buffer's content into an indeterminate state.
 */
static int32_t mali_gralloc1_lock_async(gralloc1_device_t *device,
                                        buffer_handle_t buffer,
                                        uint64_t producerUsage,
                                        uint64_t consumerUsage,
                                        const gralloc1_rect_t *accessRegion,
                                        void **outData,
                                        int32_t acquireFence)
{
	int status = 0;
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (accessRegion == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	/* The lock usage constraints
	 * 1. Exactly one of producerUsage and consumerUsage must be *_USAGE_NONE.
	 * 2. The usage which is not *_USAGE_NONE must be one of the *_USAGE_CPU_*
	 *    as applicable.
	 * are relaxed to accommodate an issue in shims with regard to lock usage mapping.
	 */
	if (!((producerUsage | consumerUsage) & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)))
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	status = mali_gralloc_lock_async(m, buffer, producerUsage | consumerUsage,
	                                 accessRegion->left, accessRegion->top,
	                                 accessRegion->width, accessRegion->height,
	                                 outData, acquireFence);
	if (status != 0)
	{
		if (status == -EINVAL)
		{
			return GRALLOC1_ERROR_BAD_VALUE;
		}

		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	return GRALLOC1_ERROR_NONE;
}

/*
 *  Locks the Gralloc 1.0 buffer, for the specified CPU usage, asynchronously.
 *
 * @param device        [in]    Gralloc1 device.
 * @param buffer        [in]    The buffer to lock.
 * @param producerUsage [in]    The producer usage flags requested (gralloc1_producer_usage_t).
 * @param consumerUsage [in]    The consumer usage flags requested (gralloc1_consumer_usage_t).
 * @param accessRegion  [in]    The portion of the buffer that the client
 *                              intends to access.
 * @param outFlexLayout [out]   To be filled with the description of the planes
 *                              in the buffer
 * @param acquireFence  [in]    Refers to an acquire sync fence object.
 *
 * @return GRALLOC1_ERROR_NONE - Locking is successful;
 *         GRALLOC1_ERROR_BAD_HANDLE - Invalid buffer handle
 *         GRALLOC1_ERROR_BAD_VALUE - Invalid usage parameters
 *         GRALLOC1_ERROR_UNSUPPORTED - Any other error, like wrong access
 *                                      region parameters, invalid buffer ownership, etc.
 *
 * @Notes: Locking a buffer simultaneously for write or read/write leaves the
 *         buffer's content into an indeterminate state.
 */
static int32_t mali_gralloc1_lock_flex_async(gralloc1_device_t *device, buffer_handle_t buffer,
                                             uint64_t producerUsage,
                                             uint64_t consumerUsage,
                                             const gralloc1_rect_t *accessRegion,
                                             struct android_flex_layout *outFlexLayout,
                                             int32_t acquireFence)
{
	int status = 0;
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outFlexLayout == NULL)
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	/* The lock usage constraints
	 * 1. Exactly one of producerUsage and consumerUsage must be *_USAGE_NONE.
	 * 2. The usage which is not *_USAGE_NONE must be one of the *_USAGE_CPU_*
	 *    as applicable.
	 * are relaxed to accommodate an issue in shims with regard to lock usage mapping.
	 */
	if (!((producerUsage | consumerUsage) & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)))
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	status = mali_gralloc_lock_flex_async(m, buffer, producerUsage | consumerUsage,
	                                      accessRegion->left, accessRegion->top,
	                                      accessRegion->width, accessRegion->height,
	                                      outFlexLayout, acquireFence);
	if (status != 0)
	{
		if (status == -EINVAL)
		{
			return GRALLOC1_ERROR_BAD_VALUE;
		}

		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	return GRALLOC1_ERROR_NONE;
}

/*
 *  Unlocks the Gralloc 1.0 buffer asynchronously.
 *
 * @param device          [in]   Gralloc1 device.
 * @param buffer          [in]   The buffer to unlock.
 * @param outReleaseFence [out]  Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 *
 * Note: unlocking a buffer which is not locked results in an unexpected behaviour.
 *       Though it is possible to create a state machine to track the buffer state to
 *       recognize erroneous conditions, it is expected of client to adhere to API
 *       call sequence
 */
static int32_t mali_gralloc1_unlock_async(const gralloc1_device_t * const device,
                                          buffer_handle_t buffer,
                                          int32_t *const outReleaseFence)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const int status = mali_gralloc_unlock_async(m, buffer, outReleaseFence);
	if (status != 0)
	{
		if (status == -EINVAL)
		{
			return GRALLOC1_ERROR_BAD_VALUE;
		}

		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	return GRALLOC1_ERROR_NONE;
}

#if PLATFORM_SDK_VERSION >= 26
static int32_t mali_gralloc1_set_layer_count(gralloc1_device_t* device,
                                             gralloc1_buffer_descriptor_t descriptor,
                                             uint32_t layerCount)
{
	int ret = 0;
	ret = mali_gralloc_set_layer_count_internal(descriptor, layerCount);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc1_get_layer_count(gralloc1_device_t* device, buffer_handle_t buffer, uint32_t* outLayerCount)
{
	int ret = 0;
	ret = mali_gralloc_get_layer_count_internal(buffer, outLayerCount);
	GRALLOC_UNUSED(device);
	return ret;
}
#endif

#if PLATFORM_SDK_VERSION >= 28
static int32_t mali_gralloc1_validate_buffer_size(gralloc1_device_t* device, buffer_handle_t buffer,
                                                  const gralloc1_buffer_descriptor_info_t* descriptorInfo,
                                                  uint32_t stride)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (buffer == 0)
	{
		AERR("Bad input buffer handle %p to validate buffer size", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Buffer: %p is corrupted, for validating its size", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (descriptorInfo->producerUsage & ~VALID_USAGE)
	{
		ALOGW("Buffer descriptor with invalid producer usage bits 0x%" PRIx64,
		       descriptorInfo->producerUsage & ~VALID_USAGE);
	}

	if (descriptorInfo->consumerUsage & ~VALID_USAGE)
	{
		ALOGW("Buffer descriptor with invalid consumer usage bits 0x%" PRIx64,
		       descriptorInfo->consumerUsage & ~VALID_USAGE);
	}

	if (!descriptorInfo->width || !descriptorInfo->height ||!descriptorInfo->layerCount)
	{
		AERR("Invalid buffer descriptor attributes, width = %d height = %d  layerCount = %d",
		      descriptorInfo->width, descriptorInfo->height, descriptorInfo->layerCount);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (descriptorInfo->format == 0)
	{
		AERR("Invalid descriptor format to validate the buffer size");
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	buffer_descriptor_t grallocDescriptor;
	grallocDescriptor.width = descriptorInfo->width;
	grallocDescriptor.height = descriptorInfo->height;
	grallocDescriptor.layer_count = descriptorInfo->layerCount;
	grallocDescriptor.hal_format = descriptorInfo->format;
	grallocDescriptor.producer_usage = descriptorInfo->producerUsage;
	grallocDescriptor.consumer_usage = descriptorInfo->consumerUsage;
	grallocDescriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;

	/* Derive the buffer size for the given descriptor */
	const int result = mali_gralloc_derive_format_and_size(m, &grallocDescriptor);
	if (result)
	{
		AERR("Failed to derive format and size for the given descriptor information. error: %d", result);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	/* Validate the buffer parameters against descriptor info */
	private_handle_t *gralloc_buffer = (private_handle_t *)buffer;

	/* The buffer size must be greater than (or equal to) what would have been
	 * allocated with descriptor
	 */
	if ((size_t)gralloc_buffer->size < grallocDescriptor.size)
	{
		ALOGW("Buf size mismatch. Buffer size = %u, Descriptor (derived) size = %zu",
		       gralloc_buffer->size, grallocDescriptor.size);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if ((uint32_t)gralloc_buffer->stride != stride)
	{
		AERR("Stride mismatch. Expected stride = %d, Buffer stride = %d",
		                       stride, gralloc_buffer->stride);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (gralloc_buffer->internal_format != grallocDescriptor.internal_format)
	{
		AERR("Buffer internal format :0x%" PRIx64" does not match descriptor (derived) internal format :0x%"
		      PRIx64, gralloc_buffer->internal_format, grallocDescriptor.internal_format);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (gralloc_buffer->alloc_format != grallocDescriptor.alloc_format)
	{
		AERR("Buffer alloc format :0x%" PRIx64" does not match descriptor (derived) alloc format :0x%"
		      PRIx64, gralloc_buffer->alloc_format, grallocDescriptor.alloc_format);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	const int format_idx = get_format_index(gralloc_buffer->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
	if (format_idx == -1)
	{
		AERR("Invalid format to validate buffer descriptor");
		return GRALLOC1_ERROR_BAD_VALUE;
	}
	else
	{
		for (int i = 0; i < formats[format_idx].npln; i++)
		{
			if (gralloc_buffer->plane_info[i].offset != grallocDescriptor.plane_info[i].offset)
			{
				AERR("Buffer offset 0x%x mismatch with desc offset 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].offset, grallocDescriptor.plane_info[i].offset, i);
				return GRALLOC1_ERROR_BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].byte_stride != grallocDescriptor.plane_info[i].byte_stride)
			{
				AERR("Buffer byte stride 0x%x mismatch with desc byte stride 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].byte_stride, grallocDescriptor.plane_info[i].byte_stride, i);
				return GRALLOC1_ERROR_BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].alloc_width != grallocDescriptor.plane_info[i].alloc_width)
			{
				AERR("Buffer alloc width 0x%x mismatch with desc alloc width 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].alloc_width, grallocDescriptor.plane_info[i].alloc_width, i);
				return GRALLOC1_ERROR_BAD_VALUE;
			}

			if (gralloc_buffer->plane_info[i].alloc_height != grallocDescriptor.plane_info[i].alloc_height)
			{
				AERR("Buffer alloc height 0x%x mismatch with desc alloc height 0x%x in plane %d ",
				      gralloc_buffer->plane_info[i].alloc_height, grallocDescriptor.plane_info[i].alloc_height, i);
				return GRALLOC1_ERROR_BAD_VALUE;
			}
		}
	}

	if ((uint32_t)gralloc_buffer->width != grallocDescriptor.width)
	{
		AERR("Width mismatch. Buffer width = %u, Descriptor width = %u",
		      gralloc_buffer->width, grallocDescriptor.width);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if ((uint32_t)gralloc_buffer->height != grallocDescriptor.height)
	{
		AERR("Height mismatch. Buffer height = %u, Descriptor height = %u",
		      gralloc_buffer->height, grallocDescriptor.height);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (gralloc_buffer->layer_count != grallocDescriptor.layer_count)
	{
		AERR("Layer Count mismatch. Buffer layer_count = %u, Descriptor layer_count width = %u",
		      gralloc_buffer->layer_count, grallocDescriptor.layer_count);
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_get_transport_size(gralloc1_device_t* device, buffer_handle_t buffer,
                                                uint32_t *outNumFds, uint32_t *outNumInts)
{
	GRALLOC_UNUSED(device);

	if (buffer == 0)
	{
		AERR("Bad input buffer handle %p to query transport size", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outNumFds == 0 || outNumInts == 0)
	{
		AERR("Bad output pointers outNumFds=%p outNumInts=%p to populate", outNumFds, outNumInts);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Buffer %p, for querying transport size, is corrupted", buffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	*outNumFds = GRALLOC_ARM_NUM_FDS;
	*outNumInts = NUM_INTS_IN_PRIVATE_HANDLE;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_import_buffer(gralloc1_device_t* device, const buffer_handle_t rawHandle,
                                           buffer_handle_t* outBuffer)
{
	mali_gralloc_module *m;
	m = reinterpret_cast<private_module_t *>(device->common.module);

	if (rawHandle == 0)
	{
		AERR("Bad input raw handle %p to import", rawHandle);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (outBuffer == 0)
	{
		AERR("Bad output buffer pointer %p to populate", outBuffer);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	/* The rawHandle could have been cloned locally or received from another
	 * HAL server/client or another process. Hence clone it locally before
	 * importing the buffer
	 */
	native_handle_t* bufferHandle = native_handle_clone(rawHandle);
	if (!bufferHandle)
	{
		AERR("Failed to clone %p for importing it", rawHandle);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	// clear the invalid mapping address
	private_handle_t *tmphandle = (private_handle_t *)bufferHandle;
	tmphandle->attr_base = MAP_FAILED;

	if (private_handle_t::validate(bufferHandle) < 0)
	{
		AERR("Failed to import corrupted buffer: %p", bufferHandle);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (mali_gralloc_reference_retain(m, bufferHandle) < 0)
	{
		AERR("Buffer: %p cannot be imported", bufferHandle);
		return GRALLOC1_ERROR_NO_RESOURCES;
	}

	*outBuffer = bufferHandle;

	return GRALLOC1_ERROR_NONE;
}
#endif

static const mali_gralloc_func mali_gralloc_func_list[] = {
	{ GRALLOC1_FUNCTION_DUMP, (gralloc1_function_pointer_t)mali_gralloc_dump },
	{ GRALLOC1_FUNCTION_CREATE_DESCRIPTOR, (gralloc1_function_pointer_t)mali_gralloc_create_descriptor },
	{ GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR, (gralloc1_function_pointer_t)mali_gralloc_destroy_descriptor },
	{ GRALLOC1_FUNCTION_SET_CONSUMER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_set_consumer_usage },
	{ GRALLOC1_FUNCTION_SET_DIMENSIONS, (gralloc1_function_pointer_t)mali_gralloc_set_dimensions },
	{ GRALLOC1_FUNCTION_SET_FORMAT, (gralloc1_function_pointer_t)mali_gralloc_set_format },
	{ GRALLOC1_FUNCTION_SET_PRODUCER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_set_producer_usage },
	{ GRALLOC1_FUNCTION_GET_BACKING_STORE, (gralloc1_function_pointer_t)mali_gralloc_get_backing_store },
	{ GRALLOC1_FUNCTION_GET_CONSUMER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_get_consumer_usage },
	{ GRALLOC1_FUNCTION_GET_DIMENSIONS, (gralloc1_function_pointer_t)mali_gralloc_get_dimensions },
	{ GRALLOC1_FUNCTION_GET_FORMAT, (gralloc1_function_pointer_t)mali_gralloc_get_format },
	{ GRALLOC1_FUNCTION_GET_PRODUCER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_get_producer_usage },
	{ GRALLOC1_FUNCTION_GET_STRIDE, (gralloc1_function_pointer_t)mali_gralloc_get_stride },
	{ GRALLOC1_FUNCTION_ALLOCATE, (gralloc1_function_pointer_t)mali_gralloc_allocate },
	{ GRALLOC1_FUNCTION_RETAIN, (gralloc1_function_pointer_t)mali_gralloc_retain },
	{ GRALLOC1_FUNCTION_RELEASE, (gralloc1_function_pointer_t)mali_gralloc_release },
	{ GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES, (gralloc1_function_pointer_t)mali_gralloc1_get_num_flex_planes },
	{ GRALLOC1_FUNCTION_LOCK, (gralloc1_function_pointer_t)mali_gralloc1_lock_async },
	{ GRALLOC1_FUNCTION_LOCK_FLEX, (gralloc1_function_pointer_t)mali_gralloc1_lock_flex_async },
	{ GRALLOC1_FUNCTION_UNLOCK, (gralloc1_function_pointer_t)mali_gralloc1_unlock_async },
#if PLATFORM_SDK_VERSION >= 26
	{ GRALLOC1_FUNCTION_SET_LAYER_COUNT, (gralloc1_function_pointer_t)mali_gralloc1_set_layer_count },
	{ GRALLOC1_FUNCTION_GET_LAYER_COUNT, (gralloc1_function_pointer_t)mali_gralloc1_get_layer_count },
#endif

#if PLATFORM_SDK_VERSION >= 28
	{ GRALLOC1_FUNCTION_VALIDATE_BUFFER_SIZE, (gralloc1_function_pointer_t)mali_gralloc1_validate_buffer_size },
	{ GRALLOC1_FUNCTION_GET_TRANSPORT_SIZE, (gralloc1_function_pointer_t)mali_gralloc1_get_transport_size },
	{ GRALLOC1_FUNCTION_IMPORT_BUFFER, (gralloc1_function_pointer_t)mali_gralloc1_import_buffer },
#endif

	/* GRALLOC1_FUNCTION_INVALID has to be the last descriptor on the list. */
	{ GRALLOC1_FUNCTION_INVALID, NULL }
};

static void mali_gralloc_getCapabilities(gralloc1_device_t *dev, uint32_t *outCount, int32_t *outCapabilities)
{
	GRALLOC_UNUSED(dev);
#if PLATFORM_SDK_VERSION >= 26
	if (outCount != NULL)
	{
		*outCount = 1;
	}

	if (outCapabilities != NULL)
	{
		*(outCapabilities++) = GRALLOC1_CAPABILITY_LAYERED_BUFFERS;
	}
#else
	GRALLOC_UNUSED(outCapabilities);
	if (outCount != NULL)
	{
		*outCount = 0;
	}
#endif
}

static gralloc1_function_pointer_t mali_gralloc_getFunction(gralloc1_device_t *dev, int32_t descriptor)
{
	GRALLOC_UNUSED(dev);
	gralloc1_function_pointer_t rval = NULL;
	uint32_t pos = 0;

	while (mali_gralloc_func_list[pos].desc != GRALLOC1_FUNCTION_INVALID)
	{
		if (mali_gralloc_func_list[pos].desc == descriptor)
		{
			rval = mali_gralloc_func_list[pos].func;
			break;
		}

		pos++;
	}

	if (rval == NULL)
	{
		rval = mali_gralloc_private_interface_getFunction(descriptor);
	}

	return rval;
}

static int mali_gralloc_device_close(struct hw_device_t *device)
{
	gralloc1_device_t *dev = reinterpret_cast<gralloc1_device_t *>(device);
	if (dev)
	{
		delete dev;
	}

	mali_gralloc_ion_close();

	return 0;
}

int mali_gralloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	gralloc1_device_t *dev;

	GRALLOC_UNUSED(name);

	dev = new gralloc1_device_t;

	if (NULL == dev)
	{
		return -1;
	}

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = mali_gralloc_device_close;

	dev->getCapabilities = mali_gralloc_getCapabilities;
	dev->getFunction = mali_gralloc_getFunction;

	*device = &dev->common;

	return 0;
}
