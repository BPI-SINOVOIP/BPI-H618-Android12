
#include <queue>
#include <thread>

#include <sys/resource.h>
#include <sync/sync.h>
#include <android-base/stringprintf.h>
#include <processgroup/sched_policy.h>

#include "Debug.h"
#include "Compositor.h"
#include "CompositionEngine.h"
#include "errorcode.h"
#include "disputils.h"
#include "helper.h"
#include "private_handle.h"
#include "RotatorManager.h"
#include "HardwareRotator.h"
#include "syncfence.h"
#include "IHWCPrivateService.h"
#include "hdr10pInterface.h"
#include "utils.h"

using android::base::StringPrintf;

namespace sunxi {

#define MAX_LAYER_COUNT 16
// default fence timeout in ms
#define DEFAULT_FENCE_TIMEOUT 3000

struct CommitThread::Frame {
    const int layerCount;
    disp_layer_config2 layerConfigs[MAX_LAYER_COUNT];
    // deferBufferFds refer to the graphic buffers pending to display,
    // We should close those buffer fds only after we had finish reading from it.
    std::vector<uniquefd> deferBufferFds;
    // all layers's acquire fence will merge into this acquireFence,
    // wait it to be signal before committing to driver.
    uniquefd acquireFence;
    // fence will signal when this frame had finish present to display
    uniquefd releaseFence;

    // sync frame number for dev_composer
    unsigned int frameNumber;

    explicit Frame(int count)
        : layerCount(count), deferBufferFds(),
        acquireFence(-1), releaseFence(-1), frameNumber(0) {
            memset(layerConfigs, 0, sizeof(disp_layer_config2) * MAX_LAYER_COUNT);
    }
};

using Frame = CommitThread::Frame;

class LayerDetector: public PlanConstraint::IHWLayerDetector {
public:
    LayerDetector(const ScreenTransform& transform,
            CompositionEngine* engine, RotatorManager* rotator)
        : mTransform(transform),
          mEngine(engine),
          mRotator(rotator) { }

    int capable(const std::shared_ptr<Layer>& layer) override {
        int error = mEngine->capable(mTransform, layer);
        if (!error && layer->transform() != HWC2::Transform::None) {
            if (mRotator == nullptr
                    || (mRotator->tryUsingHardwareRotator(layer) != 0))
                return eTransformError;
        }
        return error;
    }

private:
    const ScreenTransform& mTransform;
    CompositionEngine* mEngine;
    RotatorManager* mRotator;
};

CommitThread::CommitThread(Compositor& c)
  : mCompositor(c),
    mThread(),
    mRunning(false),
    mPendingFrames()
{ }

CommitThread::~CommitThread() = default;

void CommitThread::start(const char * name)
{
    mRunning.store(true);
    mThread = std::thread(&CommitThread::threadLoop, this);
    pthread_setname_np(mThread.native_handle(), name);
    mName = std::string(name);
}

void CommitThread::stop()
{
    mRunning.store(false);
    mCondition.notify_all();
    mThread.join();
    DLOGI("thread: %s terminating", mName.c_str());
}

std::unique_ptr<Frame> CommitThread::createFrame(int layerCount)
{
    std::unique_ptr<Frame> frame = std::make_unique<Frame>(layerCount);
    return frame;
}

void CommitThread::queueFrame(std::unique_ptr<Frame> newframe)
{
    DTRACE_FUNC();
    std::lock_guard<std::mutex> l(mLock);
    if (mCompositor.mSkipFrameEnabled) {
        int pendingFrameCount = mPendingFrames.size();
        if (pendingFrameCount > 0) {
            char tag[32] = {0};
            sprintf(tag, "skipFrame.%d", pendingFrameCount);
            DTRACE_SCOPED(tag);
            std::queue<std::unique_ptr<Frame>> empty;
            std::swap(mPendingFrames, empty);
        }
    }

    if (mPendingFrames.size() >= mMaxQueuedFrame) {
        DLOGE("too many queued frames, skip to the newest frame!");
        std::queue<std::unique_ptr<Frame>> empty;
        std::swap(mPendingFrames, empty);
    }

    mPendingFrames.push(std::move(newframe));
    mCondition.notify_all();
    DTRACE_INT(mCompositor.mTraceNameSuffix.c_str(), mPendingFrames.size());
}

void CommitThread::commitFrame(Frame* frame)
{
    DTRACE_FUNC();
    {
        DTRACE_SCOPED("wait-acquire-fence");
        mPerf.addSample(kSampleTagWaitAcquireFence);
        sync_wait(frame->acquireFence, DEFAULT_FENCE_TIMEOUT);
    }
    mPerf.addSample(kSampleTagHardwareSubmit);
    mCompositor.mDisplayEngine->submitLayer(
            frame->frameNumber, frame->layerConfigs, frame->layerCount);

    mCompositor.mFrameRateAuditor.onFramePresent();
}

void CommitThread::threadLoop()
{
    DLOGI("set CommitThread priority as HAL_PRIORITY_URGENT_DISPLAY");
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
    set_sched_policy(0, SP_FOREGROUND);
    struct sched_param param = {0};
    param.sched_priority = 3;
    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        DLOGE("Couldn't set SCHED_FIFO");
    }

