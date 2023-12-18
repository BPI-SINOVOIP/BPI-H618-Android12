/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : deinterlace.cpp
 * Description : hardware deinterlacing
 * History :
 *
 */

#include <cdx_log.h>

#include <memoryAdapter.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS // Otherwise not available in C++.
#include <inttypes.h>

#include "outputCtrl.h"
#include <iniparserapi.h>
#include <cutils/properties.h>
//-----------------------------------------------------------------------------
// relation with deinterlace
#define DI_MODULE_TIMEOUT    0x1055
#define    DI_IOC_MAGIC        'D'

//DI_IOSTART use int instead of pointer* in double 32/64 platform to avoid bad definition.
#define    DI_IOCSTART             _IOWR(DI_IOC_MAGIC, 0, DiRectSizeT)
#define    DI_IOCSTART2            _IOWR(DI_IOC_MAGIC, 1, struct DiRectSizeT)
#define    DI_IOCSETMODE           _IOWR(DI_IOC_MAGIC, 2, struct DiModeT)
#define    DI_REQUEST              _IOWR(DI_IOC_MAGIC, 3, struct DiRectSizeT)
#define    DI_RELEASE              _IOWR(DI_IOC_MAGIC, 4, struct DiRectSizeT)

//for di300
#if defined(CONF_DI_300_SUPPORT)
#define DI_IO(nr)             _IO(DI_IOC_MAGIC, nr)
#define DI_IOR(nr, size)      _IOR(DI_IOC_MAGIC, nr, size)
#define DI_IOW(nr, size)      _IOW(DI_IOC_MAGIC, nr, size)
#define DI_IOWR(nr, size)     _IOWR(DI_IOC_MAGIC, nr, size)
#define DI_IOCTL_NR(n)        _IOC_NR(n)

#define DI_IOC_GET_VERSION    DI_IOR(0x0, struct di_version)
#define DI_IOC_RESET          DI_IO(0x1)
#define DI_IOC_CHECK_PARA     DI_IO(0x2)
#define DI_IOC_SET_TIMEOUT    DI_IOW(0x3, struct di_timeout_ns)
#define DI_IOC_SET_VIDEO_SIZE DI_IOW(0x4, struct di_size)
#define DI_IOC_SET_DIT_MODE   DI_IOW(0x5, struct di_dit_mode)
#define DI_IOC_SET_TNR_MODE   DI_IOW(0x6, struct di_tnr_mode)
#define DI_IOC_SET_FMD_ENABLE DI_IOW(0x7, struct di_fmd_enable)
#define DI_IOC_PROCESS_FB     DI_IOW(0x8, struct di_process_fb_arg)
#define DI_IOC_SET_VIDEO_CROP DI_IOW(0x9, struct di_rect)
#define DI_IOC_MEM_REQUEST    DI_IOWR(0x10, struct di_mem_arg)
#define DI_IOC_MEM_RELEASE    DI_IOWR(0x11, struct di_mem_arg)
#define DI_IOC_SET_DEMO_CROP  DI_IOW(0x12, struct di_demo_crop_arg)

#define fourcc_code(a, b, c, d) ((unsigned int)(a) | ((unsigned int)(b) << 8) | \
                                ((unsigned int)(c) << 16) | ((unsigned int)(d) << 24))

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long

enum {
    DI_DIT_INTP_MODE_INVALID = 0x0,
    DI_DIT_INTP_MODE_WEAVE,
    DI_DIT_INTP_MODE_BOB,
    DI_DIT_INTP_MODE_MOTION,
};

enum {
    DI_DIT_OUT_0FRAME = 0x0,
    DI_DIT_OUT_1FRAME,
    DI_DIT_OUT_2FRAME,
};

enum {
    DI_TNR_MODE_INVALID = 0,
    DI_TNR_MODE_ADAPTIVE,
    DI_TNR_MODE_FIX,
};

enum {
    DI_TNR_LEVEL_HIGH = 0,
    DI_TNR_LEVEL_MIDDLE,
    DI_TNR_LEVEL_LOW,
};

struct di_version {
    u32 version_major;
    u32 version_minor;
    u32 version_patchlevel;

    u32 ip_version;
};

struct di_timeout_ns {
    u64 wait4start;
    u64 wait4finish;
};

/*
 * @intp_mode: see enum DI_DIT_INTP_MODE_XXX.
 * @out_frame_mode: see enum DI_DIT_OUT_XFRAME.
 */
struct di_dit_mode {
    u32 intp_mode;
    u32 out_frame_mode;
};

/*
 * @mode:
 * @level:
 */
struct di_tnr_mode {
    u32 mode;
    u32 level;
};

/* 0:disable; 1:enable. */
struct di_fmd_enable {
    u32 en;
};

struct di_size {
    u32 width;
    u32 height;
};

struct di_rect {
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
};

/*
 * support dma_buf method or phy_addr_buf method.
 * 1.On dma_buf method:
 *     @y_addr is address-offset of luma-buf to dma_addr;
 *     @cb_addr/@cr_addr is address-offset of chroma-buf to dma_addr.
 * 2.On phy_addr_buf method:
 *     @y_addr is physical address of luma-buf;
 *     @cb_addr/@cr_addr is physical address of chroma-buf.
 *
 * @ystride: line stride of luma buf. unit: byte.
 * @cstride: line stride of chroma buf. unit: byte.
 */
struct di_buf {
    u64 y_addr;
    u64 cb_addr;
    u64 cr_addr;
    u32 ystride;
    u32 cstride;
};

/*
 * @format: see DRM_FORMAT_XXX in drm_fourcc.h.
 * @dma_buf_fd: dma buf fd that from userspace.
 *    @dma_buf_fd must be invalid(<0) on phy_addr_buf method.
 * @size.width,@size.height: size of pixel datas of image. unit: pixel.
 */
struct di_fb {
    u32 format;
    int dma_buf_fd;
    struct di_buf buf;
    struct di_size size;
};

/*
 * @base_field:
 *     0: top_field;
 *     1: bottom_field.
 */
struct di_process_fb_arg {

    u8 is_interlace; /*must be set*/
    u8 is_pulldown; /* fixme: define enum value, not used in driver*/
    u8 top_field_first; /*recorrected by drivers*/
    u8 base_field; /*top_filed or bottom_filed, if use BOB, must be set*/

    struct di_fb in_fb0;
    struct di_fb in_fb1;
    struct di_fb in_fb2;

    struct di_fb out_dit_fb0;
    struct di_fb out_dit_fb1;
    struct di_fb out_tnr_fb0;

};

struct di_mem_arg {
    unsigned int size;
    unsigned int handle;
    u64 phys_addr;
};

struct di_demo_crop_arg {
    struct di_rect dit_demo;
    struct di_rect tnr_demo;
};

enum di_mode_e {
    DI_MODE_INVALID = 0,
    DI_MODE_60HZ,
    DI_MODE_30HZ,
    DI_MODE_BOB,
    DI_MODE_WEAVE,
    DI_MODE_TNR, /* only tnr */
};

typedef struct DiParaT3 {

    unsigned char is_interlace; /*must be set*/
    unsigned char is_pulldown; /* fixme: define enum value, not used in driver*/
    unsigned char top_field_first; /*recorrected by drivers*/
    unsigned char base_field; /*top_filed or bottom_filed, if use BOB, must be set*/

    struct di_fb in_fb0;
    struct di_fb in_fb1;
    struct di_fb in_fb2;

    struct di_fb out_dit_fb0;
    struct di_fb out_dit_fb1;
    struct di_fb out_tnr_fb0;

}DiParaT3;
#endif

typedef enum DiPixelformatE2 {
    DI2_FORMAT_NV12      =0x00,
    DI2_FORMAT_NV21      =0x01,
    DI2_FORMAT_MB32_12   =0x02, //UV mapping like NV12
    DI2_FORMAT_MB32_21   =0x03, //UV mapping like NV21
    DI2_FORMAT_YV12 = 0x04, /* 3-plane */
    DI2_FORMAT_YUV422_SP_UVUV = 0x08, /* 2-plane, New in DI_V2.2 */
    DI2_FORMAT_YUV422_SP_VUVU = 0x09, /* 2-plane, New in DI_V2.2 */
    DI2_FORMAT_YUV422P = 0x0c, /* 3-plane, New in DI_V2.2 */
} DiPixelformatE2;

