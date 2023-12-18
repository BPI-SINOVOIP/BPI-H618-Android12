
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
#include "gralloc1.h"

namespace android {

#if defined(__T8__)
#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                                 GRALLOC_USAGE_HW_RENDER | \
                                 GRALLOC_USAGE_SW_READ_RARELY | \
                                 GRALLOC_USAGE_SW_READ_OFTEN | \
                                 GRALLOC_USAGE_SW_WRITE_OFTEN
#elif defined(__H616__)
#define CAMHAL_GRALLOC_USAGE GRALLOC1_PRODUCER_USAGE_CAMERA | \
                                 GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER | \
                                 GRALLOC_USAGE_SW_READ_RARELY | \
                                 GRALLOC_USAGE_SW_WRITE_OFTEN
#else
#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                                 GRALLOC_USAGE_HW_RENDER | \
                                 GRALLOC_USAGE_SW_READ_RARELY | \
                                 GRALLOC_USAGE_SW_WRITE_NEVER

#endif

//#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_FB



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
    for (int i = 0; i < NB_BUFFER; i ++) {
        mBufferFlag[i] = BUFFER_NOT_OWNED;
        mBufferHandle[i] = NULL;
        mPrivateHandle[i] = NULL;
        ddr_vir[i] = NULL;
        min_undequeued_num = 0;
        enqueue_buffer_num = 0;
    }
}

PreviewWindow::~PreviewWindow()
{
    F_LOG;
    mPreviewWindow = NULL;
}

/****************************************************************************
 * Camera API
 ***************************************************************************/
int PreviewWindow::allocate(int cnt,int width, int height, uint32_t pixformat)
{
    LOGV("%s:cnt:%d, width:%d, height:%d",__FUNCTION__,cnt,width,height);
    enqueue_buffer_num = 0;
    int err = 0;
    int ret;
    struct ion_handle_data ion_handle;
    struct ion_fd_data ion_info_fd;
    memset(&ion_info_fd, 0, sizeof(ion_info_fd));
    int format = V4L2_PIX_FMT_NV21;

    if (!mPreviewWindow) {
        LOGE("Invalid native window");
        return -1;
    }

    switch (pixformat) {
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
        default:
            LOGE("preview unknown pixel format: %08x", pixformat);
            return -1;
    }

    ret = mPreviewWindow->set_usage(mPreviewWindow, CAMHAL_GRALLOC_USAGE);
    if (ret != NO_ERROR) {
        LOGE("%s: Error in set_usage %d -> %s",
                __FUNCTION__, -ret, strerror(-ret));
        return ret;
    }
    mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &min_undequeued_num);
    ret = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                                                   width,
                                                   height,
                                                   format);
    if (ret != NO_ERROR) {
        LOGE("%s: Error in set_buffers_geometry %d -> %s",
                __FUNCTION__, -ret, strerror(-ret));
        return ret;
    }
    ret = mPreviewWindow->set_crop(mPreviewWindow,0,0,width,height);
    if (cnt == 1) {
       ret = mPreviewWindow->set_buffer_count(mPreviewWindow, cnt + 2);
    } else {
       ret = mPreviewWindow->set_buffer_count(mPreviewWindow, cnt);
    }
    if (ret != NO_ERROR) {
        LOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-ret), -ret);
        if ( ENODEV == ret ) {
            LOGE("Preview surface abandoned!");
            mPreviewWindow = NULL;
        }
        return ret;
    }

    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());
    /* Allocate cnt number of buffers from native window */
    for(int i = 0; i < cnt; i++){
        int stride = 0;
        ret = mPreviewWindow->dequeue_buffer(mPreviewWindow, &mBufferHandle[i], &stride);
        if (ret != NO_ERROR) {
            LOGE("dequeue preview idx = %d err = %d",i,ret);
            return ret;
        }
        LOGV("dequeue buf hdl%d =%p", i, mBufferHandle[i]);
        mPrivateHandle[i] = (private_handle_t *)(*mBufferHandle[i]);

        /* Let the preview window to lock the buffer. */
        ret = mPreviewWindow->lock_buffer(mPreviewWindow, mBufferHandle[i]);
        if (ret != NO_ERROR) {
            LOGE("%s: Unable to lock preview window buffer: %d -> %s",
                 __FUNCTION__, -ret, strerror(-ret));
            mPreviewWindow->cancel_buffer(mPreviewWindow, mBufferHandle[i]);
            return ret;
        }

        /* let the graphics framework to lock the buffer, and get the virtual address. */

        const Rect rect(width, height);
        //GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());
        ret = grbuffer_mapper.lock(*mBufferHandle[i], CAMHAL_GRALLOC_USAGE, rect, &ddr_vir[i]);
        if (ret != NO_ERROR) {
            LOGE("%s: grbuffer_mapper.lock failure: %d -> %s",
                __FUNCTION__, ret, strerror(ret));
            mPreviewWindow->cancel_buffer(mPreviewWindow, mBufferHandle[i]);
            return ret;
        }

        //ddr_vir[i] = mPrivateHandle[i]->base;
        mBufferFlag[i] = BUFFER_OWNED;

        LOGV("index:%d,width:%d,height:%d,pri_p:%p,fd:%d,vir:%p,aw_buf_id:%lld",
                i,
                mPrivateHandle[i]->width,
                mPrivateHandle[i]->height,
                mPrivateHandle[i],
                mPrivateHandle[i]->share_fd,
                ddr_vir[i],
                mPrivateHandle[i]->aw_buf_id);

#if 0
        mMemInfo[i].main_ion_fd = open("/dev/ion", O_RDONLY);
        ion_info_fd.fd = mPrivateHandle[i]->share_fd;
        if (ioctl(mMemInfo[i].main_ion_fd,ION_IOC_IMPORT, &ion_info_fd) < 0) {
                LOGE("ION import failed\n");
                close(mMemInfo[i].main_ion_fd);
        }else{
            mMemInfo[i].fd = mPrivateHandle[i]->share_fd;
            //mMemInfo[i].size = mPrivateHandle[i]->size;
            mMemInfo[i].handle = ion_info_fd.handle;
            LOGD("mMemInfo[%d]:main_ion_fd:%d,share_fd:%d,size:%d,ion_info_fd.handle:%d",mMemInfo[i].main_ion_fd,i,mMemInfo[i].fd,mMemInfo[i].size,mMemInfo[i].handle);
        }
#endif
    }

    return NO_ERROR;
}

