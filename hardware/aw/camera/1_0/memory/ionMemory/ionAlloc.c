/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : ionAlloc.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/


/*
 * ion_alloc.c
 *
 * john.fu@allwinnertech.com
 *
 * ion memory allocate
 *
 */

//#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DETAIL
#define LOG_TAG "ionAlloc"

#include <cutils/log.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ion/ion.h>

#include "memory/ionMemory/ionAlloc.h"
#include <stdbool.h>

#include "CameraDebug.h"
#if DBG_ION_ALLOC
#define LOG_NDEBUG 0
#endif

//#include "veAdapter.h"
//#include "veInterface.h"

ion_alloc_context    *g_alloc_context = NULL;
pthread_mutex_t      g_mutex_alloc = PTHREAD_MUTEX_INITIALIZER;

/*funciton begin*/
int ion_cam_alloc_open()
{
    LOGV("zjw,in cam jiangwei,begin ion_alloc_open \n");
    int kernel_version_fd;
    int retbytes;
    char kernel_version[20];
    char kernel_num[3];
    pthread_mutex_lock(&g_mutex_alloc);
    if (g_alloc_context != NULL)
    {
        LOGV("ion allocator has already been created \n");
        goto SUCCEED_OUT;
    }

    g_alloc_context = (ion_alloc_context*)malloc(sizeof(ion_alloc_context));
    if (g_alloc_context == NULL) {
        LOGE("create ion allocator failed, out of memory \n");
        goto ERROR_OUT;
    } else {
        LOGV("pid: %d, g_alloc_context = %p \n", getpid(), g_alloc_context);
    }

    memset((void*)g_alloc_context, 0, sizeof(ion_alloc_context));

    /* Readonly should be enough. */
    g_alloc_context->fd = open(DEV_NAME, O_RDONLY, 0);

    if (g_alloc_context->fd <= 0)
    {
        LOGE("open %s failed \n", DEV_NAME);
        goto ERROR_OUT;
    }
#if 0
    kernel_version_fd = open(KERNEL_NAME,O_RDONLY,0);
    if (kernel_version_fd <= 0)
    {
        LOGE("open %s failed \n", KERNEL_NAME);
    }
    retbytes = read(kernel_version_fd,kernel_version,20);
    //plus 14 to skip the string "Linux version "!
    strncpy(kernel_num,kernel_version+14,3);
    LOGD("zjw,readkernelversion,kernel_version = %s ,kernel_num = %s,retbytes = %d !",
                kernel_version,
                kernel_num,
                retbytes);
    retbytes = strncmp(kernel_num,KERNEL_VERSION_4_9,3);
    LOGD("zjw,strncmp = %d",retbytes);
    if(strncmp(kernel_num,KERNEL_VERSION_4_9,3) == 0)
    {
        g_alloc_context->flag_iommu = 1;
        LOGD("zjw,set g_alloc_context->flag_iommu = 1, the kernel version is 4.9");
    } else {
        LOGD("zjw,g_alloc_context->flag_iommu = %d!",g_alloc_context->flag_iommu);
    }
    close(kernel_version_fd);
#endif

#ifdef USE_IOMMU_DRIVER
    g_alloc_context->flag_iommu = 1;
#else
    g_alloc_context->flag_iommu = 0;
#endif

#if DEBUG_ION_REF==1
    cam_use_mem = 0;
    memset(&ion_buf_nodes_test, 0, sizeof(ion_buf_nodes_test));
    cam_use_mem = ion_cam_alloc_get_total_size();
    LOGD("ion_open, cam_use_mem=[%dByte].", cam_use_mem);
#endif

    AW_MEM_INIT_LIST_HEAD(&g_alloc_context->list);

SUCCEED_OUT:
    g_alloc_context->ref_cnt++;
    pthread_mutex_unlock(&g_mutex_alloc);
    return 0;

ERROR_OUT:
    if (g_alloc_context != NULL
        && g_alloc_context->fd > 0)
    {
        close(g_alloc_context->fd);
        g_alloc_context->fd = 0;
    }

    if (g_alloc_context != NULL)
    {
        free(g_alloc_context);
        g_alloc_context = NULL;
    }

    pthread_mutex_unlock(&g_mutex_alloc);
    return -1;
}

