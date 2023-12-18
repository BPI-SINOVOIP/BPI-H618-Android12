/*
 * ion_alloc.c
 *
 * john.fu@allwinnertech.com
 *
 * ion memory allocate
 *
 */

#include "ion_alloc.h"
#include "ion_alloc_list.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <utils/Log.h>
#define DEV_NAME        "/dev/ion"
#include <ion/ion.h>
#include <linux/ion.h>

typedef struct BUFFER_NODE
{
    struct aw_mem_list_head i_list;
    unsigned long phy;        /*phisical address*/
    unsigned long vir;        /*virtual address*/
    unsigned int size;        /*buffer size*/
    ion_fd_data_t fd_data;
}buffer_node;

typedef struct ION_ALLOC_CONTEXT
{
    int fd;            /* driver handle */
    struct aw_mem_list_head    list; /* buffer list */
    int ref_cnt;    /* reference count */
}ion_alloc_context;

ion_alloc_context *g_alloc_context = NULL;

/*funciton begin*/
int ion_alloc_open()
{

    if (g_alloc_context != NULL)
    {
        ALOGE("ion allocator has already been created ");
        goto SUCCEED_OUT;
    }

    ALOGD("begin ion_alloc_open ");
    g_alloc_context = (ion_alloc_context*)malloc(sizeof(ion_alloc_context));
    if (g_alloc_context == NULL)
    {
        ALOGE("create ion allocator failed, out of memory ");
        goto ERROR_OUT;
    }
    else
    {
        ALOGE("pid: %d, g_alloc_context = %p ", getpid(), g_alloc_context);
    }

    memset((void*)g_alloc_context, 0, sizeof(ion_alloc_context));

    g_alloc_context->fd = open(DEV_NAME, O_RDWR, 0);

    if (g_alloc_context->fd <= 0)
    {
        ALOGE("open %s failed ", DEV_NAME);
        goto ERROR_OUT;
    }

    AW_MEM_INIT_LIST_HEAD(&g_alloc_context->list);

SUCCEED_OUT:
    g_alloc_context->ref_cnt++;
    return 0;

ERROR_OUT:
    if (g_alloc_context != NULL && g_alloc_context->fd > 0)
    {
        close(g_alloc_context->fd);
        g_alloc_context->fd = 0;
    }

    if (g_alloc_context != NULL)
    {
        free(g_alloc_context);
        g_alloc_context = NULL;
    }
    return -1;
}

int ion_alloc_close()
{
    struct aw_mem_list_head *pos, *q;
    buffer_node *tmp;

    ALOGD("ion_alloc_close ");

    if (--g_alloc_context->ref_cnt <= 0)
    {
        ALOGD("pid: %d, release g_alloc_context = %p ", getpid(), g_alloc_context);

        aw_mem_list_for_each_safe(pos, q, &g_alloc_context->list)
        {
            tmp = aw_mem_list_entry(pos, buffer_node, i_list);
            ALOGD("ion_alloc_close del item phy = 0x%lx vir = 0x%lx, size = %d ", tmp->phy, tmp->vir, tmp->size);
            aw_mem_list_del(pos);
            free(tmp);
        }

        close(g_alloc_context->fd);
        g_alloc_context->fd = 0;

        free(g_alloc_context);
        g_alloc_context = NULL;
    }
    else
    {
        ALOGE("ref cnt: %d > 0, do not free ", g_alloc_context->ref_cnt);
    }
    return 0;
}

/* return virtual address: 0 failed */
unsigned long aw_ion_alloc(int size)
{
    aw_ion_allocation_info_t alloc_data;
    ion_fd_data_t fd_data;
    struct ion_handle_data handle_data;

    int rest_size = 0;
    unsigned long addr_phy = 0;
    unsigned long addr_vir = 0;
    buffer_node * alloc_buffer = NULL;
    int ret = 0;

    if (g_alloc_context == NULL)
    {
        ALOGE("ion_alloc do not opened, should call ion_alloc_open() before ion_alloc(size) ");
        goto ALLOC_OUT;
    }

    if(size <= 0)
    {
        ALOGE("can not alloc size 0 ");
        goto ALLOC_OUT;
    }

    alloc_data.aw_len = (size_t)size;

    alloc_data.aw_heap_id_mask = AW_ION_SYSTEM_HEAP_MASK;//for IOMMU

    alloc_data.flags = AW_ION_CACHED_FLAG;

    ret = ion_alloc_fd(g_alloc_context->fd, size, 0,  AW_ION_SYSTEM_HEAP_MASK, AW_ION_CACHED_FLAG, &alloc_data.fd);
    if (ret)
    {
        ALOGE("ION_IOC_ALLOC ret:%d error:%s ",  ret, strerror(errno));
        goto ALLOC_OUT;
    }

    /* mmap to user */
    addr_vir = (unsigned long)mmap(NULL, alloc_data.aw_len, PROT_READ | PROT_WRITE, MAP_SHARED, alloc_data.fd, 0);
    if((unsigned long)MAP_FAILED == addr_vir)
    {
        ALOGE("mmap err, ret %d", (unsigned int)addr_vir);
        addr_vir = 0;
        goto ALLOC_OUT;
    }

    alloc_buffer = (buffer_node *)malloc(sizeof(buffer_node));
    if (alloc_buffer == NULL)
    {
        ALOGE("malloc buffer node failed");

        /* unmmap */
        ret = munmap((void*)addr_vir, alloc_data.aw_len);
        if(ret)
            ALOGE("munmap err, ret %d", ret);

        /* close dmabuf fd */
        close(alloc_data.fd);

        addr_phy = 0;
        addr_vir = 0; /* value of MAP_FAILED is -1, should return 0 */

        goto ALLOC_OUT;
    }
    alloc_buffer->phy     = addr_phy;
    alloc_buffer->vir     = addr_vir;
    alloc_buffer->size    = size;
    alloc_buffer->fd_data.aw_fd = alloc_data.fd;
    ALOGD("ion vir addr 0x%08x, size %d, dmabuf fd %d", addr_vir, size, alloc_data.fd);

    aw_mem_list_add_tail(&alloc_buffer->i_list, &g_alloc_context->list);

ALLOC_OUT:
    return addr_vir;
}

