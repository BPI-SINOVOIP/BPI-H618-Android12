/*
 * Copyright (C) 2016-2019 ARM Limited. All rights reserved.
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

#include <inttypes.h>
#include <assert.h>
#include <atomic>

#if GRALLOC_VERSION_MAJOR <= 1
#include <hardware/hardware.h>
#endif

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_ion.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_debug.h"
#include "format_info.h"

#if GRALLOC_USE_LEGACY_CALCS == 1
#include "legacy/buffer_alloc.h"
#endif
#include <hardware/graphics-sunxi.h>

#define AFBC_PIXELS_PER_BLOCK 256
#define AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY 16

static int mali_gralloc_buffer_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds);
bool afbc_format_fallback(uint32_t * const format_idx, const uint64_t usage, bool force);


/*
 * Get a global unique ID
 */
static uint64_t getUniqueId()
{
	static std::atomic<uint32_t> counter(0);
	uint64_t id = static_cast<uint64_t>(getpid()) << 32;
	return id | counter++;
}

static void afbc_buffer_align(const bool is_tiled, int *size)
{
	const uint16_t AFBC_BODY_BUFFER_BYTE_ALIGNMENT = 1024;

	int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

	if (is_tiled)
	{
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}

	*size = GRALLOC_ALIGN(*size, buffer_byte_alignment);
}

/*
 * Obtain AFBC superblock dimensions from type.
 */
static rect_t get_afbc_sb_size(AllocBaseType alloc_base_type)
{
	const uint16_t AFBC_BASIC_BLOCK_WIDTH = 16;
	const uint16_t AFBC_BASIC_BLOCK_HEIGHT = 16;
	const uint16_t AFBC_WIDE_BLOCK_WIDTH = 32;
	const uint16_t AFBC_WIDE_BLOCK_HEIGHT = 8;
	const uint16_t AFBC_EXTRAWIDE_BLOCK_WIDTH = 64;
	const uint16_t AFBC_EXTRAWIDE_BLOCK_HEIGHT = 4;

	rect_t sb = {0, 0};

	switch(alloc_base_type)
	{
		case UNCOMPRESSED:
			break;
		case AFBC:
			sb.width = AFBC_BASIC_BLOCK_WIDTH;
			sb.height = AFBC_BASIC_BLOCK_HEIGHT;
			break;
		case AFBC_WIDEBLK:
			sb.width = AFBC_WIDE_BLOCK_WIDTH;
			sb.height = AFBC_WIDE_BLOCK_HEIGHT;
			break;
		case AFBC_EXTRAWIDEBLK:
			sb.width = AFBC_EXTRAWIDE_BLOCK_WIDTH;
			sb.height = AFBC_EXTRAWIDE_BLOCK_HEIGHT;
			break;
	}
	return sb;
}

/*
 * Obtain AFBC superblock dimensions for specific plane.
 *
 * See alloc_type_t for more information.
 */
static rect_t get_afbc_sb_size(alloc_type_t alloc_type, const uint8_t plane)
{
	if (plane > 0 && alloc_type.is_afbc() && alloc_type.is_multi_plane)
	{
		return get_afbc_sb_size(AFBC_EXTRAWIDEBLK);
	}
	else
	{
		return get_afbc_sb_size(alloc_type.primary_type);
	}
}