void PreviewWindow::deallocate(int cnt)
{
    for (int i = 0; i < cnt; i++) {
#if 0
        struct ion_handle_data ion_handle;
        memset(&ion_handle, 0, sizeof(ion_handle));
        ion_handle.handle = mMemInfo[i].handle;
        if (ioctl(mMemInfo[i].main_ion_fd, ION_IOC_FREE, &ion_handle) < 0) {
            LOGE("ion free failed");
        }
        close(mMemInfo[i].main_ion_fd);
#endif
        if(mBufferFlag[i] != BUFFER_NOT_OWNED) {
            int err = NO_ERROR;
            err = unlockBuffer(i);
            if(err != NO_ERROR)
            {
                LOGE("unlockBuffer failed, err = %d \n", err);
            }
            if (mPreviewWindow) {
                mPreviewWindow->cancel_buffer(mPreviewWindow, mBufferHandle[i]);
                LOGV("cancel_buffer: hdl =%p", (*mBufferHandle[i]));
            } else {
                LOGE("Preview window is NULL, cannot cancel_buffer: hdl =%p",
                      (*mBufferHandle[i]));
            }
        }
        mBufferFlag[i] = BUFFER_NOT_OWNED;
        LOGV("put buffer:%d successfully", i);
    }
}

int PreviewWindow::lockBuffer(int index)
{
    int ret = -1;
    void * virFromGralloc;
    /* let the graphics framework to lock the buffer, and get the virtual address. */
    const Rect rect(mPrivateHandle[index]->width, mPrivateHandle[index]->height);
    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());

    ret = grbuffer_mapper.lock(*mBufferHandle[index], CAMHAL_GRALLOC_USAGE, rect, &virFromGralloc);
    LOGV("%s: virFromGralloc = %p ",__FUNCTION__, virFromGralloc);

        if ((ret != NO_ERROR)||(ddr_vir[index] != virFromGralloc)) {
            LOGE("%s: grbuffer_mapper.lock failure: %d -> %s",
                __FUNCTION__, ret, strerror(ret));
            //Maybe the buffer queue has been abandoned,we should not cancel buffer.
            //mPreviewWindow->cancel_buffer(mPreviewWindow, mBufferHandle[index]);
            return false;
        }

    return NO_ERROR;
}

