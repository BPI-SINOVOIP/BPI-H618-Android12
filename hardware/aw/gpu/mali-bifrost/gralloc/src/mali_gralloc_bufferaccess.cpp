/*
 * Copyright (C) 2016-2019 ARM Limited. All rights reserved.
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
#include <inttypes.h>
#include <inttypes.h>
#include <sync/sync.h>

#if GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#elif GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 2
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/common/1.0/types.h>
using android::hardware::graphics::mapper::V2_0::Error;
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "mali_gralloc_ion.h"
#include "gralloc_helper.h"
#include "format_info.h"

#if GRALLOC_USE_LEGACY_LOCK == 1
#include "legacy/buffer_access.h"
#endif


enum tx_direction
{
	TX_NONE = 0,
	TX_TO_DEVICE,
	TX_FROM_DEVICE,
	TX_BOTH,
};


static enum tx_direction get_tx_direction(const uint64_t usage)
{
	const bool read = (usage & GRALLOC_USAGE_SW_READ_MASK) ? true : false;
	const bool write = (usage & GRALLOC_USAGE_SW_WRITE_MASK) ? true : false;
	enum tx_direction dir = TX_NONE;

	if (read && write)
	{
		dir = TX_BOTH;
	}
	else if (write)
	{
		dir = TX_TO_DEVICE;
	}
	else if (read)
	{
		dir = TX_FROM_DEVICE;
	}

	return dir;
}

static void buffer_sync(private_handle_t * const hnd,
                        const enum tx_direction direction)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		if (direction != TX_NONE)
		{
			hnd->cpu_read = (direction == TX_FROM_DEVICE || direction == TX_BOTH) ? 1 : 0;
			hnd->cpu_write = (direction == TX_TO_DEVICE || direction == TX_BOTH) ? 1 : 0;

			const int status = mali_gralloc_ion_sync_start(hnd,
			                                               hnd->cpu_read ? true : false,
			                                               hnd->cpu_write ? true : false);
			if (status < 0)
			{
				return;
			}
		}
		else if (hnd->cpu_read || hnd->cpu_write)
		{
			const int status = mali_gralloc_ion_sync_end(hnd,
			                                             hnd->cpu_read ? true : false,
			                                             hnd->cpu_write ? true : false);
			if (status < 0)
			{
				return;
			}

			hnd->cpu_read = 0;
			hnd->cpu_write = 0;
		}
	}
}


/*
 *  Validates input parameters of lock request.
 *
 * @param buffer   [in]    The buffer to lock.
 * @param l        [in]    Access region left offset (in pixels).
 * @param t        [in]    Access region top offset (in pixels).
 * @param w        [in]    Access region requested width (in pixels).
 * @param h        [in]    Access region requested height (in pixels).
 * @param usage    [in]    Lock request (producer and consumer combined) usage.
 *
 * @return 0,for valid input parameters;
 *         -EINVAL, for erroneous input parameters
 */
