
#include "CameraDebug.h"
#if DBG_PREVIEW
#define LOG_NDEBUG 0
#endif
#define LOG_TAG "PreviewWindow"
#include <cutils/log.h>

#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>

#include "V4L2CameraDevice2.h"
#include "PreviewWindow.h"
//#include <system/window.h>

namespace android {

/*#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                                 GRALLOC_USAGE_HW_RENDER | \
                                 GRALLOC_USAGE_SW_READ_RARELY | \
                                 GRALLOC_USAGE_SW_WRITE_NEVER
*/
#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_SW_READ_RARELY | \
                                GRALLOC_USAGE_SW_WRITE_NEVER


static int calculateFrameSize(int width, int height, uint32_t pix_fmt)
{
    int frame_size = 0;
    switch (pix_fmt) {
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
            frame_size = (ALIGN_16B(width) * height * 12) / 8;
            break;
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUYV:
            frame_size = (width * height) << 1;
            break;
        default:
            ALOGE("%s: Unknown pixel format %d(%.4s)",
                __FUNCTION__, pix_fmt, reinterpret_cast<const char*>(&pix_fmt));
            break;
    }
    return frame_size;
}


DBG_TIME_AVG_BEGIN(TAG_CPY);
DBG_TIME_AVG_BEGIN(TAG_DQBUF);
DBG_TIME_AVG_BEGIN(TAG_LKBUF);
DBG_TIME_AVG_BEGIN(TAG_MAPPER);
DBG_TIME_AVG_BEGIN(TAG_EQBUF);
DBG_TIME_AVG_BEGIN(TAG_UNLKBUF);

PreviewWindow::PreviewWindow()
    : mPreviewWindow(NULL),
      mPreviewFrameWidth(0),
      mPreviewFrameHeight(0),
      mPreviewFrameSize(0),
      mCurPixelFormat(0),
      mPreviewEnabled(false),
      mShouldAdjustDimensions(true)
{
    F_LOG;
}

PreviewWindow::~PreviewWindow()
{
    F_LOG;
    mPreviewWindow = NULL;
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

status_t PreviewWindow::setPreviewWindow(struct preview_stream_ops* window)
{
    LOGV("%s: current: %p -> new: %p", __FUNCTION__, mPreviewWindow, window);

    status_t res = NO_ERROR;
    Mutex::Autolock locker(&mObjectLock);

    //sp<ANativeWindow> windows;
    //int result = native_window_api_connect(windows.get(), NATIVE_WINDOW_API_CAMERA);
    /* Reset preview info. */
    mPreviewFrameWidth = mPreviewFrameHeight = 0;

    if (window != NULL) {
        /* The CPU will write each frame to the preview window buffer.
         * Note that we delay setting preview window buffer geometry until
         * frames start to come in. */
        res = window->set_usage(window, /*GRALLOC_USAGE_SW_WRITE_OFTEN*/CAMHAL_GRALLOC_USAGE);
        if (res != NO_ERROR) {
            window = NULL;
            res = -res; // set_usage returns a negative errno.
            LOGE("%s: Error setting preview window usage %d -> %s",
                 __FUNCTION__, res, strerror(res));
        }
    }
    mPreviewWindow = window;

    return res;
}

status_t PreviewWindow::startPreview()
{
    F_LOG;

    Mutex::Autolock locker(&mObjectLock);
    mPreviewEnabled = true;

    DBG_TIME_AVG_INIT(TAG_CPY);
    DBG_TIME_AVG_INIT(TAG_DQBUF);
    DBG_TIME_AVG_INIT(TAG_LKBUF);
    DBG_TIME_AVG_INIT(TAG_MAPPER);
    DBG_TIME_AVG_INIT(TAG_EQBUF);
    DBG_TIME_AVG_INIT(TAG_UNLKBUF);
    return NO_ERROR;
}

void PreviewWindow::stopPreview()
{
    F_LOG;

    Mutex::Autolock locker(&mObjectLock);
    mPreviewEnabled = false;
    mShouldAdjustDimensions = true;

    DBG_TIME_AVG_END(TAG_CPY, "copy ");
    DBG_TIME_AVG_END(TAG_DQBUF, "deque ");
    DBG_TIME_AVG_END(TAG_LKBUF, "lock ");
    DBG_TIME_AVG_END(TAG_MAPPER, "mapper ");
    DBG_TIME_AVG_END(TAG_EQBUF, "enque ");
    DBG_TIME_AVG_END(TAG_UNLKBUF, "unlock ");
}

/****************************************************************************
 * Public API
 ***************************************************************************/

bool PreviewWindow::onNextFrameAvailable(const void* frame)
{
    int res;
    Mutex::Autolock locker(&mObjectLock);

    V4L2BUF_t * pv4l2_buf = (V4L2BUF_t *)frame;

    int preview_format = 0;
    unsigned long preview_addr_phy = NULL;
    unsigned long preview_addr_vir = NULL;
    int preview_width = 0;
    int preview_height = 0;
    RECT_t preview_crop;

    if (!isPreviewEnabled() || mPreviewWindow == NULL)
    {
        return true;
    }

    if ((pv4l2_buf->isThumbAvailable == 1)
        && (pv4l2_buf->thumbUsedForPreview == 1))
    {
        preview_format = pv4l2_buf->thumbFormat;
        preview_addr_phy = pv4l2_buf->thumbAddrPhyY;
        preview_addr_vir = pv4l2_buf->thumbAddrVirY;
        preview_width = pv4l2_buf->thumbWidth;
        preview_height= pv4l2_buf->thumbHeight;
        memcpy((void*)&preview_crop, (void*)&pv4l2_buf->thumb_crop_rect, sizeof(RECT_t));
    }
    else
    {
        preview_format = pv4l2_buf->format;
        preview_addr_phy = pv4l2_buf->addrPhyY;
        preview_addr_vir = pv4l2_buf->addrVirY;
        preview_width = pv4l2_buf->width;
        preview_height= pv4l2_buf->height;
        memcpy((void*)&preview_crop, (void*)&pv4l2_buf->crop_rect, sizeof(RECT_t));
    }

    /* Make sure that preview window dimensions are OK with the camera device */
    if (adjustPreviewDimensions(pv4l2_buf) || mShouldAdjustDimensions) {
        LOGD("%s: Adjusting preview windows %p geometry to %dx%d",
             __FUNCTION__, mPreviewWindow, mPreviewFrameWidth,
             mPreviewFrameHeight);
        int bufCnt = 3;
        int format = V4L2_PIX_FMT_NV21;
        switch (preview_format)
        {
            case V4L2_PIX_FMT_NV21:
                LOGV("preview format: HAL_PIXEL_FORMAT_YCrCb_420_SP");
                format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                break;
            case V4L2_PIX_FMT_NV12:
                    LOGV("preview format: V4L2_PIX_FMT_NV12");
                    format = 0x101;            // NV12
                    break;
            case V4L2_PIX_FMT_YVU420:
            case V4L2_PIX_FMT_YUV420:    // to do
                LOGV("preview format: HAL_PIXEL_FORMAT_YV12");
                format = HAL_PIXEL_FORMAT_YV12;
                break;
            case V4L2_PIX_FMT_YUYV:
                LOGV("preview format: HAL_PIXEL_FORMAT_YCbCr_422_I");
                format = HAL_PIXEL_FORMAT_YCbCr_422_I;
                break;
#if 0
            case V4L2_PIX_FMT_NV16:
                LOGV("preview format: HAL_PIXEL_FORMAT_YCbCr_422_SP");
                format = HAL_PIXEL_FORMAT_YCbCr_422_SP;
                bufCnt = 4;
                break;
            case V4L2_PIX_FMT_NV61:
                LOGV("preview format: HAL_PIXEL_FORMAT_YCrCb_422_SP");
                format = HAL_PIXEL_FORMAT_YCrCb_422_SP;
                break;
#endif
            default:
                LOGE("preview unknown pixel format: %08x", preview_format);
                return false;
        }

        res = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                                                   mPreviewFrameWidth,
                                                   mPreviewFrameHeight,
                                                   format);
        if (res != NO_ERROR) {
            LOGE("%s: Error in set_buffers_geometry %d -> %s",
                 __FUNCTION__, -res, strerror(-res));
            return false;
        }
        mShouldAdjustDimensions = false;

        res = mPreviewWindow->set_buffer_count(mPreviewWindow, bufCnt);
        if (res != 0)
        {
            LOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-res), -res);

            if ( ENODEV == res ) {
                LOGE("Preview surface abandoned!");
                mPreviewWindow = NULL;
            }

            return false;
        }
    }

    /*
     * Push new frame to the preview window.
     */

    DBG_TIME_AVG_AREA_IN(TAG_DQBUF);

    /* Dequeue preview window buffer for the frame. */
    buffer_handle_t* buffer = NULL;
    int stride = 0;
    res = mPreviewWindow->dequeue_buffer(mPreviewWindow, &buffer, &stride);
    if (res != NO_ERROR || buffer == NULL) {
        LOGE("%s: Unable to dequeue preview window buffer: %d -> %s",
            __FUNCTION__, -res, strerror(-res));

        int undequeued = 0;
        mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &undequeued);
        LOGW("now undequeued: %d", undequeued);

        mPreviewWindow = NULL;

        return false;
    }
    DBG_TIME_AVG_AREA_OUT(TAG_DQBUF);

    DBG_TIME_AVG_AREA_IN(TAG_LKBUF);

    /* Let the preview window to lock the buffer. */
    res = mPreviewWindow->lock_buffer(mPreviewWindow, buffer);
    if (res != NO_ERROR) {
        LOGE("%s: Unable to lock preview window buffer: %d -> %s",
             __FUNCTION__, -res, strerror(-res));
        mPreviewWindow->cancel_buffer(mPreviewWindow, buffer);
        return false;
    }
    DBG_TIME_AVG_AREA_OUT(TAG_LKBUF);

    DBG_TIME_AVG_AREA_IN(TAG_MAPPER);

    /* Now let the graphics framework to lock the buffer, and provide
     * us with the framebuffer data address. */
    void* img = NULL;
    const Rect rect(mPreviewFrameWidth, mPreviewFrameHeight);
    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());
    res = grbuffer_mapper.lock(*buffer, GRALLOC_USAGE_SW_WRITE_OFTEN, rect, &img);
    if (res != NO_ERROR) {
        LOGE("%s: grbuffer_mapper.lock failure: %d -> %s",
             __FUNCTION__, res, strerror(res));
        mPreviewWindow->cancel_buffer(mPreviewWindow, buffer);
        return false;
    }

    DBG_TIME_AVG_AREA_OUT(TAG_MAPPER);

    mPreviewWindow->set_crop(mPreviewWindow,
                            preview_crop.left,
                            preview_crop.top,
                            preview_crop.left + preview_crop.width,
                            preview_crop.top + preview_crop.height);

    DBG_TIME_AVG_AREA_IN(TAG_CPY);


    //camera_phy_flush_cache((void*)preview_addr_vir, mPreviewFrameSize);

    if (preview_format == V4L2_PIX_FMT_NV21
        || preview_format == V4L2_PIX_FMT_NV12)
    {
        char * src = (char *)preview_addr_vir;
        char * dst = (char *)img;

        if (stride == ALIGN_16B(preview_width))
        {
            // y
            memcpy(dst, src, ALIGN_16B(preview_width) * preview_height);
            // uv
            src = (char *)preview_addr_vir + ALIGN_16B(preview_width) * preview_height;
            dst = (char *)img + GPU_BUFFER_ALIGN(ALIGN_16B(preview_width)*preview_height);
            memcpy(dst, src, ALIGN_16B(preview_width) * preview_height >> 1);
            //camera_phy_flush_cache((void*)dst, GPU_BUFFER_ALIGN(ALIGN_16B(preview_width)*preview_height) + \
            //                                    ALIGN_16B(preview_width) * preview_height >> 1);
        }
        else if (stride == ALIGN_32B(preview_width))
        {
            // y
            for (int h = 0; h < preview_height; h++)
            {
                memcpy(dst, src, preview_width);
                src += ALIGN_16B(preview_width);
                dst += ALIGN_32B(preview_width);
            }
            // uv
            dst = (char *)img + GPU_BUFFER_ALIGN(ALIGN_32B(preview_width)*preview_height);
            for (int h = 0; h < preview_height/2; h++)
            {
                memcpy(dst, src, preview_width);
                src += ALIGN_16B(preview_width);
                dst += ALIGN_32B(preview_width);
            }
        }
    } else if (preview_format == V4L2_PIX_FMT_NV61
        || preview_format == V4L2_PIX_FMT_NV16) {
        char * src = (char *)preview_addr_vir;
        char * dst = (char *)img;

        if (stride == ALIGN_16B(preview_width)) {
            // y
            memcpy(dst, src, ALIGN_16B(preview_width) * preview_height);
            // uv
            src = (char *)preview_addr_vir + ALIGN_16B(preview_width) * preview_height;
            dst = (char *)img + GPU_BUFFER_ALIGN(ALIGN_16B(preview_width) * preview_height);
            memcpy(dst, src, ALIGN_16B(preview_width) * preview_height);
        } else {
            ALOGE("stride is not 16align, F:%s, L:%d", __FUNCTION__, __LINE__);
        }
    }

    else if(preview_format == V4L2_PIX_FMT_YUV420)
    {
        // YU12 to YV12
        char * src = (char *)preview_addr_vir;
        char * dst = (char *)img;
        if (stride == ALIGN_16B(preview_width))
        {
            // y
            memcpy(dst, src, ALIGN_16B(preview_width)*preview_height);

            // v
            src = (char *)preview_addr_vir
                    + ALIGN_16B(preview_width) * preview_height
                    + ALIGN_16B(ALIGN_16B(preview_width)/2)*preview_height/2;
            dst = (char *)img + ALIGN_16B(preview_width)*preview_height;
            for (int h = 0; h < preview_height/2; h++)
            {
                memcpy(dst, src, preview_width/2);
                src += ALIGN_16B(preview_width/2);
                dst += ALIGN_8B(preview_width/2);
            }

            // u
            src = (char *)preview_addr_vir + ALIGN_16B(preview_width) * preview_height;
            for (int h = 0; h < preview_height/2; h++)
            {
                memcpy(dst, src, preview_width/2);
                src += ALIGN_16B(preview_width/2);
                dst += ALIGN_8B(preview_width/2);
            }
        }
        else if (stride == ALIGN_32B(preview_width))
        {
            // y
        for (int h = 0; h < preview_height; h++)
        {
            memcpy(dst, src, preview_width);
            src += ALIGN_16B(preview_width);
            dst += ALIGN_32B(preview_width);
        }

        // v
            src = (char *)preview_addr_vir
                    + ALIGN_16B(preview_width)*preview_height
                    + ALIGN_16B(ALIGN_16B(preview_width)/2)*preview_height/2;
        dst = (char *)img + ALIGN_32B(preview_width)*preview_height;
        memcpy(dst, src, ALIGN_16B(preview_width/2)*preview_height >> 1);

        // u
        src = (char *)preview_addr_vir + ALIGN_16B(preview_width)*preview_height;
            dst = (char *)img
                    + ALIGN_32B(preview_width)*preview_height
                    + (ALIGN_16B(preview_width/2)*preview_height >> 1);
        memcpy(dst, src, ALIGN_16B(preview_width/2)*preview_height >> 1);
        }
    }
    else if(preview_format == V4L2_PIX_FMT_YVU420)
    {
        // YV12
        char * src = (char *)preview_addr_vir;
        char * dst = (char *)img;

        if (stride == ALIGN_32B(preview_width))
        {
            // y
        for (int h = 0; h < preview_height; h++)
        {
            memcpy(dst, src, preview_width);
            src += ALIGN_16B(preview_width);
            dst += ALIGN_32B(preview_width);
        }

        // u & v
        memcpy(dst, src, ALIGN_16B(preview_width/2)*preview_height);
        }
        else if (stride == ALIGN_16B(preview_width))
        {
            // y
            memcpy(dst, src, ALIGN_16B(preview_width)*preview_height);

            src = (char *)preview_addr_vir + ALIGN_16B(preview_width) * preview_height;
            dst = (char *)img + ALIGN_16B(preview_width)*preview_height;

            // u
            for (int h = 0; h < preview_height/2; h++)
            {
                memcpy(dst, src, preview_width/2);
                src += ALIGN_16B(preview_width/2);
                dst += ALIGN_8B(preview_width/2);
            }

            // v
            for (int h = 0; h < preview_height/2; h++)
            {
                memcpy(dst, src, preview_width/2);
                src += ALIGN_16B(preview_width/2);
                dst += ALIGN_8B(preview_width/2);
            }
        }
    }
    else
    {
        LOGE("unknown preview format");
    }

    DBG_TIME_AVG_AREA_OUT(TAG_CPY);

    DBG_TIME_AVG_AREA_IN(TAG_EQBUF);

    //mPreviewWindow->set_timestamp(mPreviewWindow, pv4l2_buf->timeStamp);
    mPreviewWindow->enqueue_buffer(mPreviewWindow, buffer);

    DBG_TIME_AVG_AREA_OUT(TAG_EQBUF);

    DBG_TIME_AVG_AREA_IN(TAG_UNLKBUF);

    grbuffer_mapper.unlock(*buffer);

    DBG_TIME_AVG_AREA_OUT(TAG_UNLKBUF);

    return true;
}

