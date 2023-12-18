
#ifndef _SUNXI_DRM_HEAP_H
#define _SUNXI_DRM_HEAP_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * struct dma_heap_allocation_data - metadata passed from userspace for
 *                                      allocations
 * @len:		size of the allocation
 * @fd:			will be populated with a fd which provides the
 *			handle to the allocated dma-buf
 * @fd_flags:		file descriptor flags used when allocating
 * @heap_flags:		flags passed to heap
 *
 * Provided by userspace as an argument to the ioctl
 */
struct dma_heap_allocation_data {
	__u64 len;
	__u32 fd;
	__u32 fd_flags;
	__u64 heap_flags;
};

typedef struct sunxi_drm_phys_data {
	int handle;
	unsigned int tee_addr;
	unsigned int phys_addr;
	unsigned int size;
} sunxi_drm_phys_data;

#define DMA_HEAP_IOC_MAGIC 'H'

/**
 * DOC: DMA_HEAP_IOC_ALLOC - allocate memory from pool
 *
 * Takes a dma_heap_allocation_data struct and returns it with the fd field
 * populated with the dmabuf handle of the allocation.
 */
#define DMA_HEAP_IOC_ALLOC                                                     \
	_IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)

#define DMA_HEAP_GET_ADDR _IOWR(DMA_HEAP_IOC_MAGIC, 0x1, sunxi_drm_phys_data)

#endif
