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
#ifndef MALI_GRALLOC_BUFFERALLOCATION_H_
#define MALI_GRALLOC_BUFFERALLOCATION_H_

#include <hardware/hardware.h>
#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "mali_gralloc_bufferdescriptor.h"

/*
 * AFBC superblock type.
 */
enum AllocBaseType
{
	UNCOMPRESSED,       /* No compression. */
	AFBC,               /* AFBC basic (16x16). */
	AFBC_WIDEBLK,       /* AFBC wide-block (32x8). */
	AFBC_EXTRAWIDEBLK,  /* AFBC extra-wide-block (64x4). */
};

/*
 * Allocation type.
 *
 * Allocation-specific properties of the AFBC format modifiers
 * described by MALI_GRALLOC_INTFMT_*.
 *
 */
typedef struct {

	/*
	 * AFBC superblock type for either:
	 * - single plane AFBC format or
	 * - first/luma plane of multi-plane AFBC format.
	 */
	AllocBaseType primary_type;

	/*
	 * Multi-plane AFBC format. AFBC chroma-only plane(s) are
	 * always compressed with superblock type 'AFBC_EXTRAWIDEBLK'.
	 */
	bool is_multi_plane;

	/*
	 * Allocate tiled AFBC headers.
	 */
	bool is_tiled;

	/*
	 * Pad AFBC header stride to 64-byte alignment
	 * (multiple of 4x16B headers).
	 */
	bool is_padded;

	/*
	 * Front-buffer rendering safe AFBC allocations include an
	 * additional 4kB-aligned body buffer.
	 */
	bool is_frontbuffer_safe;

	bool is_afbc() const
	{
		return primary_type != UNCOMPRESSED;
	}
} alloc_type_t;



int mali_gralloc_derive_format_and_size(mali_gralloc_module *m,
                                        buffer_descriptor_t * const bufDescriptor);

int mali_gralloc_buffer_allocate(mali_gralloc_module *m, const gralloc_buffer_descriptor_t *descriptors,
                                 uint32_t numDescriptors, buffer_handle_t *pHandle, bool *shared_backend);

int mali_gralloc_buffer_free(buffer_handle_t pHandle);

void init_afbc(uint8_t *buf, uint64_t internal_format, const bool is_multi_plane, int w, int h);

uint32_t lcm(uint32_t a, uint32_t b);

bool get_alloc_type(const uint64_t format_ext,
                    const uint32_t format_idx,
                    const uint64_t usage,
                    alloc_type_t * const alloc_type);

#endif /* MALI_GRALLOC_BUFFERALLOCATION_H_ */
