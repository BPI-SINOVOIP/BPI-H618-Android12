#define LOG_NDEBUG 1

#define LOG_TAG "DiProcess"

#include "DiProcess.h"
#include "deinterlace2.h"
#include <string.h>

#include <linux/videodev2.h>

#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))

namespace android {

DiProcess::DiProcess()
    :mDiFd(-1)
    ,mDiReqId(-1)
{
    ALOGD("F:%s,L:%d",__FUNCTION__,__LINE__);
}

DiProcess::~DiProcess()
{
    ALOGD("F:%s,L:%d",__FUNCTION__,__LINE__);
    release();
    ALOGD("F:%s ok ,L:%d",__FUNCTION__,__LINE__);
}

status_t DiProcess::init()
{
    int ret = -1;
    if (mDiFd > 0) {
        ALOGE("DiProcess is already init,F:%s,L:%d",__FUNCTION__,__LINE__);
        return INVALID_OPERATION;
    }
    ALOGD("open /dev/deinterlace , F:%s,L:%d",__FUNCTION__,__LINE__);

    mDiFd = open("/dev/deinterlace", O_RDWR, 0);
    if (mDiFd <= 0) {
        ALOGE("open /dev/deinterlace failed,F:%s,L:%d",__FUNCTION__,__LINE__);
        return INVALID_OPERATION;
    }

    ALOGD("open /dev/deinterlace OK, F:%s,L:%d",__FUNCTION__,__LINE__);
    return NO_ERROR;
}

status_t DiProcess::diProcess(unsigned char *pre_src, unsigned char *now_src, unsigned char *next_src,
                                int src_width, int src_height, int in_format,
                                unsigned char *dst, int dst_width, int dst_height,
                                int out_format, int nField)
{
    int ret = -1;
    __di_para_t2 di_para;
    memset(&di_para,0x0,sizeof(__di_para_t2));
    if (mDiFd < 0)
    {
        ALOGE("deinterlace is not init!F:%s,L:%d",__FUNCTION__,__LINE__);
        return NO_INIT;
    }
    if(mDiReqId < 0)
    {
        ret = ioctl(mDiFd, DI_REQUEST, &mDiReqId);
        if(ret > 0)
        {
            ALOGE("DI_REQUEST failed,F:%s,L:%d",__FUNCTION__,__LINE__);
            return INVALID_OPERATION;
        }
        //ALOGD("DI_REQUEST OK, F:%s,L:%d, id:%d",__FUNCTION__,__LINE__,mDiReqId);
    }
    di_para.dma_if = 1;
    __di_pixel_fmt_t di_in_format;
    switch (in_format) {
        case V4L2_PIX_FMT_NV21:
            di_in_format = DI_FORMAT_NV21;
            break;
        case V4L2_PIX_FMT_NV16:
            di_in_format = DI_FORMAT_YUV422_SP_UVUV;
            break;
        case V4L2_PIX_FMT_NV61:
            di_in_format = DI_FORMAT_YUV422_SP_VUVU;
            break;
        default:
            di_in_format = DI_FORMAT_NV21;
            break;
    }

    __di_pixel_fmt_t di_out_format;
    switch (out_format) {
        case V4L2_PIX_FMT_NV21:
            di_out_format = DI_FORMAT_NV21;
            break;
        case V4L2_PIX_FMT_NV16:
            di_out_format = DI_FORMAT_YUV422_SP_UVUV;
            break;
        case V4L2_PIX_FMT_NV61:
            di_out_format = DI_FORMAT_YUV422_SP_VUVU;
            break;
        default:
            di_out_format = DI_FORMAT_NV21;
            break;
    }

    di_para.pre_fb.addr[0] = (unsigned long long)pre_src;
    di_para.pre_fb.addr[1] = (unsigned long long)(pre_src + ALIGN_16B(src_width)*src_height);
    di_para.pre_fb.addr[2] = 0x0;
    di_para.pre_fb.size.width = src_width;
    di_para.pre_fb.size.height = src_height;
    di_para.pre_fb.format = di_in_format;

    di_para.input_fb.addr[0] = (unsigned long long)now_src;
    di_para.input_fb.addr[1] = (unsigned long long)(now_src + ALIGN_16B(src_width)*src_height);
    di_para.input_fb.addr[2] = 0x0;
    di_para.input_fb.size.width = src_width;
    di_para.input_fb.size.height = src_height;
    di_para.input_fb.format = di_in_format;

    di_para.next_fb.addr[0] = (unsigned long long)next_src;
    di_para.next_fb.addr[1] = (unsigned long long)(next_src + ALIGN_16B(src_width)*src_height);
    di_para.next_fb.addr[2] = 0x0;
    di_para.next_fb.size.width = src_width;
    di_para.next_fb.size.height = src_height;
    di_para.next_fb.format = di_in_format;

    di_para.source_regn.width = src_width;
    di_para.source_regn.height = src_height;

    di_para.output_fb.addr[0] = (unsigned long long)dst;
    di_para.output_fb.addr[1] = (unsigned long long)(dst + ALIGN_16B(dst_width)*dst_height);
    di_para.output_fb.addr[2] = 0x0;
    di_para.output_fb.size.width = dst_width;
    di_para.output_fb.size.height = dst_height;
    di_para.output_fb.format = di_out_format;
    di_para.out_regn.width = dst_width;
    di_para.out_regn.height = dst_height;

    di_para.field = nField;
    di_para.top_field_first = 1;//nField;
    di_para.id = mDiReqId;

    ret = ioctl(mDiFd,DI_IOCSTART2,&di_para);
    if (ret < 0) {
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t DiProcess::release()
{
    ALOGD("DiProcess::release mDiFd=%d",mDiFd);
    if (mDiFd > 0) {
        if (mDiReqId >= 0) {
            ALOGD("DI_RELEASE, F:%s,L:%d, id:%d start",__FUNCTION__,__LINE__,mDiReqId);
            ioctl(mDiFd, DI_RELEASE, &mDiReqId);
            ALOGD("DI_RELEASE, F:%s,L:%d, id:%d",__FUNCTION__,__LINE__,mDiReqId);
            mDiReqId = -1;
        }
        close(mDiFd);
        mDiFd = -1;
        ALOGD("DiProcess,release ok, F:%s,L:%d",__FUNCTION__,__LINE__);
    }
    return NO_ERROR;
}
}