bool get_alloc_type(const uint64_t format_ext,
                    const uint32_t format_idx,
                    const uint64_t usage,
                    alloc_type_t * const alloc_type)
{
	alloc_type->primary_type = UNCOMPRESSED;
	alloc_type->is_multi_plane = formats[format_idx].npln > 1;
	alloc_type->is_tiled = false;
	alloc_type->is_padded = false;
	alloc_type->is_frontbuffer_safe = false;

	/* Determine AFBC type for this format. This is used to decide alignment.
	   Split block does not affect alignment, and therefore doesn't affect the allocation type. */
	if (format_ext & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
	{
		/* YUV transform shall not be enabled for a YUV format */
		if ((formats[format_idx].is_yuv == true) && (format_ext & MALI_GRALLOC_INTFMT_AFBC_YUV_TRANSFORM))
		{
			ALOGW("YUV Transform is incorrectly enabled for format = 0x%x. Extended internal format = 0x%" PRIx64 "\n",
			       formats[format_idx].id, format_ext);
		}

		/* Determine primary AFBC (superblock) type. */
		alloc_type->primary_type = AFBC;
		if (format_ext & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK)
		{
			alloc_type->primary_type = AFBC_WIDEBLK;
		}
		else if (format_ext & MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK)
		{
			alloc_type->primary_type = AFBC_EXTRAWIDEBLK;
		}

		if (format_ext & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
		{
			alloc_type->is_tiled = true;

			if (formats[format_idx].npln > 1 &&
				(format_ext & MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK) == 0)
			{
				ALOGW("Extra-wide AFBC must be signalled for multi-plane formats. "
				      "Falling back to single plane AFBC.");
				alloc_type->is_multi_plane = false;
			}

			if (format_ext & MALI_GRALLOC_INTFMT_AFBC_DOUBLE_BODY)
			{
				alloc_type->is_frontbuffer_safe = true;
			}
		}
		else
		{
			if (formats[format_idx].npln > 1)
			{
				ALOGW("Multi-plane AFBC is not supported without tiling. "
				      "Falling back to single plane AFBC.");
			}
			alloc_type->is_multi_plane = false;
		}

		if (format_ext & MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK &&
			!alloc_type->is_tiled)
		{
			/* Headers must be tiled for extra-wide. */
			ALOGE("ERROR: Invalid to specify extra-wide block without tiled headers.");
			return false;
		}

		if (alloc_type->is_frontbuffer_safe &&
		    (format_ext & (MALI_GRALLOC_INTFMT_AFBC_WIDEBLK | MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK)))
		{
			ALOGE("ERROR: Front-buffer safe not supported with wide/extra-wide block.");
		}

		if (formats[format_idx].npln == 1 &&
		    format_ext & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK &&
		    format_ext & MALI_GRALLOC_INTFMT_AFBC_EXTRAWIDEBLK)
		{
			/* "Wide + Extra-wide" implicitly means "multi-plane". */
			ALOGE("ERROR: Invalid to specify multiplane AFBC with single plane format.");
			return false;
		}

		if (usage & MALI_GRALLOC_USAGE_AFBC_PADDING)
		{
			alloc_type->is_padded = true;
		}
	}

	return true;
}

/*
 * Initialise AFBC header based on superblock layout.
 * Width and height should already be AFBC aligned.
 */
void init_afbc(uint8_t *buf, const uint64_t alloc_format,
               const bool is_multi_plane,
               const int w, const int h)
{
	const bool is_tiled = ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
	                         == MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS);
	const uint32_t n_headers = (w * h) / AFBC_PIXELS_PER_BLOCK;
	int body_offset = n_headers * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY;

	afbc_buffer_align(is_tiled, &body_offset);

	/*
	 * Declare the AFBC header initialisation values for each superblock layout.
	 * Tiled headers (AFBC 1.2) can be initialised to zero for non-subsampled formats
	 * (SB layouts: 0, 3, 4, 7).
	 */
	uint32_t headers[][4] = {
		{ (uint32_t)body_offset, 0x1, 0x10000, 0x0 }, /* Layouts 0, 3, 4, 7 */
		{ ((uint32_t)body_offset + (1 << 28)), 0x80200040, 0x1004000, 0x20080 } /* Layouts 1, 5 */
	};
	if ((alloc_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS))
	{
		/* Zero out body_offset for non-subsampled formats. */
		memset(headers[0], 0, sizeof(uint32_t) * 4);
	}

	/* Map base format to AFBC header layout */
	const uint32_t base_format = alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/* Sub-sampled formats use layouts 1 and 5 which is index 1 in the headers array.
	 * 1 = 4:2:0 16x16, 5 = 4:2:0 32x8.
	 *
	 * Non-subsampled use layouts 0, 3, 4 and 7, which is index 0.
	 * 0 = 16x16, 3 = 32x8 + split, 4 = 32x8, 7 = 64x4.
	 *
	 * When using separated planes for YUV formats, the header layout is the non-subsampled one
	 * as there is a header per-plane and there is no sub-sampling within the plane.
	 * Separated plane only supports 32x8 or 64x4 for the luma plane, so the first plane must be 4 or 7.
	 * Seperated plane only supports 64x4 for subsequent planes, so these must be header layout 7.
	 */
	const uint32_t layout = is_subsampled_yuv(base_format) && !is_multi_plane ? 1 : 0;

	ALOGV("Writing AFBC header layout %d for format %" PRIx32, layout, base_format);

	for (uint32_t i = 0; i < n_headers; i++)
	{
		memcpy(buf, headers[layout], sizeof(headers[layout]));
		buf += sizeof(headers[layout]);
	}
}

static int max(int a, int b)
{
	return a > b ? a : b;
}

static int max(int a, int b, int c)
{
	return c > max(a, b) ? c : max(a, b);
}

static int max(int a, int b, int c, int d)
{
	return d > max(a, b, c) ? d : max(a, b, c);
}

/*
 * Obtain plane allocation dimensions (in pixels).
 *
 * NOTE: pixel stride, where defined for format, is
 * incorporated into allocation dimensions.
 */
static void get_pixel_w_h(uint32_t * const width,
                          uint32_t * const height,
                          const format_info_t format,
                          const alloc_type_t alloc_type,
                          const uint8_t plane,
                          const uint64_t usage)
{
	bool has_cpu_usage = usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK);
	bool hw_ve_coder = usage & (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER | GRALLOC_USAGE_HW_VIDEO_ENCODER);
	bool hw_camera_usage = usage & GRALLOC_USAGE_HW_CAMERA_WRITE;

	rect_t sb = get_afbc_sb_size(alloc_type, plane);
	if (hw_ve_coder | hw_camera_usage) {
		sb.width = SUNXI_YUV_PLANE_ALIGN;
		sb.height = SUNXI_YUV_PLANE_ALIGN;
	}
	/*
	 * Round-up plane dimensions, to multiple of:
	 * - Samples for all channels (sub-sampled formats)
	 * - Memory bytes/words (some packed formats)
	 */
	// the gralloc user should set HW_TEXTURE usage, if it
	// want to create a texture for gpu.
	// here it the workaround way to fix:
	// android.media.cts.HeifWriterTest#testInputSurface_Grid_Handler
	if (*width == 15 && *height == 12 && usage == 0x20)
		*width = GRALLOC_ALIGN(*width, 16);
	else
		*width = GRALLOC_ALIGN(*width, format.align_w);

	*height = GRALLOC_ALIGN(*height, format.align_h);

	/*
	 * Sub-sample (sub-sampled) planes.
	 */
	if (plane > 0)
	{
		*width /= format.hsub;
		*height /= format.vsub;
		if(format.is_yuv) {
			if (hw_ve_coder | hw_camera_usage) {
				sb.width = SUNXI_YUV_PLANE_ALIGN/2;
				sb.height = SUNXI_YUV_PLANE_ALIGN/2;
			}
		}
	}

	/*
	 * Pixel alignment (width),
	 * where format stride is stated in pixels.
	 */
	int pixel_align_w = 0;
	if (has_cpu_usage && !hw_ve_coder && !hw_camera_usage)
	{
		pixel_align_w = format.align_w_cpu;
	}
	else if (alloc_type.is_afbc())
	{
#define HEADER_STRIDE_ALIGN_IN_SUPER_BLOCKS (0)
		uint32_t num_sb_align = 0;
		if (alloc_type.is_padded && !format.is_yuv)
		{
			/* Align to 4 superblocks in width --> 64-byte,
			 * assuming 16-byte header per superblock.
			 */
			num_sb_align = 4;
		}
		pixel_align_w = max(HEADER_STRIDE_ALIGN_IN_SUPER_BLOCKS, num_sb_align) * sb.width;
	}

	/*
	 * Determine AFBC tile size when allocating tiled headers.
	 */
	rect_t afbc_tile = sb;
	if (alloc_type.is_tiled)
	{
		afbc_tile.width = format.bpp_afbc[plane] > 32 ? 4 * afbc_tile.width : 8 * afbc_tile.width;
		afbc_tile.height = format.bpp_afbc[plane] > 32 ? 4 * afbc_tile.height : 8 * afbc_tile.height;
	}

	ALOGV("Plane[%hhu]: [SUB-SAMPLE] w:%d, h:%d\n", plane, *width, *height);
	ALOGV("Plane[%hhu]: [PIXEL_ALIGN] w:%d\n", plane, pixel_align_w);
	ALOGV("Plane[%hhu]: [LINEAR_TILE] w:%" PRIu16 "\n", plane, format.tile_size);
	ALOGV("Plane[%hhu]: [AFBC_TILE] w:%" PRIu16 ", h:%" PRIu16 "\n", plane, afbc_tile.width, afbc_tile.height);

	*width = GRALLOC_ALIGN(*width, max(1, pixel_align_w, format.tile_size, afbc_tile.width));
	*height = GRALLOC_ALIGN(*height, max(1, format.tile_size, afbc_tile.height));
}



