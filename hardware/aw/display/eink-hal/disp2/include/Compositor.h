/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef _DISP2_COMPOSITOR_H_
#define _DISP2_COMPOSITOR_H_

#include <memory>
#include <thread>
#include <queue>

#include "FramerateAuditor.h"
#include "LayerPlanner.h"
#include "ScreenTransform.h"
#include "scope_lock.h"
#include "hardware/sunxi_display2.h"
#include "PerfMonitor.h"
#include "sunxi_eink.h"

namespace sunxi {

class Compositor;


// Separate thread to handle commit task
class CommitThread {
public:
    explicit CommitThread(Compositor& c);
   ~CommitThread();

    struct Frame;
    void start(const char* name);
    void stop();
    void queueFrame(std::unique_ptr<Frame> newframe);
    static std::unique_ptr<Frame> createFrame(int layerCount);
    void dump(std::string& out) const;

private:
    void threadLoop();
    void commitFrame(Frame* frame);

private:
    static const int mMaxQueuedFrame = 32;
    Compositor& mCompositor;
    std::string mName;
    std::thread mThread;
    std::atomic<bool> mRunning;
    mutable std::mutex mLock;
    mutable std::condition_variable mCondition;
    std::queue<std::unique_ptr<Frame>> mPendingFrames;
    std::unique_ptr<Frame> mPrevFrame;
    PerfMonitor mPerf;
};

class CompositionEngine;
class RotatorManager;
using Frame = CommitThread::Frame;
using RefreshCallback = std::function<void()>;

class Compositor : public FramerateAuditor::OutputFreezedHandler {
public:
    Compositor();
   ~Compositor();

    void setScreenSize(int width, int height);
    void setFramebufferSize(int width, int height);
    void setCompositionEngine(const std::shared_ptr<CompositionEngine>& engine);
    void setRotatorManager(const std::shared_ptr<RotatorManager>& rotator);
    void prepare(CompositionContext *ctx);
    void commit(CompositionContext *ctx);
    void blank(bool enabled);
    void setTraceNameSuffix(const char *suffix);
    void skipFrameEnable(bool enabled);
    void setColorTransform(const float *matrix, int32_t hint);
    void setRefreshCallback(RefreshCallback cb);
    int freeze(bool freeze);
    void dump(std::string& out);
	int setEinkMode(int mode);
	int setEinkBufferFd(int fd);
	int updateEinkRegion(int left, int top, int right, int bottom);
    int forceEinkRefresh(bool rightNow);
	int setEinkUpdateArea(int left, int top, int right, int bottom);

    friend class CommitThread;
private:
    std::unique_ptr<PlanConstraint> createPlanConstraint();
    int setupFrame(CompositionContext *ctx);
    int setupLayerConfig(const std::shared_ptr<ChannelConfig>& cfg, int slotIndex,
            disp_layer_config2 *hwlayer);
    int setupColorModeLayer(
            const std::shared_ptr<Layer>& layer, disp_layer_config2* hwlayer);
    void onOutputFreezed(bool freezed);
    StrategyType pickCompositionStrategy() const;

    std::shared_ptr<CompositionEngine> mDisplayEngine;
    std::shared_ptr<RotatorManager> mRotator;

    bool mOutput3D;
    // when mBlankOutput == true, not any frame will send to driver
    bool mBlankOutput;
    bool mFreezeContent;
    LayerPlanner mLayerPlanner;
    std::unique_ptr<PlanningOutput> mAssignment;

    // Framebuffer to screen transform
    ScreenTransform mTransform;

    // Hook for format/scaler/rotate capabilities detect
    std::shared_ptr<PlanConstraint::IHWLayerDetector> mLayerDetector;

    // commit thread
    std::unique_ptr<Frame> mCurrentFrame;
    std::unique_ptr<CommitThread> mCommitThread;

    // mutex to protect commit process
    std::mutex mLock;
    ScopeLock mCompositionLock;
    uniquefd mPreviousPresentFence;
    std::string mTraceNameSuffix;

    // when external display 's vsync frequency is slow then primary display,
    // the queued frame will block on commit thread,
    // so that we should enable skip frame feature for external display.
    bool mSkipFrameEnabled;

    // color transform which will be applied after composition.
    android_color_transform_t mColorTransform;

    // When Android stops committing for a period of time, use GPU composition to save power:
    // 1. set composition strategy as eGPUOnly
    // 2. callback into composer service for requesting a whole screen refresh.
    bool mCommitIdle;
    FramerateAuditor mFrameRateAuditor;
    RefreshCallback mRefreshCallback;
	std::shared_ptr<struct eink_img> handwrite_img;
	struct upd_win mHandwriteWin;
	int handwrite_fd;
	int mHandWriteMode;
	int mRefreshMode;
	int mScreenWidth;
	int mScreenHeight;
	bool mForceRefresh;
};

} // namespace sunxi

#endif
