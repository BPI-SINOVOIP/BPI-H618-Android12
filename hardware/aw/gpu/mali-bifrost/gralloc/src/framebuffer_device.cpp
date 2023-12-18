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
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <system/window.h>
#include <log/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/fb.h>

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#include <GLES/gl.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#include <GLES/gl.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "gralloc_vsync.h"
#include "mali_gralloc_bufferaccess.h"
#include "mali_gralloc_ion.h"
#include "gralloc_buffer_priv.h"

#define STANDARD_LINUX_SCREEN

/* Number of buffers for page flipping */
#define NUM_BUFFERS NUM_FB_BUFFERS

enum
{
	PAGE_FLIP = 0x00000001,
};

#if GRALLOC_VERSION_MAJOR <= 1
static int fb_set_swap_interval(struct framebuffer_device_t *dev, int interval)
{
	if (interval < dev->minSwapInterval)
	{
		interval = dev->minSwapInterval;
	}
	else if (interval > dev->maxSwapInterval)
	{
		interval = dev->maxSwapInterval;
	}

	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
	m->swapInterval = interval;

	if (0 == interval)
	{
		gralloc_vsync_disable(dev);
	}
	else
	{
		gralloc_vsync_enable(dev);
	}

	return 0;
}
#endif

#if GRALLOC_VERSION_MAJOR <= 1
static int fb_post(struct framebuffer_device_t *dev, buffer_handle_t buffer)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(buffer);
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	if (m->currentBuffer)
	{
		mali_gralloc_unlock(m, m->currentBuffer);
		m->currentBuffer = 0;
	}

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
#if GRALLOC_USE_LEGACY_LOCK != 1
		/* Buffer is being locked for non-cpu usage, hence no need to populate a valid cpu address parameter */
		mali_gralloc_lock(m, buffer, private_module_t::PRIV_USAGE_LOCKED_FOR_POST,
		                             0, 0, 0, 0, NULL);
#else
		mali_gralloc_lock(m, buffer, private_module_t::PRIV_USAGE_LOCKED_FOR_POST, -1, -1, -1, -1, NULL);
#endif
		int interrupt;
		m->info.activate = FB_ACTIVATE_VBL;
		m->info.yoffset = hnd->offset / m->finfo.line_length;

#ifdef STANDARD_LINUX_SCREEN

		if (ioctl(m->framebuffer->fd, FBIOPAN_DISPLAY, &m->info) == -1)
		{
			AERR("FBIOPAN_DISPLAY failed for fd: %d", m->framebuffer->fd);
			mali_gralloc_unlock(m, buffer);
			return -errno;
		}

#else /*Standard Android way*/

		if (ioctl(m->framebuffer->fd, FBIOPUT_VSCREENINFO, &m->info) == -1)
		{
			AERR("FBIOPUT_VSCREENINFO failed for fd: %d", m->framebuffer->fd);
			mali_gralloc_unlock(m, buffer);
			return -errno;
		}

