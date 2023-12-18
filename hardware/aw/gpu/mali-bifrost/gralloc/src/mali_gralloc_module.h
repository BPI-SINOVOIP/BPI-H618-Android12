/*
 * Copyright (C) 2016, 2018-2019 ARM Limited. All rights reserved.
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
#ifndef MALI_GRALLOC_MODULE_H_
#define MALI_GRALLOC_MODULE_H_

#include <system/window.h>
#include <linux/fb.h>
#include <pthread.h>
#include <log/log.h>

#if GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#elif GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#else
#if HIDL_COMMON_VERSION_SCALED == 100
#include <android/hardware/graphics/common/1.0/types.h>
#elif HIDL_COMMON_VERSION_SCALED == 110
#include <android/hardware/graphics/common/1.1/types.h>
#endif
#endif

#if GRALLOC_USE_LEGACY_ION_API != 1
#include <ion/ion_4.12.h>
#endif

typedef enum
{
	MALI_DPY_TYPE_UNKNOWN = 0,
	MALI_DPY_TYPE_CLCD,
	MALI_DPY_TYPE_HDLCD
} mali_dpy_type;

#if GRALLOC_VERSION_MAJOR <= 1
extern hw_module_methods_t mali_gralloc_module_methods;
#endif

#if GRALLOC_VERSION_MAJOR == 1
typedef struct
{
	struct hw_module_t common;
} gralloc_module_t;
#endif

#if GRALLOC_VERSION_MAJOR == 0
int gralloc_register_buffer(gralloc_module_t const *module, buffer_handle_t handle);
int gralloc_unregister_buffer(gralloc_module_t const *module, buffer_handle_t handle);
int gralloc_lock(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w, int h,
                        void **vaddr);
int gralloc_lock_ycbcr(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w,
                              int h, android_ycbcr *ycbcr);
int gralloc_unlock(gralloc_module_t const *module, buffer_handle_t handle);
int gralloc_lock_async(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w,
                              int h, void **vaddr, int32_t fence_fd);
int gralloc_lock_ycbcr_async(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t,
                                    int w, int h, android_ycbcr *ycbcr, int32_t fence_fd);
int gralloc_unlock_async(gralloc_module_t const *module, buffer_handle_t handle, int32_t *fence_fd);
#endif

struct private_module_t
{
#if GRALLOC_VERSION_MAJOR <= 1
	gralloc_module_t base;
#endif

	struct private_handle_t *framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	mali_dpy_type dpy_type;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;
	int swapInterval;
	uint64_t fbdev_format;

#ifdef __cplusplus
	/* Never intended to be used from C code */
	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};
#endif

#ifdef __cplusplus
	#define INIT_ZERO(obj) (memset(&(obj), 0, sizeof((obj))))
	private_module_t()
	{
	#if GRALLOC_VERSION_MAJOR <= 1
		base.common.tag = HARDWARE_MODULE_TAG;
		#if GRALLOC_VERSION_MAJOR == 1
		base.common.version_major = GRALLOC_MODULE_API_VERSION_1_0;
		ALOGE("Arm Module v1.0");
		#else
		base.common.version_major = GRALLOC_MODULE_API_VERSION_0_3;
		ALOGE("Arm Module v0.3");
		#endif

		base.common.version_minor = 0;
		base.common.id = GRALLOC_HARDWARE_MODULE_ID;
		base.common.name = "Graphics Memory Allocator Module";
		base.common.author = "ARM Ltd.";
		base.common.methods = &mali_gralloc_module_methods;
		base.common.dso = NULL;
		INIT_ZERO(base.common.reserved);

	#if GRALLOC_VERSION_MAJOR == 0
		base.registerBuffer = gralloc_register_buffer;
		base.unregisterBuffer = gralloc_unregister_buffer;
		base.lock = gralloc_lock;
		base.lock_ycbcr = gralloc_lock_ycbcr;
		base.unlock = gralloc_unlock;
		base.lockAsync = gralloc_lock_async;
		base.lockAsync_ycbcr = gralloc_lock_ycbcr_async;
		base.unlockAsync = gralloc_unlock_async;
		base.perform = NULL;
		INIT_ZERO(base.reserved_proc);
		#endif
	#endif

		framebuffer = NULL;
		flags = 0;
		numBuffers = 0;
		bufferMask = 0;
		pthread_mutex_init(&(lock), NULL);
		currentBuffer = NULL;
		INIT_ZERO(info);
		INIT_ZERO(finfo);
		xdpi = 0.0f;
		ydpi = 0.0f;
		fps = 0.0f;
		swapInterval = 1;
		dpy_type = MALI_DPY_TYPE_UNKNOWN;
	}
	#undef INIT_ZERO
#endif /* For #ifdef __cplusplus */
};

typedef struct private_module_t mali_gralloc_module;

#endif /* MALI_GRALLOC_MODULE_H_ */