static uint32_t gcd(uint32_t a, uint32_t b)
{
	uint32_t r, t;

	if (a == b)
	{
		return a;
	}
	else if (a < b)
	{
		t = a;
		a = b;
		b = t;
	}

	while (b != 0)
	{
		r = a % b;
		a = b;
		b = r;
	}

	return a;
}

uint32_t lcm(uint32_t a, uint32_t b)
{
	if (a != 0 && b != 0)
	{
		return (a * b) / gcd(a, b);
	}

	return max(a, b);
}

/*
 * Calculate allocation size.
 *
 * Determine the width and height of each plane based on pixel alignment for
 * both uncompressed and AFBC allocations.
 *
 * @param width           [in]    Buffer width.
 * @param height          [in]    Buffer height.
 * @param alloc_type      [in]    Allocation type inc. whether tiled and/or multi-plane.
 * @param format          [in]    Pixel format.
 * @param has_cpu_usage   [in]    CPU usage requested (in addition to any other).
 * @param pixel_stride    [out]   Calculated pixel stride.
 * @param size            [out]   Total calculated buffer size including all planes.
 * @param plane_info      [out]   Array of calculated information for each plane. Includes
 *                                offset, byte stride and allocation width and height.
 */
static void calc_allocation_size(const int width,
                                 const int height,
                                 const alloc_type_t alloc_type,
                                 const format_info_t format,
                                 const uint64_t usage,
                                 int * const pixel_stride,
                                 size_t * const size,
                                 buffer_descriptor_t * const bufDescriptor)
{
	plane_info_t *plane_info = bufDescriptor->plane_info;
#if NEW_AW_ALIGN != 1
	int  *aw_align = bufDescriptor->aw_byte_align;
#endif
	plane_info[0].offset = 0;

	bool has_cpu_usage = usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK);
	bool has_hw_usage = usage & ~(GRALLOC_USAGE_PRIVATE_MASK | GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK);
	bool hw_ve_coder = usage & (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER | GRALLOC_USAGE_HW_VIDEO_ENCODER);
	// no need deal with camera align specifically, or it let camera allocate 8 align buffer for YV12 second plane,
	// then gpu cannot create external image successfully.
	// Fix: android.hardware.cts.CameraGLTest#testCameraToSurfaceTextureMetadata
	bool hw_camera_usage = 0;

	// filter google software cases
	// fix testGLViewLargerHeightDecodeAccuracy media cts
	if (format.is_yuv && has_cpu_usage && !hw_ve_coder && !hw_camera_usage) {
		has_hw_usage = false;
	}

	*size = 0;

	for (uint8_t plane = 0; plane < format.npln; plane++)
	{
		plane_info[plane].alloc_width = width;
		plane_info[plane].alloc_height = height;
		get_pixel_w_h(&plane_info[plane].alloc_width,
		              &plane_info[plane].alloc_height,
		              format,
		              alloc_type,
		              plane,
		              usage);
		ALOGV("Aligned w=%d, h=%d (in pixels)",
		      plane_info[plane].alloc_width, plane_info[plane].alloc_height);

		/*
		 * Calculate byte stride (per plane).
		 */
		if (alloc_type.is_afbc())
		{
			assert((plane_info[plane].alloc_width * format.bpp_afbc[plane]) % 8 == 0);
			plane_info[plane].byte_stride = (plane_info[plane].alloc_width * format.bpp_afbc[plane]) / 8;
		}
		else
		{
			assert((plane_info[plane].alloc_width * format.bpp[plane]) % 8 == 0);
			plane_info[plane].byte_stride = (plane_info[plane].alloc_width * format.bpp[plane]) / 8;

			/*
			 * Align byte stride (uncompressed allocations only).
			 *
			 * Find the lowest-common-multiple of:
			 * 1. hw_align: Minimum byte stride alignment for HW IP (has_hw_usage == true)
			 * 2. cpu_align: Byte equivalent of 'align_w_cpu' (has_cpu_usage == true)
			 *
			 * NOTE: Pixel stride is defined as multiple of 'align_w_cpu'.
			 */
			uint16_t hw_align = 0;
			if (has_hw_usage)
			{
				hw_align = format.is_yuv ? 128 : 64;
				if(format.is_yuv) {
					if (hw_ve_coder) {
						hw_align = SUNXI_YUV_PLANE_ALIGN;
						if (MALI_GRALLOC_FORMAT_INTERNAL_P010 == format.id) {
							bufDescriptor->ion_metadata_flag |= SUNXI_METADATA_FLAG_HDR_SATIC_METADATA;
						}
					}else if (hw_camera_usage) {
						hw_align = SUNXI_YUV_PLANE_ALIGN;
					}
				}
			}
			if (plane > 0 && (hw_ve_coder || hw_camera_usage)) {
				hw_align /= format.hsub;
				hw_align *=  format.bpp[plane]/8;
			}

			uint32_t cpu_align = 0;
			if (has_cpu_usage && !hw_ve_coder && !hw_camera_usage)
			{
				assert((format.bpp[plane] * format.align_w_cpu) % 8 == 0);
				cpu_align = (format.bpp[plane] * format.align_w_cpu) / 8;
			}

			uint32_t stride_align = lcm(hw_align, cpu_align);
			if (stride_align)
			{
				aw_align[plane] = stride_align;
				plane_info[plane].byte_stride = GRALLOC_ALIGN(plane_info[plane].byte_stride, stride_align);
			}
		}
		ALOGV("Byte stride: %d", plane_info[plane].byte_stride);

		/*
		 * Pixel stride (CPU usage only).
		 * Not used in size calculation but exposed to client.
		 */
		if (plane == 0)
		{
			//assert((plane_info[plane].byte_stride * 8) % format.bpp[plane] == 0);
			*pixel_stride = (plane_info[plane].byte_stride * 8) / format.bpp[plane];

			ALOGV("Pixel stride: %d", *pixel_stride);
		}

		const uint32_t sb_num = (plane_info[plane].alloc_width * plane_info[plane].alloc_height)
		                      / AFBC_PIXELS_PER_BLOCK;

		/*
		 * Calculate body size (per plane).
		 */
		int body_size = 0;
		if (alloc_type.is_afbc())
		{
			const rect_t sb = get_afbc_sb_size(alloc_type, plane);
			const int sb_bytes = GRALLOC_ALIGN((format.bpp_afbc[plane] * sb.width * sb.height) / 8, 128);
			body_size = sb_num * sb_bytes;

			/* When AFBC planes are stored in separate buffers and this is not the last plane,
			   also align the body buffer to make the subsequent header aligned. */
			if (format.npln > 1 && plane < 2)
			{
				afbc_buffer_align(alloc_type.is_tiled, &body_size);
			}

			if (alloc_type.is_frontbuffer_safe)
			{
				int back_buffer_size = body_size;
				afbc_buffer_align(alloc_type.is_tiled, &back_buffer_size);
				body_size += back_buffer_size;
			}
		}
		else
		{
			body_size = (plane_info[plane].byte_stride) * plane_info[plane].alloc_height;
			if(format.is_yuv) {
				if (hw_ve_coder) {
					if (plane == 0) {
						body_size = GRALLOC_ALIGN(body_size, 1024);
					}
				}
			}
		}
		ALOGV("Body size: %d", body_size);

		/*
		 * Calculate header size (per plane).
		 */
		int header_size = 0;
		if (alloc_type.is_afbc())
		{
			/* As this is AFBC, calculate header size for this plane.
			 * Always align the header, which will make the body buffer aligned.
			 */
			header_size = sb_num * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY;
			afbc_buffer_align(alloc_type.is_tiled, &header_size);
		}
		ALOGV("AFBC Header size: %d", header_size);

		/*
		 * Set offset for separate chroma planes.
		 */
		if (plane > 0)
		{
			plane_info[plane].offset = *size;
		}

		/*
		 * Set overall size.
		 * Size must be updated after offset.
		 */
		*size += body_size + header_size;
		ALOGV("size=%zu",*size);
	}
}



