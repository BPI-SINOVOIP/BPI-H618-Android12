/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hardware/graphics-sunxi.h>

#include "Debug.h"
#include "helper.h"
#include "private_handle.h"

disp_pixel_format toDispFormat(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return DISP_FORMAT_ABGR_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return DISP_FORMAT_XBGR_8888;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return DISP_FORMAT_ARGB_8888;
    case HAL_PIXEL_FORMAT_BGRX_8888:
        return DISP_FORMAT_XRGB_8888;
    case HAL_PIXEL_FORMAT_RGBA_1010102:
        return DISP_FORMAT_A2B10G10R10;
    case HAL_PIXEL_FORMAT_RGB_888:
        return DISP_FORMAT_BGR_888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return DISP_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_YV12:
        return DISP_FORMAT_YUV420_P;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        return DISP_FORMAT_YUV420_SP_VUVU;
    case HAL_PIXEL_FORMAT_AW_NV12:
        return DISP_FORMAT_YUV420_SP_UVUV;
    case HAL_PIXEL_FORMAT_AW_YV12_10bit:
    case HAL_PIXEL_FORMAT_AW_I420_10bit:
        return DISP_FORMAT_YUV420_P;
    case HAL_PIXEL_FORMAT_AW_NV21_10bit:
        return DISP_FORMAT_YUV420_SP_VUVU;
    case HAL_PIXEL_FORMAT_AW_NV12_10bit:
        return DISP_FORMAT_YUV420_SP_UVUV;
    case HAL_PIXEL_FORMAT_AW_P010_UV:
        return DISP_FORMAT_YUV420_SP_UVUV_10BIT;
    case HAL_PIXEL_FORMAT_AW_P010_VU:
        return DISP_FORMAT_YUV420_SP_VUVU_10BIT;
    default:
        DLOGE("unknow format: 0x%08x", format);
        return (disp_pixel_format)-1;
    }
}

bool compositionEngineV2FormatFilter(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_RGB_888:
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
        case HAL_PIXEL_FORMAT_AW_NV12_10bit:
        case HAL_PIXEL_FORMAT_AW_NV21_10bit:
        case HAL_PIXEL_FORMAT_AW_I420_10bit:
        case HAL_PIXEL_FORMAT_AW_YV12_10bit:
        case HAL_PIXEL_FORMAT_AW_P010_UV:
        case HAL_PIXEL_FORMAT_AW_P010_VU:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            return true;
        default:
            return false;
    }
}

bool isAfbcBuffer(buffer_handle_t buf)
{
#ifdef GRALLOC_SUNXI_METADATA_BUF
    if (buf != nullptr) {
        private_handle_t *handle = (private_handle_t *)buf;
        int metadata_fd = handle->metadata_fd;
        unsigned int flag = handle->ion_metadata_flag;
        if ((0 <= metadata_fd)
                && (flag & SUNXI_METADATA_FLAG_AFBC_HEADER)) {
            return true;
        }
    }
#endif
    return false;
}

bool isHdrpBuffer(buffer_handle_t buf)
{
#ifdef GRALLOC_SUNXI_METADATA_BUF
    if (buf != nullptr) {
        private_handle_t *handle = (private_handle_t *)buf;
        int metadata_fd = handle->metadata_fd;
        if (0 <= metadata_fd) {
            void *map_ptr = NULL;
            int map_fd = handle->metadata_fd;
            struct sunxi_metadata *ptr = NULL;

            if (0 == handle->ion_metadata_size) {
                DLOGW("ion_metadata_size is 0");
                return false;
            }

            map_ptr = mmap(NULL, handle->ion_metadata_size, PROT_READ,
                           MAP_SHARED, map_fd, 0);
            if (map_ptr == MAP_FAILED) {
                DLOGE("ion_map failed, map_fd=%d\n", map_fd);
                return false;
            }
            ptr = (struct sunxi_metadata *)map_ptr;

            if (ptr != NULL && (ptr->flag & SUNXI_METADATA_FLAG_HDRP_HEADER)) {
                if (!(handle->ion_metadata_flag & SUNXI_METADATA_FLAG_HDRP_HEADER))
                    DLOGW("cderc flag something wrong. handle->ion_metadata_flag = %x",
                          handle->ion_metadata_flag);
                munmap(map_ptr, handle->ion_metadata_size);
                return true;
            }
            munmap(map_ptr, handle->ion_metadata_size);

        }
    }
#endif
    return false;
}

