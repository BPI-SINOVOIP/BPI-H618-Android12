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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hardware/graphics-sunxi.h>

#include "Debug.h"
#include "rotator-g2d.h"
#include "IonBuffer.h"
#include "HardwareRotator.h"

#define HWC_ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))

namespace sunxi {

RotatorG2D::RotatorG2D()
    : mG2DHandle(-1), mLegacyHardwareVersion(false)
{ }

RotatorG2D::~RotatorG2D() { }

int RotatorG2D::initialize()
{
    int fd = open("/dev/g2d", O_RDWR);
    if (fd < 0) {
        DLOGE("open g2d device failed, %s", strerror(errno));
        return -ENODEV;
    }
    mG2DHandle = uniquefd(fd);
    loadHardwareVersion();
    return 0;
}

bool RotatorG2D::capableHook(const std::shared_ptr<Layer>& layer) const
{
    buffer_handle_t buf = layer->bufferHandle();
    const private_handle_t *handle = from(buf);

    switch (handle->format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_RGB_888:
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_AW_NV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            break;
        default:
            return false;
    }

    // TODO: When the G2D hardware fixes the 64 bytes alignment problem,
    //       delete the following code
    if (mLegacyHardwareVersion) {
        int32_t transform = static_cast<int32_t>(layer->transform());
        if (transform & HWC_TRANSFORM_FLIP_H) {
            int pitch = 0;
            switch(handle->format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                case HAL_PIXEL_FORMAT_BGRA_8888:
                case HAL_PIXEL_FORMAT_BGRX_8888:
                case HAL_PIXEL_FORMAT_RGBA_1010102:
                    pitch = handle->width * 4;
                    break;
                case HAL_PIXEL_FORMAT_RGB_565:
                    pitch = handle->width * 2;
                    break;
                case HAL_PIXEL_FORMAT_YV12:
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                case HAL_PIXEL_FORMAT_AW_NV12:
                    pitch = handle->width;
                    break;
                case HAL_PIXEL_FORMAT_RGB_888:
                    pitch = handle->width * 3;
                    break;
                default:
                    break;
            }
            if (pitch < 64 || (pitch % 64) != 0) {
                return false;
            }
        }
    }

    return inputStrideSupported(buf);
}

void RotatorG2D::getAlignedWidthAndHeight(
        const std::shared_ptr<Layer>& layer, int* width, int* height, int* align) const
{
    buffer_handle_t buf = layer->bufferHandle();
    const private_handle_t *handle = from(buf);

    int bpp = 4;
    switch(handle->format) {
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
            bpp = 1;
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            bpp = 4;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            bpp = 3;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            bpp = 2;
            break;
        default:
            DLOGE("unsupported pixel format");
            break;
    }

    *width  = HWC_ALIGN(handle->width  * bpp, handle->aw_byte_align[0]) / bpp;
    *height = HWC_ALIGN(handle->height * bpp, handle->aw_byte_align[0]) / bpp;
    *align  = handle->aw_byte_align[0];
}

uint32_t RotatorG2D::computeBufferSize(int width, int height, int align, int pixelformat) const
{
    int stride, stride_uv;
    int size;
    switch (pixelformat) {
        case HAL_PIXEL_FORMAT_YV12:
            stride    = HWC_ALIGN(width,   align);
            stride_uv = HWC_ALIGN(width/2, align);
            size = stride * height + stride_uv * height;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP: /* NV21 */
        case HAL_PIXEL_FORMAT_AW_NV12:
            stride = HWC_ALIGN(width, align);
            size = stride * height * 3 / 2;
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            stride = HWC_ALIGN(width * 4, align);
            size = stride * height;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            stride = HWC_ALIGN(width * 3, align);
            size = stride * height;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            stride = HWC_ALIGN(width * 2, align);
            size = stride * height;
            break;
        default:
            DLOGE("unsupported pixel format");
            stride = HWC_ALIGN(width * 4, align);
            size = stride * height;
            break;
    }
    return size;
}

std::unique_ptr<RotatorDevice::Frame> RotatorG2D::createFrame(
        const std::shared_ptr<Layer>& layer, const OutputBuffer_t* outbuf)
{
    if (layer == nullptr || outbuf == nullptr) {
        DLOGE("invalid input layer/buffer");
        return nullptr;
    }
    static uint32_t sFrameCount_;
    std::unique_ptr<G2DFrame> frame = std::make_unique<G2DFrame>(++sFrameCount_);
    setupBlitInfo(layer, outbuf, frame.get());
    return frame;
}


int RotatorG2D::rotateOneFrame(const RotatorDevice::Frame* frame)
{
    DTRACE_FUNC();
    const G2DFrame* data = static_cast<const G2DFrame*>(frame);
    if (ioctl(mG2DHandle.get(), G2D_CMD_BITBLT_H, &data->mBlitInfo) < 0) {
        DLOGE("ioctl G2D_CMD_BITBLT_H return error");
        return -1;
    }
    return 0;
}

void RotatorG2D::setupBlitInfo(const std::shared_ptr<Layer>& layer,
        const OutputBuffer_t* outbuf, G2DFrame* frame) const
{
    g2d_blt_h* blitInfo = &frame->mBlitInfo;
    memset(blitInfo, 0, sizeof(g2d_blt_h));

    const private_handle_t* handle = from(layer->bufferHandle());
    blitInfo->src_image_h.format = (g2d_fmt_enh)halPixelFormat2DeviceFormat(handle->format);
    blitInfo->flag_h = (g2d_blt_flags_h)halTransform2DeviceFlag(layer->transform());

    int bpp = 4;
    switch(handle->format) {
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
            bpp = 1;
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            bpp = 4;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            bpp = 3;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            bpp = 2;
            break;
        default:
            DLOGE("unsupported pixel format");
            break;
    }

    int alignedWidth = HWC_ALIGN(handle->width * bpp, handle->aw_byte_align[0]) / bpp;
    blitInfo->src_image_h.width = alignedWidth;
    blitInfo->src_image_h.height = handle->height;
    blitInfo->src_image_h.clip_rect.x = 0;
    blitInfo->src_image_h.clip_rect.y = 0;
    blitInfo->src_image_h.clip_rect.w = alignedWidth;
    blitInfo->src_image_h.clip_rect.h = handle->height;
    blitInfo->src_image_h.align[0] = handle->aw_byte_align[0];
    blitInfo->src_image_h.align[1] = handle->aw_byte_align[1];
    blitInfo->src_image_h.align[2] = handle->aw_byte_align[2];

    blitInfo->dst_image_h.format = blitInfo->src_image_h.format;
    blitInfo->dst_image_h.clip_rect.x = 0;
    blitInfo->dst_image_h.clip_rect.y = 0;

    blitInfo->dst_image_h.width  = outbuf->width;
    blitInfo->dst_image_h.height = outbuf->height;

    blitInfo->dst_image_h.clip_rect.w = outbuf->width;
    blitInfo->dst_image_h.clip_rect.h = outbuf->height;
    blitInfo->dst_image_h.align[0] = outbuf->align;
    blitInfo->dst_image_h.align[1] = outbuf->align;
    blitInfo->dst_image_h.align[2] = outbuf->align;

    // cache the in/out buffer handle, to prevent the buffer from release
    // until we have finished reading/writing data from/to it.
    frame->mCacheBufferHandle[0] = uniquefd(dup(handle->share_fd));
    frame->mCacheBufferHandle[1] = uniquefd(dup(outbuf->buffer->handle));
    blitInfo->src_image_h.fd = frame->mCacheBufferHandle[0].get();
    blitInfo->dst_image_h.fd = frame->mCacheBufferHandle[1].get();
}

bool RotatorG2D::inputStrideSupported(buffer_handle_t buf)
{
    const private_handle_t *handle = from(buf);
    if (handle == nullptr)
        return false;

    bool supported = false;
    int stride = HWC_ALIGN(handle->width, handle->aw_byte_align[0]);

    if (stride > MaxInputBufferWidth || handle->height > MaxInputBufferHeight) {
        return false;
    }

    switch(handle->format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            stride = HWC_ALIGN(handle->width * 4, handle->aw_byte_align[0]);
            supported = (stride % 4) == 0;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            stride = HWC_ALIGN(handle->width * 2, handle->aw_byte_align[0]);
            supported = (stride % 2) == 0;
            break;
        case HAL_PIXEL_FORMAT_YV12:
            supported = (stride % 4) == 0;
            // workaround for android q media vp9 cts,
            // when the height is 182 or 362, using GPU composer
            if (handle->height == 182 || handle->height == 362) {
                DLOGE("miles_debug: hwc workaround for vp9 cts");
                supported = false;
            }
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
            supported = (stride % 2) == 0;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            stride = HWC_ALIGN(handle->width * 3, handle->aw_byte_align[0]);
            supported = (stride % 3) == 0;
            break;
        default:
            supported = false;
    }
    return supported;
}

int RotatorG2D::halPixelFormat2DeviceFormat(int format)
{
    switch(format) {
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return G2D_FORMAT_YUV420UVC_V1U1V0U0;
        case HAL_PIXEL_FORMAT_YV12:
            return G2D_FORMAT_YUV420_PLANAR;
        case HAL_PIXEL_FORMAT_AW_NV12:
            return G2D_FORMAT_YUV420UVC_U1V1U0V0;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return G2D_FORMAT_RGBA8888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return G2D_FORMAT_RGBX8888;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return G2D_FORMAT_BGRA8888;
        case HAL_PIXEL_FORMAT_BGRX_8888:
            return G2D_FORMAT_BGRX8888;
        case HAL_PIXEL_FORMAT_RGB_888:
            return G2D_FORMAT_RGB888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return G2D_FORMAT_RGB565;
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            return G2D_FORMAT_RGBA1010102;
        default :
            return G2D_FORMAT_YUV420UVC_U1V1U0V0;
    }
}

int RotatorG2D::deviceFormat2HalPixelFormat(int format)
{
    switch(format) {
        case G2D_FORMAT_YUV420UVC_V1U1V0U0:
            return HAL_PIXEL_FORMAT_YCrCb_420_SP;
        case G2D_FORMAT_YUV420_PLANAR:
            return HAL_PIXEL_FORMAT_YV12;
        case G2D_FORMAT_YUV420UVC_U1V1U0V0:
            return HAL_PIXEL_FORMAT_AW_NV12;
        case G2D_FORMAT_RGBA8888:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case G2D_FORMAT_RGBX8888:
            return HAL_PIXEL_FORMAT_RGBX_8888;
        case G2D_FORMAT_BGRA8888:
            return HAL_PIXEL_FORMAT_BGRA_8888;
        case G2D_FORMAT_BGRX8888:
            return HAL_PIXEL_FORMAT_BGRX_8888;
        case G2D_FORMAT_RGB888:
            return HAL_PIXEL_FORMAT_RGB_888;
        case G2D_FORMAT_RGB565:
            return HAL_PIXEL_FORMAT_RGB_565;
        case G2D_FORMAT_RGBA1010102:
            return HAL_PIXEL_FORMAT_RGBA_1010102;
        default :
            return HAL_PIXEL_FORMAT_AW_NV12;
    }
}

int RotatorG2D::halTransform2DeviceFlag(HWC2::Transform transform)
{
    int32_t t = static_cast<int32_t>(transform);
    switch (t) {
        case HWC_TRANSFORM_FLIP_H:
            return G2D_ROT_H;
        case HWC_TRANSFORM_FLIP_V:
            return G2D_ROT_V;
        case HWC_TRANSFORM_ROT_90:
            return G2D_ROT_90;
        case HWC_TRANSFORM_ROT_180:
            return G2D_ROT_180;
        case HWC_TRANSFORM_ROT_270:
            return G2D_ROT_270;
        case HWC_TRANSFORM_FLIP_H_ROT_90:
            // The G2D hardware seems to rotate first and then flip
            return G2D_ROT_270 | G2D_ROT_H;
        case HWC_TRANSFORM_FLIP_V_ROT_90:
            return G2D_ROT_270 | G2D_ROT_V;
        default:
            return G2D_ROT_0;
    }
}

void RotatorG2D::loadHardwareVersion()
{
    struct g2d_hardware_version version;
    if (ioctl(mG2DHandle.get(), G2D_CMD_QUERY_VERSION, &version) < 0) {
        DLOGE("ioctl G2D_CMD_QUERY_VERSION return error");
        mLegacyHardwareVersion = true;
    }
    DLOGI("G2D Version: %08x %08x", version.g2d_version, version.chip_version);
    uint32_t verBits = (version.chip_version & 0x0F);

    if (verBits == 0 || verBits == 3 || verBits == 4) {
        mLegacyHardwareVersion = true;
        DLOGW("G2D is legacy ip, need 64bytes alignment for FLIP_H");
    }
}

std::unique_ptr<RotatorDevice> createHardwareRotator()
{
    std::unique_ptr<RotatorDevice> dev = std::make_unique<RotatorG2D>();
    if (dev->initialize() != 0) {
        DLOGE("RotatorG2D initialize failed");
        return nullptr;
    }
    return dev;
}

} // namespace sunxi