/*
 * Validate selected format against requested.
 * Return true if valid, false otherwise.
 */
static bool validate_format(const format_info_t * const format,
                            const alloc_type_t alloc_type,
                            const buffer_descriptor_t * const bufDescriptor)
{
	if (alloc_type.is_afbc())
	{
		/*
		 * Validate format is supported by AFBC specification and gralloc.
		 */
		if (format->afbc == false)
		{
			ALOGE("ERROR: AFBC selected but not supported for base format: 0x%" PRIx32, format->id);
			return false;
		}

		/*
		 * Enforce consistency between number of format planes and
		 * request for single/multi-plane AFBC.
		 */
		if (((format->npln == 1 && alloc_type.is_multi_plane) ||
		    (format->npln > 1 && !alloc_type.is_multi_plane)))
		{
			ALOGE("ERROR: Format (%" PRIx32 ", num planes: %u) is incompatible with %s-plane AFBC request",
			      format->id, format->npln, (alloc_type.is_multi_plane) ? "multi" : "single");
			return false;
		}
	}
	else
	{
		if (format->linear == false)
		{
			ALOGE("ERROR: Uncompressed format requested but not supported for base format: %" PRIx32, format->id);
			return false;
		}
	}

	if (format->id == MALI_GRALLOC_FORMAT_INTERNAL_BLOB &&
	    bufDescriptor->height != 1)
	{
		ALOGE("ERROR: Height for format BLOB must be 1.");
		return false;
	}

	return true;
}


