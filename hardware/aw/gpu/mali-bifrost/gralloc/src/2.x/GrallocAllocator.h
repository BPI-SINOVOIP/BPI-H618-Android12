/*
 * Copyright (C) 2017-2018 ARM Limited. All rights reserved.
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

#ifndef ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V2_x_GRALLOC_H
#define ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V2_x_GRALLOC_H

#if HIDL_ALLOCATOR_VERSION_SCALED == 200
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_bufferdescriptor.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace HIDL_IALLOCATOR_NAMESPACE {
namespace implementation {

using android::hardware::graphics::mapper::V2_0::BufferDescriptor;
using android::hardware::graphics::mapper::V2_0::Error;

class GrallocAllocator : public IAllocator
{
public:
	GrallocAllocator();
	virtual ~GrallocAllocator();

	/* Override IAllocator interface */
	Return<void> dumpDebugInfo(dumpDebugInfo_cb hidl_cb) override;
	Return<void> allocate(const BufferDescriptor& descriptor, uint32_t count,
	                      allocate_cb hidl_cb) override;
private:
	struct private_module_t privateModule;
};

extern "C" IAllocator* HIDL_FETCH_IAllocator(const char* name);

} // namespace implementation
} // namespace HIDL_IALLOCATOR_NAMESPACE
} // namespace allocator
} // namespace graphics
} // namespace hardware
} // namespace android

#endif // ANDROID_HARDWARE_GRAPHICS_ALLOCATOR_V2_x_GRALLOC_H