    while (true) {
        std::unique_ptr<Frame> frame;
        { // scope of mLock
            std::unique_lock<std::mutex> lock(mLock);
            if (mPendingFrames.empty()) {
                DTRACE_SCOPED("wait-frame");
                // CommitThread will not exit until we have finished commit all pending frame
                if (mRunning.load() == false)
                    break;

                FramerateAuditor& auditor = mCompositor.mFrameRateAuditor;
                std::chrono::milliseconds timeout(auditor.getFrameTimeoutDuration());
                if (mCondition.wait_for(lock, timeout) == std::cv_status::timeout)
                    auditor.onFrameTimeout();
            }

            if (mPendingFrames.empty())
                continue;

            frame = std::move(mPendingFrames.front());
            mPendingFrames.pop();
            DTRACE_INT(mCompositor.mTraceNameSuffix.c_str(), mPendingFrames.size());

            if (frame.get() == nullptr)
                continue;
        }

        mPerf.addSample(kSampleTagCommitBegin);
        commitFrame(frame.get());

        mPerf.addSample(kSampleTagWaitReleaseFence);
        if (mPrevFrame != nullptr) {
            if (mPrevFrame->releaseFence.get() >= 0) {
                DTRACE_SCOPED("wait-prev-fence");
                sync_wait(mPrevFrame->releaseFence.get(), DEFAULT_FENCE_TIMEOUT);
            }
        }
        mPrevFrame = std::move(frame);

        mPerf.addSample(kSampleTagCommitEnd);
        mPerf.checkPerformance();
    }
}

void CommitThread::dump(std::string& out) const
{
    std::unique_lock<std::mutex> lock(mLock);
    out += StringPrintf("pending frame: %zd\n", mPendingFrames.size());
    mPerf.dump(out);
}

Compositor::Compositor()
  : mDisplayEngine(),
    mOutput3D(false),
    mBlankOutput(true),
    mFreezeContent(false),
    mLayerDetector(nullptr),
    mSkipFrameEnabled(false),
    mColorTransform(HAL_COLOR_TRANSFORM_IDENTITY),
    mCommitIdle(false),
    mForceClientCompositionOnOutputFreezed(false),
    mRefreshCallback(nullptr)
{
    mLayerPlanner.setStrategy(eHardwareFirst);
    mFrameRateAuditor.setOutputFreezedHandler(this);
    mCommitThread = std::make_unique<CommitThread>(*this);
    mCommitThread->start("CommitThread");
    m3Dmode = DISPLAY_2D_ORIGINAL;
    mDataspace = IHWCPrivateService::eDataspaceSdr;
    mIonfd = open("/dev/ion", O_RDWR);
    if (mIonfd < 0) {
        DLOGE("Failed to open ion device");
    }
}

Compositor::~Compositor() {
    if (mCommitThread)
        mCommitThread->stop();
    if (mIonfd >= 0) {
        close(mIonfd);
        mIonfd = -1;
    }
}

void Compositor::setCompositionEngine(const std::shared_ptr<CompositionEngine>& engine)
{
    if (engine.get() == nullptr) {
        DLOGE("CompositionEngine is null");
        return;
    }
    mDisplayEngine = engine;
}

void Compositor::setRotatorManager(const std::shared_ptr<RotatorManager>& rotator)
{
    mRotator = rotator;
}

void Compositor::setScreenSize(int width, int height)
{
    mTransform.setScreenSize(width, height);
}

void Compositor::setFramebufferSize(int width, int height)
{
    mTransform.setFramebufferSize(width, height);
}

void Compositor::setContentMargin(int l, int r, int t, int b)
{
    // We only support margin-left and margin-right has the same percentage,
    // also the margin-top and margin-bottom.
    float x = (float)l / 100.f;
    float y = (float)t / 100.f;
    mTransform.setContentScale(x, y);
}

void Compositor::setDataSpace(int dataspace)
{
    //DLOGI("get mDataspace =%d", dataspace);
    mDataspace = dataspace;
}

void Compositor::setDisplaydCb(void* cb)
{
    //DLOGI("get mDisplaydCb =0x%p", cb);
    mDisplaydCb = cb;
}

void Compositor::set3DMode(int mode)
{
    //DLOGI("get m3Dmode =%d", mode);
    m3Dmode = (enum display_3d_mode)mode;
    if (m3Dmode == DISPLAY_3D_LEFT_RIGHT_HDMI
        || m3Dmode == DISPLAY_3D_TOP_BOTTOM_HDMI
        || m3Dmode == DISPLAY_3D_DUAL_STREAM)
        mOutput3D = true;
    else
        mOutput3D = false;
}