bool isPhysicalContinuousBuffer(buffer_handle_t buf)
{
#if 0
    // gpu img rgx private_handle_t do not has member flags.
    if (buf != nullptr) {
        private_handle_t *handle = (private_handle_t *)buf;
        if (handle->flags & SUNXI_MEM_CONTIGUOUS)
            return true;
    }
#endif
    return false;
}

// limitations of aw hardware
#define SCALEMAX 32
#define SCALEMIN 16

#define EPSINON (0.001)
#define FLOAT_EQUAL(x, y) (fabs((x)-(y)) <= EPSINON)

using sunxi::ScreenTransform;

bool hardwareScalerCheck(
        int defreq, const ScreenTransform& transform,
        bool isyuv, const hwc_rect_t& crop, const hwc_rect_t& frame)
{
    Rect sourceCrop(crop);
    Rect displayFrame(frame);

    if (!sourceCrop.isValid() || !displayFrame.isValid()) {
        DLOGE("scaler input crop/frame is invalid");
        return false;
    }

    displayFrame = transform.transform(displayFrame);
    int srcW = sourceCrop.width();
    int srcH = sourceCrop.height();
    int dstW = displayFrame.width();
    int dstH = displayFrame.height();
    float zoomx = ((float)dstW) / ((float)srcW);
    float zoomy = ((float)dstH) / ((float)srcH);

    if (FLOAT_EQUAL(1.0f, zoomx) && FLOAT_EQUAL(1.0f, zoomy)) {
        // no scale
        return true;
    }

    // minimum input size is 8x4
    if (srcW < 8 || srcH < 4) {
        return false;
    }

    const Rect& screenBounds = transform.getScreenSize();
    const Rect& framebufferBounds = transform.getFramebufferSize();
    float uiScaleX = ((float)screenBounds.width() ) / ((float)framebufferBounds.width() );
    float uiScaleY = ((float)screenBounds.height()) / ((float)framebufferBounds.height());
    float outputScaleX, outputScaleY;
    transform.getScale(&outputScaleX, &outputScaleY);
    float globalScaleX = zoomx * outputScaleX * uiScaleX;
    float globalScaleY = zoomy * outputScaleY * uiScaleY;

    DLOGI_IF(kTagScaler, "g %f %f z %f %f o %f %f ui %f %f",
            globalScaleX, globalScaleY, zoomx, zoomy, outputScaleX, outputScaleY, uiScaleX, uiScaleY);

    // minimum output size is 8x4
    if (srcW * globalScaleX < 8 || srcH * globalScaleY < 4) {
        DLOGD_IF(kTagScaler, "display frame is less then minimum output size");
        return false;
    }

    int iArea = srcW * srcH;
    int oArea = dstW * dstH;
    if ((oArea * SCALEMIN <= iArea) || (oArea >= iArea * SCALEMAX)) {
        DLOGD_IF(kTagScaler, "area error %d %d", iArea, oArea);
        return false;
    }

    int outWidth  = (int)ceilf(srcW * globalScaleX);
    int outHeight = (int)ceilf(srcH * globalScaleY);

    long long vsyncPeroid = 1000000000 / 60;
    long long screenLinePeroid = vsyncPeroid / screenBounds.height();
    long long layerLinePeroid  = 0;

    if (defreq < 1000) {
        // defreq is too small and defreq/1000 = 0, div zero error.
        DLOGD_IF(kTagScaler, "display engine frequency is too small(%d Hz)", defreq);
        return false;
    }

    if (srcH > outHeight && !isyuv) {
        long long a = (srcW > outWidth)
            ? (1000000 * ((long long)(srcW * srcH / outHeight)) / (defreq / 1000))
            : (1000000 * ((long long)(outWidth)) / (defreq / 1000));

        long long b = 1000000 * ((long long)(screenBounds.width() - outWidth)) / (defreq / 1000);
        layerLinePeroid = a + b;
    } else {
        layerLinePeroid =
            (srcW > outWidth)
            ? (1000000 * ((long long)(screenBounds.width() - outWidth + srcW)) / (defreq / 1000))
            : (1000000 * ((long long)(screenBounds.width())) / (defreq / 1000));
    }

    DLOGI_IF(kTagScaler, "screenLinePeroid %lld layerLinePeroid %lld, defreq %d",
            screenLinePeroid, layerLinePeroid, defreq);

    if((screenLinePeroid * 4 / 5) < layerLinePeroid) {
        return false;
    } else if (!isyuv) {
        return framebufferBounds.width() < 2048;
    }

    return true;
}