int aw_ion_free(void *pbuf)
{
    int flag = 0;
    unsigned long addr_vir = (unsigned long)pbuf;
    buffer_node *tmp;
    int ret;
    struct ion_handle_data handle_data;
    int nFreeSize = 0;

    if (0 == pbuf)
    {
        ALOGE("can not free NULL buffer ");
        return 0;
    }

    if (g_alloc_context == NULL)
    {
        ALOGE("ion_alloc do not opened, should call ion_alloc_open() before ion_alloc(size) ");
        return 0;
    }

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (tmp->vir == addr_vir)
        {
            //ALOGE("ion_free item phy= 0x%lx vir= 0x%lx, size= %d ", tmp->phy, tmp->vir, tmp->size);
            /*unmap user space*/
            if (munmap(pbuf, tmp->size) < 0)
            {
                ALOGE("munmap 0x%p, size: %d failed ", (void*)addr_vir, tmp->size);
            }
            nFreeSize = tmp->size;

            ALOGD("munmap 0x%p, size", (void*)addr_vir, tmp->size);
            /*close dma buffer fd*/
            close(tmp->fd_data.aw_fd);
            aw_mem_list_del(&tmp->i_list);
            free(tmp);
            flag = 1;
            break;
        }
    }

    if (0 == flag)
        ALOGE("ion_free failed, do not find virtual address: 0x%lx ", addr_vir);
    return nFreeSize;
}

int ion_vir2fd(void *pbuf)
{
    int flag = 0, fd = -1;
    unsigned long addr_vir = (unsigned long)pbuf;
    buffer_node * tmp;

    if (0 == pbuf)
    {
        ALOGE("can not vir2phy NULL buffer ");
        return 0;
    }

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (addr_vir >= tmp->vir && addr_vir < tmp->vir + tmp->size)
        {
            fd = tmp->fd_data.aw_fd;
            //ALOGE("ion mem vir = 0x%08x, fd = %d", addr_vir, fd);
            flag = 1;
            break;
        }
    }

    if (0 == flag)
        ALOGE("ion_vir2fd failed, do not find virtual address: 0x%lx ", addr_vir);

    return fd;
}

unsigned long ion_vir2phy(void *pbuf)
{
    int flag = 0;
    unsigned long addr_vir = (unsigned long)pbuf;
    unsigned long addr_phy = 0;
    buffer_node * tmp;

    if (0 == pbuf)
    {
        ALOGE("can not vir2phy NULL buffer ");
        return 0;
    }

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (addr_vir >= tmp->vir && addr_vir < tmp->vir + tmp->size)
        {
            addr_phy = tmp->phy + addr_vir - tmp->vir;
            ALOGE("ion_vir2phy phy= 0x%08x vir= 0x%08x ", addr_phy, addr_vir);
            flag = 1;
            break;
        }
    }

    if (0 == flag)
        ALOGE("ion_vir2phy failed, do not find virtual address: 0x%lx ", addr_vir);

    return addr_phy;
}

unsigned long ion_phy2vir(void *pbuf)
{
    int flag = 0;
    unsigned long addr_vir = 0;
    unsigned long addr_phy = (unsigned long)pbuf;
    buffer_node * tmp;

    if (0 == pbuf)
    {
        ALOGE("can not phy2vir NULL buffer ");
        return 0;
    }

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (addr_phy >= tmp->phy && addr_phy < tmp->phy + tmp->size)
        {
            addr_vir = tmp->vir + addr_phy - tmp->phy;
            flag = 1;
            break;
        }
    }

    if (0 == flag)
        ALOGE("ion_phy2vir failed, do not find physical address: 0x%lx ", addr_phy);

    return addr_vir;
}