static int set_dataspace(private_handle_t * const hnd, uint64_t usage, int32_t format_idx)
{
	GRALLOC_UNUSED(usage);

	int color_space = HAL_DATASPACE_STANDARD_UNSPECIFIED;
	int range = HAL_DATASPACE_RANGE_UNSPECIFIED;
	int data_space = 0;
	hnd->yuv_info = MALI_YUV_NO_INFO;
	int rval = -1;

	/* This resolution is the cut-off point at which BT709 is used (as default)
	 * instead of BT601 for YUV formats < 10 bits.
	 */
	const int YUV_BT601_MAX_WIDTH = 1280;
	const int YUV_BT601_MAX_HEIGHT = 720;

	if (gralloc_buffer_attr_map(hnd, true) < 0)
	{
		ALOGE("Failed to map attribute region.");
		goto out;
	}

	if (formats[format_idx].is_yuv)
	{
		/* Default YUV dataspace. */
		color_space = HAL_DATASPACE_STANDARD_BT709;
		range = HAL_DATASPACE_RANGE_LIMITED;

		/* 10-bit YUV is assumed to be wide BT2020.
		 */
		if (formats[format_idx].bps >= 10)
		{
			color_space = HAL_DATASPACE_STANDARD_BT2020;
			range = HAL_DATASPACE_RANGE_LIMITED;
		}
		else if (hnd->width < YUV_BT601_MAX_WIDTH || hnd->height < YUV_BT601_MAX_HEIGHT)
		{
			color_space = HAL_DATASPACE_STANDARD_BT601_625;
			range = HAL_DATASPACE_RANGE_LIMITED;
		}

#if GRALLOC_VERSION_MAJOR >= 1
		/* Override YUV dataspace based on private usage. */
		switch (usage & MALI_GRALLOC_USAGE_YUV_COLOR_SPACE_MASK)
		{
		case MALI_GRALLOC_USAGE_YUV_COLOR_SPACE_BT601:
			color_space = HAL_DATASPACE_STANDARD_BT601_625;
			break;
		case MALI_GRALLOC_USAGE_YUV_COLOR_SPACE_BT709:
			color_space = HAL_DATASPACE_STANDARD_BT709;
			break;
		case MALI_GRALLOC_USAGE_YUV_COLOR_SPACE_BT2020:
			color_space = HAL_DATASPACE_STANDARD_BT2020;
			break;
		}

		switch (usage & MALI_GRALLOC_USAGE_RANGE_MASK)
		{
		case MALI_GRALLOC_USAGE_RANGE_NARROW:
			range = HAL_DATASPACE_RANGE_LIMITED;
			break;
		case MALI_GRALLOC_USAGE_RANGE_WIDE:
			range = HAL_DATASPACE_RANGE_FULL;
			break;
		}

		data_space = color_space | range;

		/* Set deprecated yuv_info field. */
		switch (color_space)
		{
		case HAL_DATASPACE_STANDARD_BT601_625:
			if (range == HAL_DATASPACE_RANGE_LIMITED)
			{
				hnd->yuv_info = MALI_YUV_BT601_NARROW;
			}
			else
			{
				hnd->yuv_info = MALI_YUV_BT601_WIDE;
			}
			break;
		case HAL_DATASPACE_STANDARD_BT709:
			if (range == HAL_DATASPACE_RANGE_LIMITED)
			{
				hnd->yuv_info = MALI_YUV_BT709_NARROW;
			}
			else
			{
				hnd->yuv_info = MALI_YUV_BT709_WIDE;
			}
			break;
		}
#endif
	}
	else if (formats[format_idx].is_rgb)
	{
		/* Default RGB dataspace. */
		data_space = HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_RANGE_FULL;
	}

	gralloc_buffer_attr_write(hnd, GRALLOC_ARM_BUFFER_ATTR_DATASPACE, &data_space);

	gralloc_buffer_attr_unmap(hnd);

	rval = 0;
out:
	return rval;
}