void ion_cam_alloc_close()
{
    struct aw_mem_list_head * pos, *q;

    LOGV("zjw,in cam ion_alloc_close \n");

    pthread_mutex_lock(&g_mutex_alloc);
    if (--g_alloc_context->ref_cnt <= 0)
    {
        LOGV("pid: %d, release g_alloc_context = %p \n", getpid(), g_alloc_context);

        aw_mem_list_for_each_safe(pos, q, &g_alloc_context->list)
        {
            buffer_node * tmp;
            tmp = aw_mem_list_entry(pos, buffer_node, i_list);
            LOGV("ion_alloc_close del item phy= 0x%lx vir= 0x%lx, size= %d \n", \
                tmp->phy, tmp->vir, tmp->size);
            aw_mem_list_del(pos);
            free(tmp);
        }
#if DEBUG_ION_REF==1
        LOGD("ion_close, cam_use_mem=[%d MB]", cam_use_mem/1024/1024);
        ion_cam_alloc_get_total_size();
#endif
        close(g_alloc_context->fd);
        g_alloc_context->fd = 0;

        free(g_alloc_context);
        g_alloc_context = NULL;
    }
    else {
        LOGV("ref cnt: %d > 0, do not free \n", g_alloc_context->ref_cnt);
    }
    pthread_mutex_unlock(&g_mutex_alloc);

    //--------------
#if DEBUG_ION_REF==1
    int i = 0;
    int counter = 0;
    for(i=0; i<ION_BUF_LEN; i++)
    {
        if(ion_buf_nodes_test[i].addr != 0 || ion_buf_nodes_test[i].size != 0){

            LOGE("ion mem leak????  addr->[0x%x], leak size->[%dByte]", \
                ion_buf_nodes_test[i].addr, ion_buf_nodes_test[i].size);
            counter ++;
        }
    }

    if(counter != 0)
    {
        LOGE("my god, have [%d]blocks ion mem leak.!!!!", counter);
    }
    else
    {
        LOGD("well done, no ion mem leak.");
    }
#endif
    //--------------
    return ;
}

