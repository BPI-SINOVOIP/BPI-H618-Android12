/*
 * Copyright (C) 2018 ARM Limited. All rights reserved.
 *
 * Copyright 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_GRAPHICS_V2_X_GRALLOCBUFFERDESCRIPTOR_H
#define ANDROID_HARDWARE_GRAPHICS_V2_X_GRALLOCBUFFERDESCRIPTOR_H

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#if HIDL_MAPPER_VERSION_SCALED == 210
#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#endif

#include "mali_gralloc_bufferdescriptor.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace HIDL_IMAPPER_NAMESPACE {
namespace implementation {

/**
 * BufferDescriptor is created by IMapper and consumed by IAllocator. It is
 * versioned so that IMapper and IAllocator can be updated independently.
 * Gralloc creates the same BufferDescriptor configuration across all versions of
 * IMapper (BufferDescriptorInfo)
 */
constexpr uint32_t grallocBufferDescriptorMagicVersion = HIDL_MAPPER_VERSION_SCALED;
constexpr uint32_t grallocBufferDescriptorSize = 7;

/*
 * Creates a buffer descriptor from incoming descriptor attributes
 *
 * @param descriptorInfo [in]  Specifies the (2.1 IMapper) attributes of
 *                             the descriptor.
 *
 * @return Newly created buffer descriptor;
 */
#if HIDL_MAPPER_VERSION_SCALED == 210
inline V2_0::BufferDescriptor grallocEncodeBufferDescriptor(
       const V2_1::IMapper::BufferDescriptorInfo& descriptorInfo)
{
	V2_0::BufferDescriptor descriptor;
	descriptor.resize(grallocBufferDescriptorSize);
	descriptor[0] = grallocBufferDescriptorMagicVersion;
	descriptor[1] = descriptorInfo.width;
	descriptor[2] = descriptorInfo.height;
	descriptor[3] = descriptorInfo.layerCount;
	descriptor[4] = static_cast<uint32_t>(descriptorInfo.format);
	descriptor[5] = static_cast<uint32_t>(descriptorInfo.usage);
	descriptor[6] = static_cast<uint32_t>(descriptorInfo.usage >> 32);

	return descriptor;
}
#endif

/*
 * Creates a buffer descriptor from incoming descriptor attributes
 *
 * @param descriptorInfo [in]  Specifies the (2.0 IMapper) attributes of
 *                             the descriptor.
 *
 * @return Newly created buffer descriptor;
 */
inline V2_0::BufferDescriptor grallocEncodeBufferDescriptor(
       const V2_0::IMapper::BufferDescriptorInfo& descriptorInfo)
{
	V2_0::BufferDescriptor descriptor;
	descriptor.resize(grallocBufferDescriptorSize);
	descriptor[0] = grallocBufferDescriptorMagicVersion;
	descriptor[1] = descriptorInfo.width;
	descriptor[2] = descriptorInfo.height;
	descriptor[3] = descriptorInfo.layerCount;
	descriptor[4] = static_cast<uint32_t>(descriptorInfo.format);
	descriptor[5] = static_cast<uint32_t>(descriptorInfo.usage);
	descriptor[6] = static_cast<uint32_t>(descriptorInfo.usage >> 32);

	return descriptor;
}

/*
 * Decodes incoming buffer descriptor (previously encoded by gralloc for Android)
 * and populates Gralloc's internal buffer descriptor (which is used across
 * various versions of Gralloc) for allocation purpose
 *
 * @param androidDescriptor [in]  Specifies the (encoded) properties of the buffers
 * @param grallocDescriptor [out] Gralloc's internal buffer descriptor
 *
 * @return true, upon successful decode
 *         false, corrupted descriptor encountered
 */
inline bool grallocDecodeBufferDescriptor(const V2_0::BufferDescriptor& androidDescriptor,
                                          buffer_descriptor_t& grallocDescriptor)
{
	if (androidDescriptor.size() != grallocBufferDescriptorSize)
	{
		AERR("Corrupted buffer size in descriptor = %p, pid = %d ", &androidDescriptor, getpid());
		return false;
	}

	if (androidDescriptor[0] != grallocBufferDescriptorMagicVersion)
	{
		AERR("Corrupted buffer version in descriptor = %p, pid = %d ", &androidDescriptor, getpid());
		return false;
	}

	grallocDescriptor.width = androidDescriptor[1];
	grallocDescriptor.height = androidDescriptor[2];
	grallocDescriptor.layer_count = androidDescriptor[3];
	grallocDescriptor.hal_format = static_cast<uint64_t>(androidDescriptor[4]);
	grallocDescriptor.producer_usage = (static_cast<uint64_t>(androidDescriptor[6]) << 32) | androidDescriptor[5];
	grallocDescriptor.consumer_usage = grallocDescriptor.producer_usage;
	grallocDescriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;
	grallocDescriptor.signature = sizeof(buffer_descriptor_t);

	return true;
}

} // namespace implementation
} // namespace HIDL_IMAPPER_NAMESPACE
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_GRAPHICS_V2_X_GRALLOCBUFFERDESCRIPTOR_H