std::unique_ptr<PlanConstraint> Compositor::createPlanConstraint()
{
    DTRACE_FUNC();
    std::unique_ptr<PlanConstraint> c(new PlanConstraint);
    const CompositionEngine::Attribute& attr = mDisplayEngine->getAttribute();

    if (!mLayerDetector) {
        mLayerDetector = std::make_shared<LayerDetector>(mTransform,
                mDisplayEngine.get(), mRotator.get());
    }

    c->MaxBandwidth = attr.maxBandwidth;
    c->MaxHybridChannel = attr.maxHybridChannelCount;
    c->MaxRGBChannel = attr.maxRGBChannelCount;
    c->MaxHdrHybridChannel = attr.maxHdrHybridCount;
    c->MaxHdrRGBChannel = attr.maxHdrRGBCount;
    c->usingHdr = false;
    c->totalChannelCount = c->MaxHybridChannel + c->MaxRGBChannel;
    c->HybridChannelWithAlpha =
        (attr.featureMask & CompositionEngine::eHybridChannelWithAlpha) != 0;
    c->Output3D = mOutput3D;
    c->Detector = mLayerDetector;

    mTransform.update();
    c->ScreenTransform = &mTransform;

    assert(MAX_LAYER_COUNT >=
            (c->MaxHybridChannel + c->MaxRGBChannel) * SLOT_PER_CHANNEL);

    return c;
}

void Compositor::prepare(CompositionContext *ctx)
{
    DTRACE_FUNC();
    int dataspace = IHWCPrivateService::eDataspaceSdr;
    std::scoped_lock<std::mutex> lock(mLock);
    mCompositionLock.acquire();

    if (mAssignment != nullptr) {
        DLOGW("validate but not present, just drop last frame");
        mAssignment = nullptr;
    }

    if (mBlankOutput) {
        DLOGW("refresh after blank!");
        return;
    }

    std::unique_ptr<PlanConstraint> constraint = createPlanConstraint();
    if (constraint->MaxHdrHybridChannel || constraint->MaxHdrRGBChannel) {
        for (const std::shared_ptr<Layer>& layer : ctx->InputLayers) {
            if (layer->dataspace() != HAL_DATASPACE_UNKNOWN) {
                int transfer = layer->dataspace() & HAL_DATASPACE_TRANSFER_MASK;
                int standard = layer->dataspace() & HAL_DATASPACE_STANDARD_MASK;
                if (isYuvFormat(layer) && isHdrpBuffer(layer->bufferHandle())
                    && !constraint->Detector->capable(layer)) {
                    dataspace = IHWCPrivateService::eDataspaceHdrp;
                    break;
                } else if (transfer == HAL_DATASPACE_TRANSFER_ST2084
                    || transfer == HAL_DATASPACE_TRANSFER_HLG) {
                    dataspace = IHWCPrivateService::eDataspaceHdr;
                    break;
                } else if (standard == HAL_DATASPACE_STANDARD_BT2020) {
                    dataspace = IHWCPrivateService::eDataspaceWcg;
                    break;
                }
            }
        }
        /*DLOGW("ds=%d,mds=%x,mcb=%p,ds=%x", (dataspace == IHWCPrivateService::eDataspaceSdr),
              mDataspace, mDisplaydCb, dataspace);*/
        if (mDataspace != dataspace && mDisplaydCb != nullptr)
            ((IHWCPrivateService::EventCallback*)mDisplaydCb)->onDataspaceChanged(dataspace);

        if (mDataspace != IHWCPrivateService::eDataspaceSdr) {
            constraint->MaxHybridChannel = constraint->MaxHdrHybridChannel;
            constraint->MaxRGBChannel = constraint->MaxHdrRGBChannel;
            constraint->usingHdr = true;
        }
    }

    mAssignment = std::make_unique<PlanningOutput>();
    mLayerPlanner.setStrategy(pickCompositionStrategy());
    mLayerPlanner.advanceFrame(*constraint, ctx, mAssignment.get());

    // The PlanningOutput::Configs is sort by zorder,
    // We need to rearrange to match the hardware order.
    mDisplayEngine->rearrangeChannel(mAssignment->Configs);

    // post rotate task after validate finished
    if(mRotator != nullptr) {
        for (const std::shared_ptr<Layer>& layer : ctx->InputLayers) {
            if (layer->transform() != HWC2::Transform::None
                    && layer->compositionFromValidated() == HWC2::Composition::Device)
                mRotator->postRotateTask(layer->id());
        }
    }
}

void Compositor::dumpInputBuffer(CompositionContext* ctx)
{
    private_handle_t* handle;
    int zorder = 0;
    char path[128] = {0};
    auto dumpLayer = [](char *path, private_handle_t* handle, int acquireFence) {
            if (handle == nullptr) {
                return;
            }
            sync_wait(acquireFence, 1000);
            dumpBuffer(handle, path);
            DLOGD("dump input buffer: %s", path);
    };

    for (auto& layer : ctx->InputLayers) {
        handle = (private_handle_t *)layer->bufferHandle();
        if (handle != nullptr) {
            sprintf(path, "/data/dump_%s_%dx%d_z%d.dat",
                    getHalPixelFormatString(handle->format),
                    handle->stride,
                    handle->height,
                    zorder);
            dumpLayer(path, handle, layer->acquireFence());
        }
        zorder++;
    }

    handle = (private_handle_t *)ctx->FramebufferTarget->bufferHandle();
    if (handle == nullptr)
        return;

    sprintf(path, "/data/dump_%s_%dx%d_client_target.dat",
                getHalPixelFormatString(handle->format),
                handle->stride,
                handle->height);
    dumpLayer(path, handle, ctx->FramebufferTarget->acquireFence());

}