#endif

		if (0 != gralloc_wait_for_vsync(dev))
		{
			AERR("Gralloc wait for vsync failed for fd: %d", m->framebuffer->fd);
			mali_gralloc_unlock(m, buffer);
			return -errno;
		}

		m->currentBuffer = buffer;
	}
	else
	{
		void *fb_vaddr;
		void *buffer_vaddr;

#if GRALLOC_USE_LEGACY_LOCK != 1
		mali_gralloc_lock(m, m->framebuffer, GRALLOC_USAGE_SW_WRITE_RARELY, 0, 0, 0, 0, &fb_vaddr);
		mali_gralloc_lock(m, buffer, GRALLOC_USAGE_SW_READ_RARELY, 0, 0, 0, 0, &buffer_vaddr);
#else
		mali_gralloc_lock(m, m->framebuffer, GRALLOC_USAGE_SW_WRITE_RARELY, -1, -1, -1, -1, &fb_vaddr);
		mali_gralloc_lock(m, buffer, GRALLOC_USAGE_SW_READ_RARELY, -1, -1, -1, -1, &buffer_vaddr);
#endif
		// If buffer's alignment match framebuffer alignment we can do a direct copy.
		// If not we must fallback to do an aligned copy of each line.
		if (hnd->byte_stride == (int)m->finfo.line_length)
		{
			memcpy(fb_vaddr, buffer_vaddr, m->finfo.line_length * m->info.yres);
		}
		else
		{
			uintptr_t fb_offset = 0;
			uintptr_t buffer_offset = 0;
			unsigned int i;

			for (i = 0; i < m->info.yres; i++)
			{
				memcpy((void *)((uintptr_t)fb_vaddr + fb_offset), (void *)((uintptr_t)buffer_vaddr + buffer_offset),
				       m->finfo.line_length);

				fb_offset += m->finfo.line_length;
				buffer_offset += hnd->byte_stride;
			}
		}

		mali_gralloc_unlock(m, buffer);
		mali_gralloc_unlock(m, m->framebuffer);
	}

	return 0;
}
#endif

