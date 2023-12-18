/*
 * Copyright (C) 2017-2019 ARM Limited. All rights reserved.
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

#include "GrallocAllocator.h"
#include "GrallocBufferDescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_ion.h"
#include "framebuffer_device.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace HIDL_IALLOCATOR_NAMESPACE {
namespace implementation {

/**
 *  IAllocator constructor. All the state information required for the Gralloc
 *  private module is populated in its default constructor. Gralloc 2.0 specific
 *  state information can be populated here.
 *
 * @return None
 */

GrallocAllocator::GrallocAllocator()
{
}

/*
 *  IAllocator destructor. All the resources acquired for Gralloc private module
 *  are released
 *
 * @return None
 */
GrallocAllocator::~GrallocAllocator()
{
	mali_gralloc_ion_close();
}

/*
 * Retrieves implementation-defined debug information
 *
 * Retrieves implementation-defined debug information, which will be
 * displayed during, for example, `dumpsys SurfaceFlinger`
 *
 * @param hidl_cb [in] HIDL callback function generating -
 *                String of debug information
 *
 * @return Void
 */
Return<void> GrallocAllocator::dumpDebugInfo(dumpDebugInfo_cb hidl_cb)
{
	hidl_cb(hidl_string());
	return Void();
}

/*
 * Allocates buffers with the properties specified by the descriptor
 *
 * @param descriptor: Specifies the properties of the buffers to allocate.
 * @param count: Number of buffers to allocate.
 * @param hidl_cb [in] HIDL callback function generating -
 *        error : NONE upon success. Otherwise,
 *                BAD_DESCRIPTOR when the descriptor is invalid.
 *                NO_RESOURCES when the allocation cannot be fulfilled
 *                UNSUPPORTED when any of the property encoded in the descriptor
 *                            is not supported
 *        stride: Number of pixels between two consecutive rows of the
 *                buffers, when the concept of consecutive rows is defined.
 *        buffers: An array of raw handles to the newly allocated buffers
 *
 * @return Void
 */
Return<void> GrallocAllocator::allocate(const BufferDescriptor& descriptor,
                                        uint32_t count, allocate_cb hidl_cb)
{
	buffer_descriptor_t bufferDescriptor;
	Error error = Error::NONE;
	int stride = 0, tmpStride = 0;
	std::vector<hidl_handle> grallocBuffers;
	gralloc_buffer_descriptor_t grallocBufferDescriptor[1];

	if (!mapper::HIDL_IMAPPER_NAMESPACE::implementation::grallocDecodeBufferDescriptor(descriptor, bufferDescriptor))
	{
		hidl_cb(Error::BAD_DESCRIPTOR, 0, hidl_vec<hidl_handle>());
		return Void();
	}

	grallocBufferDescriptor[0] = (gralloc_buffer_descriptor_t)(&bufferDescriptor);
	grallocBuffers.reserve(count);

	for (uint32_t i = 0; i < count; i++)
	{
		buffer_handle_t tmpBuffer;

		int allocResult = 0;
#if DISABLE_FRAMEBUFFER_HAL != 1
		if ((bufferDescriptor.producer_usage & GRALLOC_USAGE_HW_FB) ||
		    (bufferDescriptor.consumer_usage & GRALLOC_USAGE_HW_FB))
		{
			allocResult = mali_gralloc_fb_allocate(&privateModule, &bufferDescriptor,
			                                       &tmpBuffer);
		}
		else
#endif
		{
			allocResult = mali_gralloc_buffer_allocate(&privateModule, grallocBufferDescriptor,
			                                            1, &tmpBuffer, nullptr);
		}

		if (allocResult < 0)
		{
			AERR("%s, buffer allocation failed with %d", __func__, allocResult);
			error = Error::NO_RESOURCES;
			break;
		}

		mali_gralloc_query_getstride(tmpBuffer, &tmpStride);

		if (stride == 0)
		{
			stride = tmpStride;
		}
		else if (stride != tmpStride)
		{
			/* Stride must be the same for all allocations */
			mali_gralloc_buffer_free(tmpBuffer);
			stride = 0;
			error = Error::UNSUPPORTED;
			break;
		}

		grallocBuffers.emplace_back(hidl_handle(tmpBuffer));
	}

	/* Populate the array of buffers for application consumption */
	hidl_vec<hidl_handle> hidlBuffers;
	if (error == Error::NONE)
	{
		hidlBuffers.setToExternal(grallocBuffers.data(), grallocBuffers.size());
	}
	hidl_cb(error, stride, hidlBuffers);

	/* The application should import the Gralloc buffers using IMapper for
	 * further usage. Free the allocated buffers in IAllocator context
	 */
	for (const auto& buffer : grallocBuffers)
	{
		mali_gralloc_buffer_free(buffer.getNativeHandle());
	}

	return Void();
}

IAllocator* HIDL_FETCH_IAllocator(const char* /* name */)
{
	ALOGV("Arm Module IAllocator %d.%d, pid = %d ppid = %d", GRALLOC_VERSION_MAJOR,
	       (HIDL_ALLOCATOR_VERSION_SCALED - (GRALLOC_VERSION_MAJOR * 100)) / 10, getpid(), getppid());

	return new GrallocAllocator();
}

} // namespace implementation
} // namespace HIDL_IALLOCATOR_NAMESPACE
} // namespace allocator
} // namespace graphics
} // namespace hardware
} // namespace android