// return virtual address: 0 failed
void* ion_cam_alloc_palloc(int size,int * pFd)
{

    struct ion_allocation_data alloc_data;
    struct ion_fd_data fd_data;
    struct ion_handle_data handle_data;
    struct ion_custom_data custom_data;
    sunxi_phys_data   phys_data;
    LOGV("zjw,in cam ion_cam_alloc_palloc\n");

    int rest_size = 0;
    unsigned long addr_phy = 0;
    unsigned long addr_vir = 0;
    buffer_node * alloc_buffer = NULL;
    int ret = 0;

    pthread_mutex_lock(&g_mutex_alloc);

    if (g_alloc_context == NULL)
    {
        LOGE("ion_alloc do not opened, should call ion_alloc_open() \
            before ion_alloc_alloc(size) \n");
        goto ALLOC_OUT;
    }

    if(size <= 0)
    {
        LOGE("can not alloc size 0 \n");
        goto ALLOC_OUT;
    }

    alloc_data.len = (size_t)size;
    alloc_data.align = ION_ALLOC_ALIGN ;

    if(g_alloc_context->flag_iommu)
    {
        LOGV("ION_IOC_ALLOC use system heap for iommu! \n");
        alloc_data.heap_id_mask = ION_HEAP_SYSTEM_MASK | ION_HEAP_CARVEOUT_MASK;
    } else {
        LOGV("ION_IOC_ALLOC use dma heap ! \n");
        alloc_data.heap_id_mask = ION_HEAP_TYPE_DMA_MASK | ION_HEAP_CARVEOUT_MASK;
    }
    alloc_data.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

    ret = ioctl(g_alloc_context->fd, ION_IOC_ALLOC, &alloc_data);
    if (ret)
    {
        LOGE("ION_IOC_ALLOC error \n");
        goto ALLOC_OUT;
    }

    /* get dmabuf fd */
    fd_data.handle = alloc_data.handle;

    ret = ioctl(g_alloc_context->fd, ION_IOC_SHARE, &fd_data);
    if(ret) {
        LOGE("ION_IOC_SHARE err, ret %d, dmabuf fd 0x%08x\n", ret, (unsigned int)fd_data.fd);
        goto ALLOC_OUT;
    }
    if(fd_data.fd < 0) {
        LOGE("ION_IOC_SHARE ioctl returned negative fd\n");
        goto ALLOC_OUT;
    }

    *pFd = fd_data.fd;

    if(g_alloc_context->flag_iommu == 0)
    {
        memset(&phys_data, 0, sizeof(sunxi_phys_data));
        (void)(phys_data.size);
        custom_data.cmd = ION_IOC_SUNXI_PHYS_ADDR;
        phys_data.handle = (ion_user_handle_t)fd_data.handle;
        custom_data.arg = (unsigned long)&phys_data;
        ret = ioctl(g_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
        if(ret < 0)
        {
            LOGW("cam IonGethandle failed \n");
            //return 0;
        }
        addr_phy = phys_data.phys_addr;
    }

    /* mmap to user */
    addr_vir = (unsigned long)mmap(NULL, alloc_data.len, \
        PROT_READ|PROT_WRITE, MAP_SHARED, fd_data.fd, 0);
    if((unsigned long)MAP_FAILED == addr_vir)
    {
        LOGE("mmap err, ret %d\n", (unsigned int)addr_vir);
        addr_vir = 0;
        goto ALLOC_OUT;
    }

    alloc_buffer = (buffer_node *)malloc(sizeof(buffer_node));
    if (alloc_buffer == NULL)
    {
        LOGE("malloc buffer node failed");

        /* unmmap */
        ret = munmap((void*)addr_vir, alloc_data.len);
        if(ret) {
            LOGE("munmap err, ret %d\n", ret);
        }

        /* close dmabuf fd */
        close(fd_data.fd);

        /* free buffer */
        handle_data.handle = alloc_data.handle;
        ret = ioctl(g_alloc_context->fd, ION_IOC_FREE, &handle_data);

        if(ret) {
            LOGE("ION_IOC_FREE err, ret %d\n", ret);
        }

        addr_phy = 0;
        addr_vir = 0;        // value of MAP_FAILED is -1, should return 0

        goto ALLOC_OUT;
    }

    alloc_buffer->phy     = addr_phy;
    alloc_buffer->vir     = addr_vir;
    alloc_buffer->user_virt = addr_vir;
    alloc_buffer->size    = size;
    alloc_buffer->fd_data.handle = fd_data.handle;
    alloc_buffer->fd_data.fd = fd_data.fd;

    LOGV("zjw,alloc succeed, addr_phy: 0x%lx, addr_vir: 0x%lx, size: %d", addr_phy, addr_vir, size);

    aw_mem_list_add_tail(&alloc_buffer->i_list, &g_alloc_context->list);

    //------start-----------------
#if DEBUG_ION_REF==1
    cam_use_mem += size;
    LOGD("++++++cam_use_mem = [%d MB], increase size->[%d B], addr_vir=[0x%x], addr_phy=[0x%x]", \
        cam_use_mem/1024/1024, size, addr_vir, addr_phy);
    int i = 0;
    for(i=0; i<ION_BUF_LEN; i++)
    {
        if(ion_buf_nodes_test[i].addr == 0 && ion_buf_nodes_test[i].size == 0){
            ion_buf_nodes_test[i].addr = addr_vir;
            ion_buf_nodes_test[i].size = size;
            break;
        }
    }

    if(i>= ION_BUF_LEN){
        LOGE("error, ion buf len is large than [%d]", ION_BUF_LEN);
    }
#endif
//--------------------------------

ALLOC_OUT:
    pthread_mutex_unlock(&g_mutex_alloc);
    return (void*)addr_vir;
}

void ion_cam_alloc_pfree(void * pbuf)
{

    int flag = 0;
    unsigned long addr_vir = (unsigned long)pbuf;
    buffer_node * tmp;
    int ret;
    struct ion_handle_data handle_data;
    LOGV("zjw,in cam ion_cam_alloc_pfree\n");

    if (0 == pbuf)
    {
        LOGE("can not free NULL buffer \n");
        return ;
    }

    pthread_mutex_lock(&g_mutex_alloc);

    if (g_alloc_context == NULL)
    {
        LOGE("ion_alloc do not opened, should call ion_alloc_open() \
            before ion_alloc_alloc(size) \n");
        return ;
    }

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (tmp->vir == addr_vir)
        {
            LOGV("ion_alloc_free item phy= 0x%lx vir= 0x%lx, size= %d \n", \
                tmp->phy, tmp->vir, tmp->size);

            if (munmap((void *)(tmp->user_virt), tmp->size) < 0)
            {
                LOGE("munmap 0x%p, size: %d failed \n", (void*)addr_vir, tmp->size);
            }

            /*close dma buffer fd*/
            close(tmp->fd_data.fd);

            /* free buffer */
            handle_data.handle = tmp->fd_data.handle;

            ret = ioctl(g_alloc_context->fd, ION_IOC_FREE, &handle_data);
            if (ret)
            {
                LOGE("ION_IOC_FREE failed \n");
            }

            aw_mem_list_del(&tmp->i_list);
            free(tmp);

            flag = 1;

            //------start-----------------
#if DEBUG_ION_REF==1
            int i = 0;
            for(i=0; i<ION_BUF_LEN; i++)
            {
                if(ion_buf_nodes_test[i].addr == addr_vir && ion_buf_nodes_test[i].size > 0){

                    cam_use_mem -= ion_buf_nodes_test[i].size;
                    LOGV("--------cam_use_mem = [%d MB], reduce size->[%d B]",\
                        cam_use_mem/1024/1024, ion_buf_nodes_test[i].size);
                    ion_buf_nodes_test[i].addr = 0;
                    ion_buf_nodes_test[i].size = 0;

                    break;
                }
            }

            if(i>= ION_BUF_LEN){
                LOGE("error, ion buf len is large than [%d]", ION_BUF_LEN);
            }
#endif
            //--------------------------------

            break;
        }
    }

    if (0 == flag)
    {
        LOGE("ion_alloc_free failed, do not find virtual address: 0x%lx \n", addr_vir);
    }

    pthread_mutex_unlock(&g_mutex_alloc);
    return ;
}