static int init_frame_buffer_locked(struct private_module_t *module)
{
	if (module->framebuffer)
	{
		return 0; // Nothing to do, already initialized
	}

	char const *const device_template[] = { "/dev/graphics/fb%u", "/dev/fb%u", NULL };

	int fd = -1;
	int i = 0;
	char name[64];

	while ((fd == -1) && device_template[i])
	{
		snprintf(name, 64, device_template[i], 0);
		fd = open(name, O_RDWR, 0);
		i++;
	}

	if (fd < 0)
	{
		return -errno;
	}

	struct fb_fix_screeninfo finfo;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
	{
		return -errno;
	}

	struct fb_var_screeninfo info;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
	{
		return -errno;
	}

	info.reserved[0] = 0;
	info.reserved[1] = 0;
	info.reserved[2] = 0;
	info.xoffset = 0;
	info.yoffset = 0;
	info.activate = FB_ACTIVATE_NOW;

#if GRALLOC_FB_BPP == 16
	/*
	 * Explicitly request 5/6/5
	 */
	info.bits_per_pixel = 16;
	info.red.offset = 11;
	info.red.length = 5;
	info.green.offset = 5;
	info.green.length = 6;
	info.blue.offset = 0;
	info.blue.length = 5;
	info.transp.offset = 0;
	info.transp.length = 0;
#elif GRALLOC_FB_BPP == 32
	/*
	 * Explicitly request 8/8/8
	 */
	info.bits_per_pixel = 32;
	info.red.offset = 16;
	info.red.length = 8;
	info.green.offset = 8;
	info.green.length = 8;
	info.blue.offset = 0;
	info.blue.length = 8;
	info.transp.offset = 0;
	info.transp.length = 0;
#else
#error "Invalid framebuffer bit depth"
#endif

	/*
	 * Request NUM_BUFFERS screens (at lest 2 for page flipping)
	 */
	info.yres_virtual = info.yres * NUM_BUFFERS;

	uint32_t flags = PAGE_FLIP;

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1)
	{
		info.yres_virtual = info.yres;
		flags &= ~PAGE_FLIP;
		AWAR("FBIOPUT_VSCREENINFO failed, page flipping not supported fd: %d", fd);
	}

	if (info.yres_virtual < info.yres * 2)
	{
		// we need at least 2 for page-flipping
		info.yres_virtual = info.yres;
		flags &= ~PAGE_FLIP;
		AWAR("page flipping not supported (yres_virtual=%d, requested=%d)", info.yres_virtual, info.yres * 2);
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
	{
		return -errno;
	}

	int refreshRate = 0;

	if (info.pixclock > 0)
	{
		refreshRate =
		    1000000000000000LLU / (uint64_t(info.upper_margin + info.lower_margin + info.yres + info.hsync_len) *
		                           (info.left_margin + info.right_margin + info.xres + info.vsync_len) * info.pixclock);
	}
	else
	{
		AWAR("fbdev pixclock is zero for fd: %d", fd);
	}

	if (refreshRate == 0)
	{
		refreshRate = 60 * 1000; // 60 Hz
	}

	if (int(info.width) <= 0 || int(info.height) <= 0)
	{
		// the driver doesn't return that information
		// default to 160 dpi
		info.width = ((info.xres * 25.4f) / 160.0f + 0.5f);
		info.height = ((info.yres * 25.4f) / 160.0f + 0.5f);
	}

	float xdpi = (info.xres * 25.4f) / info.width;
	float ydpi = (info.yres * 25.4f) / info.height;
	float fps = refreshRate / 1000.0f;

	AINF("using (fd=%d)\n"
	     "id           = %s\n"
	     "xres         = %d px\n"
	     "yres         = %d px\n"
	     "xres_virtual = %d px\n"
	     "yres_virtual = %d px\n"
	     "bpp          = %d\n"
	     "r            = %2u:%u\n"
	     "g            = %2u:%u\n"
	     "b            = %2u:%u\n",
	     fd, finfo.id, info.xres, info.yres, info.xres_virtual, info.yres_virtual, info.bits_per_pixel, info.red.offset,
	     info.red.length, info.green.offset, info.green.length, info.blue.offset, info.blue.length);

	AINF("width        = %d mm (%f dpi)\n"
	     "height       = %d mm (%f dpi)\n"
	     "refresh rate = %.2f Hz\n",
	     info.width, xdpi, info.height, ydpi, fps);

	if (0 == strncmp(finfo.id, "CLCD FB", 7))
	{
		module->dpy_type = MALI_DPY_TYPE_CLCD;
	}
	else if (0 == strncmp(finfo.id, "ARM Mali HDLCD", 14))
	{
		module->dpy_type = MALI_DPY_TYPE_HDLCD;
	}
	else if (0 == strncmp(finfo.id, "ARM HDLCD Control", 16))
	{
		module->dpy_type = MALI_DPY_TYPE_HDLCD;
	}
	else
	{
		module->dpy_type = MALI_DPY_TYPE_UNKNOWN;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
	{
		return -errno;
	}

	if (finfo.smem_len <= 0)
	{
		return -errno;
	}

	module->flags = flags;
	module->info = info;
	module->finfo = finfo;
	module->xdpi = xdpi;
	module->ydpi = ydpi;
	module->fps = fps;
	module->swapInterval = 1;

	/*
	 * map the framebuffer
	 */
	size_t fbSize = round_up_to_page_size(finfo.line_length * info.yres_virtual);
	void *vaddr = mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (vaddr == MAP_FAILED)
	{
		AERR("Error mapping the framebuffer (%s)", strerror(errno));
		return -errno;
	}

	memset(vaddr, 0, fbSize);
	// Create a "fake" buffer object for the entire frame buffer memory, and store it in the module
	module->framebuffer = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, fbSize, vaddr,
	                                           static_cast<uint64_t>(GRALLOC_USAGE_HW_FB),
	                                           static_cast<uint64_t>(GRALLOC_USAGE_HW_FB), dup(fd), 0,
	                                           finfo.line_length, info.xres_virtual, info.yres_virtual,
	                                           module->fbdev_format);

	module->numBuffers = info.yres_virtual / info.yres;
	module->bufferMask = 0;

	return 0;
}

static int init_frame_buffer(struct private_module_t *module)
{
	pthread_mutex_lock(&module->lock);
	int err = init_frame_buffer_locked(module);
	pthread_mutex_unlock(&module->lock);
	return err;
}

#if GRALLOC_VERSION_MAJOR <= 1
static int fb_close(struct hw_device_t *device)
{
	framebuffer_device_t *dev = reinterpret_cast<framebuffer_device_t *>(device);

	if (dev)
	{
		free(dev);
	}

	return 0;
}
#endif