int validate_lock_input_parameters(const buffer_handle_t buffer, const int l,
                                   const int t, const int w, const int h,
                                   uint64_t usage)
{
	bool is_registered_process = false;
	const int lock_pid = getpid();
	const private_handle_t * const hnd = (private_handle_t *)buffer;

	if ((l < 0) || (t < 0) || (w < 0) || (h < 0))
	{
		AERR("Negative values for access region (l = %d t = %d w = %d and "
		     "h = %d) in buffer lock request are invalid. Locking PID:%d",
		      l, t, w, h, lock_pid);
		return -EINVAL;
	}

	/* Test overflow conditions on access region parameters */
	if (((l + w) < 0) || ((t + h) < 0))
	{
		AERR("Encountered overflow with access region (l = %d t = %d w = %d and"
		     " h = %d) in buffer lock request. Locking PID:%d",
		       l, t, w, h, lock_pid);
		return -EINVAL;
	}

	/* Region of interest shall be inside the allocated buffer */
	if (((t + h) > hnd->height)  || ((l + w) > hnd->width))
	{
		AERR("Buffer lock access region (l = %d t = %d w = %d "
		     "and h = %d) is outside allocated buffer (width = %d and height = %d)"
		     " Locking PID:%d", l, t, w, h, hnd->width, hnd->height, lock_pid);
		return -EINVAL;
	}

	/* Locking process should have a valid buffer virtual address. A process
	 * will have a valid buffer virtual address if it is the allocating
	 * process or it retained / registered a cloned buffer handle
	 */
	if ((hnd->allocating_pid == lock_pid) || (hnd->remote_pid == lock_pid))
	{
		is_registered_process = true;
	}

	if ((is_registered_process == false) || (hnd->base == NULL))
	{
#if GRALLOC_VERSION_MAJOR == 0
		AERR("The buffer must be registered before lock request");
#else
		AERR("The buffer must be retained before lock request");
#endif
		return -EINVAL;
	}

	/* Reject lock requests for AFBC (compressed format) enabled buffers */
	if ((hnd->alloc_format & MALI_GRALLOC_INTFMT_EXT_MASK) != 0)
	{
		AERR("Lock is not supported for AFBC enabled buffers."
		     "Internal Format:0x%" PRIx64, hnd->alloc_format);

#if GRALLOC_VERSION_MAJOR == 0
		return -EINVAL;
#elif GRALLOC_VERSION_MAJOR == 1
		return GRALLOC1_ERROR_UNSUPPORTED;
#elif GRALLOC_VERSION_MAJOR >= 2
		return static_cast<int32_t>(Error::UNSUPPORTED);
#endif
	}

#if GRALLOC_VERSION_MAJOR == 0
	/* Check if the buffer was created with a usage mask incompatible with the
	 * requested usage flags. For compatibility, requested lock usage can be a
	 * subset of allocation usage
	 */
	if ((usage & (hnd->producer_usage | hnd->consumer_usage)) == 0)
	{
		AERR("Buffer lock usage:0x%" PRIx64" does not match allocation usage:0x%"
		      PRIx64, usage, (hnd->producer_usage | hnd->consumer_usage));
		return -EINVAL;
	}
#else
	/* Producer and consumer usage is verified in gralloc1 specific code. */
	GRALLOC_UNUSED(usage);
#endif

	return 0;
}


/*
 *  Locks the given buffer for the specified CPU usage.
 *
 * @param m        [in]    Gralloc module.
 * @param buffer   [in]    The buffer to lock.
 * @param usage    [in]    Producer and consumer combined usage.
 * @param l        [in]    Access region left offset (in pixels).
 * @param t        [in]    Access region top offset (in pixels).
 * @param w        [in]    Access region requested width (in pixels).
 * @param h        [in]    Access region requested height (in pixels).
 * @param vaddr    [out]   To be filled with a CPU-accessible pointer to
 *                         the buffer data for CPU usage.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 *
 * @Note:  There is no way to ascertain whether buffer data is valid or not (for
 *         example, establishing if the h/w needs to finish rendering or if CPU
 *         caches need to be synchronized).
 *
 * @Note:  Locking a buffer simultaneously for write or read/write leaves the
 *         buffer's content in an indeterminate state.
 */
int mali_gralloc_lock(const mali_gralloc_module * const m, buffer_handle_t buffer,
                      uint64_t usage, int l, int t, int w, int h, void **vaddr)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_lock(m, buffer, usage, l, t, w, h, vaddr);
