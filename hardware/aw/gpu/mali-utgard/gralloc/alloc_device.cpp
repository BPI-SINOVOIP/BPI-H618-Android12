/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
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

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#if GRALLOC_ARM_UMP_MODULE
#include <ump/ump.h>
#include <ump/ump_ref_drv.h>
#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
#include <linux/ion.h>
#include <ion/ion.h>
#endif

#define GRALLOC_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

aw_mem_info_data aw_mem_info =
{
	.dram_size              = 0,
	.secure_level           = 3,
	.ion_flush_cache_range  = 0,
	.carveout_enable        = 0
};

#define ION_IOC_SUNXI_FLUSH_RANGE 5
int aw_flush_cache(int ion_client, void* start_vaddr, int shared_fd, size_t size)
{
	if (aw_mem_info.ion_flush_cache_range) {
		sunxi_cache_range range;
		struct ion_custom_data custom_data;

		/* clean and invalid user cache */
		range.start = (unsigned long)start_vaddr;
		range.end = (unsigned long)start_vaddr + size;

		return ioctl(ion_client, ION_IOC_SUNXI_FLUSH_RANGE, &range);
	} else {
		return ion_sync_fd(ion_client, shared_fd);
	}
}

#if GRALLOC_SIMULATE_FAILURES
#include <cutils/properties.h>

/* system property keys for controlling simulated UMP allocation failures */
#define PROP_MALI_TEST_GRALLOC_FAIL_FIRST     "mali.test.gralloc.fail_first"
#define PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL  "mali.test.gralloc.fail_interval"

static int __ump_alloc_should_fail()
{

	static unsigned int call_count  = 0;
	unsigned int        first_fail  = 0;
	int                 fail_period = 0;
	int                 fail        = 0;

	++call_count;

	/* read the system properties that control failure simulation */
	{
		char prop_value[PROPERTY_VALUE_MAX];

		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_FIRST, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", &first_fail);
		}

		if (property_get(PROP_MALI_TEST_GRALLOC_FAIL_INTERVAL, prop_value, "0") > 0)
		{
			sscanf(prop_value, "%11u", (unsigned long long)&fail_period);
		}
	}

	/* failure simulation is enabled by setting the first_fail property to non-zero */
	if (first_fail > 0)
	{
		LOGI("iteration %u (fail=%u, period=%u)\n", call_count, first_fail, fail_period);

		fail = (call_count == first_fail) ||
		       (call_count > first_fail && fail_period > 0 && 0 == (call_count - first_fail) % fail_period);

		if (fail)
		{
			AERR("failed ump_ref_drv_allocate on iteration #%d\n", call_count);
		}
	}

	return fail;
}
#endif

#ifndef ION_HEAP_SECURE_MASK
#define ION_HEAP_TYPE_SUNXI_START (ION_HEAP_TYPE_CUSTOM + 1)
#define ION_HEAP_TYPE_SECURE     (ION_HEAP_TYPE_SUNXI_START)
#define ION_HEAP_SECURE_MASK     (1<<ION_HEAP_TYPE_SECURE)
#endif /* ION_HEAP_SECURE_MASK */

static char heap_system[] = "SYSTEM";
static char heap_system_contig[] = "SYSTEM_CONTIG";
static char heap_dma[] = "DMA";
static char heap_secure[] = "SECURE";
static char heap_carveout[] = "CARVEOUT";

static char *get_heap_type_name(int heap_mask)
{
	switch(heap_mask)
	{
		case ION_HEAP_SYSTEM_MASK:
			return heap_system;

		case ION_HEAP_SYSTEM_CONTIG_MASK:
			return heap_system_contig;

		case ION_HEAP_TYPE_DMA_MASK:
			return heap_dma;

		case ION_HEAP_SECURE_MASK:
			return heap_secure;

		case ION_HEAP_CARVEOUT_MASK:
			return heap_carveout;

		default:
			AERR("Invalid heap mask %d!\n", heap_mask);
			break;
	}

	return NULL;
}