static int fb_alloc_framebuffer_dmabuf(private_module_t *m, private_handle_t *hnd)
{
	struct fb_dmabuf_export fb_dma_buf;
	int res;
	res = ioctl(m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf);

	if (res == 0)
	{
		hnd->share_fd = fb_dma_buf.fd;
		return 0;
	}
	else
	{
		AINF("FBIOGET_DMABUF ioctl failed(%d). See gralloc_priv.h and the integration manual for vendor framebuffer "
		     "integration",
		     res);
		return -1;
	}
}

static int fb_alloc_from_ion_module(mali_gralloc_module *m, int width, int height, int byte_stride, size_t buffer_size,
                                    uint64_t consumer_usage, uint64_t producer_usage, buffer_handle_t *pHandle)
{
	buffer_descriptor_t fb_buffer_descriptor;
	gralloc_buffer_descriptor_t gralloc_buffer_descriptor[1];
	bool shared = false;
	int err = 0;

	fb_buffer_descriptor.width = width;
	fb_buffer_descriptor.height = height;
	fb_buffer_descriptor.size = buffer_size;
	fb_buffer_descriptor.old_alloc_width = width;
	fb_buffer_descriptor.old_alloc_height = height;
	fb_buffer_descriptor.old_byte_stride = byte_stride;
	fb_buffer_descriptor.pixel_stride = width;

	memset(fb_buffer_descriptor.plane_info, 0, sizeof(fb_buffer_descriptor.plane_info));
	fb_buffer_descriptor.plane_info[0].alloc_width = width;
	fb_buffer_descriptor.plane_info[0].alloc_height = height;
	fb_buffer_descriptor.plane_info[0].byte_stride = byte_stride;
	fb_buffer_descriptor.plane_info[0].offset = 0;

	fb_buffer_descriptor.internal_format = m->fbdev_format;
	fb_buffer_descriptor.alloc_format = m->fbdev_format;
	fb_buffer_descriptor.consumer_usage = consumer_usage;
	fb_buffer_descriptor.producer_usage = producer_usage;
	fb_buffer_descriptor.layer_count = 1;

	gralloc_buffer_descriptor[0] = (gralloc_buffer_descriptor_t)(&fb_buffer_descriptor);

	err = mali_gralloc_ion_allocate(gralloc_buffer_descriptor, 1, pHandle, &shared);

	return err;
}

