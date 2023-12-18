/* Copyright (c) Imagination Technologies Ltd.
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IMG_GRALLOC_COMMON_PUBLIC_H
#define IMG_GRALLOC_COMMON_PUBLIC_H

#include <cutils/native_handle.h>
#include <system/graphics.h>

#define GRALLOC_BUFFER_NAME_MAX 300
#define MAX_SUB_ALLOCS (3)

typedef struct
{
	native_handle_t base;

	/* These fields can be sent cross process. They are also valid
	 * to duplicate within the same process.
	 *
	 * A table is stored within the gralloc implementation's private data
	 * structure (which is per-process) which maps stamps to a mapped
	 * PVRSRV_MEMDESC in that process. Each map entry has a lock count
	 * associated with it, satisfying the requirements of the gralloc API.
	 * This also prevents us from leaking maps/allocations.
	 */

#define IMG_NATIVE_HANDLE_NUMFDS (MAX_SUB_ALLOCS)

	/* The 'fd' field is used to "export" a meminfo to another process. */
	union{
		int fd[MAX_SUB_ALLOCS];
		struct {
			int share_fd;
			union{
				int metadata_fd;
				int iMetaDataFd;
			};
			int reserved_fd;
		};
	};

	/* The iMetaDataFd filed is used to 'export' metadata to another process.
	 */
//	int iMetaDataFd;

	/* This define should represent the number of packed 'int's required to
	 * represent the fields following it. If you add a data type that is
	 * 64-bit, for example using 'unsigned long long', you should write that
	 * as "sizeof(unsigned long long) / sizeof(int)". Please keep the order
	 * of the additions the same as the defined field order.
	 */
/*#define IMG_NATIVE_HANDLE_NUMINTS \
	(sizeof(unsigned long long) / sizeof(int) + \
	 7 + MAX_SUB_ALLOCS + MAX_SUB_ALLOCS + \
	 sizeof(unsigned long long) / sizeof(int) * MAX_SUB_ALLOCS + \
	 2)*/
	/* A KERNEL unique identifier for any exported kernel memdesc. Each
	 * exported kernel memdesc will have a unique stamp, but note that in
	 * userspace, several memdescs across multiple processes could have
	 * the same stamp. As the native_handle can be dup(2)'d, there could be
	 * multiple handles with the same stamp but different file descriptors.
	 */
	union{
		unsigned long long ui64Stamp;
		unsigned long long aw_buf_id;
	};

	/* This is used for buffer usage validation */
	int usage;

	/* In order to do efficient cache flushes we need the buffer dimensions,
	 * format and bits per pixel. There are ANativeWindow queries for the
	 * width, height and format, but the graphics HAL might have remapped the
	 * request to different values at allocation time. These are the 'true'
	 * values of the buffer allocation.
	 */
	union{
		int width;
		int iWidth;
	};
	union{
		int iHeight;
		int height;
	};
	union{
		int iFormat;
		int format;
	};
	unsigned int uiBpp;

	/* Planes are not the same as the 'fd' suballocs. A multi-planar YUV
	 * allocation has different planes (interleaved = 1, semi-planar = 2,
	 * fully-planar = 3) but might be spread across 1, 2 or 3 independent
	 * memory allocations (or not).
	 */
	int iPlanes;

	/* For multi-planar allocations, there will be multiple hstrides */
	union{
		int stride;
		int aiStride[MAX_SUB_ALLOCS];
	};
	/* For multi-planar allocations, there will be multiple vstrides */
	int aiVStride[MAX_SUB_ALLOCS];

	/* These byte offsets are reconciled with the number of sub-allocs used
	 * for a multi-planar allocation. If there is a 1:1 mapping between the
	 * number of planes and the number of sub-allocs, these will all be zero.
	 *
	 * Otherwise, normally the zeroth entry will be zero, and the latter
	 * entries will be non-zero.
	 */
	unsigned long long aulPlaneOffset[MAX_SUB_ALLOCS];

	/* Indicates what are stored in memdesc  */
	unsigned int auiMemdescUsage[MAX_SUB_ALLOCS];

	/* This records the number of MAX_SUB_ALLOCS fds actually used by the
	 * buffer allocation. File descriptors up to fd[iNumSubAllocs - 1] are
	 * guaranteed to be valid. (This does not have any bearing on the aiStride,
	 * aiVStride or aulPlaneOffset fields, as 'iPlanes' of those arrays should
	 * be initialized, not 'iNumSubAllocs'.)
	 */
	int iNumSubAllocs;

	/* How many layers a buffer contains. Layers are used to allocate for
	 * texture arrays shared between processes. The multiple layers are
	 * contained in one memory allocation. */
	int iLayers;

	/* This records reserved bits for implementation-specific usage flags */
	int iPrivUsage;

	int ion_metadata_flag;
	int ion_metadata_size;

#define NEW_AW_ALIGN 0
#if NEW_AW_ALIGN != 1
	int aw_byte_align[MAX_SUB_ALLOCS];
#endif
}
__attribute__((aligned(sizeof(int)),packed)) private_handle_t;

#endif /* IMG_GRALLOC_COMMON_PUBLIC_H */
