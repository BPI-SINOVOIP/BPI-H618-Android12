/*
 * Copyright (C) 2021 by Allwinnertech Co. Ltd.
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

#ifndef ANDROID_COMPONENTS_INCLUDE_VDECCOMPONENT_H_
#define ANDROID_COMPONENTS_INCLUDE_VDECCOMPONENT_H_

#include <C2Component.h>

#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/Mutexed.h>
#include <sys/time.h>

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "C2DebugHelper.h"
#include "C2Log.h"
#include "HwC2Interface.h"
#include "VdecParameterHelper.h"
#include "vdecoder.h"

#define DEFAULT_VE_PIXEL_FORMAT PIXEL_FORMAT_YV12
#define ENABLE_SCALEDOWN_WHEN_RESOLUTION_MOER_THAN_1080P (0)
#define ENABLE_SAVE_ES_DATA (0)
#define ENABLE_SAVE_PICTURE (0)
#define ENABLE_SHOW_BUFINFO_STATUS (0)
#define ENABLE_STATISTICS_TIME (0)
#define MAX_INOUT_DIFF_NUM (16)
#define DEFAULT_DISPLAY_HOLD_BUFFER_NUM (3)
#define DEFAULT_DEINTERLACE_HOLD_BUFFER_NUM (2)
#define DEFAULT_ROTATION_HOLD_BUFFER_NUM (0)
#define DEFAULT_SMOOTH_BUFFER_NUM (3)
#define DEFAULT_DECODE_FRAME_PACKAGE (1)
#define DEFAULT_DISPLAY_ERROR_FRAME (1)

constexpr size_t kSmoothnessFactor = 4;
constexpr size_t kRenderingDepth = 3;

namespace android {

class VdecComponent : public std::enable_shared_from_this<VdecComponent> {
 public:
  VdecComponent(const char *name,
                std::shared_ptr<VdecParameterHelper> paraHelper);
  /**
   * Initialize internal states of the component according to the config set
   * in the interface.
   *
   * This method is called during start(), but only at the first invocation or
   * after reset().
   */
  c2_status_t onInit();

  /**
   * Start the component.
   */
  c2_status_t onStart();

  /**
   * Stop the component.
   */
  c2_status_t onStop();

  /**
   * Reset the component.
   */
  void onReset();

  /**
   * Release the component.
   */
  void onRelease();

  /**
   * Flush the component.
   */
  c2_status_t onFlush_sm();

  /**
   * Process the given work and finish pending work using finish().
   *
   * \param[in,out]   work    the work to process
   * \param[in]       pool    the pool to use for allocating output blocks.
   */
  void process(const std::unique_ptr<C2Work> &work,
               const std::shared_ptr<C2BlockPool> &pool);

  /**
   * Drain the component and finish pending work using finish().
   *
   * \param[in]   drainMode   mode of drain.
   * \param[in]   pool        the pool to use for allocating output blocks.
   *
   * \retval C2_OK            The component has drained all pending output
   *                          work.
   * \retval C2_OMITTED       Unsupported mode (e.g. DRAIN_CHAIN)
   */
  c2_status_t drain(uint32_t drainMode,
                    const std::shared_ptr<C2BlockPool> &pool);

  void setWorkHandlerCb(std::function<void(std::unique_ptr<C2Work>)> cb) {
    mWorkHandlerCb = cb;
  }

  std::shared_ptr<C2Buffer> createLinearBuffer(
      const std::shared_ptr<C2LinearBlock> &block);

  std::shared_ptr<C2Buffer> createLinearBuffer(
      const std::shared_ptr<C2LinearBlock> &block, size_t offset, size_t size);
  std::shared_ptr<C2Buffer> createGraphicBuffer(
      const std::shared_ptr<C2GraphicBlock> &block);

  std::shared_ptr<C2Buffer> createGraphicBuffer(
      const std::shared_ptr<C2GraphicBlock> &block, const C2Rect &crop);

  ~VdecComponent();

  static constexpr uint32_t NO_DRAIN = ~0u;
  C2ReadView mDummyReadView;

  void setNeedsCodecConfig(bool val) { mNeedsCodecConfig = val; }

  FbmBufInfo *getVideoFbmBufInfo();
  void setIntf(std::shared_ptr<HwC2Interface<void>::BaseParams> intf) {
    mIntf = intf;
  }

 private:
  char COMPONENT_NAME[100];
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mStride;
  std::atomic_uint64_t mFrameIndex;
  std::atomic_uint64_t mInputFrameIndex;
  bool mFirstRound;
  bool mMetValidInputData;
  int mCountDownEos;
  std::shared_ptr<VdecParameterHelper> mParaHelper;
  std::atomic_bool mInitilizeDec;
  int32_t mCodecFormat;
  bool mNeedsCodecConfig;

  char mCodecSpecificData[20 * 1024];
  int mCodecSpecificDataLen;

  struct timeval mTimeStart;  // Time at the start of decode()
  struct timeval mTimeEnd;    // Time at the end of decode()