int PreviewWindow::unlockBuffer(int index)
{
    int ret = -1;
    /* let the graphics framework to lock the buffer, and get the virtual address. */
    const Rect rect(mPrivateHandle[index]->width, mPrivateHandle[index]->height);
    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());

    ret = grbuffer_mapper.unlock(*mBufferHandle[index]/*    *buffer  */);

    if ((ret != NO_ERROR)) {
        LOGE("%s: grbuffer_mapper.unlock failure: %d -> %s",
        __FUNCTION__, ret, strerror(ret));
        mPreviewWindow->cancel_buffer(mPreviewWindow, mBufferHandle[index]);
        return false;
    }

    return NO_ERROR;
}

status_t PreviewWindow::setPreviewWindow(struct preview_stream_ops* window)
{
    LOGD("%s: current: %p -> new: %p", __FUNCTION__, mPreviewWindow, window);

    status_t res = NO_ERROR;
    Mutex::Autolock locker(&mObjectLock);

    /* Reset preview info. */
    mPreviewFrameWidth = mPreviewFrameHeight = 0;

    //if (window != NULL && !take_picture_flag/*&& mPreviewWindow != NULL && mPreviewWindow != window*/) {
        //LOGD("%s:changed ! Current: %p -> new: %p", __FUNCTION__, mPreviewWindow, window);
        /* The CPU will write each frame to the preview window buffer.
         * Note that we delay setting preview window buffer geometry until
         * frames start to come in. */
        //res = window->set_usage(window, /*GRALLOC_USAGE_SW_WRITE_OFTEN*/CAMHAL_GRALLOC_USAGE);
        //if (res != NO_ERROR) {
            //window = NULL;
            //res = -res; // set_usage returns a negative errno.
            //LOGE("%s: Error setting preview window usage %d -> %s",
                 //__FUNCTION__, res, strerror(res));
        //}
    //}
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

/*===========================================================================
 * FUNCTION   : onNextFrameAvailable
 *
 * DESCRIPTION: send received frame to display
 *
 * PARAMETERS :
 *   @index   : index of preview frame
 *
 * RETURN     : int dequeuedIdx
 *              dequeue_buffer index
 *              if can not dequeue buffer,return -1
 */
int PreviewWindow::onNextFrameAvailable2(int index, RECT_t preview_crop)
{
    F_LOG;
    Mutex::Autolock locker(&mObjectLock);
    int err = NO_ERROR;
    int dequeuedIdx = -1;

    mPreviewWindow->set_crop(mPreviewWindow,
                            preview_crop.left,
                            preview_crop.top,
                            preview_crop.left + preview_crop.width,
                            preview_crop.top + preview_crop.height);
    err = unlockBuffer(index);
    if(err != NO_ERROR)
    {
        LOGE("lockBuffer failed, err = %d \n", err);
    }

    err = mPreviewWindow->enqueue_buffer(mPreviewWindow, (buffer_handle_t *)mBufferHandle[index]);
    LOGV("now enqueue num:%d,min unenqueue num:%d",enqueue_buffer_num,min_undequeued_num);
    if(err != 0) {
        LOGE("enqueue_buffer failed, err = %d", err);
    } else {
        LOGV("enqueue_buffer hdl=%p", *mBufferHandle[index]);
        mBufferFlag[index] = BUFFER_NOT_OWNED;
        enqueue_buffer_num ++;
    }

    buffer_handle_t *buffer_handle = NULL;
    int stride = 0;
    if(enqueue_buffer_num > min_undequeued_num){
        err = mPreviewWindow->dequeue_buffer(mPreviewWindow, &buffer_handle, &stride);
        if (err == NO_ERROR && buffer_handle != NULL) {
            enqueue_buffer_num --;
            int i;
            LOGV("dequed buf hdl =%p", *buffer_handle);
            for(i = 0; i < NB_BUFFER; i++) {
                if(mBufferHandle[i] == buffer_handle) {
                    LOGV("Found buffer in idx:%d", i);
                    mBufferFlag[i] = BUFFER_OWNED;
                    dequeuedIdx = i;
                    break;
                }
            }
        }
        err = lockBuffer(dequeuedIdx);
        if(err != NO_ERROR)
        {
            LOGE("Oh,lockBuffer failed, err = %d,maybe buffer queue has been abandon.But we do not care.Right?\n", err);
            dequeuedIdx = -1;
        }
    }

    return dequeuedIdx;
}

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

        res = mPreviewWindow->set_buffer_count(mPreviewWindow, 3);
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
