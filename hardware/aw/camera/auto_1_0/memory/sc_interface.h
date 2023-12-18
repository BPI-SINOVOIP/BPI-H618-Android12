/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : typedef.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#ifndef SC_INTERFACE_H
#define SC_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct ScCamMemOpsS
{
    int (*open_cam)(void);

    void (*close_cam)(void);

    int (*total_size_cam)(void);

    void *(*palloc_cam)(int /*size*/,int * );

    void  (*pfree_cam)(void* /*mem*/);

    void (*flush_cache_cam)(void * /*mem*/, int /*size*/);

    void (*sync_cache_cam)(int /*size*/);

    void *(*ve_get_phyaddr_cam)(void * /*viraddr*/);

    void *(*ve_get_viraddr_cam)(void * /*phyaddr*/);

    void *(*cpu_get_phyaddr_cam)(void * /*viraddr*/);

    void *(*cpu_get_viraddr_cam)(void * /*phyaddr*/);

    int (*mem_set_cam)(void * /*s*/, int /*c*/, size_t /*n*/);

    int (*mem_cpy_cam)(void * /*dest*/, void * /*src*/, size_t /*n*/);

    int (*mem_read_cam)(void * /*dest */, void * /*src*/, size_t /*n*/);

    int (*mem_write_cam)(void * /*dest*/, void * /*src*/, size_t /*n*/);

    int (*setup_cam)(void);

    int (*shutdown_cam)(void);

    //*the interface just for secureAlloc.c, not used by other function
    //void *(*palloc_secure_cam)(int /*size*/);

    unsigned long (*get_phyadr_cam)(int share_fd);

};

#ifdef __cplusplus
}
#endif

#endif