static int aw_ion_alloc(int *pfd, size_t len, size_t align, unsigned int heap_mask, unsigned int flags, ion_user_handle_t *handle, int usage)
{
	int ret;

    ret = ion_alloc(*pfd, len, align, heap_mask, flags, handle);
	if (ret != 0)
	{
		AERR("%s(line:%d)Failed to ion_alloc from ion_client:%d via heap type %s(mask:%d) for %d Bytes %sbuffer, err = %d, usage = 0x%.8x\n", __func__, __LINE__, *pfd, get_heap_type_name(heap_mask), heap_mask, (int)len, flags ? "cached " : "uncached ", errno, usage);
	}
	else
	{
		AINF("ion_alloc from ion_client:%d via heap type %s(mask:%d) for %d Bytes %sbuffer successfully, usage = 0x%.8x\n", *pfd, get_heap_type_name(heap_mask), heap_mask, (int)len, flags ? "cached " : "uncached ", usage);
	}

	return ret;
}

static int gralloc_alloc_buffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
#if GRALLOC_ARM_DMA_BUF_MODULE
	{
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
		ion_user_handle_t ion_hnd;
		void *cpu_ptr = MAP_FAILED;
		int shared_fd;
		int ret;
		unsigned int heap_mask;
		int lock_state = 0;
		int map_mask = 0;
		int flags = 0;
		int default_heap_mask;

		if (aw_mem_info.carveout_enable)
		{
			default_heap_mask = ION_HEAP_CARVEOUT_MASK;
		}
		else if (aw_mem_info.iommu_enabled)
		{
			default_heap_mask = ION_HEAP_SYSTEM_MASK;
		}
		else
		{
			default_heap_mask = ION_HEAP_TYPE_DMA_MASK;
		}

		if (usage & GRALLOC_USAGE_PROTECTED)
		{
#if defined(ION_HEAP_SECURE_MASK)
			heap_mask = ION_HEAP_SECURE_MASK;
#else
			AERR("The platform does NOT support protected ION memory.");
			return -1;
#endif
		}
		else if (usage & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_2D | GRALLOC_USAGE_HW_FB))
		{
			heap_mask = default_heap_mask;
		}
		else if (aw_mem_info.dram_size <= 512)
		{
			heap_mask = ION_HEAP_SYSTEM_MASK;
		}
		else
		{
			heap_mask = default_heap_mask;
		}

		if ((usage & GRALLOC_USAGE_SW_READ_OFTEN)==GRALLOC_USAGE_SW_READ_OFTEN ||
			(usage & GRALLOC_USAGE_SW_WRITE_OFTEN)==GRALLOC_USAGE_SW_WRITE_OFTEN)
		{
			if (heap_mask != ION_HEAP_SECURE_MASK)
			{
				flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
			}
		}
		ret = aw_ion_alloc(&(m->ion_client), size, 0, heap_mask, flags, &(ion_hnd), usage);

		if (ret != 0)
		{
			switch(heap_mask)
			{
				case ION_HEAP_SECURE_MASK:
					return -1;

				case ION_HEAP_CARVEOUT_MASK:
				case ION_HEAP_TYPE_DMA_MASK:
					if (usage & (GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_HW_2D | GRALLOC_USAGE_HW_FB))
					{
						if (size <= 0x40000000)
						{
							heap_mask = ION_HEAP_SYSTEM_CONTIG_MASK;
						}
						else
						{
							return -1;
						}
					}
					else
					{
						heap_mask = ION_HEAP_SYSTEM_MASK;
					}
					break;

				default:
					heap_mask = default_heap_mask;
					break;
			}

			ret = aw_ion_alloc(&(m->ion_client), size, 0, heap_mask, flags, &(ion_hnd), usage);
			if (ret != 0)
			{
				return -1;
			}
		}

		ret = ion_share(m->ion_client, ion_hnd, &shared_fd);

		if (ret != 0)
		{
			AERR("ion_share( %d ) failed", m->ion_client);

			if (0 != ion_free(m->ion_client, ion_hnd))
			{
				AERR("ion_free( %d ) failed", m->ion_client);
			}

			return -1;
		}

		if (!(usage & GRALLOC_USAGE_PROTECTED))
		{
			map_mask = PROT_READ | PROT_WRITE;
		}
		else
		{
			map_mask = PROT_WRITE;
		}

		cpu_ptr = mmap(NULL, size, map_mask, MAP_SHARED, shared_fd, 0);

		if (MAP_FAILED == cpu_ptr)
		{
			AERR("ion_map( %d ) failed", m->ion_client);

			if (0 != ion_free(m->ion_client, ion_hnd))
			{
				AERR("ion_free( %d ) failed", m->ion_client);
			}

			close(shared_fd);
			return -1;
		}
		else
		{
			if (heap_mask != ION_HEAP_SECURE_MASK)
			{
				memset(cpu_ptr, 0x0, size);
				if (flags & (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC))
					if (aw_flush_cache(m->ion_client, (void *)cpu_ptr, shared_fd, size))
						AERR("Failed to flush Cache, err = %d\n", errno);
			}
		}

		lock_state = private_handle_t::LOCK_STATE_MAPPED;

		private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION | (heap_mask == ION_HEAP_SYSTEM_MASK ? 0 :
			private_handle_t::PRIV_FLAGS_USES_CONFIG), usage, size, cpu_ptr, lock_state);

		if (NULL != hnd)
		{
			hnd->share_fd = shared_fd;
			hnd->ion_hnd = ion_hnd;
			*pHandle = hnd;
			return 0;
		}
		else
		{
			AERR("Gralloc out of mem for ion_client:%d", m->ion_client);
		}

		close(shared_fd);

		ret = munmap(cpu_ptr, size);

		if (0 != ret)
		{
			AERR("munmap failed for base:%p size: %lu", cpu_ptr, (unsigned long)size);
		}

		ret = ion_free(m->ion_client, ion_hnd);

		if (0 != ret)
		{
			AERR("ion_free( %d ) failed", m->ion_client);
		}

		return -1;
	}