void Compositor::debugFreezeCheck(CompositionContext* ctx)
{
    unsigned int freezeCount;
    bool dump = false;
    { // scope of mLock
        std::scoped_lock<std::mutex> lock(mLock);
        freezeCount = mFreezeCount;
        if (!mFreezeContent)
            return;
    }

    while (true) {
        { // scope of mLock
            std::scoped_lock<std::mutex> lock(mLock);
            dump = !mFreezeCount;
            if (freezeCount != mFreezeCount || !mFreezeContent) {
                break;
            }
        }
        usleep(10*1000);
    }

    if (dump)
        dumpInputBuffer(ctx);
}

void Compositor::commit(CompositionContext* ctx)
{
    DTRACE_FUNC();
    {
        std::scoped_lock<std::mutex> lock(mLock);

        // mCompositionLock.release() will be called be when we exiting from this scope
        auto deferReleaseFunc = [](ScopeLock* s){ s->release(); };
        std::unique_ptr<ScopeLock, std::function<void(ScopeLock *)>> scopeLockRelease(
            &mCompositionLock, deferReleaseFunc);

        if (mAssignment == nullptr) {
            DLOGE("not validateDisplay before commit!");
            return;
        }

        int layerCount = mAssignment->Configs.size() * SLOT_PER_CHANNEL;
        mCurrentFrame = CommitThread::createFrame(layerCount);
        if (mCurrentFrame == nullptr) {
            DLOGE("alloc Frame error, out of memory");
            return;
        }

        syncinfo syncpt;
        if (mDisplayEngine->createSyncpt(&syncpt)) {
            DLOGW("create sync point failed");
            syncpt.fd = -1;
            syncpt.count = 0;
        }
        uniquefd fence(syncpt.fd);
        mCurrentFrame->frameNumber = syncpt.count;
        mCurrentFrame->releaseFence = uniquefd(fence.dup());

        setupFrame();
        mCommitThread->queueFrame(std::move(mCurrentFrame));

        for (auto& layer : ctx->InputLayers) {
            if (layer->compositionFromValidated() == HWC2::Composition::Device) {
                layer->setReleaseFence(fence.dup());
            }
        }
        ctx->RetireFence = uniquefd(fence.dup());

        if(mRotator != nullptr) {
            mRotator->postCommit(fence.dup());
        }

        // clean up
        mPreviousPresentFence = std::move(fence);
        mAssignment = nullptr;
        mLayerPlanner.postCommit();
    }
    debugFreezeCheck(ctx);
}

int Compositor::setupFrame()
{
    DTRACE_FUNC();
    auto& configs = mAssignment->Configs;
    for (int i = 0; i < configs.size(); i++) {
        const std::shared_ptr<ChannelConfig>& cfg = configs[i];
        for (int s = 0; s < SLOT_PER_CHANNEL; s++) {
            int index = i * SLOT_PER_CHANNEL + s;
            disp_layer_config2* hwlayer = &(mCurrentFrame->layerConfigs[index]);
            setupLayerConfig(cfg, s, hwlayer);
        }
    }
    return 0;
}

unsigned int Compositor::ionGetMetadataFlag(buffer_handle_t handle)
{
    if (NULL != handle) {
        private_handle_t *hdl = (private_handle_t *)handle;
        int ret = 0;
        void *map_ptr = NULL;
        int map_fd = hdl->metadata_fd;
        unsigned int flag = 0;

        if (0 == hdl->ion_metadata_size) {
            DLOGW("ion_metadata_size is 0");
            return 0;
        }

        if(mIonfd < 0) {
            DLOGE("Failed to open ion device");
            return 0;
        }

        map_ptr = mmap(NULL, hdl->ion_metadata_size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, map_fd, 0);
        if (map_ptr == MAP_FAILED) {
            DLOGE("ion_map failed, map_fd=%d\n", map_fd);
            return 0;
        }
        struct sunxi_metadata *ptr = (struct sunxi_metadata *)map_ptr;
        flag = ptr->flag;

        if (flag & SUNXI_METADATA_FLAG_HDRP_HEADER) {
            processHdr10p(ptr);
        }
        /*struct afbc_header *p = &(ptr->afbc_head);
        DLOGD("&&&&& afbc header:");
        DLOGD("%u,%u,%u,%u;\n"
              "%u,%u,%u,%u;\n"
              "%u,%u,%u,%u;\n"
              "%u,%u,%u,%u\n"
              "%u,%u,%u @@@.",
              p->signature, p->filehdr_size, p->version, p->body_size,
              p->ncomponents, p->header_layout, p->yuv_transform, p->block_split,
              p->inputbits[0], p->inputbits[1], p->inputbits[2], p->inputbits[3],
              p->block_width, p->block_height, p->width, p->height,
              p->left_crop, p->top_crop, p->block_layout);

        DLOGD("afbc flag=%x", ptr->flag);
        DLOGD("hdrp lum=%d", ptr->hdr10_plus_smetada.targeted_system_display_maximum_luminance);
        DLOGD("hdrp maxrgb=%d", ptr->hdr10_plus_smetada.average_maxrgb);
        for (int i = 0; i < 10; i++)
            DLOGD("hdrp val[%d]=%d", i, ptr->hdr10_plus_smetada.distribution_values[i]);
        DLOGD("hdrp knee x=%d", ptr->hdr10_plus_smetada.knee_point_x);
        DLOGD("hdrp knee y=%d", ptr->hdr10_plus_smetada.knee_point_y);
        for (int i = 0; i < 9; i++)
            DLOGD("hdrp bezi[%d]=%d", i, ptr->hdr10_plus_smetada.bezier_curve_anchors[i]);*/

        ret = munmap(map_ptr, hdl->ion_metadata_size);
        if (0 != ret) {
            DLOGD("munmap sunxi metadata failed ! ret=%d", ret);
        }
        return flag;
    } else {
        DLOGE("%s,%d", __FUNCTION__, __LINE__);
    }
    return 0;
}


