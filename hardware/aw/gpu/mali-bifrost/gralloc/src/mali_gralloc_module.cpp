/*
 * Copyright (C) 2016-2018 ARM Limited. All rights reserved.
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

#include <errno.h>
#include <pthread.h>
#include <inttypes.h>

#include <log/log.h>
#include <cutils/atomic.h>
#include <system/window.h>
#include <hardware/hardware.h>
#include <hardware/fb.h>

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "mali_gralloc_bufferaccess.h"
#include "mali_gralloc_reference.h"

#if GRALLOC_VERSION_MAJOR == 1
#include "mali_gralloc_public_interface.h"
#elif GRALLOC_VERSION_MAJOR == 0
#include "legacy/alloc_device.h"
#endif

static int mali_gralloc_module_device_open(const hw_module_t *module, const char *name,
                                           hw_device_t **device)
{
	int status = -EINVAL;

#if GRALLOC_VERSION_MAJOR == 1

	if (!strncmp(name, GRALLOC_HARDWARE_MODULE_ID, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = mali_gralloc_device_open(module, name, device);
	}

#else

	if (!strncmp(name, GRALLOC_HARDWARE_GPU0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = alloc_device_open(module, name, device);
	}

#endif
	else if (!strncmp(name, GRALLOC_HARDWARE_FB0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = framebuffer_device_open(module, name, device);
	}

	return status;
}

int gralloc_register_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_reference_retain(m, handle);
}

int gralloc_unregister_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_reference_release(m, handle, false);
}

int gralloc_lock(gralloc_module_t const *module, buffer_handle_t handle, int usage,
                 int l, int t, int w, int h, void **vaddr)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock(m, handle, (unsigned int)usage, l, t, w, h, vaddr);
}

int gralloc_lock_ycbcr(gralloc_module_t const *module, buffer_handle_t handle,
                       int usage, int l, int t, int w, int h, android_ycbcr *ycbcr)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_ycbcr(m, handle, (unsigned int)usage, l, t, w, h, ycbcr);
}

int gralloc_unlock(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_unlock(m, handle);
}

int gralloc_lock_async(gralloc_module_t const *module, buffer_handle_t handle,
                       int usage, int l, int t, int w, int h, void **vaddr,
                       int32_t fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_async(m, handle, (unsigned int)usage, l, t, w, h, vaddr, fence_fd);
}

int gralloc_lock_ycbcr_async(gralloc_module_t const *module, buffer_handle_t handle,
                             int usage, int l, int t, int w, int h,
                             android_ycbcr *ycbcr, int32_t fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_ycbcr_async(m, handle, (unsigned int)usage, l, t, w, h, ycbcr, fence_fd);
}

int gralloc_unlock_async(gralloc_module_t const *module, buffer_handle_t handle,
                         int32_t *fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_unlock_async(m, handle, fence_fd);
}

struct hw_module_methods_t mali_gralloc_module_methods = { mali_gralloc_module_device_open };

/* Export the private module as HAL_MODULE_INFO_SYM symbol as required by Gralloc v0.3 / v1.0 */
struct private_module_t HAL_MODULE_INFO_SYM;