#if 1
unsigned long ion_cam_get_phyadr(int share_fd)
{

    int ret = 0;
    struct ion_handle_data handle_data;
    struct ion_custom_data custom_data;
    sunxi_phys_data phys_data;
    struct ion_fd_data fd_data;
    LOGV("cam ion_cam_get_phyadr \n");

//get ion_handle
    memset(&fd_data, 0, sizeof(struct ion_fd_data));
    fd_data.fd = share_fd;
    ret = ioctl(g_alloc_context->fd, ION_IOC_IMPORT, &fd_data);
    if(ret)
    {
        LOGE("cam IonImport get ion_handle err, ret %d\n",ret);
        return -1;
    }

    handle_data.handle = fd_data.handle;

    memset(&phys_data, 0, sizeof(sunxi_phys_data));
    (void)(phys_data.size);
    custom_data.cmd = ION_IOC_SUNXI_PHYS_ADDR;
    phys_data.handle = handle_data.handle;
    custom_data.arg = (unsigned long)&phys_data;
    ret = ioctl(g_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
    if(ret < 0)
    {
        LOGE("cam IonGethandle failed \n");
        return 0;
    }
    LOGV("cam IonGethandle sucess\n");

    return phys_data.phys_addr;

}
#endif

void* ion_cam_alloc_vir2phy_cpu(void * pbuf)
{
    int flag = 0;
    unsigned long addr_vir = (unsigned long)pbuf;
    unsigned long addr_phy = 0;
    buffer_node * tmp;

    if (0 == pbuf)
    {
        // LOGV("can not vir2phy NULL buffer \n");
        return 0;
    }

    pthread_mutex_lock(&g_mutex_alloc);

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (addr_vir >= tmp->vir
            && addr_vir < tmp->vir + tmp->size)
        {
            addr_phy = tmp->phy + addr_vir - tmp->vir;
            //LOGV("ion_alloc_vir2phy phy= 0x%08x vir= 0x%08x \n", addr_phy, addr_vir);
            flag = 1;
            break;
        }
    }

    if (0 == flag)
    {
        LOGE("ion_alloc_vir2phy failed, do not find virtual address: 0x%lx \n", addr_vir);
    }

    pthread_mutex_unlock(&g_mutex_alloc);

    return (void*)addr_phy;
}