void Compositor::setupAfbcMetadata(buffer_handle_t buf, disp_layer_info2 *layer_info) {
#ifdef GRALLOC_SUNXI_METADATA_BUF
    if (buf != nullptr) {
        private_handle_t *handle = (private_handle_t *)buf;
        int metadata_fd = handle->metadata_fd;
        unsigned int flag = handle->ion_metadata_flag;
        if ((0 <= metadata_fd)
                && (flag & (SUNXI_METADATA_FLAG_AFBC_HEADER
                            | SUNXI_METADATA_FLAG_HDRP_HEADER))) {
            handle->ion_metadata_flag = ionGetMetadataFlag((buffer_handle_t)handle);
            layer_info->fb.metadata_flag = handle->ion_metadata_flag;
            layer_info->fb.metadata_size = handle->ion_metadata_size;
            layer_info->fb.metadata_fd = handle->metadata_fd;
            if (handle->ion_metadata_flag & SUNXI_METADATA_FLAG_AFBC_HEADER)
                layer_info->fb.fbd_en = 1;
        }
    }
#endif
}

void Compositor::setup3dUiLayer(disp_layer_info2 *info)
{
    if (info == nullptr) {
        DLOGW("info = null %s,%d", __FUNCTION__, __LINE__);
        return;
    }

    info->fb.flags = DISP_BF_STEREO_2D_DEPTH;
    info->fb.depth = 3;

    switch(m3Dmode) {
        case DISPLAY_2D_LEFT:
        case DISPLAY_2D_TOP:
        case DISPLAY_2D_DUAL_STREAM:
            info->b_trd_out = 0;
            info->fb.flags = DISP_BF_NORMAL;
            info->fb.depth = 0;
            break;
        case DISPLAY_3D_LEFT_RIGHT_HDMI:
        case DISPLAY_3D_TOP_BOTTOM_HDMI:
        case DISPLAY_3D_DUAL_STREAM:
            info->b_trd_out = 1;
            info->out_trd_mode = DISP_3D_OUT_MODE_FP;
            info->fb.flags = DISP_BF_STEREO_2D_DEPTH;
            info->fb.depth = 3;
            break;
        default :
            info->b_trd_out = 0;
            info->fb.flags = DISP_BF_NORMAL;
            info->fb.depth = 0;
            break;
    }
}

void Compositor::setup3dVideoLayer(disp_layer_info2 *info)
{
    if (info == nullptr) {
        DLOGW("info = null %s,%d", __FUNCTION__, __LINE__);
        return;
    }

    switch(m3Dmode) {
        case DISPLAY_2D_LEFT:
            info->b_trd_out = 0;
            info->fb.flags = DISP_BF_STEREO_SSH;
            break;
        case DISPLAY_2D_TOP:
            info->b_trd_out = 0;
            info->fb.flags = DISP_BF_STEREO_TB;
            break;
        case DISPLAY_3D_LEFT_RIGHT_HDMI:
            info->b_trd_out = 1;
            info->out_trd_mode = DISP_3D_OUT_MODE_FP;
            info->fb.flags = DISP_BF_STEREO_SSH;
            /*info->screen_win.x = 0;
             *info->screen_win.y = 0;
             *info->screen_win.width = 1920;
             *info->screen_win.height = 1080;*/
            break;
        case DISPLAY_3D_TOP_BOTTOM_HDMI:
            info->b_trd_out = 1;
            info->out_trd_mode = DISP_3D_OUT_MODE_FP;
            info->fb.flags = DISP_BF_STEREO_TB;
            /*info->screen_win.x = 0;
             *info->screen_win.y = 0;
             *info->screen_win.width = 1920;
             *info->screen_win.height = 1080;*/
            break;
        case DISPLAY_3D_DUAL_STREAM:
            info->b_trd_out = 1;
            info->out_trd_mode = DISP_3D_OUT_MODE_FP;
            info->fb.flags = DISP_BF_STEREO_FP;
            /*info->screen_win.x = 0;
             *info->screen_win.y = 0;
             *info->screen_win.width = 1920;
             *info->screen_win.height = 1080;*/
            break;
        case DISPLAY_2D_DUAL_STREAM:
        default :
            info->b_trd_out = 0;
            info->fb.flags = DISP_BF_NORMAL;
            break;
    }
}