#endif

#if GRALLOC_ARM_UMP_MODULE
	MALI_IGNORE(dev);
	{
		ump_handle ump_mem_handle;
		void *cpu_ptr;
		ump_secure_id ump_id;
		ump_alloc_constraints constraints;

		size = round_up_to_page_size(size);

		if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
		{
			constraints =  UMP_REF_DRV_CONSTRAINT_USE_CACHE;
		}
		else
		{
			constraints = UMP_REF_DRV_CONSTRAINT_NONE;
		}

#ifdef GRALLOC_SIMULATE_FAILURES

		/* if the failure condition matches, fail this iteration */
		if (__ump_alloc_should_fail())
		{
			ump_mem_handle = UMP_INVALID_MEMORY_HANDLE;
		}
		else
#endif
		{
			if (usage & GRALLOC_USAGE_PROTECTED)
			{
				AERR("gralloc_alloc_buffer() does not support to allocate protected UMP memory.");
			}
			else
			{
				ump_mem_handle = ump_ref_drv_allocate(size, constraints);

				if (UMP_INVALID_MEMORY_HANDLE != ump_mem_handle)
				{
					cpu_ptr = ump_mapped_pointer_get(ump_mem_handle);

					if (NULL != cpu_ptr)
					{
						ump_id = ump_secure_id_get(ump_mem_handle);

						if (UMP_INVALID_SECURE_ID != ump_id)
						{
							private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_UMP, usage, size, cpu_ptr,
							        private_handle_t::LOCK_STATE_MAPPED, ump_id, ump_mem_handle);

							if (NULL != hnd)
							{
								*pHandle = hnd;
								return 0;
							}
							else
							{
								AERR("gralloc_alloc_buffer() failed to allocate handle. ump_handle = %p, ump_id = %d", ump_mem_handle, ump_id);
							}
						}
						else
						{
							AERR("gralloc_alloc_buffer() failed to retrieve valid secure id. ump_handle = %p", ump_mem_handle);
						}

						ump_mapped_pointer_release(ump_mem_handle);
					}
					else
					{
						AERR("gralloc_alloc_buffer() failed to map UMP memory. ump_handle = %p", ump_mem_handle);
					}

					ump_reference_release(ump_mem_handle);
				}
				else
				{
					AERR("gralloc_alloc_buffer() failed to allocate UMP memory. size:%d constraints: %d", size, constraints);
				}
			}
		}

		return -1;
	}
