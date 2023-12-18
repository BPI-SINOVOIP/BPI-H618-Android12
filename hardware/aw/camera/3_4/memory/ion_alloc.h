#ifndef _ION_ALLOCATOR_
#define _ION_ALLOCATOR_

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
#include <pthread.h>
#include <asm-generic/ioctl.h>

#if (defined CONF_KERNEL_VERSION_3_10) ||      \
    (defined CONF_KERNEL_VERSION_4_4) ||       \
    (defined CONF_KERNEL_VERSION_4_9)
    typedef int aw_ion_user_handle_t;
    #define ION_USER_HANDLE_INIT_VALUE (-1)
#else
    typedef void* aw_ion_user_handle_t;
    #define ION_USER_HANDLE_INIT_VALUE (NULL)
#endif

typedef struct aw_ion_allocation_info
{
    size_t aw_len;
    //size_t aw_align;
    unsigned int aw_heap_id_mask;
    unsigned int flags;
    int fd;
    unsigned int unsused;
    //aw_ion_user_handle_t handle;
} aw_ion_allocation_info_t;

typedef struct aw_ion_fd_data {
    aw_ion_user_handle_t handle;
    int aw_fd;
} ion_fd_data_t;



typedef struct {
    long    start;
    long    end;
} sunxi_cache_range;
#ifdef __cplusplus
extern "C" {
    unsigned long aw_ion_alloc(int size);
    int aw_ion_free(void *pbuf);
    int ion_vir2fd(void *pbuf);
    int ion_alloc_close();
    int ion_alloc_open();
}
#endif
#define SZ_64M        0x04000000
#define SZ_4M        0x00400000
#define SZ_1M        0x00100000
#define SZ_64K        0x00010000
#define SZ_4k        0x00001000
#define SZ_1k        0x00000400



#define AW_ION_CACHED_FLAG 1        /* mappings of this buffer should be cached, ion will do cache maintenance when the buffer is mapped for dma */
#define AW_ION_CACHED_NEEDS_SYNC_FLAG 2    /* mappings of this buffer will created at mmap time, if this is set caches must be managed manually */

#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6
#define ION_IOC_SUNXI_PHYS_ADDR             7
#define ION_IOC_SUNXI_DMA_COPY              8

#define AW_ION_SYSTEM_HEAP_TYPE 0
#define AW_ION_TYPE_HEAP_DMA 1
#define AW_ION_TYPE_HEAP_SECURE 6

#define AW_ION_SYSTEM_HEAP_MASK         (1 << AW_ION_SYSTEM_HEAP_TYPE)
#define AW_ION_DMA_HEAP_MASK            (1 << AW_ION_TYPE_HEAP_DMA)
#define AW_ION_SECURE_HEAP_MASK      (1 << AW_ION_TYPE_HEAP_SECURE)

#endif /*_ION_ALLOCATOR_*/