void setupDataspace(disp_layer_info2 *layer_info, int32_t dataspace)
{
    unsigned int transfer = (dataspace & HAL_DATASPACE_TRANSFER_MASK)
        >> HAL_DATASPACE_TRANSFER_SHIFT;
    unsigned int standard = (dataspace & HAL_DATASPACE_STANDARD_MASK)
        >> HAL_DATASPACE_STANDARD_SHIFT;
    unsigned int range = (HAL_DATASPACE_RANGE_FULL
            != (dataspace & HAL_DATASPACE_RANGE_MASK)) ? 0 : 1; /* 0: limit. 1: full */

    /* color space table [standard][range] */
    const disp_color_space cs_table[][2] = {
        {DISP_UNDEF, DISP_UNDEF_F},
        {DISP_BT709, DISP_BT709_F},
        {DISP_BT470BG, DISP_BT470BG_F},
        {DISP_BT470BG, DISP_BT470BG_F},
        {DISP_BT601, DISP_BT601_F},
        {DISP_BT601, DISP_BT601_F},
        {DISP_BT2020NC, DISP_BT2020NC_F},
        {DISP_BT2020C, DISP_BT2020C_F},
        {DISP_FCC, DISP_FCC_F},
        {DISP_BT709, DISP_BT709_F},
        {DISP_BT709, DISP_BT709_F},
        {DISP_BT709, DISP_BT709_F},
    };
    if ((range < sizeof(cs_table[0]) / sizeof(cs_table[0][0]))
        && (standard < sizeof(cs_table) / sizeof(cs_table[0]))) {
        layer_info->fb.color_space = cs_table[standard][range];
    } else {
        DLOGW("unknown dataspace standard(0x%x) range(0x%x)", standard, range);
        layer_info->fb.color_space = range ? DISP_UNDEF_F : DISP_UNDEF;
    }

    const disp_eotf eotf_table[] = {
        DISP_EOTF_UNDEF,
        DISP_EOTF_LINEAR,
        DISP_EOTF_IEC61966_2_1,
        DISP_EOTF_BT601,
        DISP_EOTF_GAMMA22,
        DISP_EOTF_GAMMA28,  /* HAL_DATASPACE_TRANSFER_GAMMA2_6 */
        DISP_EOTF_GAMMA28,
        DISP_EOTF_SMPTE2084,
        DISP_EOTF_ARIB_STD_B67
    };
    if (transfer < sizeof(eotf_table) / sizeof(eotf_table[0])) {
        layer_info->fb.eotf = eotf_table[transfer];
    } else {
        DLOGW("unknown dataspace Transfer(0x%x)", transfer);
        layer_info->fb.eotf = DISP_EOTF_UNDEF;
    }
    //DLOGD("layer_info_fb: eotf=%d, cs=%d", layer_info->fb.eotf, layer_info->fb.color_space);
}


#define SUNXI_PIXEL_ALPHA  0
#define SUNXI_GLOBAL_ALPHA 1
#define SUNXI_GLOBAL_AND_PIXEL_ALPHA 2