/*
 * check if de can handle ui scale.
 *
 * input params:
 * ovlw, ovlh : source size
 * outw, outh : frame size
 * lcdw, lcdh : screen size
 * lcd_fps : frame rate in Hz
 * de_freq : de clock freqence in Hz
 *
 * return:
 * 1 -- can use de scale
 * 0 -- can NOT use de scale
 */
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
static int de_scale_capability_detect(
        unsigned int ovlw, unsigned int ovlh,
        unsigned int outw, unsigned int outh,
        unsigned long long lcdw,
        unsigned long long lcdh,
        unsigned long long lcd_fps,
        unsigned long long de_freq)
{
    unsigned long long de_freq_req;
    unsigned long long lcd_freq;
    unsigned long long layer_cycle_num;
    unsigned int dram_efficience = 95;

    /*
     *  ovlh > outh : vertical scale down
     *  ovlh < outh : vertical scale up
     */
    if (ovlh > outh)
        layer_cycle_num = max(ovlw, outw) + max((ovlh - outh) * ovlw / outh, lcdw - outw);
    else
        layer_cycle_num = max(ovlw, outw) + (lcdw - outw);

    lcd_freq = (lcdh) * (lcdw) * (lcd_fps) * 10 / 9;
    de_freq_req = lcd_freq * layer_cycle_num * 100 / dram_efficience;
    de_freq_req = de_freq_req / lcdw;

    if (de_freq > de_freq_req)
        return 1;
    else
        return 0;
}

bool afbcBufferScaleCheck(
        int defreq, const ScreenTransform& transform,
        const hwc_rect_t& crop, const hwc_rect_t& frame)
{
    Rect sourceCrop(crop);
    Rect displayFrame(frame);
    const Rect& screenBounds = transform.getScreenSize();
    const Rect& framebufferBounds = transform.getFramebufferSize();
    float uiScaleX = ((float)screenBounds.width() ) / ((float)framebufferBounds.width() );
    float uiScaleY = ((float)screenBounds.height()) / ((float)framebufferBounds.height());
    float outputScaleX, outputScaleY;
    transform.getScale(&outputScaleX, &outputScaleY);

    int src_w = sourceCrop.width();
    int src_h = sourceCrop.height();
    int out_w = displayFrame.width();
    int out_h = displayFrame.height();

    out_w = out_w * uiScaleX * outputScaleX;
    out_h = out_h * uiScaleY * outputScaleY;

    out_w = out_w == 0 ? 1 : out_w;
    out_h = out_h == 0 ? 1 : out_h;

    return de_scale_capability_detect(src_w, src_h, out_w, out_h,
            screenBounds.width(), screenBounds.height(), 60, defreq);
}

void hw_layer_info_revise(disp_layer_info2* info)
{
    long long tmp;
    if (info->mode == LAYER_MODE_COLOR) {
        tmp = info->fb.crop.width >> 32;
    if (tmp < 8)
        info->fb.crop.width = 8LL << 32;

        tmp = info->fb.crop.height >> 32;
    if (tmp < 8)
        info->fb.crop.height = 8LL << 32;

    return;
    }
    if (info->fb.format == DISP_FORMAT_YUV420_P ||
        info->fb.format == DISP_FORMAT_YUV420_SP_VUVU ||
        info->fb.format == DISP_FORMAT_YUV420_SP_UVUV ||
        info->fb.format == DISP_FORMAT_YUV420_SP_UVUV_10BIT ||
        info->fb.format == DISP_FORMAT_YUV420_SP_VUVU_10BIT) {
        tmp = info->fb.crop.width >> 32;
        if (tmp % 2)
            info->fb.crop.width = (tmp == 1 ? 2LL : tmp - 1) << 32;
        tmp = info->fb.crop.height >> 32;
        if (tmp % 2)
            info->fb.crop.height = (tmp == 1 ? 2LL : tmp - 1) << 32;
    }
}