void* ion_cam_alloc_phy2vir_cpu(void * pbuf)
{
    int flag = 0;
    unsigned long addr_vir = 0;
    unsigned long addr_phy = (unsigned long)pbuf;
    buffer_node * tmp;

    if (0 == pbuf)
    {
        LOGE("can not phy2vir NULL buffer \n");
        return 0;
    }

    pthread_mutex_lock(&g_mutex_alloc);

    aw_mem_list_for_each_entry(tmp, &g_alloc_context->list, i_list)
    {
        if (addr_phy >= tmp->phy
            && addr_phy < tmp->phy + tmp->size)
        {
            addr_vir = tmp->vir + addr_phy - tmp->phy;
            flag = 1;
            break;
        }
    }

    if (0 == flag)
    {
        LOGE("ion_alloc_phy2vir failed, do not find physical address: 0x%lx \n", addr_phy);
    }

    pthread_mutex_unlock(&g_mutex_alloc);

    return (void*)addr_vir;
}

void* ion_cam_alloc_vir2phy_ve(void * pbuf)
{
    LOGV("**11 phy offset = %x",g_alloc_context->phyOffset);

    return (void*)((unsigned long)ion_cam_alloc_vir2phy_cpu(pbuf) - g_alloc_context->phyOffset);
}

void* ion_cam_alloc_phy2vir_ve(void * pbuf)
{
    LOGV("**22 phy offset = %x",g_alloc_context->phyOffset);

    return (void*)((unsigned long)ion_cam_alloc_phy2vir_cpu(pbuf) - g_alloc_context->phyOffset);
}

#ifdef CONF_KERNEL_VERSION_3_10
void ion_cam_alloc_flush_cache(void* startAddr, int size)
{
    sunxi_cache_range range;
    int ret;

    /* clean and invalid user cache */
    range.start = (unsigned long)startAddr;
    range.end = (unsigned long)startAddr + size;
    ret = ioctl(g_alloc_context->fd, ION_IOC_SUNXI_FLUSH_RANGE, &range);
    if (ret)
    {
        LOGE("ION_IOC_SUNXI_FLUSH_RANGE failed \n");
    }

    return;
}
#else
void ion_cam_alloc_flush_cache(void* startAddr, int size)
{
    sunxi_cache_range range;
    int ret;

    struct ion_custom_data custom_data;

    /* clean and invalid user cache */
    range.start = (unsigned long)startAddr;
    range.end = (unsigned long)startAddr + size;

    LOGV("start:%p, end:%lx, size:%lx(%ld)\n", startAddr, range.end, (long)size, (long)size, (long)size);
    ret = ioctl(g_alloc_context->fd, ION_IOC_SUNXI_FLUSH_RANGE, &range);

    if (ret)
    {
        LOGE("ION_IOC_SUNXI_FLUSH_RANGE failed \n");
    }

    return;
}
#endif
void ion_cam_alloc_sync_cache(int handle_fd)
{
    sunxi_cache_range range;
    int ret;

    struct ion_fd_data fd_data;

    fd_data.fd = handle_fd;

    ret = ioctl(g_alloc_context->fd, ION_IOC_SYNC, &fd_data);

    if (ret)
    {
        LOGE("ION_IOC_SYNC failed \n");
    }

    return;
}