typedef enum DiPixelformatE {
    DI_FORMAT_NV12      =0x00,
    DI_FORMAT_NV21      =0x01,
    DI_FORMAT_MB32_12   =0x02, //UV mapping like NV12
    DI_FORMAT_MB32_21   =0x03, //UV mapping like NV21
} DiPixelformatE;

typedef struct DiRectSizeT {
    unsigned int nWidth;
    unsigned int nHeight;
} DiRectSizeT;

typedef struct DiVersionT {
    unsigned int version_major;
    unsigned int version_minor;
    unsigned int version_patchlevel;

    unsigned int ip_version;
}DiVersionT;

typedef struct DiTimeoutNsT {
    unsigned long long wait4start;
    unsigned long long wait4finish;
}DiTimeoutNsT;

typedef struct DiFbT {
#if (CONF_KERN_BITWIDE == 64)
    unsigned long long         addr[2];  // the address of frame buffer
#elif(CONF_KERN_BITWIDE == 32)
    uintptr_t         addr[2];  // the address of frame buffer
#else
#error "CONF_KERN_BITWIDE config error"
#endif
    DiRectSizeT       mRectSize;
    DiPixelformatE    eFormat;
} DiFbT;

typedef struct DiFbT2 {
    int               fd;
#if (CONF_KERN_BITWIDE == 64)
    unsigned long long         addr[3];  // the address of frame buffer
#elif(CONF_KERN_BITWIDE == 32)
    uintptr_t         addr[3];  // the address of frame buffer
#else
#error "CONF_KERN_BITWIDE config error"
#endif
    DiRectSizeT       mRectSize;
    DiPixelformatE2    eFormat;
} DiFbT2;

typedef struct DiParaT {
    DiFbT         mInputFb;           //current frame fb
    DiFbT         mPreFb;          //previous frame fb
    DiRectSizeT   mSourceRegion;   //current frame and previous frame process region
    DiFbT         mOutputFb;       //output frame fb
    DiRectSizeT   mOutRegion;       //output frame region
    unsigned int  nField;          //process field <0-top field ; 1-bottom field>
    /* video infomation <0-is not top_field_first; 1-is top_field_first> */
    unsigned int  bTopFieldFirst;
} DiParaT;

typedef struct DiParaT2 {
    DiFbT2         mInputFb;       /* current frame fb */
    DiFbT2         mPreFb;         /* previous frame fb */
    DiFbT2         mNextFb;        /* next frame fb */

    /* current frame /previous frame and next frame process region */
    DiRectSizeT   mSourceRegion;

    DiFbT2         mOutputFb;      /* output frame fb */
    DiRectSizeT   mOutRegion;     /* output frame region */
    unsigned int  nField;         /* process field <0-first field ; 1-second field> */

    /* video infomation <0-is not top_field_first; 1-is top_field_first> */
    unsigned int  bTopFieldFirst;

    /* unsigned int update_mode; */
    /* update buffer mode <0-update 1 frame, output 2 frames; 1-update 1 frame, output 1 frame> */

#if defined(CONF_DI_MULPLEX)
    int id;
#endif

} DiParaT2;

#if !defined(CONF_DI_300_SUPPORT)
typedef enum DiIntpModeT {
    DI_MODE_WEAVE = 0x0, /* Copy source to destination */
    DI_MODE_INTP = 0x1, /* Use current field to interpolate another field */
    DI_MODE_MOTION = 0x2, /* Use 4-field to interpolate another field */
} DiIntpModeT;

typedef enum DiUpdModeT {
    DI_UPDMODE_FIELD = 0x0, /* Output 2 frames when updated 1 input frame */
    DI_UPDMODE_FRAME = 0x1, /* Output 1 frame when updated 1 input frame */
} DiUpdModeT;

typedef struct DiModeT {
    DiIntpModeT di_mode;
    DiUpdModeT update_mode;
} DiModeT;
#endif

enum {
    IOMEM_UNKNOWN,
    IOMEM_CMA,
    IOMEM_IOMMU,
};

struct DiContext
{
    Deinterlace base;

    int fd;
    int picCount;
    int iomemType;
#if !defined(CONF_DI_300_SUPPORT)
    DiIntpModeT diMode;
#endif

#if defined(CONF_DI_MULPLEX)
    int id;
#endif

#if defined(CONF_DI_300_SUPPORT)
    enum di_mode_e mode;
    int tnr_en;
    int fmd_en;
    int nWidth;
    int nHeight;
    int64_t nLastPts;
    struct di_size di_local_size;
    struct di_rect di_local_rect;
    struct di_dit_mode di_local_dit_mode;
    struct di_tnr_mode di_local_tnr_mode;
    struct di_fmd_enable di_local_fmd_en;
    struct di_demo_crop_arg di_local_demo_arg;
#endif
};

/*
static int dumpPara(DiParaT *para)
{
    CEDARX_UNUSE(dumpPara);
    logd("**************************************************************");
    logd("*********************deinterlace info*************************");

    logd(" input_fb: addr=0x%x, 0x%x, format=%d, size=(%d, %d)",
        (int)para->mInputFb.addr[0], (int)para->mInputFb.addr[1], (int)para->mInputFb.eFormat,
        (int)para->mInputFb.mRectSize.nWidth, (int)para->mInputFb.mRectSize.nHeight);
    logd("   pre_fb: addr=0x%x, 0x%x, format=%d, size=(%d, %d)",
        (int)para->mPreFb.addr[0], (int)para->mPreFb.addr[1], (int)para->mPreFb.eFormat,
        (int)para->mPreFb.mRectSize.nWidth, (int)para->mPreFb.mRectSize.nHeight);
    logd("output_fb: addr=0x%x, 0x%x, format=%d, size=(%d, %d)",
        (int)para->mOutputFb.addr[0], (int)para->mOutputFb.addr[1], (int)para->mOutputFb.eFormat,
        (int)para->mOutputFb.mRectSize.nWidth, (int)para->mOutputFb.mRectSize.nHeight);
    logd("top_field_first=%d, field=%d", para->bTopFieldFirst, para->nField);
    logd("source_regn=(%d, %d), out_regn=(%d, %d)",
        para->mSourceRegion.nWidth, para->mSourceRegion.nHeight,
        para->mOutRegion.nWidth, para->mOutRegion.nHeight);

    logd("****************************end*******************************");
    logd("**************************************************************\n\n");
    return 0;
}

static int dumpPara2(DiParaT2 *para)
{
    CEDARX_UNUSE(dumpPara2);
    logd("**************************************************************");
    logd("*********************deinterlace info*************************");

    logd(" input_fb: fd=%d, addr[0~2]=0x%" PRIx64", 0x%" PRIx64", 0x%" PRIx64", format=%d, size=(%d, %d)",
        para->mInputFb.fd, para->mInputFb.addr[0], para->mInputFb.addr[1], para->mInputFb.addr[2],
        (int)para->mInputFb.eFormat,
        (int)para->mInputFb.mRectSize.nWidth,
        (int)para->mInputFb.mRectSize.nHeight);

    logd("   pre_fb: fd=%d, addr[0~2]=0x%" PRIx64", 0x%" PRIx64", 0x%" PRIx64", format=%d, size=(%d, %d)",
        para->mPreFb.fd, para->mPreFb.addr[0], para->mPreFb.addr[1], para->mPreFb.addr[2],
        (int)para->mPreFb.eFormat,
        (int)para->mPreFb.mRectSize.nWidth,
        (int)para->mPreFb.mRectSize.nHeight);

    logd("  next_fb: fd=%d, addr[0~2]=0x%" PRIx64", 0x%" PRIx64", 0x%" PRIx64", format=%d, size=(%d, %d)",
        para->mNextFb.fd, para->mNextFb.addr[0], para->mNextFb.addr[1], para->mNextFb.addr[2],
        (int)para->mNextFb.eFormat,
        (int)para->mNextFb.mRectSize.nWidth,
        (int)para->mNextFb.mRectSize.nHeight);

    logd("output_fb: fd=%d, addr[0~2]=0x%" PRIx64", 0x%" PRIx64", 0x%" PRIx64", format=%d, size=(%d, %d)",
        para->mOutputFb.fd, para->mOutputFb.addr[0], para->mOutputFb.addr[1], para->mOutputFb.addr[2],
        (int)para->mOutputFb.eFormat,
        (int)para->mOutputFb.mRectSize.nWidth,
        (int)para->mOutputFb.mRectSize.nHeight);

    logd("top_field_first=%d, field=%d", para->bTopFieldFirst, para->nField);
    logd("source_regn=(%d, %d), out_regn=(%d, %d)",
        para->mSourceRegion.nWidth, para->mSourceRegion.nHeight,
        para->mOutRegion.nWidth, para->mOutRegion.nHeight);

#if defined(CONF_DI_MULPLEX)
    logd("id=%d", para->id);
#endif

    logd("****************************end*******************************");
    logd("**************************************************************\n\n");
    return 0;
}*/