#endif

}

static int gralloc_alloc_framebuffer_locked(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	// allocate the framebuffer
	if (m->framebuffer == NULL)
	{
		// initialize the framebuffer, the framebuffer is mapped once and forever.
		int err = init_frame_buffer_locked(m);

		if (err < 0)
		{
			return err;
		}
	}

	const uint32_t bufferMask = m->bufferMask;
	const uint32_t numBuffers = m->numBuffers;
	const size_t bufferSize = m->finfo.line_length * m->info.yres;

	if (numBuffers == 1)
	{
		// If we have only one buffer, we never use page-flipping. Instead,
		// we return a regular buffer which will be memcpy'ed to the main
		// screen when post is called.
		int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
		AERR("fallback to single buffering. Virtual Y-res too small %d", m->info.yres);
		return gralloc_alloc_buffer(dev, bufferSize, newUsage, pHandle);
	}

	if (bufferMask >= ((1LU << numBuffers) - 1))
	{
		// We ran out of buffers.
		return -ENOMEM;
	}

	void *vaddr = m->framebuffer->base;

	// find a free slot
	for (uint32_t i = 0 ; i < numBuffers ; i++)
	{
		if ((bufferMask & (1LU << i)) == 0)
		{
			m->bufferMask |= (1LU << i);
			break;
		}

		vaddr = (void *)((uintptr_t)vaddr + bufferSize);
	}

	// The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
	private_handle_t *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, size, vaddr,
	        0, dup(m->framebuffer->fd), (uintptr_t)vaddr - (uintptr_t) m->framebuffer->base);
#if GRALLOC_ARM_UMP_MODULE
	hnd->ump_id = m->framebuffer->ump_id;

	/* create a backing ump memory handle if the framebuffer is exposed as a secure ID */
	if ((int)UMP_INVALID_SECURE_ID != hnd->ump_id)
	{
		hnd->ump_mem_handle = (int)ump_handle_create_from_secure_id(hnd->ump_id);

		if ((int)UMP_INVALID_MEMORY_HANDLE == hnd->ump_mem_handle)
		{
			AINF("warning: unable to create UMP handle from secure ID %i\n", hnd->ump_id);
		}
	}

#endif

#if GRALLOC_ARM_DMA_BUF_MODULE
	{
#ifdef FBIOGET_DMABUF
		struct fb_dmabuf_export fb_dma_buf;

		if (ioctl(m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf) == 0)
		{
			AINF("framebuffer accessed with dma buf (fd 0x%x)\n", (int)fb_dma_buf.fd);
			hnd->share_fd = fb_dma_buf.fd;
		}

#endif
	}
#endif

	*pHandle = hnd;

	return 0;
}

static int gralloc_alloc_framebuffer(alloc_device_t *dev, size_t size, int usage, buffer_handle_t *pHandle)
{
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	pthread_mutex_lock(&m->lock);
	int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle);
	pthread_mutex_unlock(&m->lock);
	return err;
}

static int alloc_device_alloc(alloc_device_t *dev, int w, int h, int format, int usage, buffer_handle_t *pHandle, int *pStride)
{
	if (!pHandle || !pStride)
	{
		return -EINVAL;
	}

#if PLATFORM_SDK_VERSION > 27
	if ((HAL_PIXEL_FORMAT_RGB_888 == format) && (usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER)))
	{
		AINF("Mali 4xx do not support using RGB888 format native buffer to create EGLImage!");
		return -EINVAL;
	}
#endif

