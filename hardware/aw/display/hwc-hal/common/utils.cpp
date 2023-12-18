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

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <system/graphics.h>
#include <hardware/graphics-sunxi.h>
#include "private_handle.h"
#include "Debug.h"

int bitsPerPixel(int format)
{
    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_BGRX_8888:
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            return 32;
        case HAL_PIXEL_FORMAT_RGB_888:
            return 24;
        case HAL_PIXEL_FORMAT_RGB_565:
            return 16;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        case HAL_PIXEL_FORMAT_AW_NV12:
        case HAL_PIXEL_FORMAT_AW_YV12_10bit:
        case HAL_PIXEL_FORMAT_AW_I420_10bit:
        case HAL_PIXEL_FORMAT_AW_NV21_10bit:
        case HAL_PIXEL_FORMAT_AW_NV12_10bit:
            return 12;
        case HAL_PIXEL_FORMAT_AW_P010_UV:
        case HAL_PIXEL_FORMAT_AW_P010_VU:
            return 24;
        default:
            DLOGE("unknow format: 0x%08x", format);
            return 8;
    }
}

const char* getHalPixelFormatString(int format)
{
    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return "RGBA8888";
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return "RGBX8888";
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return "BGRA8888";
        case HAL_PIXEL_FORMAT_BGRX_8888:
            return "BGRX8888";
        case HAL_PIXEL_FORMAT_RGBA_1010102:
            return "RGBA_1010102";
        case HAL_PIXEL_FORMAT_RGB_888:
            return "RGB888";
        case HAL_PIXEL_FORMAT_RGB_565:
            return "RGB565";
        case HAL_PIXEL_FORMAT_YV12:
            return "YV12";
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            return "YCrCb420SP";
        case HAL_PIXEL_FORMAT_AW_NV12:
            return "AW_NV12";
        case HAL_PIXEL_FORMAT_AW_YV12_10bit:
            return "AW_YV12_10BIT";
        case HAL_PIXEL_FORMAT_AW_I420_10bit:
            return "AW_I420_10BIT";
        case HAL_PIXEL_FORMAT_AW_NV21_10bit:
            return "AW_NV21_10BIT";
        case HAL_PIXEL_FORMAT_AW_NV12_10bit:
            return "AW_NV12_10BIT";
        case HAL_PIXEL_FORMAT_AW_P010_UV:
            return "AW_P010_UV";
        case HAL_PIXEL_FORMAT_AW_P010_VU:
            return "AW_P010_VU";
        default:
            return "unknow";
    }
}

int dumpBuffer(buffer_handle_t buf, const char* path)
{
    const private_handle_t* handle = sunxi::from(buf);
    if (handle == nullptr) {
        DLOGE("request dump null buffer");
        return -EINVAL;
    }

    int fd = open(path, O_RDWR|O_CREAT, 0644);
    if (fd < 0) {
        DLOGE("open failed: %s", path);
        return -1;
    }

    int size = handle->stride * handle->height * bitsPerPixel(handle->format) / 8;
    void* vaddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, handle->share_fd, 0);
    if (vaddr == MAP_FAILED) {
        close(fd);
        DLOGE("mmap failed: %s", strerror(errno));
        return -1;
    }

    int ret = write(fd, vaddr, size);
    if(ret != size) {
        DLOGE("dumpBuffer error: buffer size %d writed size %d", size, ret);
    }
    munmap(vaddr, size);
    close(fd);
    return 0;
}

int dumpBuffer(int buf, int w, int h, int bpp, const char* path)
{
    int fd = open(path, O_RDWR|O_CREAT, 0644);
    if (fd < 0) {
        DLOGE("open failed: %s", path);
        return -1;
    }

    int size = w * h * bpp / 8;
    void* vaddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, buf, 0);
    if (vaddr == MAP_FAILED) {
        close(fd);
        DLOGE("mmap failed: %s", strerror(errno));
        return -1;
    }

    int ret = write(fd, vaddr, size);
    if(ret != size) {
        DLOGE("dumpBuffer error: buffer size %d writed size %d", size, ret);
    }
    munmap(vaddr, size);
    close(fd);
    return 0;
}