#if defined(CONF_DI_V2X_SUPPORT)
static int setMode2(Deinterlace* di )
{
    struct DiContext* dc = (struct DiContext*)di;
    DiModeT mDiModeT2;
    mDiModeT2.di_mode = dc->diMode;
    mDiModeT2.update_mode = DI_UPDMODE_FIELD;
    logv(" di_mode = %d , update_mode = %d ",
           mDiModeT2.di_mode, mDiModeT2.update_mode);
    if (ioctl(dc->fd, DI_IOCSETMODE, &mDiModeT2) != 0)
    {
        loge("aw_di_setmode2 failed!");
        return -1;
    }
    return 0;
}
#endif
static int getIOMemType() {
    int type = IOMEM_CMA;

#ifdef CONF_USE_IOMMU
    type = IOMEM_IOMMU;
#endif

    return type;
}

#if defined(CONF_DI_300_SUPPORT)
static int getVersion(int fd)
{
    int ret;
    struct di_version v;

    ret = ioctl(fd, DI_IOC_GET_VERSION, &v);
    if (ret) {
        logw("DI_IOC_GET_VERSION failed\n");
        return ret;
    }

    logd("di version[%d.%d.%d], hw_ip[%d]\n",
        v.version_major,
        v.version_minor,
        v.version_patchlevel,
        v.ip_version);
    return ret;
}

static int setTimeout(int fd)
{
    int ret = 0;
    struct di_timeout_ns t;

    t.wait4start = 500000000ULL;
    t.wait4finish = 600000000ULL;
    ret = ioctl(fd, DI_IOC_SET_TIMEOUT, &t);
    if (ret) {
        logw("DI_IOC_SET_TIMEOUT failed\n");
        return ret;
    }

    return ret;
}

#endif
static void __DiDestroy(Deinterlace* di)
{
    struct DiContext* dc;
    dc = (struct DiContext*)di;

#if defined(CONF_DI_MULPLEX)
    if (ioctl(dc->fd, DI_RELEASE, &dc->id) != 0)
    {
        loge("aw_di_release failed!");
    }
#endif

    if (dc->fd != -1)
    {
        close(dc->fd);
    }
    free(dc);
}

static int __DiInit(Deinterlace* di)
{
    struct DiContext* dc;
    dc = (struct DiContext*)di;

    logv("%s", __FUNCTION__);
    if (dc->fd != -1)
    {
        logw("already init...");
        return 0;
    }

    dc->fd = open("/dev/deinterlace", O_RDWR);
    if (dc->fd == -1)
    {
        loge("open hw devices failure, errno(%d)", errno);
        return -1;
    }

#if defined(CONF_DI_MULPLEX)
    if (ioctl(dc->fd, DI_REQUEST, &dc->id) != 0)
    {
        loge("aw_di_request failed!");
        return -1;
    }
    logv("get di id=%d", dc->id);
#endif

#if defined(CONF_DI_300_SUPPORT)
    /*Test the API of HW IP version*/
    if (getVersion(dc->fd) < 0)
    {
        logw("getVersion failed!\n");
    }

    /*set deinterlace waiting start and finishing timeout*/
    if (setTimeout(dc->fd) < 0)
    {
        logw("setTimeout failed!\n");
    }
    dc->nLastPts = 0;

    memset(&dc->di_local_size, 0, sizeof(struct di_size));
    memset(&dc->di_local_rect, 0, sizeof(struct di_rect));
    memset(&dc->di_local_dit_mode, 0, sizeof(struct di_dit_mode));
    memset(&dc->di_local_tnr_mode, 0, sizeof(struct di_tnr_mode));
    memset(&dc->di_local_fmd_en, 0, sizeof(struct di_fmd_enable));
    memset(&dc->di_local_demo_arg, 0, sizeof(struct di_demo_crop_arg));
#endif
    dc->picCount = 0;
    dc->iomemType = getIOMemType();
    logv("iomem type=%d", dc->iomemType);
    logv("hw deinterlace init success...");
    return 0;
}

static int __DiReset(Deinterlace* di)
{
    struct DiContext* dc = (struct DiContext*)di;

#if defined(CONF_DI_MULPLEX)
    if (ioctl(dc->fd, DI_RELEASE, &dc->id) != 0)
    {
        loge("aw_di_release failed!");
    }
#endif

    logd("%s", __FUNCTION__);
    if (dc->fd != -1)
    {
        close(dc->fd);
        dc->fd = -1;
    }
    return __DiInit(di);
}

static enum EPIXELFORMAT __DiExpectPixelFormat(Deinterlace* di)
{
    struct DiContext* dc = (struct DiContext*)di;
    CEDARX_UNUSE(dc);

    enum EPIXELFORMAT pixfmt;
#if defined(CONF_DI_300_SUPPORT)
    pixfmt = PIXEL_FORMAT_YV12; //di300 60Hz mode needed
#else
    pixfmt = PIXEL_FORMAT_NV21;
#endif

    return pixfmt;
}

static int __DiFlag(Deinterlace* di)
{
    struct DiContext* dc = (struct DiContext*)di;
    CEDARX_UNUSE(dc);
    return DE_INTERLACE_HW;
}

#if defined(CONF_DI_300_SUPPORT)
static int modeSetting(struct DiContext *dc, int fd, struct di_size *size,
                        struct di_rect *rect,
                        struct di_dit_mode *dit_mode,
                        struct di_tnr_mode *tnr_mode,
                        struct di_fmd_enable *fmd_en,
                        struct di_demo_crop_arg *demo_arg)
{
    int ret = 0;
    int change = 0;

    if ((size->width != dc->di_local_size.width)
        || (size->height != dc->di_local_size.height))
    {
        logd("di_size change!");
        change = 1;
        memcpy(&dc->di_local_size, size, sizeof(*size));
    }

    if ((rect->left != dc->di_local_rect.left)
        || (rect->top != dc->di_local_rect.top)
        || (rect->right != dc->di_local_rect.right)
        || (rect->bottom != dc->di_local_rect.bottom))
    {
        logd("rect(coordinate) change!");
        change = 1;
        memcpy(&dc->di_local_rect, rect, sizeof(*rect));
    }

    if ((dit_mode->intp_mode != dc->di_local_dit_mode.intp_mode)
        || (dit_mode->out_frame_mode != dc->di_local_dit_mode.out_frame_mode))
    {
        logd("dit mode change!");
        memcpy(&dc->di_local_dit_mode, dit_mode, sizeof(*dit_mode));
        change = 1;
    }

    if ((tnr_mode->mode != dc->di_local_tnr_mode.mode)
        || (tnr_mode->level != dc->di_local_tnr_mode.level)) {
        logd("tnr mode change!");
        memcpy(&dc->di_local_tnr_mode, tnr_mode, sizeof(*tnr_mode));
        change = 1;
    }

    if (fmd_en->en != dc->di_local_fmd_en.en) {
        logd("fmd mode change!");
        memcpy(&dc->di_local_fmd_en, fmd_en, sizeof(*fmd_en));
        change = 1;
    }