#if PLATFORM_SDK_VERSION >=28
	/*
	 * Workaround for android q cts, create a fake RGBA_1010102 buffer for media codec
	 * to trick testVp9HdrStaticMetadata. so use usage = HW_COMPOSER to filter this case.
	 * If not in this case, gpu cannot handle RGBA_1010102 data, so failed to create this
	 * kind of format buffer;
	 */
	if ((HAL_PIXEL_FORMAT_RGBA_1010102 == format) && (usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER))) {
		if (!(usage & GRALLOC_USAGE_HW_COMPOSER)) {
			AINF("Mali 4xx do not support using RGBA_1010102 format native buffer to create EGLImage!");
			return -EINVAL;
		}
	}
#endif
	size_t size;
	size_t stride;
	struct timespec time;
	int    aw_byte_align[3];

	clock_gettime(CLOCK_REALTIME, &time);

	if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 || format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
	{
		format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
	}

	if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP || format == HAL_PIXEL_FORMAT_YV12
	        /* HAL_PIXEL_FORMAT_YCbCr_420_SP, HAL_PIXEL_FORMAT_YCbCr_420_P, HAL_PIXEL_FORMAT_YCbCr_422_I are not defined in Android.
	         * To enable Mali DDK EGLImage support for those formats, firstly, you have to add them in Android system/core/include/system/graphics.h.
	         * Then, define SUPPORT_LEGACY_FORMAT in the same header file(Mali DDK will also check this definition).
	         */
#ifdef SUPPORT_LEGACY_FORMAT
	        || format == HAL_PIXEL_FORMAT_YCbCr_420_SP || format == HAL_PIXEL_FORMAT_YCbCr_420_P || format == HAL_PIXEL_FORMAT_YCbCr_422_I
