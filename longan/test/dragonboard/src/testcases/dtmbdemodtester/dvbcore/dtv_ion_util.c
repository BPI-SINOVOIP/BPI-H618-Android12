#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include "dtmbip_sunxi.h"

enum aw_ion_new_heap_type {
    CDC_ION_HEAP_TYPE_SYSTEM = 0,
    CDC_ION_HEAP_TYPE_DMA = 2,
};

/**
 * struct ion_new_allocation_data - metadata passed from userspace for allocations
 * @len:            size of the allocation
 * @heap_id_mask:   mask of heap ids to allocate from
 * @flags:          flags passed to heap
 * @handle:         pointer that will be populated with a cookie to use to
 *                  refer to this allocation
 *
 * Provided by userspace as an argument to the ioctl - added _new to denote
 * this belongs to the new ION interface.
 */
struct aw_ion_new_alloc_data {
    __u64 len;
    __u32 heap_id_mask;
    __u32 flags;
    __u32 fd;
    __u32 unused;
};

struct dtv_ion_priv {
    int fd_ion;
    int fd_dtmb;

    int ref;
} gContext;

/**
 * DOC: ION_IOC_NEW_ALLOC - allocate memory
 *
 * Takes an ion_allocation_data struct and returns it with the handle field
 * populated with the opaque handle for the allocation.
 * TODO: This IOCTL will clash by design; however, only one of
 *  ION_IOC_ALLOC or ION_IOC_NEW_ALLOC paths will be exercised,
 *  so this should not conflict.
 */
#define AW_ION_IOC_NEW_ALLOC        _IOWR(AW_MEM_ION_IOC_MAGIC, 0, struct aw_ion_new_alloc_data)

#define AW_MEM_ION_IOC_MAGIC        'I'
#define AW_CARVEROUT_HEAP_MASK       0
#define AW_ION_NEW_SYSTEM_HEAP_MASK (1 << CDC_ION_HEAP_TYPE_SYSTEM)
#define	AW_SYSTEM_HEAP_MASK         AW_ION_NEW_SYSTEM_HEAP_MASK

int dtv_ion_get_phyAddr(int dma_buf_fd, unsigned long *pAddr)
{
    int ret;
    struct dtmb_user_iommu_param nIommuBuffer;

    nIommuBuffer.fd = dma_buf_fd;
    ret = ioctl(gContext.fd_dtmb, DTMBIP_CMD_MALLOC_IOMMU_ADDR, &nIommuBuffer);
    if(ret < 0 || nIommuBuffer.iommu_addr & 0xff) {
        printf("get iommu addr maybe wrong:%x", nIommuBuffer.iommu_addr);
        return -1;
    }

    *pAddr = (unsigned long)nIommuBuffer.iommu_addr;

    return 0;
}

int dtv_ion_FreeFd(int dma_buf_fd)
{
    int ret;
    struct dtmb_user_iommu_param nIommuBuffer;

    nIommuBuffer.fd = dma_buf_fd;
    ret = ioctl(gContext.fd_dtmb, DTMBIP_CMD_FREE_IOMMU_ADDR, &nIommuBuffer);
    if(ret < 0) {
        printf("free iommu addr maybe wrong:%x fd=%d", nIommuBuffer.iommu_addr, nIommuBuffer.fd);
        return -1;
    }

    return 0;
}

void dtv_ion_CloseFd(int dma_buf_fd)
{
    close(dma_buf_fd);
}

int dtv_ion_open(void)
{
    int fd_ion, fd_dtmb;

    if (gContext.ref == 0) {
        gContext.fd_ion = open("/dev/ion", O_RDWR);
        if (gContext.fd_ion < 0) {
            printf("open ion node fail fd_ion=%d\n", gContext.fd_ion);
            return -1;
        }

        gContext.fd_dtmb = open(DTMBIP_DEV, O_RDWR);
        if (gContext.fd_dtmb < 0) {
            printf("open %s node fail fd_dtmb=%d\n", DTMBIP_DEV, gContext.fd_dtmb);
            return -1;
        }
    }
    gContext.ref++;

    return 0;
}

