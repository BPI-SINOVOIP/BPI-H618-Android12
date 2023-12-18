
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

using android::base::StringPrintf;

namespace sunxi {

#define MAX_LAYER_COUNT 16
// default fence timeout in ms
#define DEFAULT_FENCE_TIMEOUT 3000

struct CommitThread::Frame {
    const int layerCount;
    disp_layer_config2 layerConfigs[MAX_LAYER_COUNT];
	std::shared_ptr<struct eink_img> cur_img;
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
			cur_img = std::make_shared<struct eink_img>();
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
            if (mRotator == nullptr)
                return eTransformError;
            else
                return mRotator->tryUsingHardwareRotator(layer);
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
        frame->frameNumber, frame->layerConfigs, frame->layerCount,
        frame->cur_img, mCompositor.mForceRefresh, &mCompositor.mHandwriteWin);
    if (mCompositor.mForceRefresh == true) {
        mCompositor.mForceRefresh = false;
	}

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
    mRefreshCallback(nullptr)
{
    mLayerPlanner.setStrategy(eHardwareFirst);
    mFrameRateAuditor.setOutputFreezedHandler(this);
    mCommitThread = std::make_unique<CommitThread>(*this);
    mCommitThread->start("CommitThread");
	mRefreshMode = HWC2_EINK_GU16_MODE;
	mHandWriteMode = HWC2_EINK_DU_MODE;

	handwrite_img = std::make_shared<struct eink_img>();

	handwrite_img->win_calc_en = 1;
	handwrite_img->upd_all_en = 0;
	handwrite_img->upd_mode = (enum upd_mode)HWC2_EINK_DU_MODE;

	mForceRefresh = false;

	handwrite_img->fd = -1;
	::getDisplayOutputSize(0, (int *)&mScreenWidth, (int *)&mScreenHeight);
	handwrite_img->size.width = mScreenWidth;
	handwrite_img->size.height = mScreenHeight;

	handwrite_img->size.align = 4;
	handwrite_img->pitch = EINK_ALIGN(handwrite_img->size.width, handwrite_img->size.align);
	handwrite_img->out_fmt = EINK_Y8;
	handwrite_img->dither_mode = QUANTIZATION;
	handwrite_img->fd = -1;
	handwrite_fd = -1;
}

Compositor::~Compositor() {
    if (mCommitThread)
        mCommitThread->stop();
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
    std::scoped_lock<std::mutex> lock(mLock);
    mCompositionLock.acquire();

    if (mBlankOutput) {
        DLOGW("refresh after blank!");
        return;
    }

    if (mFreezeContent) {
        for (const std::shared_ptr<Layer>& layer : ctx->InputLayers) {
            layer->setReleaseFence(-1);
        }
        return;
    }

    std::unique_ptr<PlanConstraint> constraint = createPlanConstraint();
    mAssignment = std::make_unique<PlanningOutput>();
    mLayerPlanner.setStrategy(pickCompositionStrategy());
    mLayerPlanner.advanceFrame(*constraint, ctx, mAssignment.get());

    // The PlanningOutput::Configs is sort by zorder,
    // We need to rearrange to match the hardware order.
    mDisplayEngine->rearrangeChannel(mAssignment->Configs);

    // post rotate task after validate finished
    if(mRotator != nullptr) {
        mRotator->postRotateTask();
    }
}


void Compositor::commit(CompositionContext* ctx)
{
    DTRACE_FUNC();
    std::scoped_lock<std::mutex> lock(mLock);

    // mCompositionLock.release() will be called be when we exiting from this scope
    auto deferReleaseFunc = [](ScopeLock* s){ s->release(); };
    std::unique_ptr<ScopeLock, std::function<void(ScopeLock *)>> scopeLockRelease(
            &mCompositionLock, deferReleaseFunc);

    if (mAssignment == nullptr || mFreezeContent) {
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

    setupFrame(ctx);
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

int Compositor::setupFrame(CompositionContext *ctx)
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

	mCurrentFrame->cur_img->win_calc_en = 1;
	mCurrentFrame->cur_img->upd_all_en = 0;
	mCurrentFrame->cur_img->upd_mode = (enum upd_mode)mRefreshMode;


	mCurrentFrame->cur_img->upd_win.left = 0;
	mCurrentFrame->cur_img->upd_win.right = 0;
	mCurrentFrame->cur_img->upd_win.top = 0;
	mCurrentFrame->cur_img->upd_win.bottom = 0;

	mCurrentFrame->cur_img->size.width = mScreenWidth;
	mCurrentFrame->cur_img->size.height = mScreenHeight;
	mCurrentFrame->cur_img->size.align = 4;
	mCurrentFrame->cur_img->pitch = EINK_ALIGN(mScreenWidth, mCurrentFrame->cur_img->size.align);
	mCurrentFrame->cur_img->out_fmt = EINK_Y8;
	mCurrentFrame->cur_img->dither_mode = QUANTIZATION;

    return 0;
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
	info->alpha_mode = isBlendingLayer(layer) ?
		SUNXI_GLOBAL_AND_PIXEL_ALPHA : SUNXI_PIXEL_ALPHA;
	info->fb.pre_multiply = isPremultLayer(layer) ? 1 : 0;

    // solid color layer config
    if (slot.DimLayer) {
        return setupColorModeLayer(layer, hwlayer);
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
    dispFrame = mTransform.transform(dispFrame);
    info->screen_win.x = dispFrame.left;
    info->screen_win.y = dispFrame.top;
    info->screen_win.width  = dispFrame.width();
    info->screen_win.height = dispFrame.height();

    // crop and fb size
    Rect crop = dispFrame;
    info->fb.crop.x = (long long)crop.left << 32;
    info->fb.crop.y = (long long)crop.top  << 32;
    info->fb.crop.width  = (long long)(crop.right - crop.left) << 32;
    info->fb.crop.height = (long long)(crop.bottom - crop.top) << 32;
    info->fb.size[0].width  = crop.right - crop.left;
    info->fb.size[0].height = crop.bottom - crop.top;
    return 0;
}

void Compositor::blank(bool enabled)
{
    std::unique_lock<std::mutex> lock(mLock);
    // wait until commit process is finish
    mCompositionLock.wait(lock);
    mBlankOutput = enabled;

    if (!mBlankOutput)
        return;

    sync_wait(mPreviousPresentFence.get(), DEFAULT_FENCE_TIMEOUT);
}

int Compositor::freeze(bool freeze)
{
    std::unique_lock<std::mutex> lock(mLock);
    // wait until commit process is finish
    mCompositionLock.wait(lock);
    mFreezeContent = freeze;
    DLOGD("enable: %d", mFreezeContent);
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

void Compositor::onOutputFreezed(bool freezed)
{
    mCommitIdle = freezed;
    if (mCommitIdle && mRefreshCallback != nullptr) {
        // Request a whole screen refresh.
        mRefreshCallback();
    }
}

StrategyType Compositor::pickCompositionStrategy() const
{
    if (mColorTransform != HAL_COLOR_TRANSFORM_IDENTITY)
        return eGPUOnly;
    return eHardwareFirst;
}

void Compositor::dump(std::string& out)
{
    out += StringPrintf("\tBlank %d\n", mBlankOutput);
    out += StringPrintf("\t3D output %d\n", mOutput3D);
    out += StringPrintf("\tColor Transform [%d]\n", mColorTransform);
    out += std::string("\tScreen Transform: ") + mTransform.toString();
    out += StringPrintf("\tEink Refresh mode:[0x%.8x] handwrite:[0x%.8x]\n", mRefreshMode, mHandWriteMode);

    out += std::string("\tCommitThread: ");
    mCommitThread->dump(out);

    if (mRotator) {
        out += std::string("\tHardwareRotator: ");
        mRotator->dump(out);
    }

}

int Compositor::setEinkMode(int mode)
{
	std::scoped_lock<std::mutex> lock(mLock);
	if (mode & HWC_EINK_HANDWRITTEN) {
		mHandWriteMode = mode;
		mRefreshMode = mRefreshMode | HWC_EINK_HANDWRITTEN;
	} else {
		if (handwrite_fd >= 0) {
			if(::eink_handwrite_dma_map(0, handwrite_fd, NULL))
				DLOGE("handwrite dma unmap fail!");
			::close(handwrite_fd);
			handwrite_fd = -1;
			handwrite_img->paddr = 0;
		}
		mRefreshMode = mode;
		mHandWriteMode = mode;
	}

	return 0;
}

int Compositor::setEinkBufferFd(int fd)
{
	std::scoped_lock<std::mutex> lock(mLock);
	if (fd >= 0) {
		if (handwrite_fd >= 0) {
			if(::eink_handwrite_dma_map(0, handwrite_fd, NULL))
				DLOGE("handwrite dma unmap fail!");
			::close(handwrite_fd);
			handwrite_fd = -1;
			handwrite_img->paddr = 0;
		}
		handwrite_fd = ::dup(fd);
		::close(fd);
		if(::eink_handwrite_dma_map(1, handwrite_fd, &handwrite_img->paddr))
			DLOGE("get y8 paddr fail!");
	}
	return 0;
}

#define WAIT_SUBMIT_TIME 40
static nsecs_t lastTime;
int Compositor::updateEinkRegion(int left, int top, int right, int bottom)
{
	std::scoped_lock<std::mutex> lock(mLock);
	if (handwrite_img->paddr == 0 || !(mHandWriteMode & HWC_EINK_HANDWRITTEN)) {
		return -1;
	}

    nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
    double interval = (startTime - lastTime)/1000000;
    if (interval < WAIT_SUBMIT_TIME) {
        return -1;
    }

	handwrite_img->upd_win.left = left;
	handwrite_img->upd_win.right = right;
	handwrite_img->upd_win.top = top;
	handwrite_img->upd_win.bottom = bottom;
	handwrite_img->upd_all_en = 0;
	handwrite_img->upd_mode = (enum upd_mode)mHandWriteMode;
	mDisplayEngine->submitLayer(0, NULL, 0, handwrite_img, mForceRefresh, &mHandwriteWin);
	if (mForceRefresh == true) {
		mForceRefresh = false;
	}
    //usleep(60000);
    lastTime = systemTime(SYSTEM_TIME_MONOTONIC);

	return 0;
}

int Compositor::setEinkUpdateArea(int left, int top, int right, int bottom)
{
	mHandwriteWin.left = left;
	mHandwriteWin.top = top;
	mHandwriteWin.right = right;
	mHandwriteWin.bottom = bottom;

	return 0;
}

int Compositor::forceEinkRefresh(bool rightNow)
{
	mForceRefresh = true;
    return 0;
}
} // namespace sunxi