#endif

	int status;
	GRALLOC_UNUSED(m);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Locking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}

	/* Validate input parameters for lock request */
	status = validate_lock_input_parameters(buffer, l, t, w, h, usage);
	if (status != 0)
	{
		return status;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;

	/* HAL_PIXEL_FORMAT_YCbCr_*_888 buffers 'must' be locked with lock_ycbcr() */
	if ((hnd->req_format == HAL_PIXEL_FORMAT_YCbCr_420_888) ||
	    (hnd->req_format == HAL_PIXEL_FORMAT_YCbCr_422_888) ||
	    (hnd->req_format == HAL_PIXEL_FORMAT_YCbCr_444_888))
	{
		AERR("Buffers with format YCbCr_*_888 must be locked using "
		     "(*lock_ycbcr). Requested format is:0x%x", hnd->req_format);
		return -EINVAL;
	}

	/* YUV compatible formats 'should' be locked with lock_ycbcr() */
	const int32_t format_idx = get_format_index(hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
	if (format_idx == -1)
	{
		AERR("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].is_yuv == true)
	{
#if GRALLOC_VERSION_MAJOR == 0
		AWAR("Buffers with YUV compatible formats should be locked using "
		     "(*lock_ycbcr). Requested format is:0x%x", hnd->req_format);
#elif GRALLOC_VERSION_MAJOR == 1
		AWAR("Buffers with YUV compatible formats should be locked using "
		     "GRALLOC1_FUNCTION_LOCK_FLEX. Requested format is:0x%x", hnd->req_format);
#endif
	}

	/* Populate CPU-accessible pointer when requested for CPU usage */
	if ((usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK)) != 0)
	{
		if (vaddr == NULL)
		{
			return -EINVAL;
		}
		*vaddr = (void *)hnd->base;

		buffer_sync(hnd, get_tx_direction(usage));
	}

	return 0;
}

/*
 *  Locks the given ycbcr buffer for the specified CPU usage. This function can
 *  only be used for buffers with "8 bit sample depth"
 *
 * @param m        [in]    Gralloc module.
 * @param buffer   [in]    The buffer to lock.
 * @param usage    [in]    Producer and consumer combined usage.
 * @param l        [in]    Access region left offset (in pixels).
 * @param t        [in]    Access region top offset (in pixels).
 * @param w        [in]    Access region requested width (in pixels).
 * @param h        [in]    Access region requested height (in pixels).
 * @param ycbcr    [out]   Describes YCbCr formats for consumption by applications.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 *
 * @Note:  There is no way to ascertain whether buffer data is valid or not (for
 *         example, establishing if the h/w needs to finish rendering or if CPU
 *         caches need to be synchronized).
 *
 * @Note:  Locking a buffer simultaneously for write or read/write leaves the
 *         buffer's content in an indeterminate state.
 *
 */
int mali_gralloc_lock_ycbcr(const mali_gralloc_module *m,
                            const buffer_handle_t buffer,
                            const uint64_t usage, const int l, const int t,
                            const int w, const int h, android_ycbcr *ycbcr)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_lock_ycbcr(m, buffer, usage, l, t, w, h, ycbcr);
#endif

	GRALLOC_UNUSED(m);
	int status;

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Locking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}

	private_handle_t * const hnd = (private_handle_t *)buffer;
	const uint32_t base_format = hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/* Validate input parameters for lock request */
	status = validate_lock_input_parameters(buffer, l, t, w, h, usage);
	if (status != 0)
	{
		return status;
	}

	const int32_t format_idx = get_format_index(base_format);
	if (format_idx == -1)
	{
		AERR("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].is_yuv != true)
	{
		AERR("Buffer format:0x%" PRIx64 " is not a YUV compatible format", hnd->alloc_format);
		return -EINVAL;
	}

	if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
	{
		if (NULL == ycbcr)
		{
			return -EINVAL;
		}

		ycbcr->y = (char *)hnd->base;
		ycbcr->ystride = hnd->plane_info[0].byte_stride;

		switch (base_format)
		{
		case MALI_GRALLOC_FORMAT_INTERNAL_Y8:
		case MALI_GRALLOC_FORMAT_INTERNAL_Y16:
			/* No UV plane */
			ycbcr->cstride = 0;
			ycbcr->cb = NULL;
			ycbcr->cr = NULL;
			ycbcr->chroma_step = 0;
		break;

		case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
			/* UV plane */
			ycbcr->cstride = hnd->plane_info[1].byte_stride;
			ycbcr->cb = (char *)hnd->base + hnd->plane_info[1].offset;
			ycbcr->cr = (char *)ycbcr->cb + 1;
			ycbcr->chroma_step = 2;
			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
			/* VU plane */
			ycbcr->cstride = hnd->plane_info[1].byte_stride;
			ycbcr->cr = (char *)hnd->base + hnd->plane_info[1].offset;
			ycbcr->cb = (char *)ycbcr->cr + 1;
			ycbcr->chroma_step = 2;
			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
			/* V plane, U plane */
			ycbcr->cstride = hnd->plane_info[1].byte_stride;
			ycbcr->cr = (char *)hnd->base + hnd->plane_info[1].offset;
			ycbcr->cb = (char *)hnd->base + hnd->plane_info[2].offset;
			ycbcr->chroma_step = 1;
			break;

		default:
			AERR("Buffer:%p of format 0x%" PRIx64 "can't be represented in"
			     " android_ycbcr format", hnd, hnd->alloc_format);
			return -EINVAL;
		}

		buffer_sync(hnd, get_tx_direction(usage));
	}
	else
	{
		ycbcr->y = NULL;
		ycbcr->cb = NULL;
		ycbcr->cr = NULL;
		ycbcr->ystride = 0;
		ycbcr->cstride = 0;
		ycbcr->chroma_step = 0;
	}

	/* Reserved parameters should be set to 0 by gralloc's (*lock_ycbcr)()*/
	memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));

	return 0;
}

