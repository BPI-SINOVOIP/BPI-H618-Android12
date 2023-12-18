/*
 * Copyright (C) 2010-2019 ARM Limited. All rights reserved.
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
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <log/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "mali_gralloc_module.h"
#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "mali_gralloc_ion.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"

static int alloc_device_alloc(alloc_device_t *dev, int w, int h, int format,
                              int _usage, buffer_handle_t *pHandle,
                              int *pStride)
{
	mali_gralloc_module *m;
	uint64_t usage = (unsigned int)_usage;
	buffer_descriptor_t buffer_descriptor;
	int err = -EINVAL;

	if (!dev || !pHandle || !pStride)
	{
		return err;
	}

	/* Initialise output parameters. */
	*pHandle = NULL;
	*pStride = 0;

	m = reinterpret_cast<private_module_t *>(dev->common.module);

	memset((void*)&buffer_descriptor, 0, sizeof(buffer_descriptor));
	buffer_descriptor.hal_format = format;
	buffer_descriptor.consumer_usage = usage;
	buffer_descriptor.producer_usage = usage;
	buffer_descriptor.width = w;
	buffer_descriptor.height = h;
	buffer_descriptor.layer_count = 1;
	buffer_descriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;

#if DISABLE_FRAMEBUFFER_HAL != 1
	if (usage & GRALLOC_USAGE_HW_FB)
	{
		if (mali_gralloc_fb_allocate(m, &buffer_descriptor, pHandle) < 0)
		{
			err = -ENOMEM;
		}
		else
		{
			err = mali_gralloc_query_getstride(*pHandle, pStride);
		}
	}
	else
#endif
	{
		gralloc_buffer_descriptor_t gralloc_buffer_descriptor[1];
		gralloc_buffer_descriptor[0] = (gralloc_buffer_descriptor_t)(&buffer_descriptor);

		if (mali_gralloc_buffer_allocate(m, gralloc_buffer_descriptor, 1, pHandle, NULL) < 0)
		{
			ALOGE("Failed to allocate buffer.");
			err = -ENOMEM;
		}
		else
		{
			err = mali_gralloc_query_getstride(*pHandle, pStride);
		}
	}

	return err;
}

static int alloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(handle);
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		close(hnd->fd);
	}
	else
	{
		mali_gralloc_buffer_free(handle);
	}

	delete hnd;

	return 0;
}


static int alloc_device_close(struct hw_device_t *device)
{
	alloc_device_t *dev = reinterpret_cast<alloc_device_t *>(device);
	if (dev)
	{
		delete dev;
	}

	mali_gralloc_ion_close();

	return 0;
}

int alloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	alloc_device_t *dev;

	GRALLOC_UNUSED(name);

	dev = new alloc_device_t;

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
	dev->common.close = alloc_device_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

	*device = &dev->common;

	return 0;
}
