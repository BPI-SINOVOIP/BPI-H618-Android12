#include <stdio.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/properties.h>
#include <errno.h>
#include "g2d_driver.h"
#include <memory/memoryAdapter.h>
#include <memory/sc_interface.h>
//#include "memory/ionMemory/ionAllocEntry.h"
#include "G2dApi.h"

int g2dInit()
{
    int g2dHandle = open("/dev/g2d", O_RDWR, 0);
    if (g2dHandle < 0)
    {
         ALOGE("open /dev/g2d failed");
         return -1;
    }

    //ALOGD("------------------------------g2dInit------------------------------------------g2dHandle:%d",g2dHandle);
    return g2dHandle;
}

int g2dUnit(int g2dHandle)
{
    if(g2dHandle > 0)
    {
        close(g2dHandle);
    }
    return 0;
}

int g2dAllocBuff(stV4L2BUF* bufHandle,int width, int hight)
{
    struct ScCamMemOpsS* memOps  = MemCamAdapterGetOpsS();

    if (memOps == NULL)
    {
        ALOGE("------------------------g2dAllocBuff memOps is NULL");
        return -1;
    }
    int ret = memOps->open_cam();
    if (ret < 0)
    {
        ALOGE("------------------------g2dAllocBuff ion_alloc_open failed");
        return -1;
    }

    int size = width*hight*3/2;
    bufHandle->addrVirY = (unsigned long)memOps->palloc_cam(size,&bufHandle->nShareBufFd);
    bufHandle->addrPhyY = (unsigned long)memOps->cpu_get_phyaddr_cam((void*)bufHandle->addrVirY);
    memset((void*)bufHandle->addrVirY, 0x10, width*hight);
    memset((void*)(bufHandle->addrVirY + width*hight),
                0x80, width*hight/2);
    ALOGD("------------------------------g2dAllocBuff ion_alloc success");
    return 0;
}

int g2dFreeBuff(stV4L2BUF* bufHandle)
{
    struct ScCamMemOpsS* memOps  = MemCamAdapterGetOpsS();

    if (memOps == NULL)
    {
        ALOGE("------------------------g2dAllocBuff memOps is NULL");
        return -1;
    }

    memOps->pfree_cam((void*)bufHandle->addrVirY);
    bufHandle->addrPhyY = 0;
    memOps->close_cam();
    return 0;
}


int g2dScale(int g2dHandle,unsigned int psrc, int src_w, int src_h, int src_x, int src_y, int src_rectw, int src_recth, unsigned int pdst, int dst_w, int dst_h, int dst_x, int dst_y,int dst_rectw, int dst_recth)
{
    g2d_stretchblt stret_para;
	int err = 0;

    if(g2dHandle<=0)
    {
    	ALOGE("g2dScale g2dHandle error!");
    	return -1;
    }

	stret_para.flag = G2D_BLT_NONE;
    stret_para.src_image.addr[0] = (int)psrc;
	stret_para.src_image.addr[1] = (int)psrc + src_w * src_h;
	stret_para.src_image.w = src_w;	      /* src buffer width in pixel units */
	stret_para.src_image.h = src_h;	      /* src buffer height in pixel units */

	stret_para.src_image.format = G2D_FMT_PYUV420UVC;

	stret_para.src_image.pixel_seq = G2D_SEQ_NORMAL;          /* not use now */
	stret_para.src_rect.x = src_x;						/* src rect->x in pixel */
	stret_para.src_rect.y = src_y;						/* src rect->y in pixel */
	stret_para.src_rect.w = src_rectw;			/* src rect->w in pixel */
	stret_para.src_rect.h = src_recth;			/* src rect->h in pixel */

	stret_para.dst_image.addr[0] = (int)pdst;//yuv420;
	stret_para.dst_image.addr[1] = (int)pdst + dst_w * dst_h;
	stret_para.dst_image.w = dst_w;	      /* dst buffer width in pixel units */
	stret_para.dst_image.h = dst_h;	      /* dst buffer height in pixel units */

	stret_para.dst_image.format = G2D_FMT_PYUV420UVC;//G2D_FMT_PYUV422UVC;

	stret_para.dst_image.pixel_seq = G2D_SEQ_VYUY;//G2D_SEQ_NORMAL;          /* not use now */
	stret_para.dst_rect.x= dst_x;
	stret_para.dst_rect.y= dst_y;
	stret_para.dst_rect.w = dst_rectw;
	stret_para.dst_rect.h = dst_recth;

	err = ioctl(g2dHandle, G2D_CMD_STRETCHBLT, (unsigned long)&stret_para);

	if(0 > err)
	{
		ALOGE("g2d_scale: failed to call G2D_CMD_STRETCHBLT!!!\n");
		return -1;
	}

	return 0;
}