#ifdef FILE_DUMP_ENABLE
  char mInFile[200];
  char mOutFile[200];
  int OutFileYuvCounts;
#endif /* FILE_DUMP_ENABLE */

  std::shared_ptr<C2DebugHelper> mDebug;

  typedef struct DECODERCONTEXT {
    int32_t mIonFd;
    typedef void *VideoDecoder;
    VideoDecoder *pVideoDec;
    struct ScMemOpsS *memops;
    VideoStreamInfo mVideoStreamInfo;
    FbmBufInfo mFbmBufInfo;
    VConfig mVConfig;
    bool mHadGetVideoFbmBufInfo;
    bool mInitOutBuffer;
    // FOR keep holding buffers that are still refered by decoder.
    // erase one if released.
    std::map<const void *, std::shared_ptr<C2Buffer>> mHoldingC2Buffers;
    // FOR keep track of buffers/pictures sent to decoder.
    // remove one if transform to C2Buffer.
    std::map<const void *, std::shared_ptr<C2GraphicBlock>> mHoldingOutBlocks;

    typedef struct BUFFERINFO {
      int32_t nBufNum;
      int32_t nWidth;   // buffer width
      int32_t nHeight;  // buffer height
      int32_t nRealWidth;
      int32_t nRealHeight;
      int32_t nLeftOffset;
      int32_t nTopOffset;
    } BufferInfo;
    BufferInfo mBufferInfo;
  } Decoder;

  std::mutex mDecLock;
  std::mutex mWorkLock;
  std::mutex mSubmitLock;

  typedef struct INPUTDATA {
    char *addr;
    size_t size;
    int64_t pts;
    bool eos;
  } InputData;

  Decoder mDecoder;
  Mutexed<std::list<std::unique_ptr<InputData>>> mSubmitWorkQueue;
  std::atomic_bool mInputMetEos;
  std::atomic_bool mOutEosFlag;
  std::atomic_bool mSendOutEosFlag;
  std::atomic_uint64_t mSubmitCounts;
  std::atomic_uint64_t mRenderCounts;
  std::atomic_bool mFlushFlag;
  uint32_t mFrameRate;
  uint32_t mEmptyPictureLimit;
  uint32_t mValidPictureLimit;
  bool mResolutionChange;
  bool mOutputDelayInited;
  bool mDecoderErrorFlag;
  bool mSignalledError;
  uint32_t mEosCountDown;
  uint32_t mGetFbmRetryCount;

  c2_status_t createDecoder();

  c2_status_t initDecoder();

  c2_status_t renderOutputBufferIfNecessary(
      const std::unique_ptr<C2Work> &work);

  c2_status_t submitInputStream(const std::unique_ptr<InputData> &inputData);

  c2_status_t drainInternal(uint32_t drainMode,
                            const std::shared_ptr<C2BlockPool> &pool,
                            const std::unique_ptr<C2Work> &work);

  c2_status_t resetDecoder();

  void resetPlugin();

  void checkAllPicturesStatus();

  c2_status_t ensureDecoderState(const std::shared_ptr<C2BlockPool> &pool);
  c2_status_t initDecoderAddress(const std::shared_ptr<C2BlockPool> &pool);

  void fillWork(const std::unique_ptr<C2Work> &work,
                std::shared_ptr<C2GraphicBlock> &nOutBlock,
                VideoPicture *pPicture);
  void fillEmptyWork(const std::unique_ptr<C2Work> &work);

  c2_status_t cloneAndSend(std::shared_ptr<C2GraphicBlock> &nOutBlock,
                           VideoPicture *pVideoPicture, uint32_t flags);

  void evaluatePerformance(enum FLAG_WORK_OPERATION flag);

  void transformVideoPictureToOutBlock(
      std::map<const void *, std::shared_ptr<C2GraphicBlock>>
          *mHoldingOutBlocks,
      VideoPicture *pVideoPicture, std::shared_ptr<C2GraphicBlock> *nOutBlock);

  int getVideoCodecFormat(const char *name, int32_t *codecFormat);
  void updateColorAspects(VideoPicture *pVideoPicture);

 private:
  // TODO(kay): A thread test demo is required to test its stability.
  // TODO(kay): reconstruct DecodeManager to a ThreadManager with a
  // string name "decode" or "render".
  // Log should be inserted in every thread control function.
  //
  // bool (android::VdecComponent::*)()
  class VdecBaseThread : public android::Thread {
   public:
    explicit VdecBaseThread(VdecComponent *vc, std::string threadTag,
                            std::function<bool(void)> func)
        : Thread(false),
          mVdecComponent(vc),
          mRequestExit(false),
          mRequestSuspend(false),
          mThreadTag(threadTag),
          mFuncs(func) {}

    void startThread() {
      ALOGD("%s thread %s", __func__, mThreadTag.c_str());
      mRequestSuspend = false;
      run(mThreadTag.c_str(), android::PRIORITY_NORMAL);
    }

    void pauseThread() {
      std::lock_guard<std::mutex> lock(mLock);
      ALOGD("%s thread %s", __func__, mThreadTag.c_str());
      mRequestSuspend = true;
    }

    void resumeThread() {
      std::lock_guard<std::mutex> lock(mLock);
      ALOGD("%s thread %s", __func__, mThreadTag.c_str());
      mRequestSuspend = false;
    }

    void stopThread() {
      std::lock_guard<std::mutex> lock(mLock);
      ALOGD("%s thread %s", __func__, mThreadTag.c_str());
      mRequestExit = true;
    }

    bool isPaused() { return mRequestSuspend; }

    bool isRequestExit() { return mRequestExit; }

    virtual bool threadLoop() {
      {
        std::lock_guard<std::mutex> lock(mLock);
        if (mRequestExit) {
          return false;
        }
        if (mRequestSuspend) {
          return true;
        }
      }
      return mFuncs();
    }
    std::mutex mLock;

   protected:
    VdecComponent *mVdecComponent;
    std::atomic<bool> mRequestExit;
    std::atomic<bool> mRequestSuspend;
    std::string mThreadTag;
    std::function<bool(void)> mFuncs;
  };

  class SubmitThread : public VdecBaseThread {
   public:
    explicit SubmitThread(VdecComponent *vc, std::function<bool(void)> func)
        : VdecBaseThread(vc, "submit-thread", func) {}
  };

  class DecodeThread : public VdecBaseThread {
   public:
    explicit DecodeThread(VdecComponent *vc, std::function<bool(void)> func)
        : VdecBaseThread(vc, "decode-thread", func) {}
  };

  class WorkerThread : public VdecBaseThread {
   public:
    explicit WorkerThread(VdecComponent *vc, std::function<bool(void)> func)
        : VdecBaseThread(vc, "worker-thread", func) {}
  };

  android::sp<SubmitThread> mSubmitThread;
  android::sp<DecodeThread> mDecodeThread;
  android::sp<WorkerThread> mWorkerThread;

 private:
  std::shared_ptr<C2BlockPool> mPool;
  bool handleDecodingStream();
  bool handleSubmit();
  bool handleWork();
  bool processRemainPic();
  bool reopenVideoEngine();

  std::function<void(std::unique_ptr<C2Work>)> mWorkHandlerCb;
  std::shared_ptr<HwC2Interface<void>::BaseParams> mIntf;
};
}  // namespace android
#endif  // ANDROID_COMPONENTS_INCLUDE_VDECCOMPONENT_H_