void ion_cam_flush_cache_all()
{
#if 0 //jiangwei
    CEDARC_UNUSE(ion_flush_cache_all);
#endif
    ioctl(g_alloc_context->fd, ION_IOC_SUNXI_FLUSH_ALL, 0);
}

//return total meminfo with MB
int ion_cam_alloc_get_total_size()
{
    int ret = 0;

    int ion_fd = open(DEV_NAME, O_WRONLY);

    if (ion_fd < 0) {
        LOGE("open ion dev failed, cannot get ion mem.");
        goto err;
    }

    struct sunxi_pool_info binfo = {
        .total   = 0,    // mb
        .free_kb  = 0, //the same to free_mb
        .free_mb = 0,
    };

    struct ion_custom_data cdata;

    cdata.cmd = ION_IOC_SUNXI_POOL_INFO;
    cdata.arg = (unsigned long)&binfo;
    ret = ioctl(ion_fd,ION_IOC_CUSTOM, &cdata);
    if (ret < 0){
        LOGE("Failed to ioctl ion device, errno:%s\n", strerror(errno));
        goto err;
    }

    LOGV(" ion dev get free pool [%d MB], total [%d MB]\n", binfo.free_mb, binfo.total / 1024);
    ret = binfo.total;
err:
    if(ion_fd > 0){
        close(ion_fd);
    }
    return ret;
}

int ion_cam_alloc_memset(void* buf, int value, size_t n)
{
    memset(buf, value, n);
    return -1;
}

int ion_cam_alloc_copy(void* dst, void* src, size_t n)
{
    memcpy(dst, src, n);
    return -1;
}

int ion_cam_alloc_read(void* dst, void* src, size_t n)
{
    memcpy(dst, src, n);
    return -1;
}

int ion_cam_alloc_write(void* dst, void* src, size_t n)
{
    memcpy(dst, src, n);
    return -1;
}

int ion_cam_alloc_setup()
{
    return -1;
}

int ion_cam_alloc_shutdown()
{
    return -1;
}

struct ScCamMemOpsS _ionCamMemOpsS =
{
    open_cam:                 ion_cam_alloc_open,
    close_cam:                 ion_cam_alloc_close,
    total_size_cam:         ion_cam_alloc_get_total_size,
    palloc_cam:             ion_cam_alloc_palloc,
    pfree_cam:                ion_cam_alloc_pfree,
    flush_cache_cam:        ion_cam_alloc_flush_cache,
    sync_cache_cam:		ion_cam_alloc_sync_cache,
    ve_get_phyaddr_cam:     ion_cam_alloc_vir2phy_ve,
    ve_get_viraddr_cam:     ion_cam_alloc_phy2vir_ve,
    cpu_get_phyaddr_cam:    ion_cam_alloc_vir2phy_cpu,
    cpu_get_viraddr_cam:    ion_cam_alloc_phy2vir_cpu,
    mem_set_cam:            ion_cam_alloc_memset,
    mem_cpy_cam:            ion_cam_alloc_copy,
    mem_read_cam:            ion_cam_alloc_read,
    mem_write_cam:            ion_cam_alloc_write,
    setup_cam:                ion_cam_alloc_setup,
    shutdown_cam:            ion_cam_alloc_shutdown,
    //palloc_secure_cam:      ion_cam_alloc_alloc_drm,
    get_phyadr_cam:      ion_cam_get_phyadr,
	//get_phyadr_cam: 	 ion_test,

};

struct ScCamMemOpsS* __GetIonCamMemOpsS()
{
    LOGV("*** get __GetIonCamMemOpsS ***");
    return &_ionCamMemOpsS;
}