static int fb_alloc_framebuffer_locked(mali_gralloc_module *m, uint64_t consumer_usage, uint64_t producer_usage,
                                       buffer_handle_t *pHandle, int *stride, int *byte_stride)
{
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

	uint32_t bufferMask = m->bufferMask;
	const uint32_t numBuffers = m->numBuffers;
	/* framebufferSize is used for allocating the handle to the framebuffer and refers
	 *                 to the size of the actual framebuffer.
	 * alignedFramebufferSize is used for allocating a possible internal buffer and
	 *                        thus need to consider internal alignment requirements. */
	const size_t framebufferSize = m->finfo.line_length * m->info.yres;
	const size_t alignedFramebufferSize = GRALLOC_ALIGN(m->finfo.line_length, 64) * m->info.yres;

	*stride = m->info.xres;

	if (numBuffers == 1)
	{
		// If we have only one buffer, we never use page-flipping. Instead,
		// we return a regular buffer which will be memcpy'ed to the main
		// screen when post is called.
		uint64_t newConsumerUsage = (consumer_usage & ~(static_cast<uint64_t>(GRALLOC_USAGE_HW_FB)));
		uint64_t newProducerUsage = (producer_usage & ~(static_cast<uint64_t>(GRALLOC_USAGE_HW_FB))) |
		                                               static_cast<uint64_t>(GRALLOC_USAGE_HW_2D);
		AWAR("fallback to single buffering. Virtual Y-res too small %d", m->info.yres);
		*byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
		return fb_alloc_from_ion_module(m, m->info.xres, m->info.yres, *byte_stride,
		                                alignedFramebufferSize, newConsumerUsage, newProducerUsage, pHandle);
	}

	if (bufferMask >= ((1LU << numBuffers) - 1))
	{
		// We ran out of buffers, reset bufferMask
		bufferMask = 0;
		m->bufferMask = 0;
	}

	uintptr_t framebufferVaddr = (uintptr_t)m->framebuffer->base;

	// find a free slot
	for (uint32_t i = 0; i < numBuffers; i++)
	{
		if ((bufferMask & (1LU << i)) == 0)
		{
			m->bufferMask |= (1LU << i);
			break;
		}

		framebufferVaddr += framebufferSize;
	}

	// The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
	private_handle_t *hnd = new private_handle_t(
	    private_handle_t::PRIV_FLAGS_FRAMEBUFFER, framebufferSize, (void *)framebufferVaddr, consumer_usage,
	    producer_usage, dup(m->framebuffer->fd), (framebufferVaddr - (uintptr_t)m->framebuffer->base),
	    m->finfo.line_length, m->info.xres, m->info.yres, m->fbdev_format);

	/*
	 * Perform allocator specific actions. If these fail we fall back to a regular buffer
	 * which will be memcpy'ed to the main screen when fb_post is called.
	 */
	if (fb_alloc_framebuffer_dmabuf(m, hnd) == -1)
	{
		delete hnd;
		uint64_t newConsumerUsage = (consumer_usage & ~(static_cast<uint64_t>(GRALLOC_USAGE_HW_FB)));
		uint64_t newProducerUsage = (producer_usage & ~(static_cast<uint64_t>(GRALLOC_USAGE_HW_FB))) |
		                                               static_cast<uint64_t>(GRALLOC_USAGE_HW_2D);
		AERR("Fallback to single buffering. Unable to map framebuffer memory to handle:%p", hnd);
		*byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
		return fb_alloc_from_ion_module(m, m->info.xres, m->info.yres, *byte_stride,
		                                alignedFramebufferSize, newConsumerUsage, newProducerUsage, pHandle);
	}

	*pHandle = hnd;
	*byte_stride = m->finfo.line_length;

	return 0;
}

#if GRALLOC_VERSION_MAJOR <= 1
int framebuffer_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	int status = -EINVAL;

	GRALLOC_UNUSED(name);

#if GRALLOC_VERSION_MAJOR == 1
	gralloc1_device_t *gralloc_device;
#else
	alloc_device_t *gralloc_device;
#endif

#if DISABLE_FRAMEBUFFER_HAL == 1
	AERR("Framebuffer HAL not support/disabled %s",
#ifdef MALI_DISPLAY_VERSION
	     "with MALI display enable");
#else
	     "");
#endif
	return -ENODEV;
#endif

#if GRALLOC_VERSION_MAJOR == 1
	status = gralloc1_open(module, &gralloc_device);
#else
	status = gralloc_open(module, &gralloc_device);
#endif

	if (status < 0)
	{
		return status;
	}

	private_module_t *m = (private_module_t *)module;

	/* Populate frame buffer pixel format */
#if GRALLOC_FB_BPP == 16
	m->fbdev_format = MALI_GRALLOC_FORMAT_INTERNAL_RGB_565;
#elif GRALLOC_FB_BPP == 32
	m->fbdev_format = MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888;