/*
 *  Unlocks the given buffer.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer to unlock.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 *
 * Note: unlocking a buffer which is not locked results in an unexpected behaviour.
 *       Though it is possible to create a state machine to track the buffer state to
 *       recognize erroneous conditions, it is expected of client to adhere to API
 *       call sequence
 */
int mali_gralloc_unlock(const mali_gralloc_module *m, buffer_handle_t buffer)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_unlock(m, buffer);
#endif

	GRALLOC_UNUSED(m);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Unlocking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}

	private_handle_t *hnd = (private_handle_t *)buffer;
	buffer_sync(hnd, TX_NONE);

	return 0;
}

#if GRALLOC_VERSION_MAJOR == 1
/*
 *  Returns the number of flex layout planes which are needed to represent the
 *  given buffer.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer handle for which the number of planes should be queried
 * @param num_planes  [out]  The number of flex planes required to describe the given buffer
 *
 * @return GRALLOC1_ERROR_NONE         The buffer's format can be represented in flex layout
 *         GRALLOC1_ERROR_UNSUPPORTED - The buffer's format can't be represented in flex layout
 */
int mali_gralloc_get_num_flex_planes(const mali_gralloc_module *const m,
                                     const buffer_handle_t buffer,
                                     uint32_t * const num_planes)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_get_num_flex_planes(m, buffer, num_planes);