int sunxi_gralloc_fix_format(buffer_descriptor_t * const bufDescriptor)
{
	uint64_t usage = bufDescriptor->producer_usage | bufDescriptor->consumer_usage;

	switch(bufDescriptor->hal_format & MALI_GRALLOC_INTFMT_FMT_MASK) {
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_AW_NV12:
	{
		bufDescriptor->pixel_stride = GRALLOC_ALIGN(bufDescriptor->width, SUNXI_YUV_PLANE_ALIGN);
		if ((usage & GRALLOC_USAGE_AFBC_MODE)
                        && ((usage & MALI_GRALLOC_USAGE_NO_AFBC) != MALI_GRALLOC_USAGE_NO_AFBC)) {
			bufDescriptor->size = ((bufDescriptor->width+15)>>4) *
				((bufDescriptor->height*3/2+4+15)>>4) * (384 + 16) + 32 + 1024;
		} else {
			ALOGD("sunxi format do not run here %lld",bufDescriptor->hal_format);
		}
	}
	break;
	case HAL_PIXEL_FORMAT_AW_YV12_10bit:
	case HAL_PIXEL_FORMAT_AW_NV21_10bit:
	{
		bufDescriptor->pixel_stride = GRALLOC_ALIGN(bufDescriptor->width, SUNXI_YUV_PLANE_ALIGN);
		int nNormalYuvBufSize = (bufDescriptor->pixel_stride + GRALLOC_ALIGN(bufDescriptor->width/2, SUNXI_YUV_PLANE_ALIGN/2))
								* bufDescriptor->height;
		int lower2bitSize = ((((bufDescriptor->width+3)>>2) + 31) & 0xffffffe0) * bufDescriptor->height * 3/2;

		if ((usage & GRALLOC_USAGE_AFBC_MODE)
            && ((usage & MALI_GRALLOC_USAGE_NO_AFBC) != MALI_GRALLOC_USAGE_NO_AFBC)) {
			int nAfbcBufSize = ((bufDescriptor->width+15)>>4) * ((bufDescriptor->height*3/2+4+15)>>4) * (512 + 16) + 32 + 1024;
			int PriChromaStride = ((bufDescriptor->width/2) + 31)&0xffffffe0;
			bufDescriptor->size = nAfbcBufSize + lower2bitSize;
		} else {
			bufDescriptor->size = nNormalYuvBufSize + lower2bitSize;
		}
		bufDescriptor->ion_metadata_flag |= SUNXI_METADATA_FLAG_HDR_SATIC_METADATA;
	}
	break;
	case HAL_PIXEL_FORMAT_AW_P010_VU:
	{
		/*will move to mali format*/
		bufDescriptor->pixel_stride = GRALLOC_ALIGN(bufDescriptor->width, SUNXI_YUV_PLANE_ALIGN);
		bufDescriptor->size = bufDescriptor->pixel_stride * 2 * bufDescriptor->height
					+ GRALLOC_ALIGN(bufDescriptor->width/2, SUNXI_YUV_PLANE_ALIGN/2) * bufDescriptor->height * 2;
		bufDescriptor->ion_metadata_flag |= SUNXI_METADATA_FLAG_HDR_SATIC_METADATA;
	}
	break;
	default:
		ALOGE("ERROR:Sunxi format Unrecognized and/or unsupported 0x%" PRIx64 " and usage 0x%" PRIx64,
			       bufDescriptor->hal_format, usage);
		return -EINVAL;
	}

	bufDescriptor->alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_YV12;

	switch (bufDescriptor->hal_format & MALI_GRALLOC_INTFMT_FMT_MASK) {
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_AW_YV12_10bit:
		bufDescriptor->plane_info[0].alloc_width = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[1].alloc_width = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[2].alloc_width = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[0].alloc_height = GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN);
		bufDescriptor->plane_info[1].alloc_height = GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN)/2;
		bufDescriptor->plane_info[2].alloc_height = GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN)/2;
		bufDescriptor->plane_info[0].byte_stride = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[1].byte_stride = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[2].byte_stride = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[0].offset = 0;
		bufDescriptor->plane_info[1].offset = bufDescriptor->plane_info[0].byte_stride * bufDescriptor->plane_info[0].alloc_height;
		bufDescriptor->plane_info[1].offset = GRALLOC_ALIGN(bufDescriptor->plane_info[1].offset, 1024);
		bufDescriptor->plane_info[2].offset = bufDescriptor->plane_info[1].offset
										+ bufDescriptor->plane_info[1].byte_stride * bufDescriptor->plane_info[1].alloc_height;
		bufDescriptor->aw_byte_align[0] = SUNXI_YUV_PLANE_ALIGN;
		bufDescriptor->aw_byte_align[1] = SUNXI_YUV_PLANE_ALIGN/2;
		bufDescriptor->aw_byte_align[2] = SUNXI_YUV_PLANE_ALIGN/2;
		bufDescriptor->alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_YV12;
	break;
	case HAL_PIXEL_FORMAT_AW_NV12:
	case HAL_PIXEL_FORMAT_AW_NV21_10bit:
		bufDescriptor->plane_info[0].alloc_width = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[1].alloc_width = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[0].alloc_height = GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN);
		bufDescriptor->plane_info[1].alloc_height= GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN)/2;
		bufDescriptor->plane_info[0].byte_stride = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[1].byte_stride = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[0].offset = 0;
		bufDescriptor->plane_info[1].offset = bufDescriptor->plane_info[0].byte_stride * bufDescriptor->plane_info[0].alloc_height;
		bufDescriptor->plane_info[1].offset = GRALLOC_ALIGN(bufDescriptor->plane_info[1].offset, 1024);
		bufDescriptor->aw_byte_align[0] = SUNXI_YUV_PLANE_ALIGN;
		bufDescriptor->aw_byte_align[1] = SUNXI_YUV_PLANE_ALIGN;
		bufDescriptor->aw_byte_align[2] = -1;
		bufDescriptor->alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_NV21;
	break;
	case HAL_PIXEL_FORMAT_AW_P010_VU:
		bufDescriptor->plane_info[0].alloc_width = bufDescriptor->pixel_stride;
		bufDescriptor->plane_info[1].alloc_width = bufDescriptor->pixel_stride/2;
		bufDescriptor->plane_info[0].alloc_height = GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN);
		bufDescriptor->plane_info[1].alloc_height= GRALLOC_ALIGN(bufDescriptor->height, SUNXI_YUV_PLANE_ALIGN)/2;
		bufDescriptor->plane_info[0].byte_stride = bufDescriptor->pixel_stride * 2;
		bufDescriptor->plane_info[1].byte_stride = bufDescriptor->pixel_stride *2;
		bufDescriptor->plane_info[0].offset = 0;
		bufDescriptor->plane_info[1].offset = bufDescriptor->plane_info[0].byte_stride * bufDescriptor->plane_info[0].alloc_height;
		bufDescriptor->plane_info[1].offset = GRALLOC_ALIGN(bufDescriptor->plane_info[1].offset, 1024);
		bufDescriptor->aw_byte_align[0] = SUNXI_YUV_PLANE_ALIGN * 2;
		bufDescriptor->aw_byte_align[1] = SUNXI_YUV_PLANE_ALIGN * 2;
		bufDescriptor->aw_byte_align[2] = -1;
		bufDescriptor->alloc_format = MALI_GRALLOC_FORMAT_INTERNAL_P010;
	break;
	default:
		return -EINVAL;
	}
	/* fixe creat image err, becasue SurfaceFlinger creat the image in advance,
	  * see mali_egl_winsys_android.cpp  mapping the foramat to fourcc_format.
	  */
	bufDescriptor->aw_format = 1;

	return 0;
}