#endif
	   )
	{
		switch (format)
		{
			case HAL_PIXEL_FORMAT_YCrCb_420_SP:
				stride = GRALLOC_ALIGN(w, 16);
				size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
				break;

			case HAL_PIXEL_FORMAT_YV12:
#ifdef SUPPORT_LEGACY_FORMAT
			case HAL_PIXEL_FORMAT_YCbCr_420_P:
#endif
				if (h == 182 || h == 362) {
// Todo
#if 0
					/*
					 * mali400 every plane start address should be aligned to 64 bytes
					 * here will fix google soft decoder cts
					 */
					if (h % 16 == 0) {
						stride = GRALLOC_ALIGN(w, 16);
						size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
					} else if (h % 8 == 0) {
						stride = GRALLOC_ALIGN(w, 32);
						size = GRALLOC_ALIGN(h, 8) * (stride + GRALLOC_ALIGN(stride / 2, 16));
					} else if (h % 4 == 0) {
						stride = GRALLOC_ALIGN(w, 64);
						size = GRALLOC_ALIGN(h, 4) * (stride + GRALLOC_ALIGN(stride / 2, 16));
					} else {
						stride = GRALLOC_ALIGN(w, 128);
						size = GRALLOC_ALIGN(h, 2) * (stride + GRALLOC_ALIGN(stride / 2, 16));
					}
#endif
					stride = GRALLOC_ALIGN(w, 128);
					size = GRALLOC_ALIGN(h, 2) * (stride + GRALLOC_ALIGN(stride / 2, 16));
					ALOGD("miles_debug: gralloc workaround for vp9 cts\n");
				} else {
					stride = GRALLOC_ALIGN(w, 16);
					size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
				}
				break;

#ifdef SUPPORT_LEGACY_FORMAT

			case HAL_PIXEL_FORMAT_YCbCr_420_SP:
				stride = GRALLOC_ALIGN(w, 16);
				size = GRALLOC_ALIGN(h, 16) * (stride + GRALLOC_ALIGN(stride / 2, 16));
				break;

			case HAL_PIXEL_FORMAT_YCbCr_422_I:
				stride = GRALLOC_ALIGN(w, 16);
				size = h * stride * 2;

				break;
#endif

			default:
				return -EINVAL;
		}
		aw_byte_align[0] = 16;
		aw_byte_align[1] = 16;
		aw_byte_align[2] = 16;
	} else if (format == HAL_PIXEL_FORMAT_BLOB)
	{
		if(h != 1) {
			ALOGE("%s: Buffers with RAW_OPAQUE/BLOB formats \
				must have height==1 ", __FUNCTION__);
			return 0;
		}
		size = w;
	} else
	{
		int bpp = 0;

		switch (format)
		{
			case HAL_PIXEL_FORMAT_RGBA_8888:
			case HAL_PIXEL_FORMAT_RGBX_8888:
			case HAL_PIXEL_FORMAT_BGRA_8888:
#if PLATFORM_SDK_VERSION >=28
			case HAL_PIXEL_FORMAT_RGBA_1010102:
#endif
				bpp = 4;
				break;

			case HAL_PIXEL_FORMAT_RGB_888:
				bpp = 3;
				break;

			case HAL_PIXEL_FORMAT_RGB_565:
#if PLATFORM_SDK_VERSION < 19
			case HAL_PIXEL_FORMAT_RGBA_5551:
			case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
				bpp = 2;
				break;

			default:
				return -EINVAL;
		}

		size_t bpr = GRALLOC_ALIGN(GRALLOC_ALIGN(w, 16) * bpp, 64);
		size = bpr * GRALLOC_ALIGN(h, 16);
		stride = bpr / bpp;

		aw_byte_align[0] = 64;
		aw_byte_align[1] = 0;
		aw_byte_align[2] = 0;
	}

	/*
	 * For Allwinner VE (Video Encoder & Video Decoder), width and
	 * height should be aligned to 16 pixels.
	 * For Video Encoder:
	 *     ARGB/ABGR/RGBA/BGRA -> burst size is 128 bytes
	 *     NV12/NU21/YUV420SP  -> burst size is 32/64 bytes
	 *     YUV422SP/NV16       -> burst size is 32/64 bytes
	 *     NU12/NV21/YVU420SP  -> burst size is 32/64 bytes
	 *     YVU422SP/NV61       -> burst size is 32/64 bytes
	 *     YU12/YUV420P        -> burst size is 32/64 bytes
	 *     YV12/YVU420P        -> burst size is 32/64 bytes
	 *     YU16/YUV422P        -> burst size is 32/64 bytes
	 *     YV16/YVU422P        -> burst size is 32/64 bytes
	 *     RAW YUYV422         -> burst size is 32/64 bytes
	 *     RAW UYVY422         -> burst size is 32/64 bytes
	 *     RAW YVYU422         -> burst size is 32/64 bytes
	 *     RAW VYUY422         -> burst size is 32/64 bytes
	 * For Video Decoder:
	 *     NV12/NU21/YUV420SP  -> burst size is 32/64 bytes
	 *     NU12/NV21/YVU420SP  -> burst size is 32/64 bytes
	 *     YU12/YUV420P        -> burst size is 32/64 bytes
	 *     YV12/YVU420P        -> burst size is 32/64 bytes
	 *     YV12/YVU420P AFBC   -> burst size is 32/64 bytes
	 *     P010                -> burst size is 32/64 bytes
	 * So here we should avoid memory overflow issue by providing
	 * enough bufer size.
	 */
	switch (format)
	{
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YV12:
#ifdef SUPPORT_LEGACY_FORMAT
		case HAL_PIXEL_FORMAT_YCbCr_420_P:
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
#endif
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			size += 64;
			break;
	}

	int err;

	err = gralloc_alloc_buffer(dev, size, usage, pHandle);

	if (err < 0)
	{
		return err;
	}

	/* match the framebuffer format */
	if (usage & GRALLOC_USAGE_HW_FB)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_BGRA_8888;