int dtv_ion_close(void)
{
    int ret;

    gContext.ref--;
    if (gContext.ref == 0) {
        ret = close(gContext.fd_ion);
        if (ret < 0) {
            printf("close ion node failed with code %d: %s\n", ret, strerror(errno));
            return -errno;
        }

        ret = close(gContext.fd_dtmb);
        if (ret < 0) {
            printf("close ion node failed with code %d: %s\n", ret, strerror(errno));
            return -errno;
        }
    }

    return 0;
}

int dtv_ion_AllocFd(size_t len, int *dma_buf_fd)
{
    int ret = 0;
    struct aw_ion_new_alloc_data data = {
        .len = len,
        .heap_id_mask = AW_SYSTEM_HEAP_MASK | AW_CARVEROUT_HEAP_MASK,
        .flags = 1,
    };

    if (!dma_buf_fd) {
        printf("fd pointer is null");
        return -1;
    }

    ret = ioctl(gContext.fd_ion, AW_ION_IOC_NEW_ALLOC, &data);
    if (ret < 0) {
        printf("new alloc fail");
        return -1;
    }

    *dma_buf_fd = data.fd;

    return ret;
}

int dtv_ion_mmap(int buf_fd, size_t len, unsigned char **pVirAddr)
{
    /* mmap to user */
    *pVirAddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, buf_fd, 0);
    if(MAP_FAILED == pVirAddr)
    {
        printf("dtv_ion_mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    return 0;
}

int dtv_ion_munmap(size_t len, unsigned char *pVirAddr)
{
    int ret;
    ret = munmap((void*)pVirAddr, len);
    if(ret) {
        printf("dtv_ion_munmap failed: %s\n", strerror(errno));
        return -errno;
    }

    return 0;
}

void dtv_ion_flush_cache(void *startAddr, int size)
{
    int ret;
    struct cache_range range;
    unsigned long addr = 0;

    /* clean and invalid user cache */
    addr = (unsigned long)startAddr;
    range.start = (long long)addr;
    range.end = (long long)addr + size;
    ret = ioctl(gContext.fd_dtmb, DTMBIP_CMD_FLUSH_CACHE_RANGE, &range);
    if (ret) {
        printf("DTMBIP_CMD_FLUSH_CACHE_RANGE failed ret: %d start:%llx, end:%llx, size:%d %u",
                ret, range.start, range.end, size, sizeof(struct cache_range));
    }
}

int dtv_ion_testsample(void)
{
    int dma_buf_fd, ret;
    unsigned char *addr_vir = NULL;
    unsigned long addr_phy = 0;
    int size = 4 * 1024 * 1024;

    ret = dtv_ion_open();
    if (ret < 0) {
        printf("dtv_ion_open error\n");
        return -1;
    }

    ret = dtv_ion_AllocFd(size, &dma_buf_fd);
    if (ret < 0) {
        printf("dtv_ion_AllocFd error\n");
        return -1;
    }

    ret = dtv_ion_mmap(dma_buf_fd, size, &addr_vir);
    if (ret < 0) {
        printf("dtv_ion_mmap error\n");
        return -1;
    }

    ret = dtv_ion_get_phyAddr(dma_buf_fd, &addr_phy);
    if(ret < 0) {
        printf("get phy addr error\n");
        dtv_ion_munmap(size, addr_vir);
        return -1;
    }

    printf("DTMBIP_CMD_TEST success phy_addr=0x%lx vir_addr=%p\n", addr_phy, addr_vir);

    /* test vir addr */
    sprintf((char *)addr_vir, "hello world!");
    printf("%s\n", addr_vir);

    dtv_ion_munmap(size, addr_vir);
    dtv_ion_FreeFd(dma_buf_fd);
    dtv_ion_CloseFd(dma_buf_fd);
    dtv_ion_close();

    return 0;
}