#endif

	GRALLOC_UNUSED(m);

	private_handle_t *hnd = (private_handle_t *)buffer;
	const uint32_t base_format = hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	if ((hnd->alloc_format & MALI_GRALLOC_INTFMT_EXT_MASK) != 0)
	{
		AERR("AFBC enabled buffers can't be represented in flex layout."
		     "Internal Format:0x%" PRIx64, hnd->alloc_format);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	const int32_t format_idx = get_format_index(base_format);
	if (format_idx == -1)
	{
		AERR("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].flex != true)
	{
		AERR("Format 0x%" PRIx64 " of %p can't be represented in flex", hnd->alloc_format, hnd);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	*num_planes = formats[format_idx].ncmp;

	return GRALLOC1_ERROR_NONE;
}
#endif

/*
 *  Locks the given buffer asynchronously for the specified CPU usage.
 *
 * @param m        [in]    Gralloc1 module.
 * @param buffer   [in]    The buffer to lock.
 * @param usage    [in]    Producer and consumer combined usage.
 * @param l        [in]    Access region left offset (in bytes).
 * @param t        [in]    Access region top offset (in bytes).
 * @param h        [in]    Access region requested height (in bytes).
 * @param w        [in]    Access region requested width (in bytes).
 * @param vaddr    [out]   To be filled with a CPU-accessible pointer to
 *                         the buffer data for CPU usage.
 * @param fence_fd [in]    Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 */
int mali_gralloc_lock_async(const mali_gralloc_module *m, buffer_handle_t buffer,
                            uint64_t usage, int l, int t, int w, int h,
                            void **vaddr, int32_t fence_fd)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_lock_async(m, buffer, usage, l, t, w, h, vaddr, fence_fd);
#endif

	if (fence_fd >= 0)
	{
		sync_wait(fence_fd, -1);
		close(fence_fd);
	}

	return mali_gralloc_lock(m, buffer, usage, l, t, w, h, vaddr);
}

/*
 *  Locks the given ycbcr buffer for the specified CPU usage asynchronously.
 *  This function can only be used for buffers with "8 bit sample depth"
 *
 * @param m        [in]    Gralloc module.
 * @param buffer   [in]    The buffer to lock.
 * @param usage    [in]    Producer and consumer combined usage.
 * @param l        [in]    Access region left offset (in pixels).
 * @param t        [in]    Access region top offset (in pixels).
 * @param w        [in]    Access region requested width (in pixels).
 * @param h        [in]    Access region requested height (in pixels).
 * @param ycbcr    [out]   Describes YCbCr formats for consumption by applications.
 * @param fence_fd [in]    Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 */
int mali_gralloc_lock_ycbcr_async(const mali_gralloc_module *m,
                                  const buffer_handle_t buffer, const uint64_t usage,
                                  const int l, const int t, const int w,
                                  const int h, android_ycbcr *ycbcr,
                                  const int32_t fence_fd)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_lock_ycbcr_async(m, buffer, usage, l, t, w, h, ycbcr, fence_fd);
#endif

	if (fence_fd >= 0)
	{
		sync_wait(fence_fd, -1);
		close(fence_fd);
	}

	return mali_gralloc_lock_ycbcr(m, buffer, usage, l, t, w, h, ycbcr);
}

#if GRALLOC_VERSION_MAJOR == 1

/*
 *  Sets Android flex layout parameters.
 *
 * @param top_left            [in]  Pointer to the first byte of the top-left
 *                                  pixel of the plane.
 * @param component           [in]  Plane's flex format (YUVA/RGBA).
 * @param bits_per_component  [in]  Bits allocated for the component in each pixel.
 * @param bits_used           [in]  Number of the most significant bits used in
 *                                  the format for this component.
 * @param h_increment         [in]  Horizontal increment, in bytes, in plane to
 *                                  traverse to next horizontal pixel
 * @param v_increment         [in]  Vertical increment, in bytes, in plane to
 *                                  traverse to next vertical pixel
 * @param h_subsampling       [in]  Horizontal subsampling. Must be a positive power of 2.
 * @param v_subsampling       [in]  Vertical subsampling. Must be a positive power of 2.
 * @param plane               [out] Flex plane layout, to be composed.
 *
 */
static void set_flex_plane_params(uint8_t * const top_left,
                                  const android_flex_component_t component,
                                  const int32_t bits_per_component,
                                  const int32_t bits_used,
                                  const int32_t h_increment,
                                  const int32_t v_increment,
                                  const int32_t h_subsampling,
                                  const int32_t v_subsampling,
                                  android_flex_plane_t * const plane)
{
	plane->top_left = top_left;
	plane->component = component;
	plane->bits_per_component = bits_per_component;
	plane->bits_used = bits_used;
	plane->h_increment = h_increment;
	plane->v_increment = v_increment;
	plane->h_subsampling = h_subsampling;
	plane->v_subsampling = v_subsampling;

	return;
}

/*
 *  Locks the Gralloc 1.0 buffer, for the specified CPU usage, asynchronously.
 *  This function can be called on any format but populates layout parameters
 *  only for formats compatible with Android Flex Format.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer to lock.
 * @param usage       [in]   Producer and consumer combined usage.
 * @param l           [in]   Access region left offset (in pixels).
 * @param t           [in]   Access region top offset (in pixels).
 * @param w           [in]   Access region requested width (in pixels).
 * @param h           [in]   Access region requested height (in pixels).
 * @param flex_layout [out]  Describes flex YUV format for consumption by applications.
 * @param fence_fd    [in]   Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 */
int mali_gralloc_lock_flex_async(const mali_gralloc_module *m,
                                 const buffer_handle_t buffer,
                                 const uint64_t usage, const int l, const int t,
                                 const int w, const  int h,
                                 struct android_flex_layout * const flex_layout,
                                 const int32_t fence_fd)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_lock_flex_async(m, buffer, usage, l, t, w, h, flex_layout, fence_fd);
#endif

	GRALLOC_UNUSED(m);

	if (fence_fd >= 0)
	{
		sync_wait(fence_fd, -1);
		close(fence_fd);
	}

	private_handle_t * const hnd = (private_handle_t *)buffer;
	const uint32_t base_format = hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/* Validate input parameters for lock request */
	int status = validate_lock_input_parameters(buffer, l, t, w, h, usage);
	if (status != 0)
	{
		return status;
	}

	const int32_t format_idx = get_format_index(base_format);
	if (format_idx == -1)
	{
		AERR("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].flex != true)
	{
		AERR("Format 0x%" PRIx64 " of %p can't be represented in flex", hnd->alloc_format, hnd);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	flex_layout->num_planes = formats[format_idx].ncmp;
	switch (base_format)
	{
	case MALI_GRALLOC_FORMAT_INTERNAL_Y8:
		flex_layout->format = FLEX_FORMAT_Y;
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_Y16:
		flex_layout->format = FLEX_FORMAT_Y;
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 16, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
		/* Y:UV 4:2:0 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
		/* Y:VU 4:2:0 ordering. The flex format plane order must still
		 * follow YCbCr order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
		/* Y:V:U 4:2:0 . The flex format plane order must still follow YCbCr
		 * order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[2].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 1,
		                      hnd->plane_info[2].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cr, 8, 8, 1,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_P010:
		/* Y:UV 4:2:0 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 2,
		                      FLEX_COMPONENT_Cr, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_P210:
		/* Y:UV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 2,
		                      FLEX_COMPONENT_Cr, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_YUV422_8BIT:
		/* YUYV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_Cb, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_Cr, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);

		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV16:
		/* Y:UV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);

		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_Y210:
		/* YUYV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_Cb, 16, 10, 8,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 6, FLEX_COMPONENT_Cr, 16, 10, 8,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);

		break;

#if PLATFORM_SDK_VERSION >= 26
	/* 64-bit format that has 16-bit R, G, B, and A components, in that order */
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616:
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_G, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 4, FLEX_COMPONENT_B, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 6, FLEX_COMPONENT_A, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;
#endif

	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_8888:
		/* 32-bit format that has 8-bit R, G, B, and A components, in that order */
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_A, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_RGBX_8888:
		/* 32-bit format that has 8-bit R, G, B, and unused components, in that order */
		flex_layout->format = FLEX_FORMAT_RGB;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_888:
		/* 24-bit format that has 8-bit R, G, and B components, in that order */
		flex_layout->format = FLEX_FORMAT_RGB;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888:
		/* 32-bit format that has 8-bit B, G, R, and A components, in that order.
		 * The flex format plane order must still follow FLEX_FORMAT_RGBA
		 * order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_A, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;

	default:
		AERR("Can't lock buffer %p: format 0x%" PRIx64 " not handled", hnd, hnd->alloc_format);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	buffer_sync(hnd, get_tx_direction(usage));

	return GRALLOC1_ERROR_NONE;
}
#endif

/*
 *  Unlocks the buffer asynchronously.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer to unlock.
 * @param fence_fd    [out]  Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 *
 * Note: unlocking a buffer which is not locked results in an unexpected behaviour.
 *       Though it is possible to create a state machine to track the buffer state to
 *       recognize erroneous conditions, it is expected of client to adhere to API
 *       call sequence
 */
int mali_gralloc_unlock_async(const mali_gralloc_module *m, buffer_handle_t buffer,
                              int32_t * const fence_fd)
{
	/* Legacy support for old buffer size/stride calculations. */
#if GRALLOC_USE_LEGACY_LOCK == 1
	return legacy::mali_gralloc_unlock_async(m, buffer, fence_fd);
#endif

	*fence_fd = -1;

	if (mali_gralloc_unlock(m, buffer) < 0)
	{
		return -EINVAL;
	}

	return 0;
}
