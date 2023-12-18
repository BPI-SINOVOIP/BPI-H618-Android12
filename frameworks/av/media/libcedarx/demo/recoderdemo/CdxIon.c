/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : ionAlloc.c
* Description :
* History :
*   Author  : weihai <liweihai@allwinnertech.com>
*   Date    : 2017/07/21
*   Comment :
*
*
*/

//#define CONFIG_LOG_LEVEL    LOG_LEVEL_VERBOSE
#define LOG_TAG "ionAlloc"

#include <cdx_log.h>
#include <CdxIon.h>
#include <CdxList.h>

#include <sys/ioctl.h>
#include <errno.h>
#include <sc_interface.h>
#include <memoryAdapter.h>
#include <vencoder.h>
#include <ion_mem_alloc.h>
static struct SunxiMemOpsS *memops = NULL;

/*funciton begin*/
int CdxIonOpen()
{
    logd("begin ion_alloc_open \n");
    memops = GetMemAdapterOpsS();
    return SunxiMemOpen(memops);
}

void CdxIonClose()
{
    SunxiMemClose(memops);
}

// return virtual address: 0 failed
void * CdxIonPalloc(int size)
{
    return SunxiMemPalloc(memops,  size);
}

void CdxIonPfree(void * pbuf)
{
    return SunxiMemPfree(memops, pbuf);
}

void *CdxIonVir2Phy(void * pbuf)
{
    return SunxiMemGetPhysicAddressCpu(memops, pbuf);
}


void * CdxIonPhy2Vir(void * pbuf)
{
    return SunxiMemGetVirtualAddressCpu(memops, pbuf);
}

void CdxIonFlushCache(void* startAddr, int size)
{
    SunxiMemFlushCache(memops, startAddr, size);
    return;
}

