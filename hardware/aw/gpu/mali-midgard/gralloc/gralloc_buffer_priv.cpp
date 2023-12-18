/*
 * Copyright (C) 2014-2017 ARM Limited. All rights reserved.
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

#include <cutils/ashmem.h>
#include <android/log.h>
#include <sys/mman.h>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#include <ion/ion.h>

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_buffer_priv.h"
#include "gralloc_sunxi.h"

/*
 * Allocate shared memory for attribute storage. Only to be
 * used by gralloc internally.
 *
 * Return 0 on success.
 */
int gralloc_buffer_attr_allocate(mali_gralloc_module *m, private_handle_t *hnd)
{
	int rval = -1;
	int heap_mask = ION_HEAP_SYSTEM_MASK;
	ion_user_handle_t ion_hnd;

	if (!hnd)
	{
		return -1;
	}

	if (hnd->share_attr_fd >= 0)
	{
		ALOGW("Warning share attribute fd already exists during create. Closing.");
		close(hnd->share_attr_fd);
	}

	rval = ion_alloc(m->ion_client, PAGE_SIZE, 0, heap_mask, 0, &ion_hnd);
	if (rval < 0)
	{
		AERR("Failed to ion_alloc from ion_client %d for %d bytes metadata buffer via heap type %s (mask:%d)!\n",
			m->ion_client, PAGE_SIZE, get_heap_type_name(heap_mask), heap_mask);
		heap_mask = ION_HEAP_TYPE_DMA_MASK;
		rval = ion_alloc(m->ion_client, PAGE_SIZE, 0, heap_mask, 0, &ion_hnd);
		if (rval)
		{
			AERR("Failed to ion_alloc from ion_client %d for %d bytes metadata buffer via heap type %s (mask:%d)!\n",
				m->ion_client, PAGE_SIZE, get_heap_type_name(heap_mask), heap_mask);
			return -1;
		}
	}
	rval = ion_share(m->ion_client, ion_hnd, &hnd->share_attr_fd);
	if (rval < 0)
	{
		AERR("ion_share metadata buffer from ion client %d failed", m->ion_client);
		if (0 != ion_free(m->ion_client, ion_hnd))
			AERR("ion_free from ion client %d failed", m->ion_client);
		return -1;
	}

	hnd->attr_base = (unsigned char*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->share_attr_fd, 0);
	if (MAP_FAILED == hnd->attr_base)
	{
		AERR("mmap metadata buffer from ion client %d failed", m->ion_client);
		if (0 != ion_free(m->ion_client, ion_hnd))
			AERR( "ion_free from ion client %d failed", m->ion_client);
		close(hnd->share_attr_fd);
		hnd->share_attr_fd = -1;
		return -1;
	}

	memset(hnd->attr_base, 0x0, PAGE_SIZE);

	rval = munmap(hnd->attr_base, PAGE_SIZE);
	if (rval < 0)
	{
		AERR("gralloc_buffer_attr_allocate munmap failed, aw_buf_id = %lld", hnd->aw_buf_id);
	}

	if (0 != ion_free(m->ion_client, ion_hnd))
	{
		AERR("Failed to ion_free(ion_client: %d ion_metadata_hnd: %d), aw_buf_id=%lld", m->ion_client,ion_hnd, hnd->aw_buf_id);
	}

	hnd->ion_metadata_size = PAGE_SIZE;

	return 0;
}

/*
 * Frees the shared memory allocated for attribute storage.
 * Only to be used by gralloc internally.

 * Return 0 on success.
 */
int gralloc_buffer_attr_free(private_handle_t *hnd)
{
	int rval = -1;

	if (!hnd)
	{
		goto out;
	}

	if (hnd->share_attr_fd < 0)
	{
		ALOGE("Shared attribute region not avail to free");
		goto out;
	}

	close(hnd->share_attr_fd);
	hnd->share_attr_fd = -1;
	rval = 0;

out:
	return rval;
}
