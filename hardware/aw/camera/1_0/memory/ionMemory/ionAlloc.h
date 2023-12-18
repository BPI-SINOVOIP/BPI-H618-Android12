/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : ionAlloc.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

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


#include "memory/ionMemory/ionAllocList.h"
#include "memory/ionMemory/ionAllocEntry.h"
#include "memory/sc_interface.h"

#define DEBUG_ION_REF  0   //just for H3 ION memery info debug

#define SZ_64M        0x04000000
#define SZ_4M        0x00400000
#define SZ_1M        0x00100000
#define SZ_64K        0x00010000
#define SZ_4k       0x00001000
#define SZ_1k       0x00000400

#define ION_ALLOC_ALIGN    SZ_4k

#define DEV_NAME                     "/dev/ion"
#define KERNEL_NAME                     "/proc/version"
#define KERNEL_VERSION_4_9        "4.9"


#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6
#define ION_IOC_SUNXI_PHYS_ADDR             7
#define ION_IOC_SUNXI_DMA_COPY              8
#define ION_IOC_SUNXI_POOL_INFO        10
#define ION_IOC_SUNXI_TEE_ADDR              17

#define ION_HEAP_SYSTEM_MASK (1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK (1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_HEAP_CARVEOUT_MASK (1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK (1 << ION_HEAP_TYPE_DMA)
#define ION_NUM_HEAP_IDS (sizeof(unsigned int) * 8)
#define ION_FLAG_CACHED 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ION_FLAG_CACHED_NEEDS_SYNC 2


#define UNUSA_PARAM(param) (void)param


//----------------------
#if DEBUG_ION_REF==1
    int   cam_use_mem = 0;
    typedef struct ION_BUF_NODE_TEST
    {
        unsigned int addr;
        int size;
    } ion_buf_node_test;

    #define ION_BUF_LEN  50
    ion_buf_node_test ion_buf_nodes_test[ION_BUF_LEN];
#endif
//----------------------

struct sunxi_pool_info {
    unsigned int total;     //unit kb
    unsigned int free_kb;  // size kb
    unsigned int free_mb;  // size mb
};

typedef struct BUFFER_NODE
{
    struct aw_mem_list_head i_list;
    unsigned long phy;        //phisical address
    unsigned long vir;        //virtual address
    unsigned int size;        //buffer size
    unsigned int tee;      //
    unsigned long user_virt;//
    struct ion_fd_data fd_data;
}buffer_node;

typedef struct ION_ALLOC_CONTEXT
{
    int                    fd;            // driver handle
    struct aw_mem_list_head    list;        // buffer list
    int                    ref_cnt;    // reference count
    unsigned int           phyOffset;
    int flag_iommu;
}ion_alloc_context;

typedef struct SUNXI_PHYS_DATA
{
    ion_user_handle_t handle;
    unsigned int  phys_addr;
    unsigned int  size;
}sunxi_phys_data;


typedef struct {
    long    start;
    long    end;
}sunxi_cache_range;

#endif//  _ION_ALLOCATOR_