    if ((demo_arg->dit_demo.left != dc->di_local_demo_arg.dit_demo.left)
        || (demo_arg->dit_demo.top != dc->di_local_demo_arg.dit_demo.top)
        || (demo_arg->dit_demo.right != dc->di_local_demo_arg.dit_demo.right)
        || (demo_arg->dit_demo.bottom != dc->di_local_demo_arg.dit_demo.bottom))
    {
        logd("dit demo rect(coordinate) change!");
        memcpy(&dc->di_local_demo_arg.dit_demo, &demo_arg->dit_demo, sizeof(demo_arg->dit_demo));
        change = 1;
    }

    if ((demo_arg->tnr_demo.left != dc->di_local_demo_arg.tnr_demo.left)
        || (demo_arg->tnr_demo.top != dc->di_local_demo_arg.tnr_demo.top)
        || (demo_arg->tnr_demo.right != dc->di_local_demo_arg.tnr_demo.right)
        || (demo_arg->tnr_demo.bottom != dc->di_local_demo_arg.tnr_demo.bottom))
    {
        logd("tnr demo rect(coordinate) change!");
        memcpy(&dc->di_local_demo_arg.tnr_demo, &demo_arg->tnr_demo, sizeof(demo_arg->tnr_demo));
        change = 1;
    }

    if (!change)
        return 0;