/***************************************************************************
 * Private API
 **************************************************************************/

bool PreviewWindow::adjustPreviewDimensions(V4L2BUF_t* pbuf)
{
    /* Match the cached frame dimensions against the actual ones. */
    if ((pbuf->isThumbAvailable == 1)
        && (pbuf->thumbUsedForPreview == 1))        // use thumb frame for preview
    {
        if ((mPreviewFrameWidth == pbuf->thumbWidth)
            && (mPreviewFrameHeight == pbuf->thumbHeight)
            && (mCurPixelFormat == pbuf->thumbFormat))
        {
            /* They match. */
            return false;
        }

        LOGV("cru: [%d, %d], get: [%d, %d]", mPreviewFrameWidth, mPreviewFrameHeight,
            pbuf->thumbWidth, pbuf->thumbHeight);
        /* They don't match: adjust the cache. */
        mPreviewFrameWidth = pbuf->thumbWidth;
        mPreviewFrameHeight = pbuf->thumbHeight;
        mCurPixelFormat = pbuf->thumbFormat;

        mPreviewFrameSize = calculateFrameSize(pbuf->thumbWidth, pbuf->thumbHeight, pbuf->thumbFormat);
    }
    else
    {
        if ((mPreviewFrameWidth == pbuf->width)
            && (mPreviewFrameHeight == pbuf->height)
            && (mCurPixelFormat == pbuf->format))
        {
            /* They match. */
            return false;
        }

        LOGV("cru: [%d, %d], get: [%d, %d]", mPreviewFrameWidth, mPreviewFrameHeight,
            pbuf->width, pbuf->height);
        /* They don't match: adjust the cache. */
        mPreviewFrameWidth = pbuf->width;
        mPreviewFrameHeight = pbuf->height;
        mCurPixelFormat = pbuf->format;

        mPreviewFrameSize = calculateFrameSize(pbuf->width, pbuf->height, pbuf->format);
    }

    mShouldAdjustDimensions = false;
    return true;
}

}; /* namespace android */