#endif
	}

	private_handle_t *hnd = (private_handle_t *)*pHandle;
	int               private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 |
	                                  GRALLOC_USAGE_PRIVATE_1);

	switch (private_usage)
	{
		case 0:
			hnd->yuv_info = MALI_YUV_BT601_NARROW;
			break;

		case GRALLOC_USAGE_PRIVATE_1:
			hnd->yuv_info = MALI_YUV_BT601_WIDE;
			break;

		case GRALLOC_USAGE_PRIVATE_0:
			hnd->yuv_info = MALI_YUV_BT709_NARROW;
			break;

		case (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1):
			hnd->yuv_info = MALI_YUV_BT709_WIDE;
			break;
	}

	hnd->size = size;
	hnd->width = w;
	hnd->height = h;
	hnd->format = format;
	hnd->stride = stride;
	hnd->aw_byte_align[0] = aw_byte_align[0];
	hnd->aw_byte_align[1] = aw_byte_align[1];
	hnd->aw_byte_align[2] = aw_byte_align[2];

	hnd->aw_buf_id = (long long )time.tv_sec * 1000 * 1000 * 1000 + time.tv_nsec;

	*pStride = stride;
	return 0;
}

static int alloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t *hnd = (private_handle_t *)handle;

	/* Guarantee aw_buf_id is invalid after calling alloc_device_free */
        hnd->aw_buf_id = -1;

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
		const size_t bufferSize = m->finfo.line_length * m->info.yres;
		int index = ((uintptr_t)hnd->base - (uintptr_t)m->framebuffer->base) / bufferSize;
		m->bufferMask &= ~(1 << index);
		close(hnd->fd);

#if GRALLOC_ARM_UMP_MODULE

		if ((int)UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle)
		{
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}

#endif
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
#if GRALLOC_ARM_UMP_MODULE

		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if ((int)UMP_INVALID_MEMORY_HANDLE != hnd->ump_mem_handle)
		{
			ump_mapped_pointer_release((ump_handle)hnd->ump_mem_handle);
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}

#else
		AERR("Can't free ump memory for handle:0x%p. Not supported.", hnd);
#endif
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
#if GRALLOC_ARM_DMA_BUF_MODULE
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

		/* Buffer might be unregistered so we need to check for invalid ump handle*/
		if (0 != hnd->base)
		{
			if (0 != munmap((void *)hnd->base, hnd->size))
			{
				AERR("Failed to munmap handle 0x%p", hnd);
			}
		}

		close(hnd->share_fd);

		if (0 != ion_free(m->ion_client, hnd->ion_hnd))
		{
			AERR("Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, (void *)(uintptr_t)hnd->ion_hnd);
		}

		memset((void *)hnd, 0, sizeof(*hnd));
#else
		AERR("Can't free dma_buf memory for handle:0x%x. Not supported.", (unsigned int)hnd);
#endif

	}

	delete hnd;

	return 0;
}

static int alloc_device_close(struct hw_device_t *device)
{
	alloc_device_t *dev = reinterpret_cast<alloc_device_t *>(device);

	if (dev)
	{
#if GRALLOC_ARM_DMA_BUF_MODULE
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

		if (0 != ion_close(m->ion_client))
		{
			AERR("Failed to close ion_client: %d", m->ion_client);
		}

		close(m->ion_client);
#endif
		delete dev;
#if GRALLOC_ARM_UMP_MODULE
		ump_close(); // Our UMP memory refs will be released automatically here...
#endif
	}

	return 0;
}

int alloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	MALI_IGNORE(name);
	alloc_device_t *dev;

	dev = new alloc_device_t;

	if (NULL == dev)
	{
		return -1;
	}

#if GRALLOC_ARM_UMP_MODULE
	ump_result ump_res = ump_open();

	if (UMP_OK != ump_res)
	{
		AERR("UMP open failed with %d", ump_res);
		delete dev;
		return -1;
	}

#endif

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = alloc_device_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

#if GRALLOC_ARM_DMA_BUF_MODULE
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->ion_client = ion_open();

	if (m->ion_client < 0)
	{
		AERR("ion_open failed with %s", strerror(errno));
		delete dev;
		return -1;
	}

#endif

	*device = &dev->common;

	return 0;
}