    ret = ioctl(fd, DI_IOC_RESET, 0);
    if (ret)
    {
        loge("DI_IOC_RESET failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_VIDEO_SIZE, size);
    if (ret)
    {
        loge("DI_IOC_SET_VIDEO_SIZE failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_VIDEO_CROP, rect);
    if (ret)
    {
        loge("DI_IOC_SET_VIDEO_CROP failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_DIT_MODE, dit_mode);
    if (ret)
    {
        loge("DI_IOC_SET_DIT_MODE failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_FMD_ENABLE, fmd_en);
    if (ret)
    {
        loge("DI_IOC_SET_FMD_ENABLE failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_TNR_MODE, tnr_mode);
    if (ret) {
        loge("DI_IOC_SET_TNR_MODE failed\n");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_SET_DEMO_CROP, demo_arg);
    if (ret < 0)
    {
        loge("DI_IOC_SET_DEMO_CROP failed.");
        return ret;
    }

    ret = ioctl(fd, DI_IOC_CHECK_PARA, 0);
    if (ret)
    {
        loge("DI_IOC_CHECK_PARA\n");
        return ret;
    }

    return ret;
}

static int preProcess(struct DiContext *dc, CdxDeinterlaceParamT *diParams)
{
    /*setting di working mode related para*/
    enum di_mode_e mode;
    unsigned char tnr_en; // enable tnr(3D noise reduction), only be used in 30/60HZ mode
    unsigned char fmd_en; // enable movie mode detection
    struct di_rect rect;
    struct di_dit_mode dit_mode;
    struct di_tnr_mode tnr_mode;
    struct di_fmd_enable fmd_enable;
    struct di_demo_crop_arg demo_arg;
    struct di_size size_in;

    memset(&rect, 0, sizeof(rect));
    memset(&dit_mode, 0, sizeof(dit_mode));
    memset(&tnr_mode, 0, sizeof(tnr_mode));
    memset(&fmd_enable, 0, sizeof(fmd_enable));
    memset(&size_in, 0, sizeof(size_in));

    //* 0. set mode
    mode = dc->mode;
    tnr_en = dc->tnr_en;
    fmd_en = dc->fmd_en;

    //* 1. size_in
    size_in.width = dc->nWidth;
    size_in.height = dc->nHeight;

    //* 2. rect
    rect.left = 0;
    rect.top = 0;
    rect.right = size_in.width;
    rect.bottom = size_in.height;

    //* 3. dit_mode
    if ((mode == DI_MODE_60HZ) || (mode == DI_MODE_30HZ))
        dit_mode.intp_mode = DI_DIT_INTP_MODE_MOTION;
    else if (mode == DI_MODE_BOB)
        dit_mode.intp_mode = DI_DIT_INTP_MODE_BOB;
    else if (mode == DI_MODE_WEAVE)
        dit_mode.intp_mode = DI_DIT_INTP_MODE_WEAVE;
    else
        dit_mode.intp_mode = DI_DIT_INTP_MODE_INVALID;

    if (mode == DI_MODE_60HZ)
        dit_mode.out_frame_mode = DI_DIT_OUT_2FRAME;
    else if ((mode == DI_MODE_30HZ)
            || (mode == DI_MODE_BOB)
            || (mode == DI_MODE_WEAVE))
        dit_mode.out_frame_mode = DI_DIT_OUT_1FRAME;
    else
        dit_mode.out_frame_mode = DI_DIT_OUT_0FRAME;

    //* 4. tnr_mode
    if ((mode == DI_MODE_TNR) || (tnr_en == 1))
    {
        tnr_mode.mode = DI_TNR_MODE_FIX;
        tnr_mode.level = DI_TNR_LEVEL_HIGH;
    }
    else
        tnr_mode.mode = DI_TNR_MODE_INVALID;

    if (dit_mode.intp_mode == DI_DIT_INTP_MODE_MOTION || mode == DI_MODE_TNR)
    {
        if (diParams->diEnableTnr == 1 && ((mode == DI_MODE_TNR) || (tnr_en == 1)))
        {
            tnr_mode.mode = diParams->diTnrMode;
            tnr_mode.level = diParams->diTnrStrength;
        }
    }

    //* 5. fmd_enable
    if (diParams->diEnableFmd == 1 && fmd_en == 1 && diParams->diEnableDemo == 0)
        fmd_enable.en = 1;
    else
        fmd_enable.en = 0;

    //* 6. demo_arg
    if (dit_mode.intp_mode == DI_DIT_INTP_MODE_MOTION || mode == DI_MODE_TNR)
    {
        if (diParams->diEnableDemo == 1)
        {
            //* tnr
            //if(tnr_mode.mode == DI_TNR_MODE_INVALID)
            if(diParams->diEnableTnr == 1 && ((mode == DI_MODE_TNR) || (tnr_en == 1)))
            {
                demo_arg.tnr_demo.left = diParams->diRect.left;
                demo_arg.tnr_demo.top = diParams->diRect.top;
                demo_arg.tnr_demo.right = diParams->diRect.right;
                demo_arg.tnr_demo.bottom = diParams->diRect.bottom;
            }
            else
            {
                demo_arg.tnr_demo.left = 0;
                demo_arg.tnr_demo.top = 0;
                demo_arg.tnr_demo.right = 0;
                demo_arg.tnr_demo.bottom = 0;
                logv("close tnr");
            }

            //* dit
            demo_arg.dit_demo.left = diParams->diRect.left;
            demo_arg.dit_demo.top = diParams->diRect.top;
            demo_arg.dit_demo.right = diParams->diRect.right;
            demo_arg.dit_demo.bottom = diParams->diRect.bottom;
        }
        else
        {
            //* tnr
            //if(tnr_mode.mode == DI_TNR_MODE_INVALID)
            if(diParams->diEnableTnr == 1 && ((mode == DI_MODE_TNR) || (tnr_en == 1)))
            {
                demo_arg.tnr_demo.left = 0;
                demo_arg.tnr_demo.top = 0;
                demo_arg.tnr_demo.right = dc->nWidth;
                demo_arg.tnr_demo.bottom = dc->nHeight;
            }
            else
            {
                demo_arg.tnr_demo.left = 0;
                demo_arg.tnr_demo.top = 0;
                demo_arg.tnr_demo.right = 0;
                demo_arg.tnr_demo.bottom = 0;
                logv("close tnr");
            }

            //* dit
            demo_arg.dit_demo.left = 0;
            demo_arg.dit_demo.top = 0;
            demo_arg.dit_demo.right = dc->nWidth;
            demo_arg.dit_demo.bottom = dc->nHeight;
        }
    }

    if(modeSetting(dc, dc->fd,
                    &size_in,
                    &rect,
                    &dit_mode,
                    &tnr_mode,
                    &fmd_enable,
                    &demo_arg) < 0)
    {
        loge("modeSetting fail.");
        return -1;
    }

    return 0;
}

#define DRM_FORMAT_YUV422   fourcc_code('Y', 'U', '1', '6') /* 2x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YUV420   fourcc_code('Y', 'U', '1', '2') /* 2x2 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU420   fourcc_code('Y', 'V', '1', '2') /* 2x2 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_NV12     fourcc_code('N', 'V', '1', '2') /* 2x2 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV21     fourcc_code('N', 'V', '2', '1') /* 2x2 subsampled Cb:Cr plane */

static unsigned int getDiFbFormat(unsigned int fmt)
{
    switch (fmt) {
    case DI2_FORMAT_YUV422P:
        return DRM_FORMAT_YUV422;

    case DI2_FORMAT_YV12:
        logv("DI2_FORMAT_YV12");
        return DRM_FORMAT_YVU420;
    case DI2_FORMAT_NV12:
        return DRM_FORMAT_NV12;

    case DI2_FORMAT_NV21:
        return DRM_FORMAT_NV21;

    default:
        logw("ERROR FORMAT(0x%x)!", fmt);
        return 0;
    }

    return 0;
}

/*process para setting, most important args*/
static void diSetPara(struct DiContext* dc,
                        DiParaT3 *mDiParaT3,
                        VideoPicture *pPic0,
                        VideoPicture *pPic1,
                        VideoPicture *pPic2,
                        VideoPicture *pOutPic0,
                        VideoPicture *pOutPic1,
                        VideoPicture *pOutTnr,
                        int nField,
                        unsigned int fmtIn,
                        unsigned int fmtOut,
                        int nTnrOnly)
{
    struct di_fb *fb;

    logv("di set para:\n");
    if(nTnrOnly == 0)
    {
        /*set de-interlace para*/
        mDiParaT3->is_interlace = 1;
        mDiParaT3->is_pulldown = 0;
        mDiParaT3->top_field_first = pPic2->bTopFieldFirst;
        mDiParaT3->base_field = nField;
    }

    /*For in_fb0*/
    fb = &mDiParaT3->in_fb0;
    fb->format = getDiFbFormat(fmtIn);
    fb->size.width = dc->nWidth;
    fb->size.height = dc->nHeight;

    if (dc->iomemType == IOMEM_CMA)
    {
        fb->dma_buf_fd = -1;
        fb->buf.y_addr = pPic0->phyYBufAddr + CONF_VE_PHY_OFFSET;
        fb->buf.cb_addr = pPic0->phyCBufAddr + CONF_VE_PHY_OFFSET;
        if (fmtOut == DI2_FORMAT_YV12)
            fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
        else if(fmtOut == DI2_FORMAT_YUV422P)
            fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
        else
            fb->buf.cr_addr = 0;
    }
    else
    {
        fb->dma_buf_fd = pPic0->nBufFd;
        fb->buf.y_addr = 0;
        fb->buf.cb_addr = fb->size.width * fb->size.height;

        if (fmtOut == DI2_FORMAT_YV12)
            fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
        else if(fmtOut == DI2_FORMAT_YUV422P)
            fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
        else
            fb->buf.cr_addr = 0;
    }

    fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
    fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    logv("in0: Format:%d width:%u height:%u dma_buf_fd:%d "
        "y_addr:%llx cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);

    /*For in_fb1*/
    if ((dc->mode == DI_MODE_60HZ)
        || (dc->mode == DI_MODE_30HZ)
        || (dc->mode == DI_MODE_TNR)
        || (dc->tnr_en == 1))
    {
        fb = &mDiParaT3->in_fb1;
        fb->format = getDiFbFormat(fmtIn);
        fb->size.width = dc->nWidth;
        fb->size.height = dc->nHeight;
        if (dc->iomemType == IOMEM_CMA)
        {
            fb->dma_buf_fd = -1;

            fb->buf.y_addr = pPic1->phyYBufAddr + CONF_VE_PHY_OFFSET;
            fb->buf.cb_addr = pPic1->phyCBufAddr + CONF_VE_PHY_OFFSET;
            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
            else if(fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
            else
                fb->buf.cr_addr = 0;
        }
        else
        {
            fb->dma_buf_fd = pPic1->nBufFd;
            fb->buf.y_addr = 0;
            fb->buf.cb_addr = fb->size.width * fb->size.height;

            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
            else if (fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
            else
                fb->buf.cr_addr = 0;
        }
        fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
        fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    }
    else
    {
        fb = &mDiParaT3->in_fb1;
        memset(fb, 0, sizeof(struct di_fb));
        fb->dma_buf_fd = -1;
    }
    logv("in1: Format:%d width:%u height:%u dma_buf_fd:%d "
        "y_addr:%llx cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
            fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
            fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
            fb->buf.ystride, fb->buf.cstride);

    /*For in_fb2*/
    if (dc->mode == DI_MODE_60HZ)
    {
        fb = &mDiParaT3->in_fb2;
        fb->format = getDiFbFormat(fmtIn);
        fb->size.width = dc->nWidth;
        fb->size.height = dc->nHeight;
        if (dc->iomemType == IOMEM_CMA)
        {
            fb->dma_buf_fd = -1;

            fb->buf.y_addr = pPic2->phyYBufAddr + CONF_VE_PHY_OFFSET;
            fb->buf.cb_addr = pPic2->phyCBufAddr + CONF_VE_PHY_OFFSET;
            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
            else if(fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
            else
                fb->buf.cr_addr = 0;
        }
        else
        {
            fb->dma_buf_fd = pPic2->nBufFd;
            fb->buf.y_addr = 0;
            fb->buf.cb_addr = fb->size.width * fb->size.height;

            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
            else if (fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
            else
                fb->buf.cr_addr = 0;
        }
        fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
        fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    }
    else
    {
        fb = &mDiParaT3->in_fb2;
        memset(fb, 0, sizeof(struct di_fb));
        fb->dma_buf_fd = -1;
    }
    logv("in2: Format:%d width:%u height:%u dma_buf_fd:%d "
        "y_addr:%llx cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);

    /*For out_fb0*/
    if (dc->mode != DI_MODE_TNR)
    {
        fb = &mDiParaT3->out_dit_fb0;
        fb->format = getDiFbFormat(fmtOut);
        fb->size.width = dc->nWidth;
        fb->size.height = dc->nHeight;
        if (dc->iomemType == IOMEM_CMA)
        {
            fb->dma_buf_fd = -1;
            fb->buf.y_addr = pOutPic0->phyYBufAddr + CONF_VE_PHY_OFFSET;
            fb->buf.cb_addr = pOutPic0->phyCBufAddr + CONF_VE_PHY_OFFSET;
            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
            else if(fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
            else
                fb->buf.cr_addr = 0;
        }
        else
        {
            fb->dma_buf_fd = pOutPic0->nBufFd;
            fb->buf.y_addr = 0;
            fb->buf.cb_addr = fb->size.width * fb->size.height;

            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
            else if (fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
            else
                fb->buf.cr_addr = 0;
        }
        fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
        fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    }
    else
    {
        fb = &mDiParaT3->out_dit_fb0;
        memset(fb, 0, sizeof(struct di_fb));
        fb->dma_buf_fd = -1;
    }
    logv("out0: Format:%d width:%u height:%u dma_buf_fd:%d "
        "y_addr:%llx cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);

    /*For out_fb1*/
    if (dc->mode == DI_MODE_60HZ)
    {
        fb = &mDiParaT3->out_dit_fb1;
        fb->format = getDiFbFormat(fmtOut);
        fb->size.width = dc->nWidth;
        fb->size.height = dc->nHeight;
        if (dc->iomemType == IOMEM_CMA)
        {
            fb->dma_buf_fd = -1;
            fb->buf.y_addr = pOutPic1->phyYBufAddr + CONF_VE_PHY_OFFSET;
            fb->buf.cb_addr = pOutPic1->phyCBufAddr + CONF_VE_PHY_OFFSET;
            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
            else if(fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
            else
                fb->buf.cr_addr = 0;
        }
        else
        {
            fb->dma_buf_fd = pOutPic1->nBufFd;
            fb->buf.y_addr = 0;
            fb->buf.cb_addr = fb->size.width * fb->size.height;

            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
            else if (fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
            else
                fb->buf.cr_addr = 0;
        }
        fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
        fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    }
    else
    {
        fb = &mDiParaT3->out_dit_fb1;
        memset(fb, 0, sizeof(struct di_fb));
        fb->dma_buf_fd = -1;
    }
    logv("out1: Format:%d width:%u height:%u dma_buf_fd:%d y_addr:%llx "
            "cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);

    /*For out_fbtnr*/
    if ((dc->mode == DI_MODE_TNR) || dc->tnr_en)
    {
        fb = &mDiParaT3->out_tnr_fb0;
        fb->format = getDiFbFormat(fmtOut);
        fb->size.width = dc->nWidth;
        fb->size.height = dc->nHeight;
        if (dc->iomemType == IOMEM_CMA)
        {
            fb->dma_buf_fd = -1;
            fb->buf.y_addr = pOutTnr->phyYBufAddr + CONF_VE_PHY_OFFSET;
            fb->buf.cb_addr = pOutTnr->phyCBufAddr + CONF_VE_PHY_OFFSET;
            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 4;
            else if(fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->buf.cb_addr + fb->size.width * fb->size.height / 2;
            else
                fb->buf.cr_addr = 0;
        }
        else
        {
            fb->dma_buf_fd = pOutTnr->nBufFd;
            fb->buf.y_addr = 0;
            fb->buf.cb_addr = fb->size.width * fb->size.height;

            if (fmtOut == DI2_FORMAT_YV12)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 5/4;
            else if (fmtOut == DI2_FORMAT_YUV422P)
                fb->buf.cr_addr = fb->size.width * fb->size.height * 3/2;
            else
                fb->buf.cr_addr = 0;
        }
        fb->buf.ystride = fb->size.width;//better to do it in di300 driver.
        fb->buf.cstride = fb->size.width/2*1;//* attention: yv12
    }
    else
    {
        fb = &mDiParaT3->out_tnr_fb0;
        memset(fb, 0, sizeof(struct di_fb));
        fb->dma_buf_fd = -1;
    }
    logv("outtnr: Format:%d width:%u height:%u dma_buf_fd:%d y_addr:%llx "
        "cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);

    return;
}
#endif

#if defined(CONF_DI_V2X_SUPPORT)
static int __DiProcess2(Deinterlace* di,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pNextPicture,
                    VideoPicture *pOutPicture,
                    int nField)
{
    struct DiContext* dc;
    dc = (struct DiContext*)di;

    logv("call DeinterlaceProcess2");

    if(pPrePicture == NULL || pCurPicture == NULL || pNextPicture == NULL || pNextPicture == NULL)
    {
        loge("the input param is null : %p, %p, %p, %p",
              pPrePicture, pCurPicture, pNextPicture, pOutPicture);
        return -1;
    }

    if(dc->fd == -1)
    {
        loge("not init...");
        return -1;
    }

    if (dc->iomemType == IOMEM_UNKNOWN)
    {
        loge("iomem type unknown.");
        return -1;
    }

    if((pNextPicture == pCurPicture) && (pPrePicture == pCurPicture))
    {
        dc->diMode = DI_MODE_INTP;
        setMode2(di);
    }
    else if(dc->diMode == DI_MODE_INTP &&
            (pPrePicture != pCurPicture) && (pNextPicture != pCurPicture))
    {
        __DiReset(di);
        dc->diMode = DI_MODE_MOTION;
        setMode2(di);
    }
    DiParaT2       mDiParaT2;

    DiRectSizeT    mRealSize;
    DiRectSizeT    mAlignSize;

    DiPixelformatE2 eInFormat;
    DiPixelformatE2 eOutFormat;

    memset(&mDiParaT2, 0, sizeof(DiParaT2));
    //* compute pts again, specifically for dealing with the abnormity of first and second pts.
    int nDefaultFrameRate = 30000; //* we set the frameRate to 30
    int nDefaultFrameDuration = 1000*1000*1000/nDefaultFrameRate;
    if (dc->picCount < 2)
    {
        pOutPicture->nPts = pNextPicture->nPts + nField*nDefaultFrameDuration/2;
    }
    else
    {
        if((pNextPicture->nPts - pCurPicture->nPts) > 0)
        {
            pOutPicture->nPts = pNextPicture->nPts
                + nField*(pNextPicture->nPts - pCurPicture->nPts)/2;
        }
        else
        {
            pOutPicture->nPts = pNextPicture->nPts + nField*nDefaultFrameDuration/2;
        }
    }

    logv("pNextPicture->nPts = %lld  ms, pOutPicture->nPts = %lld ms, diff = %lld ms ",
          pNextPicture->nPts/1000,pOutPicture->nPts/1000,
          (pOutPicture->nPts -  pNextPicture->nPts)/1000);

    if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV12)
    {
        eInFormat = DI2_FORMAT_NV12;
    }
    else if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV21)
    {
        eInFormat = DI2_FORMAT_NV21;
    }
    else if (pCurPicture->ePixelFormat == PIXEL_FORMAT_YV12)
    {
        eInFormat = DI2_FORMAT_YV12;
    }
    else
    {
        loge("the inputPixelFormat is not support : %d",pCurPicture->ePixelFormat);
        return -1;
    }

    if (pOutPicture->ePixelFormat == PIXEL_FORMAT_NV12)
    {
        eOutFormat = DI2_FORMAT_NV12;
    }
    else if (pOutPicture->ePixelFormat == PIXEL_FORMAT_NV21)
    {
        eOutFormat = DI2_FORMAT_NV21;
    }
    else if (pOutPicture->ePixelFormat == PIXEL_FORMAT_YV12)
    {
        eOutFormat = DI2_FORMAT_YV12;
    }
    else
    {
        loge("the outputPixelFormat is not support : %d",pOutPicture->ePixelFormat);
        return -1;
    }

    mRealSize.nWidth  = pCurPicture->nRightOffset - pCurPicture->nLeftOffset;
    mRealSize.nHeight = pCurPicture->nBottomOffset - pCurPicture->nTopOffset;

    mAlignSize.nWidth  = pOutPicture->nLineStride;
    mAlignSize.nHeight = pOutPicture->nHeight;

    if (dc->iomemType == IOMEM_CMA)
    {
        mDiParaT2.mInputFb.addr[0]    = pCurPicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
        mDiParaT2.mInputFb.addr[1]    = pCurPicture->phyCBufAddr + CONF_VE_PHY_OFFSET;
    }
    mDiParaT2.mInputFb.mRectSize.nWidth   = mAlignSize.nWidth ;
    mDiParaT2.mInputFb.mRectSize.nHeight  = mAlignSize.nHeight;
    mDiParaT2.mInputFb.eFormat    = eInFormat;
    if(pCurPicture->nBufFd > 0)
         mDiParaT2.mInputFb.fd = pCurPicture->nBufFd;

    if (dc->iomemType == IOMEM_CMA)
    {
        mDiParaT2.mPreFb.addr[0]      = pPrePicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
        mDiParaT2.mPreFb.addr[1]      = pPrePicture->phyCBufAddr + CONF_VE_PHY_OFFSET;

    }
    mDiParaT2.mPreFb.mRectSize.nWidth   = mAlignSize.nWidth ;
    mDiParaT2.mPreFb.mRectSize.nHeight  = mAlignSize.nHeight;
    mDiParaT2.mPreFb.eFormat      = eInFormat;
    if(pPrePicture->nBufFd > 0)
         mDiParaT2.mPreFb.fd = pPrePicture->nBufFd;

    if (dc->iomemType == IOMEM_CMA)
    {
        mDiParaT2.mNextFb.addr[0]     = pNextPicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
        mDiParaT2.mNextFb.addr[1]     = pNextPicture->phyCBufAddr + CONF_VE_PHY_OFFSET;
    }
    mDiParaT2.mNextFb.mRectSize.nWidth   = mAlignSize.nWidth ;
    mDiParaT2.mNextFb.mRectSize.nHeight  = mAlignSize.nHeight;
    mDiParaT2.mNextFb.eFormat     = eInFormat;
    if(pNextPicture->nBufFd > 0)
         mDiParaT2.mNextFb.fd = pNextPicture->nBufFd;
    mDiParaT2.mSourceRegion.nWidth   = mRealSize.nWidth;
    mDiParaT2.mSourceRegion.nHeight  = mRealSize.nHeight;
    mDiParaT2.mOutRegion.nWidth      = mRealSize.nWidth;
    mDiParaT2.mOutRegion.nHeight     = mRealSize.nHeight;

    if (dc->iomemType == IOMEM_CMA)
    {
        mDiParaT2.mOutputFb.addr[0]   = pOutPicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
        mDiParaT2.mOutputFb.addr[1]   = pOutPicture->phyCBufAddr + CONF_VE_PHY_OFFSET;
    }
    mDiParaT2.mOutputFb.mRectSize.nWidth   = mAlignSize.nWidth ;
    mDiParaT2.mOutputFb.mRectSize.nHeight  = mAlignSize.nHeight;
    mDiParaT2.mOutputFb.eFormat   = eOutFormat;
    if(pOutPicture->nBufFd > 0)
         mDiParaT2.mOutputFb.fd = pOutPicture->nBufFd;

    mDiParaT2.nField              = nField;
    mDiParaT2.bTopFieldFirst      = pCurPicture->bTopFieldFirst;

#if defined(CONF_DI_MULPLEX)
    mDiParaT2.id = dc->id;
#endif

    //dumpPara2(&mDiParaT2);

    if (ioctl(dc->fd, DI_IOCSTART2, &mDiParaT2) != 0)
    {
        loge("aw_di_setpara2 failed!");
        return -1;
    }

    dc->picCount++;

    return 0;
}

#elif defined(CONF_DI_300_SUPPORT)
//* use DI_MODE_60HZ mode, and first 2 frame use DI_MODE_BOB
//* use di demo to deal with TNR
static int __DiProcess3(Deinterlace* di,
                    VideoPicture *pPrePrePicture,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pOutPicture0,
                    VideoPicture *pOutPicture1,
                    VideoPicture *pOutTnr,
                    int nField,
                    int nTnrOnly,
                    CdxDeinterlaceParamT *diParams)
{
    struct DiContext* dc;
    dc = (struct DiContext*)di;

    if(dc->fd == -1)
    {
        loge("not init...");
        return -1;
    }

    if (dc->iomemType == IOMEM_UNKNOWN)
    {
        loge("iomem type unknown.");
        return -1;
    }

    if(nTnrOnly == 1)
    {
        if (pPrePicture == NULL || pCurPicture == NULL || pOutTnr == NULL)
        {
            loge("the input param is null : %p, %p, %p",
                  pPrePicture, pCurPicture, pOutTnr);
            return -1;
        }
    }
    else
    {
        if (pPrePrePicture == NULL || pPrePicture == NULL ||
            pCurPicture == NULL || pOutPicture0 == NULL)
        {
            loge("the input param is null : %p, %p, %p, %p",
                  pPrePrePicture, pPrePicture, pCurPicture, pOutPicture0);
            return -1;
        }
    }

    int mAlignSizeWidth;
    int mAlignSizeHeight;
    if(nTnrOnly == 1)
    {
        mAlignSizeWidth  = pOutTnr->nLineStride;
        mAlignSizeHeight = pOutTnr->nHeight;
    }
    else
    {
        mAlignSizeWidth  = pOutPicture0->nLineStride;
        mAlignSizeHeight = pOutPicture0->nHeight;
    }

    // when resolution change in auto mode, nWidth and nHeight will change, too
    dc->nWidth  = mAlignSizeWidth;
    dc->nHeight = mAlignSizeHeight;

    if(nTnrOnly == 1)
    {
        dc->mode = DI_MODE_TNR;
        dc->tnr_en = 0;
        dc->fmd_en = 0;
        if (preProcess(dc, diParams) < 0)
        {
            loge("preProcess failed.");
            return -1;
        }
        dc->picCount = 0;
    }
    else
    {
        if((pPrePrePicture == pCurPicture) && (pPrePicture == pCurPicture))
        {
            dc->mode = DI_MODE_BOB;
            dc->tnr_en = 0;
            dc->fmd_en = 0;
            if (preProcess(dc, diParams) < 0)
            {
                loge("preProcess failed.");
                return -1;
            }
            logv("BOB first");

            dc->picCount = 0;
        }
        else if(dc->mode == DI_MODE_BOB &&
                (pPrePrePicture == pPrePicture) && (pPrePicture != pCurPicture))
        {
            logv("BOB second");
        }
        else
        {
            dc->mode = DI_MODE_60HZ;
            dc->tnr_en = 1;
            dc->fmd_en = 1;
            if(preProcess(dc, diParams) < 0)
            {
                loge("preProcess failed.");
                return -1;
            }
        }
    }

    DiParaT3 mDiParaT3;
    memset(&mDiParaT3, 0, sizeof(DiParaT3));

    if(nTnrOnly == 1)
    {
        pOutTnr->nPts = pCurPicture->nPts;
    }
    else
    {
/*
 *note: dequeue 5 buffer at most, so pic_4list_num should be 6.
 *process times: 0(bob)-----1(bob)-------2(60HZ)-------3(60HZ)---------4(60HZ)-----5(60HZ)---
              p0--   --f0      p0--   --f0      p0--  --f0        p1--     --f0    tnr0--  --f0      tnr1--  --f0
              p0--DI--         p0--DI--        p1--DI--f1    tnr0--DI--f1       tnr1--DI--f1     tnr2--DI--f1
              p0--   --         p1--   --         p2--  --tnr0    p3--  --tnr1        p4--  --tnr2     p5--  --tnr
*/
        //* compute pts again, specifically for dealing with the abnormity of first and second pts.
        int nDefaultFrameRate = 30000; //* we set the frameRate to 30
        int nDefaultFrameDuration = 1000*1000*1000/nDefaultFrameRate;
        if (dc->picCount < 2)
        {
            pOutPicture0->nPts = pCurPicture->nPts + nDefaultFrameDuration/2;
            dc->nLastPts = pCurPicture->nPts;
        }
        else
        {
            if((pCurPicture->nPts - dc->nLastPts) > 0)
            {
                pOutPicture0->nPts = pCurPicture->nPts
                    + (pCurPicture->nPts - dc->nLastPts)/2;
                pOutPicture1->nPts = pOutPicture0->nPts
                    + (pCurPicture->nPts - dc->nLastPts)/2;
            }
            else
            {
                pOutPicture0->nPts = pCurPicture->nPts + nDefaultFrameDuration/2;
                pOutPicture1->nPts = pOutPicture0->nPts + nDefaultFrameDuration/2;
            }
            dc->nLastPts = pCurPicture->nPts;
            logv("pOutPicture0->nPts = %lld ms, pOutPicture1->nPts = %lld ms, diff0 = %lld ms, diff1 = %lld ms,"
                " cout=%d, pCurPicture->nPts = %lld  ms, pPrePicture->nPts = %lld ms, pPrePrePicture->nPts = %lld ms",
                  pOutPicture0->nPts/1000, pOutPicture1->nPts/1000,
                  (pOutPicture0->nPts -  pCurPicture->nPts)/1000, (pOutPicture1->nPts -  pCurPicture->nPts)/1000,
                  dc->picCount, pCurPicture->nPts/1000, pPrePicture->nPts/1000, pPrePrePicture->nPts/1000);
        }
    }

    DiPixelformatE2 eInFormat;
    DiPixelformatE2 eOutFormat;
    if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV12)
    {
        eInFormat = DI2_FORMAT_NV12;
    }
    else if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV21)
    {
        eInFormat = DI2_FORMAT_NV21;
    }
    else if (pCurPicture->ePixelFormat == PIXEL_FORMAT_YV12)
    {
        eInFormat = DI2_FORMAT_YV12;
    }
    else
    {
        loge("the inputPixelFormat is not support : %d", pCurPicture->ePixelFormat);
        return -1;
    }

    if(nTnrOnly == 1)
    {
        if (pOutTnr->ePixelFormat == PIXEL_FORMAT_NV12)
        {
            eOutFormat = DI2_FORMAT_NV12;
        }
        else if (pOutTnr->ePixelFormat == PIXEL_FORMAT_NV21)
        {
            eOutFormat = DI2_FORMAT_NV21;
        }
        else if (pOutTnr->ePixelFormat == PIXEL_FORMAT_YV12)
        {
            eOutFormat = DI2_FORMAT_YV12;
        }
        else
        {
            loge("the outputPixelFormat is not support : %d", pOutTnr->ePixelFormat);
            return -1;
        }
    }
    else
    {
        if (pOutPicture0->ePixelFormat == PIXEL_FORMAT_NV12)
        {
            eOutFormat = DI2_FORMAT_NV12;
        }
        else if (pOutPicture0->ePixelFormat == PIXEL_FORMAT_NV21)
        {
            eOutFormat = DI2_FORMAT_NV21;
        }
        else if (pOutPicture0->ePixelFormat == PIXEL_FORMAT_YV12)
        {
            eOutFormat = DI2_FORMAT_YV12;
        }
        else
        {
            loge("the outputPixelFormat is not support : %d", pOutPicture0->ePixelFormat);
            return -1;
        }

    }

    //* check parameters
    if((dc->tnr_en == 1 || dc->mode == DI_MODE_TNR) && (eOutFormat != DI2_FORMAT_YV12 &&  eOutFormat != DI2_FORMAT_YUV422P))
    {
        loge("should NOT use TNR, for TNR only support YV12/16 output");
        //return -1;
    }
    if(dc->mode == 0)
    {
        loge("mode not set");
        return -1;
    }
    if (dc->tnr_en && (dc->mode == DI_MODE_BOB))
    {
        loge("TNR can NOT be used when di mode is BOB");
        return -1;
    }
    if (dc->tnr_en && (dc->mode == DI_MODE_TNR))
    {
        loge("The para tnr_en is only be used in 30/60HZ mode, TNR_ONLY mode do NOT use tnr_en para");
        return -1;
    }

    if(nTnrOnly == 1) //TNR ONLY
    {
        diSetPara(dc, &mDiParaT3,
                  pPrePicture, pCurPicture, NULL,
                  NULL, NULL, pOutTnr,
                  nField, eInFormat, eOutFormat, nTnrOnly);
    }
    else
    {
        diSetPara(dc, &mDiParaT3,
                  pPrePrePicture, pPrePicture, pCurPicture,
                  pOutPicture0, pOutPicture1, pOutTnr,
                  nField, eInFormat, eOutFormat, nTnrOnly);
    }

    if (ioctl(dc->fd, DI_IOC_PROCESS_FB, (unsigned long)&mDiParaT3) < 0)
    {
        loge("di process failed.");
        return -1;
    }

    dc->picCount++;

    return 0;
}

#else

static int __DiProcess(Deinterlace* di,
                    VideoPicture *pPrePicture,
                    VideoPicture *pCurPicture,
                    VideoPicture *pOutPicture,
                    int nField)
{
    struct DiContext* dc;
    dc = (struct DiContext*)di;

    logv("call DeinterlaceProcess");

    if(pPrePicture == NULL || pCurPicture == NULL || pOutPicture == NULL)
    {
        loge("the input param is null : %p, %p, %p", pPrePicture, pCurPicture, pOutPicture);
        return -1;
    }

    if(dc->fd == -1)
    {
        loge("not init...");
        return -1;
    }

    DiParaT        mDiParaT;
    DiRectSizeT    mRealSize;
    DiRectSizeT    mAlignSize;
    DiPixelformatE eInFormat;
    DiPixelformatE eOutFormat;

    //* compute pts again
    if (dc->picCount < 2)
    {
        int nFrameRate = 30000; //* we set the frameRate to 30
        pOutPicture->nPts = pCurPicture->nPts + nField * (1000 * 1000 * 1000 / nFrameRate) / 2;
    }
    else
    {
        pOutPicture->nPts = pCurPicture->nPts + nField * (pCurPicture->nPts - pPrePicture->nPts)/2;
    }

    logv("pCurPicture->nPts = %lld  ms, pOutPicture->nPts = %lld ms, diff = %lld ms ",
          pCurPicture->nPts/1000,
          pOutPicture->nPts/1000,
          (pOutPicture->nPts -  pCurPicture->nPts)/1000
          );

    if (pOutPicture->ePixelFormat == PIXEL_FORMAT_NV12)
    {
        eOutFormat = DI_FORMAT_NV12;
    }
    else if (pOutPicture->ePixelFormat == PIXEL_FORMAT_NV21)
    {
        eOutFormat = DI_FORMAT_NV21;
    }
    else
    {
        loge("the outputPixelFormat is not support : %d",pOutPicture->ePixelFormat);
        return -1;
    }

    const char *str_difmt = GetConfigParamterString("deinterlace_fmt", NULL);
    CDX_LOG_CHECK(str_difmt, "'deinterlace_fmt' not define, pls check your cedarx.conf");
    if (strcmp(str_difmt, "mb32") == 0)
    {
        eInFormat = DI_FORMAT_MB32_12;
    }
    else if (strcmp(str_difmt, "nv") == 0)
    {
        if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV12)
        {
            eInFormat = DI_FORMAT_NV12;
        }
        else if (pCurPicture->ePixelFormat == PIXEL_FORMAT_NV21)
        {
            eInFormat = DI_FORMAT_NV21;
        }
        else
        {
            loge("the inputPixelFormat is not support : %d",pCurPicture->ePixelFormat);
            return -1;
        }
    }
    else if (strcmp(str_difmt, "nv21") == 0)
    {
        eInFormat = DI_FORMAT_NV21;
    }
    else
    {
        eInFormat = DI_FORMAT_NV12;
    }

    mRealSize.nWidth  = pCurPicture->nRightOffset - pCurPicture->nLeftOffset;
    mRealSize.nHeight = pCurPicture->nBottomOffset - pCurPicture->nTopOffset;
    mAlignSize.nWidth  = pOutPicture->nWidth;
    mAlignSize.nHeight = pOutPicture->nHeight;

    mDiParaT.mInputFb.mRectSize  = mAlignSize;
    mDiParaT.mInputFb.eFormat    = eInFormat;
    mDiParaT.mPreFb.mRectSize    = mAlignSize;
    mDiParaT.mPreFb.eFormat      = eInFormat;
    mDiParaT.mOutputFb.mRectSize = mAlignSize;
    mDiParaT.mOutputFb.eFormat   = eOutFormat;
    mDiParaT.mSourceRegion       = mRealSize;
    mDiParaT.mOutRegion          = mRealSize;
    mDiParaT.nField              = nField;
    mDiParaT.bTopFieldFirst      = pCurPicture->bTopFieldFirst;

    //* we can use the phy address
    mDiParaT.mInputFb.addr[0]    = pCurPicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
    mDiParaT.mInputFb.addr[1]    = pCurPicture->phyCBufAddr + CONF_VE_PHY_OFFSET;
    mDiParaT.mPreFb.addr[0]      = pPrePicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
    mDiParaT.mPreFb.addr[1]      = pPrePicture->phyCBufAddr + CONF_VE_PHY_OFFSET;
    mDiParaT.mOutputFb.addr[0]   = pOutPicture->phyYBufAddr + CONF_VE_PHY_OFFSET;
    mDiParaT.mOutputFb.addr[1]   = pOutPicture->phyCBufAddr + CONF_VE_PHY_OFFSET;

    logv("VideoRender_CopyFrameToGPUBuffer aw_di_setpara start");

    //dumpPara(&mDiParaT);

    if (ioctl(dc->fd, DI_IOCSTART, &mDiParaT) != 0)
    {
        loge("aw_di_setpara failed!");
        return -1;
    }
    dc->picCount++;

    return 0;
}


#endif


static struct DeinterlaceOps mDi =
{
    .destroy           = __DiDestroy,
    .init              = __DiInit,
    .reset             = __DiReset,
    .expectPixelFormat = __DiExpectPixelFormat,
    .flag              = __DiFlag,
#if defined(CONF_DI_V2X_SUPPORT)
    .process2          = __DiProcess2,
#elif defined(CONF_DI_300_SUPPORT)
    .process3          = __DiProcess3,
#else
    .process           = __DiProcess,
#endif
};

Deinterlace* DeinterlaceCreate()
{
    struct DiContext* dc = (struct DiContext*)malloc(sizeof(struct DiContext));
    if(dc == NULL)
    {
        logw("deinterlace create failed");
        return NULL;
    }
    memset(dc, 0, sizeof(struct DiContext));

    // we must set the fd to -1; or it will close the file which fd is 0 when destroy
    dc->fd = -1;

    dc->base.ops = &mDi;

    return &dc->base;
}
