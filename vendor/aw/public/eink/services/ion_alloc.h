/*
 * ion_head.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi ion test head file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef ION_ALLOC__H
#define ION_ALLOC__H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <asm-generic/ioctl.h>

#define ION_TEST_ID_BASE                     	 100
#define ION_TEST_FUNC_ALLOC                      (ION_TEST_ID_BASE + 1)
#define ION_TEST_FUNC_FREE                       (ION_TEST_ID_BASE + 2)
#define ION_TEST_FUNC_SHARE                      (ION_TEST_ID_BASE + 3)
#define ION_TEST_FUNC_IMPORT                     (ION_TEST_ID_BASE + 4)
#define ION_TEST_FUNC_SYNC                       (ION_TEST_ID_BASE + 5)
#define ION_TEST_FUNC_FLUSH_RANGE                (ION_TEST_ID_BASE + 6)
#define ION_TEST_FUNC_PHYS_ADDR                  (ION_TEST_ID_BASE + 7)
#define ION_TEST_FUNC_DMA_COPY                   (ION_TEST_ID_BASE + 8)
#define ION_TEST_FUNC_DUMP_CARVEOUT_BITMAP       (ION_TEST_ID_BASE + 9)

int ion_function_alloc(void);
int ion_function_free(void);
int ion_function_share(void);
int ion_function_import(void);
int ion_function_sync(void);
int ion_function_flush_range(void);
int ion_function_phys_addr(void);
int ion_function_dma_copy(void);
int ion_function_dump_carveout_bitmap(void);

#define ION_DEV_NAME		"/dev/ion"

#define SZ_64M			0x04000000
#define SZ_4M			0x00400000
#define SZ_1M			0x00100000
#define SZ_64K			0x00010000
#define SZ_4K			0x00001000
#define ION_ALLOC_SIZE		(SZ_4M + SZ_1M - SZ_64K)
#define ION_ALLOC_ALIGN		(SZ_4K)

typedef int ion_user_handle_t;

struct ion_allocation_data {
	size_t len;
	size_t align;
	unsigned int heap_id_mask;
	unsigned int flags;
	ion_user_handle_t handle;
};

struct ion_handle_data {
	ion_user_handle_t handle;
};

struct ion_fd_data {
	ion_user_handle_t handle;
	int fd;
};

struct ion_custom_data {
	unsigned int cmd;
	unsigned long arg;
};

enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_CHUNK,
	ION_HEAP_TYPE_DMA,
	ION_HEAP_TYPE_CUSTOM, /* must be last so device specific heaps always
				 are at the end of this enum */
	ION_NUM_HEAPS = 16,
};

/* standard cmd from kernel head file */
#define ION_IOC_MAGIC		'I'
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP		_IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_SHARE		_IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)
#define ION_IOC_IMPORT		_IOWR(ION_IOC_MAGIC, 5, struct ion_fd_data)
#define ION_IOC_SYNC		_IOWR(ION_IOC_MAGIC, 7, struct ion_fd_data)
#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

/* extend cmd of sunxi platform */
#define ION_IOC_SUNXI_FLUSH_RANGE       5
#define ION_IOC_SUNXI_FLUSH_ALL         6
#define ION_IOC_SUNXI_PHYS_ADDR         7
#define ION_IOC_SUNXI_DMA_COPY          8
#define ION_IOC_SUNXI_DUMP              9

/* heap mask from which to alloc buffer */
#define ION_HEAP_SYSTEM_MASK		(1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK	(1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
#define ION_HEAP_CARVEOUT_MASK		(1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK		(1 << ION_HEAP_TYPE_DMA)

#define ION_FLAG_CACHED 1		/* mappings of this buffer should be
					   cached, ion will do cache
					   maintenance when the buffer is
					   mapped for dma */
#define ION_FLAG_CACHED_NEEDS_SYNC 2	/* mappings of this buffer will created
					   at mmap time, if this is set
					   caches must be managed manually */

/* used for ION_IOC_SUNXI_FLUSH_RANGE cmd */
typedef struct {
	long 	start;			/* start virtual address */
	long 	end;			/* end virtual address */
}sunxi_cache_range;

/* used for ION_IOC_SUNXI_PHYS_ADDR cmd */
typedef struct {
	ion_user_handle_t handle;
	unsigned int phys_addr;
	unsigned int size;
}sunxi_phys_data;

struct buffer_info_t
{
	int fd;
	void *virt_addr;
	int mem_size;
	struct ion_allocation_data alloc_data;
};

void ion_init();
int ion_memory_request(struct buffer_info_t* buffer_info);
int ion_memory_release(struct buffer_info_t* buffer_info);
void ion_dump();
void ion_deinit();

#endif /* ION_ALLOC__H */