#else
#error "Invalid framebuffer bit depth"
#endif

	status = init_frame_buffer(m);

	/* malloc is used instead of 'new' to instantiate the struct framebuffer_device_t
	 * C++11 spec specifies that if a class/struct has a const member,default constructor
	 * is deleted. So, if 'new' is used to instantiate the class/struct, it will throw
	 * error complaining about deleted constructor. Even if the struct is wrapped in a class
	 * it will still try to use the base class constructor to initialize the members, resulting
	 * in error 'deleted constructor'.
	 * This leaves two options
	 * Option 1: initialize the const members at the instantiation time. With {value1, value2 ..}
	 * Which relies on the order of the members, and if members are reordered or a new member is introduced
	 * it will end up assiging wrong value to members. Designated assignment as well has been removed in C++11
	 * Option 2: use malloc instead of 'new' to allocate the class/struct and initialize the members in code.
	 * This is the only maintainable option available.
	 */

	framebuffer_device_t *dev = reinterpret_cast<framebuffer_device_t *>(malloc(sizeof(framebuffer_device_t)));

	/* if either or both of init_frame_buffer() and malloc failed */
	if ((status < 0) || (!dev))
	{
#if GRALLOC_VERSION_MAJOR == 1
		gralloc1_close(gralloc_device);
#else
		gralloc_close(gralloc_device);
#endif
		(!dev) ? (void)(status = -ENOMEM) : free(dev);
		return status;
	}

	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = fb_close;
	dev->setSwapInterval = fb_set_swap_interval;
	dev->post = fb_post;
	dev->setUpdateRect = 0;

	int stride = m->finfo.line_length / (m->info.bits_per_pixel >> 3);
	const_cast<uint32_t &>(dev->flags) = 0;
	const_cast<uint32_t &>(dev->width) = m->info.xres;
	const_cast<uint32_t &>(dev->height) = m->info.yres;
	const_cast<int &>(dev->stride) = stride;
	const_cast<int &>(dev->format) = m->fbdev_format;
	const_cast<float &>(dev->xdpi) = m->xdpi;
	const_cast<float &>(dev->ydpi) = m->ydpi;
	const_cast<float &>(dev->fps) = m->fps;
	const_cast<int &>(dev->minSwapInterval) = 0;
	const_cast<int &>(dev->maxSwapInterval) = 1;
	*device = &dev->common;

	gralloc_vsync_enable(dev);

	return status;
}
#endif

#if DISABLE_FRAMEBUFFER_HAL != 1
int32_t mali_gralloc_fb_allocate(private_module_t * const module,
                                 const buffer_descriptor_t * const bufDescriptor,
                                 buffer_handle_t * const outBuffers)
{
	uint64_t format = bufDescriptor->hal_format;

#if GRALLOC_FB_SWAP_RED_BLUE == 1
#if GRALLOC_FB_BPP == 16
	format = HAL_PIXEL_FORMAT_RGB_565;
#elif GRALLOC_FB_BPP == 32
	format = HAL_PIXEL_FORMAT_BGRA_8888;
#else
#error "Invalid framebuffer bit depth"
#endif
#endif

	int byte_stride, pixel_stride;
	pthread_mutex_lock(&module->lock);
	const int status = fb_alloc_framebuffer_locked(module, bufDescriptor->consumer_usage,
	                                               bufDescriptor->producer_usage,
	                                               outBuffers, &pixel_stride, &byte_stride);
	pthread_mutex_unlock(&module->lock);
	if (status < 0)
	{
		return status;
	}
	else
	{
		private_handle_t *hnd = (private_handle_t *)*outBuffers;

		/* Allocate a meta-data buffer for framebuffer too. fbhal
		 * ones wont need it but for hwc they will.
		 */
		(void)gralloc_buffer_attr_allocate(hnd);

		hnd->req_format = format;
		hnd->yuv_info = MALI_YUV_BT601_NARROW;
		hnd->internal_format = format;
		hnd->alloc_format = format;
		hnd->byte_stride = byte_stride;
		hnd->width = bufDescriptor->width;
		hnd->height = bufDescriptor->height;
		hnd->stride = pixel_stride;
		hnd->internalWidth = bufDescriptor->width;
		hnd->internalHeight = bufDescriptor->height;
		hnd->layer_count = 1;
		return 0;
	}
}
#endif