int Compositor::setupLayerConfig(
        const std::shared_ptr<ChannelConfig>& cfg, int slotIndex,
        disp_layer_config2 *hwlayer)
{
    const ChannelConfig::Slot& slot = cfg->Slots[slotIndex];
    const std::shared_ptr<Layer>& layer = slot.InputLayer;

    hwlayer->enable = slot.Enable;
    hwlayer->channel = cfg->Index;
    hwlayer->layer_id = slotIndex;

    if (!slot.Enable) {
        memset(&hwlayer->info, 0, sizeof(hwlayer->info));
        return 0;
    }

    // zorder and alpha config
    disp_layer_info2* info = &hwlayer->info;
    info->zorder = slot.Z;
    info->alpha_value = (int)ceil(layer->alpha() * 255.0f);
    info->alpha_mode = isBlendingLayer(layer) ? SUNXI_GLOBAL_AND_PIXEL_ALPHA
                                              : SUNXI_PIXEL_ALPHA;
    info->fb.pre_multiply = isPremultLayer(layer) ? 1 : 0;

    // solid color layer config
    if (slot.DimLayer) {
        return setupColorModeLayer(layer, hwlayer);
    }

    setupDataspace(info, layer->dataspace());
    setupAfbcMetadata(layer->bufferHandle(), &hwlayer->info);

    if (m3Dmode != DISPLAY_2D_ORIGINAL) {
        if (isYuvFormat(layer))
            setup3dVideoLayer(&hwlayer->info);
        else
            setup3dUiLayer(&hwlayer->info);
    }

    // buffer layer config
    assert(layer->bufferHandle() != nullptr);
    info->mode = LAYER_MODE_BUFFER;

    private_handle_t* handle = (private_handle_t *)layer->bufferHandle();
    uniquefd acquireFence(dup(layer->acquireFence()));
    hwc_rect_t crop = layer->sourceCrop();

    bool useRotateBuffer = false;
    uniquefd buffer(dup(handle->share_fd));

    // handle transform layer:
    // 1. replace the layer buffer handle with rotated buffer from RotatorManager
    // 2. transform the source crop
    // 3. replace the acquire fence
    if (layer->transform() != HWC2::Transform::None && mRotator!= nullptr) {
        const OutputBuffer_t* rotatedBuf;
        int acqfence;
        int error = mRotator->getRotatedBuffer(layer, &rotatedBuf, &acqfence);
        if (error == 0) {
            int32_t transform = static_cast<int32_t>(layer->transform());
            hwc_rect_t tmp = crop;
            switch (transform) {
                case HAL_TRANSFORM_FLIP_V:
                    crop.top    = handle->height - tmp.bottom;
                    crop.bottom = handle->height - tmp.top;
                    break;
                case HAL_TRANSFORM_FLIP_H:
                    crop.left   = rotatedBuf->width - tmp.right;
                    crop.right  = rotatedBuf->width - tmp.left;
                    break;
                case HAL_TRANSFORM_ROT_90:
                    // After rotate 90 degree, the last line of source buffer has been place
                    // at the first column of rotated buffer.
                    // crop.left   = (rotatedBuf->width - tmp.bottom) - (rotatedBuf->width - handle->height);
                    // crop.right  = (rotatedBuf->width - tmp.top)    - (rotatedBuf->width - handle->height);
                    crop.left   = handle->height - tmp.bottom;
                    crop.right  = handle->height - tmp.top;
                    crop.top    = tmp.left;
                    crop.bottom = tmp.right;
                    break;
                case HAL_TRANSFORM_ROT_180:
                    crop.left   = rotatedBuf->width - tmp.right;
                    crop.right  = rotatedBuf->width - tmp.left;
                    crop.top    = handle->height - tmp.bottom;
                    crop.bottom = handle->height - tmp.top;
                    break;
                case HAL_TRANSFORM_ROT_270:
                    crop.left   = tmp.top;
                    crop.right  = tmp.bottom;
                    crop.top    = rotatedBuf->height - tmp.right;
                    crop.bottom = rotatedBuf->height - tmp.left;
                    break;
                case HWC_TRANSFORM_FLIP_H_ROT_90:
                    crop.left   = handle->height - tmp.bottom;
                    crop.right  = handle->height - tmp.top;
                    crop.top    = handle->width - tmp.right;
                    crop.bottom = handle->width - tmp.left;
                    break;
                case HWC_TRANSFORM_FLIP_V_ROT_90:
                    crop.left   = tmp.top;
                    crop.right  = tmp.bottom;
                    crop.top    = tmp.left;
                    crop.bottom = tmp.right;
                    break;
            }

            acquireFence = uniquefd(acqfence);
            buffer = uniquefd(dup(rotatedBuf->buffer->handle));

            // use transform buffer to setup layer info
            info->fb.size[0].width = rotatedBuf->width;
            info->fb.size[1].width = rotatedBuf->width / 2;
            info->fb.size[2].width = rotatedBuf->width / 2;
            info->fb.size[0].height = rotatedBuf->height;
            info->fb.size[1].height = rotatedBuf->height / 2;
            info->fb.size[2].height = rotatedBuf->height / 2;
            info->fb.align[0] = rotatedBuf->align;
            info->fb.align[1] = rotatedBuf->align;
            info->fb.align[2] = rotatedBuf->align;
            info->fb.format = toDispFormat(rotatedBuf->format);
            useRotateBuffer = true;
        } else {
            DLOGE("rotate layer marked as device composition, but failed");
        }
    }

    info->fb.crop.x = (long long)crop.left << 32;
    info->fb.crop.y = (long long)crop.top  << 32;
    info->fb.crop.width  = (long long)(crop.right - crop.left) << 32;
    info->fb.crop.height = (long long)(crop.bottom - crop.top) << 32;

    if (!useRotateBuffer) {
        info->fb.size[0].width = handle->stride;
        info->fb.size[1].width = handle->stride / 2;
        info->fb.size[2].width = handle->stride / 2;
        info->fb.size[0].height = handle->height;
        info->fb.size[1].height = handle->height / 2;
        info->fb.size[2].height = handle->height / 2;
        info->fb.align[0] = handle->aw_byte_align[0];
        info->fb.align[1] = handle->aw_byte_align[1];
        info->fb.align[2] = handle->aw_byte_align[2];
        info->fb.format = toDispFormat(handle->format);
    }

    // display frame
    // TODO: apply video ratio config
    Rect dispFrame(layer->displayFrame());
    dispFrame = mTransform.transform(dispFrame);
    info->screen_win.x = dispFrame.left;
    info->screen_win.y = dispFrame.top;
    info->screen_win.width  = dispFrame.width();
    info->screen_win.height = dispFrame.height();

    info->fb.fd = buffer.get();
    hw_layer_info_revise(info);
    mCurrentFrame->deferBufferFds.push_back(std::move(buffer));

    // merge acquire fence here
    int fd = fence_merge("hwcacquire", acquireFence.get(),
            mCurrentFrame->acquireFence.get());
    mCurrentFrame->acquireFence = uniquefd(fd);
    return 0;
}