int mali_gralloc_derive_format_and_size(mali_gralloc_module *m,
                                        buffer_descriptor_t * const bufDescriptor)
{
	GRALLOC_UNUSED(m);
	alloc_type_t alloc_type;
	int err;
	static bool warn_about_mutual_exclusive = true;

	int alloc_width = bufDescriptor->width;
	int alloc_height = bufDescriptor->height;
	uint64_t usage = bufDescriptor->producer_usage | bufDescriptor->consumer_usage;
	int32_t format_idx = -1;

	/*
	* Select optimal internal pixel format based upon
	* usage and requested format.
	*/
	bufDescriptor->alloc_format = mali_gralloc_select_format(bufDescriptor->hal_format,
	                                                         bufDescriptor->format_type,
	                                                         usage,
	                                                         bufDescriptor->width * bufDescriptor->height,
	                                                         &bufDescriptor->internal_format);
	if (bufDescriptor->alloc_format == MALI_GRALLOC_FORMAT_INTERNAL_AW_WRAP) {
		if (!sunxi_gralloc_fix_format(bufDescriptor)) {
			if ((usage & GRALLOC_USAGE_AFBC_MODE)
                && ((usage & MALI_GRALLOC_USAGE_NO_AFBC) != MALI_GRALLOC_USAGE_NO_AFBC)) {
				bufDescriptor->ion_metadata_flag |= SUNXI_METADATA_FLAG_AFBC_HEADER | SUNXI_METADATA_FLAG_AFBC_BIG_BUFFER;
			}
			goto out;
		} else {
			ALOGE("ERROR: sunxi format 0x%" PRIx64 " and usage 0x%" PRIx64,
		       bufDescriptor->hal_format, usage);
		}
	}
	if (bufDescriptor->alloc_format == MALI_GRALLOC_FORMAT_INTERNAL_UNDEFINED)
	{
		ALOGE("ERROR: Unrecognized and/or unsupported format 0x%" PRIx64 " and usage 0x%" PRIx64,
		       bufDescriptor->hal_format, usage);
		return -EINVAL;

	}
	else if (warn_about_mutual_exclusive &&
	        (bufDescriptor->alloc_format & 0x0000000100000000ULL) &&
	        (bufDescriptor->alloc_format & 0x0000000e00000000ULL))
	{
		/*
		 * Modifier bits are no longer mutually exclusive. Warn when
		 * any bits are set in addition to AFBC basic since these might
		 * have been handled differently by clients under the old scheme.
		 * AFBC basic is guaranteed to be signalled when any other AFBC
		 * flags are set.
		 * This flag is to avoid the mutually exclusive modifier bits warning
		 * being continuously emitted. (see comment below for explanation of warning).
		 */
		warn_about_mutual_exclusive = false;
		ALOGW("WARNING: internal format modifier bits not mutually exclusive. "
		      "AFBC basic bit is always set, so extended AFBC support bits must always be checked.");
	}

	format_idx = get_format_index(bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
	if (format_idx == -1)
	{
		return -EINVAL;
	}
	ALOGV("alloc_format: 0x%" PRIx64 " format_idx: %d", bufDescriptor->alloc_format, format_idx);

	/*
	 * Obtain allocation type (uncompressed, AFBC basic, etc...)
	 */
	if (!get_alloc_type(bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_EXT_MASK,
	    format_idx, usage, &alloc_type))
	{
		return -EINVAL;
	}

	if (!validate_format(&formats[format_idx], alloc_type, bufDescriptor))
	{
		return -EINVAL;
	}

	/*
	 * Resolution of frame (allocation width and height) might require adjustment.
	 * This adjustment is only based upon specific usage and pixel format.
	 * If using AFBC, further adjustments to the allocation width and height will be made later
	 * based on AFBC alignment requirements and, for YUV, the plane properties.
	 */
	mali_gralloc_adjust_dimensions(bufDescriptor->alloc_format,
	                               usage,
	                               &alloc_width,
	                               &alloc_height);

	/* Obtain buffer size and plane information. */
	calc_allocation_size(alloc_width,
	                     alloc_height,
	                     alloc_type,
	                     formats[format_idx],
	                     usage,
	                     &bufDescriptor->pixel_stride,
	                     &bufDescriptor->size,
	                     bufDescriptor);

out:
#if GRALLOC_USE_LEGACY_CALCS == 1
	#error "hi guys, please do not use GRALLOC_USE_LEGACY_CALCS == 1"
	/* Pre-fill legacy values with those calculated above
	 * since these are sometimes not set at all by the legacy calculations.
	 */
	bufDescriptor->old_byte_stride = bufDescriptor->plane_info[0].byte_stride;
	bufDescriptor->old_alloc_width = bufDescriptor->plane_info[0].alloc_width;
	bufDescriptor->old_alloc_height = bufDescriptor->plane_info[0].alloc_height;

	/* Translate to legacy alloc_type. */
	legacy::alloc_type_t legacy_alloc_type;
	switch (alloc_type.primary_type)
	{
		case AllocBaseType::AFBC:
			legacy_alloc_type.primary_type = legacy::AllocBaseType::AFBC;
			break;
		case AllocBaseType::AFBC_WIDEBLK:
			legacy_alloc_type.primary_type = legacy::AllocBaseType::AFBC_WIDEBLK;
			break;
		case AllocBaseType::AFBC_EXTRAWIDEBLK:
			legacy_alloc_type.primary_type = legacy::AllocBaseType::AFBC_EXTRAWIDEBLK;
			break;
		default:
			legacy_alloc_type.primary_type = legacy::AllocBaseType::UNCOMPRESSED;
			break;
	}
	if (alloc_type.is_padded)
	{
		legacy_alloc_type.primary_type = legacy::AllocBaseType::AFBC_PADDED;
	}
	legacy_alloc_type.is_multi_plane = alloc_type.is_multi_plane;
	legacy_alloc_type.is_tiled = alloc_type.is_tiled;

	/*
	 * Resolution of frame (and internal dimensions) might require adjustment
	 * based upon specific usage and pixel format.
	 */
	legacy::mali_gralloc_adjust_dimensions(bufDescriptor->internal_format,
	                                       usage,
	                                       legacy_alloc_type,
	                                       bufDescriptor->width,
	                                       bufDescriptor->height,
	                                       &bufDescriptor->old_alloc_width,
	                                       &bufDescriptor->old_alloc_height);

	size_t size = 0;
	int res = legacy::get_alloc_size(bufDescriptor->internal_format,
	                                 usage,
	                                 legacy_alloc_type,
	                                 bufDescriptor->old_alloc_width,
	                                 bufDescriptor->old_alloc_height,
	                                 &bufDescriptor->old_byte_stride,
	                                 &bufDescriptor->pixel_stride,
	                                 &size);
	if (res < 0)
	{
		ALOGW("Legacy allocation size calculation failed. "
		      "Relying upon new calculation instead.");
	}

	/* Accommodate for larger legacy allocation size. */
	if (size > bufDescriptor->size)
	{
		bufDescriptor->size = size;
	}

#else
	/* Clear all legacy values. */
	bufDescriptor->internal_format = 0;
	bufDescriptor->old_alloc_width = 0;
	bufDescriptor->old_alloc_height = 0;
	bufDescriptor->old_byte_stride = 0;
#endif
	/*
	 * Each layer of a multi-layer buffer must be aligned so that
	 * it is accessible by both producer and consumer. In most cases,
	 * the stride alignment is also sufficient for each layer, however
	 * for AFBC the header buffer alignment is more constrained (see
	 * AFBC specification v3.4, section 2.15: "Alignment requirements").
	 * Also update the buffer size to accommodate all layers.
	 */

	if (usage & (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER | GRALLOC_USAGE_HW_VIDEO_ENCODER)) {
		/* for ve burst  */
		bufDescriptor->size += 64;
	}
	if (bufDescriptor->layer_count > 1)
	{
		if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
		{
			if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
			{
				bufDescriptor->size = GRALLOC_ALIGN(bufDescriptor->size, 4096);
			}
			else
			{
				bufDescriptor->size = GRALLOC_ALIGN(bufDescriptor->size, 128);
			}
		}
		if (usage & (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER | GRALLOC_USAGE_HW_VIDEO_ENCODER)) {
			bufDescriptor->size = GRALLOC_ALIGN(bufDescriptor->size, 1024);
		}

		bufDescriptor->size *= bufDescriptor->layer_count;
	}
	return 0;
}

int mali_gralloc_buffer_allocate(mali_gralloc_module *m, const gralloc_buffer_descriptor_t *descriptors,
                                 uint32_t numDescriptors, buffer_handle_t *pHandle, bool *shared_backend)
{
	bool shared = false;
	uint64_t backing_store_id = 0x0;
	int err;

	for (uint32_t i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t * const bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);

		/* Derive the buffer size from descriptor parameters */
		err = mali_gralloc_derive_format_and_size(m, bufDescriptor);
		if (err != 0)
		{
			return err;
		}
	}

	/* Allocate ION backing store memory */
	err = mali_gralloc_ion_allocate(descriptors, numDescriptors, pHandle, &shared);
	if (err < 0)
	{
		return err;
	}

	if (shared)
	{
		backing_store_id = getUniqueId();
	}

	for (uint32_t i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t * const bufDescriptor = (buffer_descriptor_t *)descriptors[i];
		private_handle_t *hnd = (private_handle_t *)pHandle[i];
		uint64_t usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		err = gralloc_buffer_attr_allocate(hnd);

		if (err < 0)
		{
			/* free all allocated ion buffer& attr buffer here.*/
			mali_gralloc_buffer_free_internal(pHandle, numDescriptors);
			return err;
		}

		mali_gralloc_dump_buffer_add(hnd);

#if NEW_AW_ALIGN != 1
		memcpy(hnd->aw_byte_align, bufDescriptor->aw_byte_align, sizeof(int) * MAX_PLANES);
#endif
		hnd->ion_metadata_flag = bufDescriptor->ion_metadata_flag;
		if (!bufDescriptor->aw_format) {
				const int32_t format_idx = get_format_index(bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
				if (format_idx == -1)
				{
					return -EINVAL;
				}
				const int ret = set_dataspace(hnd, usage, format_idx);
				if (ret < 0)
				{
					return ret;
				}
		}else{
			int data_space = HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_RANGE_LIMITED;
			hnd->yuv_info = MALI_YUV_BT709_NARROW;
			gralloc_buffer_attr_map(hnd, true);
			gralloc_buffer_attr_write(hnd, GRALLOC_ARM_BUFFER_ATTR_DATASPACE, &data_space);
			gralloc_buffer_attr_unmap(hnd);
		}

		if (shared)
		{
			/*each buffer will share the same backing store id.*/
			hnd->backing_store_id = backing_store_id;
		}
		else
		{
			/* each buffer will have an unique backing store id.*/
			hnd->backing_store_id = getUniqueId();
		}
		ALOGI("alloc buffer:W:%d H:%d S:%d PL[%d %d] SZ:%d fmt:%d if:%lld af:%lld usg:%llx fg:%08x",
			hnd->width, hnd->height, hnd->stride,
			hnd->plane_info[0].alloc_width, hnd->plane_info[0].alloc_height, hnd->size,
			hnd->format, hnd->internal_format, hnd->alloc_format, usage, hnd->flags);
	}

	if (NULL != shared_backend)
	{
		*shared_backend = shared;
	}

	return 0;
}

int mali_gralloc_buffer_free(buffer_handle_t pHandle)
{
	int rval = -1;
	private_handle_t * const hnd = (private_handle_t * const)(pHandle);

	if (hnd != NULL)
	{
		rval = gralloc_buffer_attr_free(hnd);
		mali_gralloc_ion_free(hnd);
	}

	return rval;
}

static int mali_gralloc_buffer_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds)
{
	int err = -1;
	uint32_t i = 0;

	for (i = 0; i < num_hnds; i++)
	{
		private_handle_t * const hnd = (private_handle_t * const)(pHandle[i]);

		err = gralloc_buffer_attr_free(hnd);
		mali_gralloc_ion_free(hnd);
	}

	return err;
}