int g2dClip(int g2dHandle,void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y)
{
    g2d_blt     blit_para;
    int         err;

    if(g2dHandle<=0)
        return -1;
    blit_para.src_image.addr[0]      = (int)psrc;
    blit_para.src_image.addr[1]      = (int)psrc + src_w * src_h;
    blit_para.src_image.w            = src_w;
    blit_para.src_image.h            = src_h;
    blit_para.src_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.src_image.pixel_seq    = G2D_SEQ_NORMAL;
    blit_para.src_rect.x             = src_x;
    blit_para.src_rect.y             = src_y;
    blit_para.src_rect.w             = width;
    blit_para.src_rect.h             = height;

    blit_para.dst_image.addr[0]      = (int)pdst;
    blit_para.dst_image.addr[1]      = (int)pdst + dst_w * dst_h;
    blit_para.dst_image.w            = dst_w;
    blit_para.dst_image.h            = dst_h;
    blit_para.dst_image.format       = G2D_FMT_PYUV420UVC;
    blit_para.dst_image.pixel_seq    = G2D_SEQ_NORMAL;
    blit_para.dst_x                  = dst_x;
    blit_para.dst_y                  = dst_y;
    blit_para.color                  = 0xff;
    blit_para.alpha                  = 0xff;

    blit_para.flag = G2D_BLT_NONE;

    err = ioctl(g2dHandle, G2D_CMD_BITBLT, (unsigned long)&blit_para);
    if(err < 0)
    {
        ALOGE("-------------------------------------------------g2d_clip: failed to call G2D_CMD_BITBLT!!!\n");
        return -1;
    }

    return 0;
}


int g2dRotate(int g2dHandle,g2dRotateAngle angle, unsigned char *src, int src_width, int src_height, int src_x, int src_y, int width, int height,unsigned char *dst, int dst_width, int dst_height,int dst_x, int dst_y)
{
    g2d_blt	blit_para;

    if(g2dHandle<=0)
        return -1;
    switch(angle)
    {
        case G2D_ROTATE90:
            blit_para.flag = G2D_BLT_ROTATE90;
            break;
        case G2D_ROTATE180:
            blit_para.flag = G2D_BLT_ROTATE180;
            break;
        case G2D_ROTATE270:
            blit_para.flag = G2D_BLT_ROTATE270;
            break;
        case G2D_FLIP_HORIZONTAL:
            blit_para.flag = G2D_BLT_FLIP_HORIZONTAL;
            break;
        case G2D_FLIP_VERTICAL:
            blit_para.flag = G2D_BLT_FLIP_VERTICAL;
            break;
        case G2D_MIRROR45:
            blit_para.flag = G2D_BLT_MIRROR45;
            break;
        case G2D_MIRROR135:
            blit_para.flag = G2D_BLT_MIRROR135;
            break;
    }
    //str.flag = G2D_BLT_NONE;//G2D_BLT_NONE;//G2D_BLT_PIXEL_ALPHA|G2D_BLT_ROTATE90;
	blit_para.src_image.addr[0] = (int)src;
	blit_para.src_image.addr[1] = (int)src + src_width * src_height;
	blit_para.src_image.w = src_width;	      /* src buffer width in pixel units */
	blit_para.src_image.h = src_height;	      /* src buffer height in pixel units */

    blit_para.src_image.format = G2D_FMT_PYUV420UVC;//G2D_FMT_PYUV422UVC;
    blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;          /* not use now */
	blit_para.src_rect.x = src_x;						/* src rect->x in pixel */
	blit_para.src_rect.y = src_y;						/* src rect->y in pixel */
	blit_para.src_rect.w = width;			/* src rect->w in pixel */
	blit_para.src_rect.h = height;			/* src rect->h in pixel */

    blit_para.dst_image.addr[0] = (int)dst;//yuv420;
	blit_para.dst_image.addr[1] = (int)dst + dst_width * dst_height;
	//blit_para.dst_image.addr[1] = (int)pRecorderBuffer->top_c;//yuv420 + width * height;
	blit_para.dst_image.w = dst_width;	      /* dst buffer width in pixel units */
	blit_para.dst_image.h = dst_height;	      /* dst buffer height in pixel units */
	blit_para.dst_image.format = G2D_FMT_PYUV420UVC;//G2D_FMT_PYUV422UVC;
	blit_para.dst_image.pixel_seq = G2D_SEQ_VYUY;          /* not use now */
	blit_para.dst_x = dst_x;
	blit_para.dst_y = dst_y;

	blit_para.color = 0xFF;          				/* fix me*/
	blit_para.alpha = 0xFF;                /* globe alpha */

	if(ioctl(g2dHandle, G2D_CMD_BITBLT, (unsigned long)&blit_para) < 0)
	{
        ALOGE("--------------------------------------------g2dRotate: failed to call G2D_CMD_BITBLT!!!\n");
        return -1;
    }
    return 0;
}

struct G2dOpsS _G2dOpsS =
{
    .fpG2dInit=g2dInit,
    .fpG2dUnit=g2dUnit,
    .fpG2dAllocBuff=g2dAllocBuff,
    .fpG2dFreeBuff=g2dFreeBuff,
    .fpG2dScale=g2dScale,
    .fpG2dClip=g2dClip,
    .fpG2dRotate=g2dRotate
};

struct G2dOpsS* GetG2dOpsS()
{
    ALOGD("------------------------------_GetG2dOpsS------------------------------------------");
    //ALOGD("*** _GetG2dOpsS ***");
    return &_G2dOpsS;
}

//};