int Compositor::setupColorModeLayer(
        const std::shared_ptr<Layer>& layer, disp_layer_config2* hwlayer)
{
    hwc_color_t color = layer->solidColor();
    disp_layer_info2* info = &hwlayer->info;

    info->mode = LAYER_MODE_COLOR;
    info->color = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
    info->fb.format = DISP_FORMAT_ARGB_8888;

    // display frame
    Rect dispFrame(layer->displayFrame());

    // crop and fb size
    Rect crop = dispFrame;
    info->fb.crop.x = (long long)crop.left << 32;
    info->fb.crop.y = (long long)crop.top  << 32;
    info->fb.crop.width  = (long long)(crop.right - crop.left) << 32;
    info->fb.crop.height = (long long)(crop.bottom - crop.top) << 32;
    info->fb.size[0].width  = crop.right - crop.left;
    info->fb.size[0].height = crop.bottom - crop.top;

    dispFrame = mTransform.transform(dispFrame);
    info->screen_win.x = dispFrame.left;
    info->screen_win.y = dispFrame.top;
    info->screen_win.width  = dispFrame.width();
    info->screen_win.height = dispFrame.height();
    hw_layer_info_revise(info);
    return 0;
}

void Compositor::blank(bool enabled)
{
    std::unique_lock<std::mutex> lock(mLock);
    mBlankOutput = enabled;

    if (!mBlankOutput)
        return;

    // wait until commit process is finish
    mCompositionLock.wait(lock);

    // on display output blank, we need to send a black frame to driver,
    // so that it can replace current frame and signal the release fence
    const CompositionEngine::Attribute& attr = mDisplayEngine->getAttribute();
    int totalChannl = attr.maxHybridChannelCount + attr.maxRGBChannelCount;
    int layerCount = totalChannl * SLOT_PER_CHANNEL;
    std::unique_ptr<Frame> blackFrame = CommitThread::createFrame(layerCount);
    if (blackFrame == nullptr) {
        DLOGE("alloc black frame error, out of memory");
        return;
    }

    syncinfo syncpt;
    if (mDisplayEngine->createSyncpt(&syncpt)) {
        DLOGW("create sync point failed");
        syncpt.fd = -1;
        syncpt.count = 0;
    }
    uniquefd fence(syncpt.fd);
    blackFrame->frameNumber = syncpt.count;
    blackFrame->releaseFence = uniquefd(fence.dup());

    // mark all layer as disable
    for (int i = 0; i < totalChannl; i++) {
        for (int s = 0; s < SLOT_PER_CHANNEL; s++) {
            int index = i * SLOT_PER_CHANNEL + s;
            disp_layer_config2* cfg = &(blackFrame->layerConfigs[index]);
            cfg->enable   = 0;
            cfg->channel  = i;
            cfg->layer_id = s;
        }
    }
    mCommitThread->queueFrame(std::move(blackFrame));

    // block here until this black frame has been present on display.
    sync_wait(mPreviousPresentFence.get(), DEFAULT_FENCE_TIMEOUT);
    mPreviousPresentFence = std::move(fence);
}

int Compositor::freeze(bool freeze)
{
    std::unique_lock<std::mutex> lock(mLock);
    // wait until commit process is finish
    mCompositionLock.wait(lock);
    mFreezeContent = freeze;
    if (!freeze)
        mFreezeCount = 0;
    else
        mFreezeCount += 1;
    DLOGD("enable: %d count %d", mFreezeContent, mFreezeCount);
    return 0;
}

void Compositor::setTraceNameSuffix(const char *suffix)
{
    mTraceNameSuffix = std::string("queueFrame.");
    mTraceNameSuffix.append(suffix);
}

void Compositor::skipFrameEnable(bool enabled)
{
    mSkipFrameEnabled = enabled;
}

void Compositor::setColorTransform(const float *matrix, int32_t hint)
{
    mColorTransform = static_cast<android_color_transform_t>(hint);
}

void Compositor::setRefreshCallback(RefreshCallback cb)
{
    mRefreshCallback = cb;
}

void Compositor::setForceClientCompositionOnFreezed(bool enable)
{
    mForceClientCompositionOnOutputFreezed = enable;
}

void Compositor::onOutputFreezed(bool freezed)
{
    if (mForceClientCompositionOnOutputFreezed) {
        mCommitIdle = freezed;
        if (mCommitIdle && mRefreshCallback != nullptr) {
            // Request a whole screen refresh.
            mRefreshCallback();
        }
    }
}

StrategyType Compositor::pickCompositionStrategy() const
{
    if (mColorTransform != HAL_COLOR_TRANSFORM_IDENTITY
        || (mCommitIdle && mFrameRateAuditor.isNeedGPUComposiLastFrame())) {
        return eGPUOnly;
    }
    return eHardwareFirst;
}

void Compositor::dump(std::string& out)
{
    out += StringPrintf("\tBlank %d\n", mBlankOutput);
    out += StringPrintf("\t3D output %d\n", mOutput3D);
    out += StringPrintf("\tColor Transform [%d]\n", mColorTransform);
    out += std::string("\tScreen Transform: ") + mTransform.toString();

    out += std::string("\tCommitThread: ");
    mCommitThread->dump(out);

    if (mRotator) {
        out += std::string("\tHardwareRotator: ");
        mRotator->dump(out);
    }

}

} // namespace sunxi
