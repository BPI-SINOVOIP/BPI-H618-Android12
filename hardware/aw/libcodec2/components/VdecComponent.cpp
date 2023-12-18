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

//#define LOG_NDEBUG 0
#define ATRACE_TAG ATRACE_TAG_VIDEO
#include <utils/Trace.h>
#define LOG_TAG "VdecComponent"
#include <C2Config.h>
#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2BufferUtils.h>
#include <cutils/properties.h>
#include <gralloc1.h>
#include <hardware/gralloc.h>
#include <inttypes.h>
#include <ion/ion.h>
#include <linux/ion.h>
#include <media/stagefright/foundation/AMessage.h>
#include <memoryAdapter.h>
#include <sc_interface.h>
#include <vdecoder.h>

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <utility>

#include "HwC2Interface.h"
#include "VdecComponent.h"

#include GPU_PUBLIC_INCLUDE
typedef ion_user_handle_t ion_handle_abstract_t;
#define ION_NULL_VALUE (0)
#define GPU_ALIGN_STRIDE (32)
#define MAX_OUTPUT_YUV_DUMP_COUNTS 10
#define WIDTH_LIMIT 2048
#define HEIGHT_LIMIT 2048
#define EOS_COUNTS_LIMIT 100

const char *strDecResult[] = {"VDECODE_RESULT_OK",
                              "VDECODE_RESULT_FRAME_DECODED",
                              "VDECODE_RESULT_CONTINUE",
                              "VDECODE_RESULT_KEYFRAME_DECODED",
                              "VDECODE_RESULT_NO_FRAME_BUFFER",
                              "VDECODE_RESULT_NO_BITSTREAM",
                              "VDECODE_RESULT_RESOLUTION_CHANGE"};

const char *strPixelFormat[] = {
    "DEFAULT", "YUV_PLANER_420", "YUV_PLANER_420", "YUV_PLANER_444", "YV12",
    "NV1",     "YUV_MB32_420",   "MB32_422",       "MB32_444",
};

enum {
  FLAG_FREE_RELEASED_PICTURE_OUTBLOACK = 0x0001U << 1,
};

enum {
  VDECODER_FLAG_FORBID_USE_FBM = 0x0001U << 0,
  VDECODER_FLAG_RETURN_RELEASE_PIC = 0x0001U << 1,
};

namespace android {

namespace {

struct DummyReadView : public C2ReadView {
  DummyReadView() : C2ReadView(C2_NO_INIT) {}
};

}  // namespace

int VdecComponent::getVideoCodecFormat(const char *name, int32_t *codecFormat) {
  if (name == nullptr || codecFormat == nullptr) {
    return -1;
  }
  if (strstr(name, "avc") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_H264;
  } else if (strstr(name, "hevc") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_H265;
  } else if (strstr(name, "mjpeg") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_MJPEG;
  } else if (strstr(name, "mpeg1") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_MPEG1;
  } else if (strstr(name, "mpeg2") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_MPEG2;
  } else if (strstr(name, "mpeg4") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_XVID;
  } else if (strstr(name, "vp9") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_VP9;
  } else if (strstr(name, "vp6") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_VP6;
  } else if (strstr(name, "vp8") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_VP8;
  } else if (strstr(name, "vc1") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_WMV3;
  } else if (strstr(name, "msmpeg4v1") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_MSMPEG4V1;
  } else if (strstr(name, "msmpeg4v2") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_MSMPEG4V2;
  } else if (strstr(name, "divx") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_DIVX3;
  } else if (strstr(name, "xvid") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_XVID;
  } else if (strstr(name, "s263") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_H263;
  } else if (strstr(name, "h263") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_H263;
  } else if (strstr(name, "rxg2") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_RXG2;
  } else if (strstr(name, "wmv1") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_WMV1;
  } else if (strstr(name, "wmv2") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_WMV2;
  } else if (strstr(name, "avs") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_AVS;
  } else if (strstr(name, "avs2") != nullptr) {
    *codecFormat = (int32_t)VIDEO_CODEC_FORMAT_AVS2;
  } else {
    c2_loge("unknown codec format %s.", name);
    return -1;
  }
  return 0;
}

void VdecComponent::evaluatePerformance(enum FLAG_WORK_OPERATION flag) {
  // TODO(kay):
  // evaluate speed of callbacking valid work, average, min, Max,
  // delay time and count
  // evaluate decoding time
  // evaluate process function duration
  // evaluate buffer allocation time, average, min, max
  if (flag == FLAG_SUBMIT_WORK) {
    if (mDebug->mFirstSubmtiStream) {
      mDebug->mFirstSubmtiStream = false;
      GETTIME(&mDebug->mFirstSubmtiStreamSysPts, nullptr);
      GETTIME(&mDebug->mLastSubmtiStreamSysPts, nullptr);
    }
  } else if (flag == FLAG_FILL_WORK) {
    c2_logv("evaluatePerformance fill work.");
    if (mDebug->mFirstFillWork) {
      mDebug->mFirstFillWork = false;
      GETTIME(&mDebug->mFirstFillWorkSysPts, nullptr);
      GETTIME(&mDebug->mLastFillWorkSysPts, nullptr);
      mDebug->mFillWorkCount++;
      int64_t duration = 0;
      TIME_DIFF(mDebug->mFirstSubmtiStreamSysPts, mDebug->mFirstFillWorkSysPts,
                duration);
      c2_logv(
          "[c2 performance] time from initial input to "
          "initial output %llu ms.",
          duration / 1000);
    } else {
      GETTIME(&mDebug->mCurrentFillWorkSysPts, nullptr);
      TIME_DIFF(mDebug->mLastFillWorkSysPts, mDebug->mCurrentFillWorkSysPts,
                mDebug->mFillWorkTimeDiff);
      TIME_DIFF(mDebug->mFirstFillWorkSysPts, mDebug->mCurrentFillWorkSysPts,
                mDebug->mTotalFillWorkTime);
      mDebug->mFillWorkCount++;
      if (mDebug->mFillWorkCount % 30 == 0) {
        mDebug->mAverageFillWorkTime =
            mDebug->mTotalFillWorkTime / mDebug->mFillWorkCount;
        c2_logv(
            "[c2 performance] output count %8llu. "
            "current duration %5llu ms. average fill work duration %5lld ms.",
            mDebug->mFillWorkCount, mDebug->mFillWorkTimeDiff / 1000,
            mDebug->mAverageFillWorkTime / 1000);
      }
      mDebug->mLastFillWorkSysPts = mDebug->mCurrentFillWorkSysPts;
    }
  }
  return;
}

VdecComponent::VdecComponent(const char *name,
                             std::shared_ptr<VdecParameterHelper> paraHelper)
    : mDummyReadView(DummyReadView()),
      mWidth(0u),
      mHeight(0u),
      mStride(0u),
      mInputFrameIndex(0),
      mFirstRound(true),
      mMetValidInputData(false),
      mCountDownEos(0),
      mParaHelper(paraHelper),
      mInitilizeDec(false),
      mCodecFormat(VIDEO_CODEC_FORMAT_UNKNOWN),
      mNeedsCodecConfig(true),
      mInputMetEos(false),
      mOutEosFlag(false),
      mSendOutEosFlag(false),
      mSubmitCounts(0),
      mRenderCounts(0),
      mFlushFlag(false),
      mFrameRate(0u),
      mEmptyPictureLimit(3),
      mValidPictureLimit(3),
      mResolutionChange(false),
      mOutputDelayInited(false),
      mDecoderErrorFlag(false),
      mSignalledError(false),
      mGetFbmRetryCount(0),
      mPool(nullptr),
      mIntf(nullptr) {
  memset(COMPONENT_NAME, 0, sizeof(COMPONENT_NAME));
  strncpy(COMPONENT_NAME, name, strlen(name));
  mDecoder.mIonFd = 0;
  mDecoder.pVideoDec = nullptr;
  mDecoder.memops = nullptr;
  memset(&mDecoder.mVideoStreamInfo, 0x00, sizeof(VideoStreamInfo));
  memset(&mDecoder.mFbmBufInfo, 0X00, sizeof(FbmBufInfo));
  memset(&mDecoder.mVConfig, 0x00, sizeof(VConfig));
  memset(&mDecoder.mBufferInfo, 0x00, sizeof(DECODERCONTEXT::BufferInfo));
  mDecoder.mHadGetVideoFbmBufInfo = false;
  getVideoCodecFormat(name, &mCodecFormat);
  mDebug = std::make_shared<C2DebugHelper>();
  mDebug->mFirstSubmtiStream = true;
  mDebug->mFirstFillWork = true;
  c2_logd("VdecComponent %s init.", name);
#ifdef FILE_DUMP_ENABLE
  OutFileYuvCounts = 0;
  GENERATE_VDEC_FILE_NAMES();
  CREATE_DUMP_FILE(mInFile);
  CREATE_DUMP_FILE(mOutFile);
#endif
}

VdecComponent ::~VdecComponent() { C2_TRACE(); }

void VdecComponent::fillEmptyWork(const std::unique_ptr<C2Work> &work) {
  uint32_t flags = 0;
  work->worklets.front()->output.flags = (C2FrameData::flags_t)flags;
  work->worklets.front()->output.buffers.clear();
  work->worklets.front()->output.ordinal = work->input.ordinal;
  // work->worklets.front()->output.ordinal.frameIndex = mFrameIndex++;
  work->worklets.front()->output.ordinal.frameIndex = 0;
  work->workletsProcessed = 1u;
}

void VdecComponent::transformVideoPictureToOutBlock(
    std::map<const void *, std::shared_ptr<C2GraphicBlock>> *mHoldingOutBlocks,
    VideoPicture *pVideoPicture, std::shared_ptr<C2GraphicBlock> *nOutBlock) {
  // how can we pick out the outBlock??
  // we map picture with outBlock in a map??
  if ((*mHoldingOutBlocks).count(pVideoPicture) == 0u) {
    c2_loge(
        "RequestPicture but C2GraphicBlock for video picture [%p]"
        " is not tracked.",
        pVideoPicture);
    return;
  }

  // work is unique from ccodec. we push back buffer to work.
  // how could we promise unique work? just refer it with &.
  // we will keep C2Buffer whose picture still refered by decoder and free it
  // when not refered anymore.

  *nOutBlock = (*mHoldingOutBlocks)[pVideoPicture];
  (*mHoldingOutBlocks).erase(pVideoPicture);
  SetReleasePicture(mDecoder.pVideoDec, pVideoPicture);
  c2_logv("mHoldingOutBlocks erase picture [%p].", pVideoPicture);
  return;
}

void VdecComponent::checkAllPicturesStatus() {
  c2_logv("decoder holds on [%d] pictures.", mDecoder.mHoldingOutBlocks.size());

  for (auto &it : mDecoder.mHoldingOutBlocks) {
    c2_logv("decoder holds on picture [%p] C2GraphicBlock [%p].", it.first,
            it.second.get());
  }
  c2_logv("codec2 holds on [%d] C2Buffer.", mDecoder.mHoldingC2Buffers.size());

  for (auto &it : mDecoder.mHoldingC2Buffers) {
    c2_logv("codec2 holds on c2buffer <-- picture [%p] C2Buffer [%p].",
            it.first, it.second.get());
  }
}

c2_status_t VdecComponent::submitInputStream(
    const std::unique_ptr<InputData> &inputData) {
  size_t inSize = 0u;
  char *addr = inputData->addr;
  inSize = inputData->size;
  int32_t nRequireSize = inSize;
  char *ppBuf = NULL;
  char *ppRingBuf = NULL;
  int32_t nBufSize = 0;
  int32_t nRingBufSize = 0;
  if (inSize) {
    int32_t ret = RequestVideoStreamBuffer(mDecoder.pVideoDec, nRequireSize,
                                           &ppBuf, &nBufSize, &ppRingBuf,
                                           &nRingBufSize, 0 /* nStreamIndex */);
    if (ret) {
      c2_loge(
          "fail to RequestVideoStreamBuffer "
          "mSubmitCounts = %lld mRenderCounts = %lld",
          mSubmitCounts.load(), mRenderCounts.load());
      return C2_NO_MEMORY;
    }
#ifdef FILE_DUMP_ENABLE
    DUMP_TO_FILE(mInFile, addr, inSize);
#endif
    if (nBufSize >= inSize) {
      memcpy(ppBuf, addr, inSize);
      char *tmp = addr;
      c2_logv("*ppBuf %x %x %x %x. data %x %x %x %x.", *ppBuf, *(ppBuf + 3),
              *(ppBuf + 4), *(ppBuf + 5), *tmp, *(tmp + 3), *(tmp + 4),
              *(tmp + 5));
    } else {
      memcpy(ppBuf, addr, nBufSize);
      memcpy(ppRingBuf, addr + nBufSize, nRingBufSize);
    }
    VideoStreamDataInfo nStreamInfo;
    memset(&nStreamInfo, 0x00, sizeof(VideoStreamDataInfo));
    nStreamInfo.pData = ppBuf;
    nStreamInfo.nLength = inSize;
    nStreamInfo.nPts = (int64_t)inputData->pts;
    nStreamInfo.bIsFirstPart = 1;
    nStreamInfo.bIsLastPart = 1;
    nStreamInfo.nStreamIndex = 0;
    // BAD & UNNECESSARY variable!!
    nStreamInfo.bValid = 1;
    evaluatePerformance(FLAG_SUBMIT_WORK);
    SubmitVideoStreamData(mDecoder.pVideoDec, &nStreamInfo,
                          0 /* nStreamBufIndex */);
    c2_logv("submit video stream. data [%p]. length [%d]. pts [%lld].",
            nStreamInfo.pData, nStreamInfo.nLength, nStreamInfo.nPts);
    free(addr);
    mSubmitCounts++;
  }

  return C2_OK;
}

c2_status_t VdecComponent::initDecoderAddress(
    const std::shared_ptr<C2BlockPool> &pool) {
  C2_TRACE();
  ATRACE_CALL();
  if (pool == nullptr) {
    c2_logd("pool is null");
    return C2_CORRUPTED;
  }
  struct timeval nTimeStart, nTimeEnd;
  int32_t fetchTime;
  uint32_t format = HAL_PIXEL_FORMAT_YV12;
  uint64_t grallocUsage =
      GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_SW_READ_OFTEN;
  C2MemoryUsage usage = C2AndroidMemoryUsage::FromGrallocUsage(grallocUsage);
  std::shared_ptr<C2GraphicBlock> nOutBlock;
  while (mDecoder.mHadGetVideoFbmBufInfo &&
         (mDecoder.mHoldingOutBlocks.size() < (mDecoder.mBufferInfo.nBufNum))) {
    c2_logv("start %d %d", mDecoder.mHoldingOutBlocks.size(),
            mDecoder.mBufferInfo.nBufNum);
    GETTIME(&nTimeStart, nullptr);
    ATRACE_BEGIN("fetchGraphicBlock");
    c2_status_t err =
        pool->fetchGraphicBlock(mStride, mHeight, format, usage, &nOutBlock);
    GETTIME(&nTimeEnd, nullptr);
    TIME_DIFF(nTimeStart, nTimeEnd, fetchTime);
    if (err != C2_OK) {
      c2_loge("fetchGraphicBlock for Output failed with status %d", err);
      return err;
    }
    c2_logv(
        "fetchGraphicBlock provided (%dx%d) "
        "required (%dx%d) with gralloc usage %#llx and fetchTime %d.",
        nOutBlock->width(), nOutBlock->height(), mStride, mHeight, grallocUsage,
        fetchTime);
    ATRACE_END();
    native_handle_t *pBufferHandle =
        const_cast<native_handle_t *>(nOutBlock->handle());
    private_handle_t *hnd = reinterpret_cast<private_handle_t *>(pBufferHandle);
    c2_logv("OutBlock handle share_fd [%d]. width [%d] height [%d] addr [%p].",
            hnd->share_fd, hnd->width, hnd->height, pBufferHandle);
    C2GraphicView wView = nOutBlock->map().get();
    VideoPicture picture;
    memset(&picture, 0x00, sizeof(VideoPicture));
    picture.nWidth = mDecoder.mBufferInfo.nWidth;
    picture.nHeight = mDecoder.mBufferInfo.nHeight;
    picture.nLineStride = mDecoder.mBufferInfo.nWidth;
    picture.pData0 =
        reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_Y]);
    picture.pData1 =
        reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_U]);
    picture.pData2 =
        reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_V]);
    picture.nBufFd = hnd->share_fd;  // distinguish nBufFd from nBufId.
    picture.ePixelFormat = mDecoder.mVConfig.eOutputPixelFormat;
    c2_logv(
        "nWidth [%d] nHeight [%d] pData0 [%p] pData1[%p] pData2[%p] "
        "phyYBufAddr [%p] phyCBufAddr [%p] BufFd [%d] ePixelFormat [%d].",
        picture.nWidth, picture.nHeight, picture.pData0, picture.pData1,
        picture.pData2, reinterpret_cast<void *>(picture.phyYBufAddr),
        reinterpret_cast<void *>(picture.phyCBufAddr), picture.nBufFd,
        picture.ePixelFormat);
    VideoPicture *pOutputPicture = nullptr;
    pOutputPicture = SetVideoFbmBufAddress(mDecoder.pVideoDec, &picture, 0);
    if (pOutputPicture == nullptr) {
      c2_loge("fail to setDecoderOneOutputPicture");
      nOutBlock.reset();
      return C2_CORRUPTED;
    }
    mDecoder.mHoldingOutBlocks[pOutputPicture] = nOutBlock;
    c2_logv("mHoldingOutBlocks holds on picture [%p]", pOutputPicture);
  }
  c2_logd("Inited out buffer.");
  return C2_OK;
}

c2_status_t VdecComponent::ensureDecoderState(
    const std::shared_ptr<C2BlockPool> &pool) {
  C2_TRACE_VERBOSE();
  ATRACE_CALL();
  if (mDecoder.mHadGetVideoFbmBufInfo &&
      ((EmptyPictureNum(mDecoder.pVideoDec, 0) < mEmptyPictureLimit) &&
       (ValidPictureNum(mDecoder.pVideoDec, 0) < mValidPictureLimit))) {
    std::shared_ptr<C2GraphicBlock> nOutBlock;
    struct timeval nTimeStart, nTimeEnd;
    int32_t fetchTime;
    uint32_t format = HAL_PIXEL_FORMAT_YV12;
    uint64_t grallocUsage =
        GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_SW_READ_OFTEN;
    C2MemoryUsage usage = C2AndroidMemoryUsage::FromGrallocUsage(grallocUsage);
    VideoPicture *pReleasedPicture = nullptr;
    if ((pReleasedPicture = RequestReleasePicture(mDecoder.pVideoDec)) !=
        nullptr) {
      mDecoder.mHoldingC2Buffers.erase(pReleasedPicture);
      GETTIME(&nTimeStart, nullptr);
      ATRACE_BEGIN("fetchGraphicBlock");
      c2_status_t err =
          pool->fetchGraphicBlock(mStride, mHeight, format, usage, &nOutBlock);
      GETTIME(&nTimeEnd, nullptr);
      TIME_DIFF(nTimeStart, nTimeEnd, fetchTime);
      if (err != C2_OK) {
        c2_loge("fetchGraphicBlock for Output failed with status %d", err);
        return err;
      }
      c2_logv(
          "fetchGraphicBlock provided (%dx%d) "
          "required (%dx%d) with gralloc usage %#llx and fetchTime %d.",
          nOutBlock->width(), nOutBlock->height(), mStride, mHeight,
          grallocUsage, fetchTime);
      ATRACE_END();
      native_handle_t *pBufferHandle =
          const_cast<native_handle_t *>(nOutBlock->handle());
      private_handle_t *hnd =
          reinterpret_cast<private_handle_t *>(pBufferHandle);
      c2_logv("OutBlock handle share_fd [%d]. width [%d] height [%d] addr[%p].",
              hnd->share_fd, hnd->width, hnd->height, pBufferHandle);
      C2GraphicView wView = nOutBlock->map().get();
      pReleasedPicture->nWidth = mDecoder.mBufferInfo.nWidth;
      pReleasedPicture->nHeight = mDecoder.mBufferInfo.nHeight;
      pReleasedPicture->nLineStride = mDecoder.mBufferInfo.nWidth;
      pReleasedPicture->pData0 =
          reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_Y]);
      pReleasedPicture->pData1 =
          reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_U]);
      pReleasedPicture->pData2 =
          reinterpret_cast<char *>(wView.data()[C2PlanarLayout::PLANE_V]);
      pReleasedPicture->nBufFd = hnd->share_fd;
      pReleasedPicture->ePixelFormat = mDecoder.mVConfig.eOutputPixelFormat;
      c2_logv(
          "nWidth [%d] nHeight [%d] pData0 [%p] pData1[%p] pData2[%p] "
          "phyYBufAddr [%p] phyCBufAddr [%p] BufFd [%d] ePixelFormat [%d].",
          pReleasedPicture->nWidth, pReleasedPicture->nHeight,
          pReleasedPicture->pData0, pReleasedPicture->pData1,
          pReleasedPicture->pData2,
          reinterpret_cast<void *>(pReleasedPicture->phyYBufAddr),
          reinterpret_cast<void *>(pReleasedPicture->phyCBufAddr),
          pReleasedPicture->nBufFd, pReleasedPicture->ePixelFormat);
      VideoPicture *pOutputPicture = nullptr;
      pOutputPicture =
          ReturnReleasePicture(mDecoder.pVideoDec, pReleasedPicture, 0);
      c2_logv("pOutputPicture %p pReleasedPicture %p", pOutputPicture,
              pReleasedPicture);
      if (pOutputPicture == nullptr) {
        c2_loge("fail to setDecoderOneOutputPicture");
        nOutBlock.reset();
        return C2_CORRUPTED;
      }
      // set picture to mHoldingOutBlocks
      mDecoder.mHoldingOutBlocks[pOutputPicture] = nOutBlock;
      c2_logv("mHoldingOutBlocks holds on picture [%p]", pOutputPicture);
    }
  }
  c2_logv("end");
  return C2_OK;
}

c2_status_t VdecComponent::onInit() {
  C2_TRACE();
  ATRACE_CALL();
  // initialize vdecoder, set vconfig and stream info
  if (OK != createDecoder()) return C2_CORRUPTED;
  mSubmitThread =
      new SubmitThread(this, std::bind(&VdecComponent::handleSubmit, this));
  mDecodeThread = new DecodeThread(
      this, std::bind(&VdecComponent::handleDecodingStream, this));
  mWorkerThread =
      new WorkerThread(this, std::bind(&VdecComponent::handleWork, this));
  return C2_OK;
}

c2_status_t VdecComponent::onStart() {
  C2_TRACE();
  c2_logd("onStart start");
  memset(mCodecSpecificData, 0x00, sizeof(mCodecSpecificData));
  mCodecSpecificDataLen = 0;
  mFrameIndex = 0;
  mSubmitCounts = 0;
  mRenderCounts = 0;
  mMetValidInputData = false;
  mDecoderErrorFlag = false;
  mSignalledError = false;
  mInputMetEos = false;
  mOutEosFlag = false;
  mEosCountDown = 0;
  mGetFbmRetryCount = 0;
  mSendOutEosFlag = false;
  if (mInitilizeDec) {
    c2_logd("onStart after initilize! Start threads again!");
    mSubmitThread->resumeThread();
    mDecodeThread->resumeThread();
    mWorkerThread->resumeThread();
  }
  c2_logd("onStart finsih!");
  return C2_OK;
}

c2_status_t VdecComponent::onStop() {
  C2_TRACE();
  ATRACE_CALL();

  mSubmitThread->pauseThread();
  mDecodeThread->pauseThread();
  mWorkerThread->pauseThread();
  if (mDecoder.pVideoDec == nullptr) {
    c2_loge("video decoder is nullptr!!");
    return C2_OK;
  }

  {
    std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
    ResetVideoDecoder(mDecoder.pVideoDec);
  }
  mMetValidInputData = false;
  return C2_OK;
}

void VdecComponent::onReset() {
  C2_TRACE();
  onStop();
}

void VdecComponent::onRelease() {
  C2_TRACE();
  ATRACE_CALL();
  // TODO(kay): post a sem, lock the decoder
  // destroy vdecoder, free memory(maybe use unique ptr)
  if (mDecoder.pVideoDec == nullptr) {
    c2_loge("video decoder is nullptr!!");
    return;
  }

  mSubmitThread->stopThread();
  mDecodeThread->stopThread();
  mWorkerThread->stopThread();
  mDecoder.mHadGetVideoFbmBufInfo = false;
  VideoPicture *pReleasedPicture = nullptr;
  while ((pReleasedPicture = RequestReleasePicture(mDecoder.pVideoDec)) !=
         nullptr) {
    c2_logv("clear ion memory fd = %d", pReleasedPicture->nBufFd);
  }

  {
    std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
    if (mDecoder.pVideoDec != nullptr) {
      DestroyVideoDecoder(mDecoder.pVideoDec);
      mDecoder.pVideoDec = nullptr;
    }
  }

  if (mDecoder.memops != nullptr) {
    CdcMemClose(mDecoder.memops);
    mDecoder.memops = nullptr;
  }
  if (mDecoder.mIonFd > 0) {
    ion_close(mDecoder.mIonFd);
    mDecoder.mIonFd = -1;
  }
  // clear memory, DON'T ignore any one.
  for (auto &it : mDecoder.mHoldingC2Buffers) {
    it.second.reset();
  }
  mDecoder.mHoldingC2Buffers.clear();

  for (auto &it : mDecoder.mHoldingOutBlocks) {
    it.second.reset();
  }
  mDecoder.mHoldingOutBlocks.clear();

  {
    Mutexed<std::list<std::unique_ptr<InputData>>>::Locked queue(
        mSubmitWorkQueue);
    queue->clear();
  }
  mOutputDelayInited = false;
  mCountDownEos = 0;
  c2_logd("VdecComponent release !");
}

c2_status_t VdecComponent::onFlush_sm() {
  C2_TRACE();
  ATRACE_CALL();
  // Lock work, flush work, reset decoder
  // todo(kay): process and decoding thread should be pause.
  mFlushFlag = true;
  ATRACE_BEGIN("pause WorkerThread");
  mWorkerThread->pauseThread();
  ATRACE_END();

  ATRACE_BEGIN("pause DecodeThread");
  mDecodeThread->pauseThread();
  ATRACE_END();

  ATRACE_BEGIN("pause SubmitThread");
  mSubmitThread->pauseThread();
  ATRACE_END();

  mInputMetEos = false;
  mOutEosFlag = false;
  mSendOutEosFlag = false;
  mEosCountDown = 0;
  mGetFbmRetryCount = 0;
  {
    Mutexed<std::list<std::unique_ptr<InputData>>>::Locked queue(
        mSubmitWorkQueue);
    queue->clear();
  }

  mSubmitCounts = 0;
  mRenderCounts = 0;

  if (mDecoder.mVideoStreamInfo.pCodecSpecificData != NULL) {
    free(mDecoder.mVideoStreamInfo.pCodecSpecificData);
    mDecoder.mVideoStreamInfo.pCodecSpecificData = NULL;
    mDecoder.mVideoStreamInfo.nCodecSpecificDataLen = 0;
  }
  memset(mCodecSpecificData, 0x00, sizeof(mCodecSpecificData));
  mCodecSpecificDataLen = 0;

  ATRACE_BEGIN("ResetVideoDecoder");
  {
    std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
    ResetVideoDecoder(mDecoder.pVideoDec);
  }
  ATRACE_END();
  mSubmitThread->resumeThread();
  mDecodeThread->resumeThread();
  mWorkerThread->resumeThread();
  mFlushFlag = false;
  return C2_OK;
}

c2_status_t VdecComponent::createDecoder() {
  C2_TRACE();
  // create vdecoder, initilize memory
  memset(&mDecoder.mVConfig, 0, sizeof(VConfig));
  memset(&mDecoder.mVideoStreamInfo, 0, sizeof(VideoStreamInfo));
  mDecoder.memops = MemAdapterGetOpsS();
  if (mDecoder.memops == nullptr) {
    c2_loge("memops is nullptr");
    return C2_CORRUPTED;
  }
  CdcMemOpen(mDecoder.memops);
  mDecoder.pVideoDec = CreateVideoDecoder();
  if (mDecoder.pVideoDec == nullptr) {
    c2_loge("decoder demom CreateVideoDecoder() error ");
    return C2_CORRUPTED;
  }
  mDecoder.mIonFd = -1;
  mDecoder.mIonFd = ion_open();
  if (mDecoder.mIonFd < 1) {
    c2_loge("fail to open ion device [%d].", mDecoder.mIonFd);
    return C2_CORRUPTED;
  }
  return C2_OK;
}

c2_status_t VdecComponent::initDecoder() {
  C2_TRACE();
  mDecoder.mVideoStreamInfo.eCodecFormat = mCodecFormat;
  // PIXEL_FORMAT_YV12;
  mDecoder.mVConfig.eOutputPixelFormat = DEFAULT_VE_PIXEL_FORMAT;
  mDecoder.mVConfig.nDeInterlaceHoldingFrameBufferNum =
      DEFAULT_DEINTERLACE_HOLD_BUFFER_NUM;
  mDecoder.mVConfig.nDisplayHoldingFrameBufferNum =
      DEFAULT_DISPLAY_HOLD_BUFFER_NUM;
  mDecoder.mVConfig.nRotateHoldingFrameBufferNum =
      DEFAULT_ROTATION_HOLD_BUFFER_NUM;
  mDecoder.mVConfig.nDecodeSmoothFrameBufferNum = DEFAULT_SMOOTH_BUFFER_NUM;
  mDecoder.mVConfig.memops = mDecoder.memops;
  mDecoder.mVideoStreamInfo.bIsFramePackage = DEFAULT_DECODE_FRAME_PACKAGE;
  mDecoder.mVConfig.bGpuBufValid = 1;
  mDecoder.mVConfig.bDispErrorFrame = DEFAULT_DISPLAY_ERROR_FRAME;
  mDecoder.mVConfig.nAlignStride = GPU_ALIGN_STRIDE;

  if (mCodecSpecificDataLen > 0) {
    mDecoder.mVideoStreamInfo.pCodecSpecificData =
        reinterpret_cast<char *>(malloc(mCodecSpecificDataLen));
    memcpy(mDecoder.mVideoStreamInfo.pCodecSpecificData, mCodecSpecificData,
           mCodecSpecificDataLen);
    mDecoder.mVideoStreamInfo.nCodecSpecificDataLen = mCodecSpecificDataLen;
  } else {
    mDecoder.mVideoStreamInfo.pCodecSpecificData = NULL;
    mDecoder.mVideoStreamInfo.nCodecSpecificDataLen = 0;
  }

  if (mCodecFormat == VIDEO_CODEC_FORMAT_WMV3) {
    mDecoder.mVideoStreamInfo.bIsFramePackage = 1;
  }
  // search real width and height.
  std::vector<std::unique_ptr<C2Param>> queried;
  c2_status_t c2err =
      mIntf->query({}, {C2StreamPictureSizeInfo::output::PARAM_TYPE},
                   C2_DONT_BLOCK, &queried);
  if (c2err != C2_OK && queried.size() == 0) {
    ALOGE("Query media type failed => %d", c2err);
  } else {
    uint32_t height =
        ((C2StreamPictureSizeInfo::output *)queried[0].get())->height;
    uint32_t width =
        ((C2StreamPictureSizeInfo::output *)queried[0].get())->width;
    c2_logd("C2StreamPictureSizeInfo width = %d height = %d", width, height);
    if (width > WIDTH_LIMIT || height > HEIGHT_LIMIT) {
      mDecoder.mVConfig.bScaleDownEn = 1;
      mDecoder.mVConfig.nHorizonScaleDownRatio = 1;
      mDecoder.mVConfig.nVerticalScaleDownRatio = 1;
      mDecoder.mVConfig.eCtlAfbcMode = ENABLE_AFBC_JUST_BIG_SIZE;
      c2_logd(
          "enable scale down,nHorizonScaleDownRatio = %d,"
          "nVerticalScaleDownRatio = %d,eCtlAfbcMode %d !",
          mDecoder.mVConfig.nHorizonScaleDownRatio,
          mDecoder.mVConfig.nVerticalScaleDownRatio, mDecoder.mVConfig.eCtlAfbcMode);
    }
  }

  if (mCodecFormat == VIDEO_CODEC_FORMAT_H265
      ||mCodecFormat == VIDEO_CODEC_FORMAT_H264){
      mDecoder.mVConfig.bScaleDownEn = 1;
      c2_logw("enable scale down  for %s",
        (mCodecFormat == VIDEO_CODEC_FORMAT_H265)?"H265":"H264");
  }

  int32_t nRet = InitializeVideoDecoder(
      mDecoder.pVideoDec, &mDecoder.mVideoStreamInfo, &mDecoder.mVConfig);
  if (mDecoder.mVideoStreamInfo.pCodecSpecificData != NULL) {
    free(mDecoder.mVideoStreamInfo.pCodecSpecificData);
  }
  mDecoder.mVideoStreamInfo.pCodecSpecificData = NULL;
  memset(mCodecSpecificData, 0x00, sizeof(mCodecSpecificData));
  mCodecSpecificDataLen = 0;
  if (nRet != 0) {
    c2_loge("decoder demom initialize video decoder fail.");
    DestroyVideoDecoder(mDecoder.pVideoDec);
    mDecoder.pVideoDec = nullptr;
    return C2_NO_INIT;
  }

  c2_logd("initDecoder OK ");
  mInitilizeDec = true;
  c2_logd("Allwinner codec2.0 hw decoder version 1.0.");
  return C2_OK;
}

c2_status_t VdecComponent::cloneAndSend(
    std::shared_ptr<C2GraphicBlock> &nOutBlock, VideoPicture *pPicture,
    uint32_t flags) {
  C2_TRACE_VERBOSE();
  std::unique_ptr<C2Work> work(new C2Work);
  work->result = C2_OK;
  work->worklets.emplace_back(new C2Worklet);
  if (work) {
    int32_t realWidth = pPicture->nRightOffset - pPicture->nLeftOffset;
    int32_t realHeight = pPicture->nBottomOffset - pPicture->nTopOffset;
    int32_t leftOffset = pPicture->nLeftOffset;
    int32_t topOffset = pPicture->nTopOffset;
    if (realWidth != mDecoder.mBufferInfo.nRealWidth) {
      mDecoder.mBufferInfo.nRealWidth = realWidth;
    }
    if (realHeight != mDecoder.mBufferInfo.nRealHeight) {
      mDecoder.mBufferInfo.nRealHeight = realHeight;
    }
    if (leftOffset != mDecoder.mBufferInfo.nLeftOffset) {
      mDecoder.mBufferInfo.nLeftOffset = leftOffset;
    }
    if (topOffset != mDecoder.mBufferInfo.nTopOffset) {
      mDecoder.mBufferInfo.nTopOffset = topOffset;
    }
    std::shared_ptr<C2Buffer> buffer = createGraphicBuffer(
        std::move(nOutBlock),
        C2Rect(realWidth, realHeight).at(leftOffset, topOffset));
    // hold one reference of this buffer in a map
    if (mParaHelper->getColorAspects_l() != nullptr) {
      updateColorAspects(pPicture);
      buffer->setInfo(mParaHelper->getColorAspects_l());
    }
    mDecoder.mHoldingC2Buffers[pPicture] = buffer;
    c2_logv("mHoldingC2Buffers hold on Picture [%p]", pPicture);
    work->worklets.front()->output.flags = (C2FrameData::flags_t)flags;
    work->worklets.front()->output.buffers.clear();
    work->worklets.front()->output.buffers.push_back(buffer);
    work->worklets.front()->output.ordinal.timestamp = pPicture->nPts;
    work->worklets.front()->output.ordinal.frameIndex = mFrameIndex++;
    c2_logv("cloneAndSend with buffers size %d, frame index %llu pts %lld.",
            work->worklets.front()->output.buffers.size(),
            work->worklets.front()->output.ordinal.frameIndex.peekull() - 1,
            pPicture->nPts);
    evaluatePerformance(FLAG_FILL_WORK);
    work->input.ordinal.frameIndex = mInputFrameIndex + 1;
    ATRACE_BEGIN("mWorkHandlerCb");
    mWorkHandlerCb(std::move(work));
    mRenderCounts++;
    ATRACE_END();
    return C2_OK;
  } else {
    return C2_CORRUPTED;
  }
}

void VdecComponent::process(const std::unique_ptr<C2Work> &work,
                            const std::shared_ptr<C2BlockPool> &pool) {
  C2_TRACE();
  ATRACE_BEGIN("process");
  int32_t num = 0;

  if (mSignalledError) {
    work->result = C2_BAD_VALUE;
    return;
  }

  if (mDecoderErrorFlag) {
    mSignalledError = true;
    work->workletsProcessed = 1u;
    work->result = C2_CORRUPTED;
    return;
  }

  if (mDecoder.mHadGetVideoFbmBufInfo) {
    if (!mOutputDelayInited && mDecoder.mBufferInfo.nBufNum != 0) {
      int32_t actualNum = (mDecoder.mBufferInfo.nBufNum < 22)
                                       ? 22
                                       : mDecoder.mBufferInfo.nBufNum;

      C2PortActualDelayTuning::output outputDelay(
          actualNum - kSmoothnessFactor - kRenderingDepth);
      std::vector<std::unique_ptr<C2SettingResult>> failures;
      c2_status_t err = mIntf->config({&outputDelay}, C2_MAY_BLOCK, &failures);
      if (err == OK) {
        c2_logd("config ok! outputDelay = %d", outputDelay.value);
        work->worklets.front()->output.configUpdate.push_back(
            C2Param::Copy(outputDelay));
      } else {
        c2_logd("config failed! err = %d mDecoder.mBufferInfo.nBufNum = %d",
                err, mDecoder.mBufferInfo.nBufNum);
      }
      mOutputDelayInited = true;
    }
  }

  if (mPool == nullptr) {
    c2_logd("reset mPool");
    mPool = pool;
  }

  // Initialize output work
  bool codecConfig = (work->input.flags & C2FrameData::FLAG_CODEC_CONFIG) != 0;
  bool eos = ((work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0);
  c2_logv("codecConfig is %d eos %d", codecConfig, eos);
  if (!codecConfig && !eos) {
    mMetValidInputData = true;
  }
  work->result = C2_OK;
  fillEmptyWork(work);

  if (codecConfig && !mInitilizeDec) {
    size_t inSize = 0u;
    C2ReadView rView = mDummyReadView;
    if (!work->input.buffers.empty()) {
      rView = work->input.buffers[0]->data().linearBlocks().front().map().get();
      inSize = rView.capacity();
      if (inSize && rView.error()) {
        c2_loge("read view map failed %d", rView.error());
        mSignalledError = true;
        work->workletsProcessed = 1u;
        work->result = C2_CORRUPTED;
        ATRACE_END();
        return;
      }
    }
    memcpy(mCodecSpecificData + mCodecSpecificDataLen, rView.data(), inSize);
    mCodecSpecificDataLen += inSize;
    c2_logd("mCodecSpecificDataLen = %d", mCodecSpecificDataLen);
#ifdef FILE_DUMP_ENABLE
    DUMP_TO_FILE(mInFile, mCodecSpecificData, inSize);
#endif
    c2_logd("It's a csd data, now return");
    return;
  }

  if (!mInitilizeDec) {
    {
      std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
      ATRACE_BEGIN("initDecoder");
      c2_status_t ret = initDecoder();
      if (ret != C2_OK) {
        c2_loge("initDecoder failed with ret %d", ret);
        mSignalledError = true;
        work->workletsProcessed = 1u;
        work->result = C2_CORRUPTED;
        ATRACE_END();
        return;
      }
    }
    ATRACE_END();
    mSubmitThread->startThread();
    mDecodeThread->startThread();
    mWorkerThread->startThread();
  }

  {
    Mutexed<std::list<std::unique_ptr<InputData>>>::Locked queue(
        mSubmitWorkQueue);
    while (!mInputMetEos && queue->size() > 100) {
      usleep(10 * 1000);
      num++;
      c2_logd("submit usleep num = %d ", num);
      if (num == 100) {
        break;
      }
    }
    std::unique_ptr<InputData> inputData(new InputData{nullptr, 0, 0, false});
    if (!work->input.buffers.empty()) {
      C2ReadView source =
          work->input.buffers[0]->data().linearBlocks().front().map().get();
      inputData->addr = reinterpret_cast<char *>(malloc(source.capacity()));
      memcpy(inputData->addr, const_cast<uint8_t *>(source.data()),
             source.capacity());
      inputData->size = source.capacity();
    } else {
      inputData->addr = nullptr;
      inputData->size = 0;
    }
    inputData->pts = (int64_t)work->input.ordinal.timestamp.peeku();
    inputData->eos = eos;
    c2_logv("input pts = %lld mSubmitWorkQueue size = %d", inputData->pts,
            queue->size());
    queue->push_back(std::move(inputData));
    mInputFrameIndex = work->input.ordinal.frameIndex.peekull();
    c2_logv("input mInputFrameIndex = %llu", mInputFrameIndex.load());
  }
  if (eos) {
    c2_logd("get eos!! pts = %lld",
            (int64_t)work->input.ordinal.timestamp.peeku());
    // met eos, WorkerThread should use drainInternal instead.
    if (!mMetValidInputData) {
      work->worklets.front()->output.buffers.clear();
      work->worklets.front()->output.ordinal.frameIndex = mFrameIndex++;
      work->worklets.front()->output.ordinal.timestamp =
          work->input.ordinal.timestamp.peeku();
      uint32_t flags = 0;
      flags |= C2FrameData::FLAG_END_OF_STREAM;
      work->worklets.front()->output.flags = (C2FrameData::flags_t)flags;
      c2_logd("now get only eos!");
      mInputMetEos = true;
      mOutEosFlag = true;
      mSendOutEosFlag = true;
    }
  }

  usleep(2 * 1000);
  ATRACE_END();
  return;
}

// a block function. drain rest pictures totally.
// TODO(kay): we need a non-block way to realize it.
// TODO(kay): check decoding thread status and flush command.
// take responding actions.
c2_status_t VdecComponent::drainInternal(
    uint32_t drainMode, const std::shared_ptr<C2BlockPool> & /*pool*/,
    const std::unique_ptr<C2Work> & /*work*/) {
  C2_TRACE_VERBOSE();
  if (drainMode != C2Component::DRAIN_COMPONENT_WITH_EOS) {
    c2_logw(
        "other drain mode except DRAIN_COMPONENT_WITH_EOS "
        "is not supported yet.");
    return C2_OK;  // Maybe C2_OMITTED
  }
  if (mEosCountDown > EOS_COUNTS_LIMIT) {
    return C2_OK;
  }

  int valid_picture_num = 0;
  if ((valid_picture_num = ValidPictureNum(mDecoder.pVideoDec, 0)) > 0) {
    if (!mInputMetEos) {
      c2_logd("jump out of drainInternal");
      return C2_OK;
    }
    if (mFlushFlag) {
      c2_logd("mFlushFlag is set true!");
      return C2_OK;
    }
    VideoPicture *pVideoPicture = RequestPicture(mDecoder.pVideoDec, 0);
    if (nullptr == pVideoPicture) {
      c2_loge(
          "fail to RequestPicture while "
          "its ValidPictureNum is NOT zero.");
      usleep(1 * 1000);
      return C2_OK;
    }
    c2_logd("RequestPicture pts = %lld", pVideoPicture->nPts);
#ifdef FILE_DUMP_ENABLE
    if (OutFileYuvCounts++ < MAX_OUTPUT_YUV_DUMP_COUNTS) {
      DUMP_TO_FILE(mOutFile, pVideoPicture->pData0,
                   pVideoPicture->nWidth * pVideoPicture->nHeight * 3 / 2);
    }
#endif
    std::shared_ptr<C2GraphicBlock> nOutBlock = nullptr;
    transformVideoPictureToOutBlock(&mDecoder.mHoldingOutBlocks, pVideoPicture,
                                    &nOutBlock);
    if (nOutBlock == nullptr) {
      c2_logw("fail to transformVideoPictureToOutBlock");
      return C2_OK;
    }
    c2_status_t ret =
        cloneAndSend(nOutBlock, pVideoPicture, C2FrameData::FLAG_END_OF_STREAM);
    if (ret != C2_OK) {
      c2_loge("cloneAndSend failed with eos");
    }
    mSendOutEosFlag = true;
    c2_logd("now exit, pts %lld !", pVideoPicture->nPts);
  } else {
    if (++mEosCountDown > EOS_COUNTS_LIMIT) {
      std::unique_ptr<C2Work> work(new C2Work);
      work->worklets.emplace_back(new C2Worklet);
      work->input.ordinal.frameIndex = mFrameIndex.load();
      work->result = C2_OK;
      work->worklets.front()->output.flags = C2FrameData::FLAG_END_OF_STREAM;
      work->worklets.front()->output.buffers.clear();
      work->worklets.front()->output.ordinal.frameIndex = mFrameIndex++;
      mWorkHandlerCb(std::move(work));
      mSendOutEosFlag = true;
      c2_logd("send eos back for mEosCountDown reach %d", mEosCountDown);
    }
  }

  return C2_OK;
}

bool VdecComponent::handleSubmit() {
  C2_TRACE_VERBOSE();
  if (!mInitilizeDec) {
    c2_logv("decoder no initialized");
    usleep(2 * 1000);
    return true;
  }
  std::unique_ptr<InputData> inputData = nullptr;
  {
    Mutexed<std::list<std::unique_ptr<InputData>>>::Locked queue(
        mSubmitWorkQueue);
    if (!queue->empty()) {
      inputData = std::move(queue->front());
      queue->pop_front();
    }
  }

  if (inputData == nullptr) {
    usleep(5 * 1000);
    return true;
  }

  ATRACE_BEGIN("submitInputStream");
  {
    std::lock_guard<std::mutex> submitLock(mSubmitThread->mLock);
    c2_status_t status = submitInputStream(inputData);
    if (status == C2_NO_MEMORY) {
      // ALOGD("wait for stream buffer for pts %lld !", inputData->pts);
      Mutexed<std::list<std::unique_ptr<InputData>>>::Locked queue(
          mSubmitWorkQueue);
      queue->push_front(std::move(inputData));
      usleep(2 * 1000);
      return true;
    }
  }
  if (inputData->eos) {
    mInputMetEos = true;
    c2_logd("mInputMetEos set to true");
  }
  ATRACE_END();
  usleep(1 * 1000);
  return true;
}

FbmBufInfo *VdecComponent::getVideoFbmBufInfo() {
  if (!mInitilizeDec) {
    return nullptr;
  } else {
    return GetVideoFbmBufInfo(mDecoder.pVideoDec);
  }
}

bool VdecComponent::handleWork() {
  C2_TRACE_VERBOSE();
  if (!mInitilizeDec) {
    c2_logv("decoder no initialized");
    usleep(2 * 1000);
    return true;
  }

  if (mFlushFlag || mSendOutEosFlag) {
    c2_logv("mFlushFlag is set true!");
    usleep(2 * 1000);
    return true;
  }

  if (mPool == nullptr) {
    c2_logv("has no mPool");
    usleep(10 * 1000);
    return true;
  }

  {
    std::lock_guard<std::mutex> workLock(mWorkerThread->mLock);

    if (!mDecoder.mHadGetVideoFbmBufInfo) {
      FbmBufInfo *pFbmBufInfo = getVideoFbmBufInfo();
      if (pFbmBufInfo != NULL) {
        c2_logd(
            "buffer info: buf num [%d], buf width [%d], buf height"
            " [%d], bProgressiveFlag [%s], bIsSoftDecoderFlag [%s]",
            pFbmBufInfo->nBufNum, pFbmBufInfo->nBufWidth,
            pFbmBufInfo->nBufHeight,
            pFbmBufInfo->bProgressiveFlag ? "true" : "false",
            pFbmBufInfo->bIsSoftDecoderFlag ? "true" : "false");
        c2_logd(
            "buffer size: nTopOffset [%d] nBottomOffset [%d] "
            "nLeftOffset[%d] nRightOffset [%d]",
            pFbmBufInfo->nTopOffset, pFbmBufInfo->nBottomOffset,
            pFbmBufInfo->nLeftOffset, pFbmBufInfo->nRightOffset);
        mDecoder.mBufferInfo.nRealHeight =
            pFbmBufInfo->nBottomOffset - pFbmBufInfo->nTopOffset;
        mDecoder.mBufferInfo.nRealWidth =
            pFbmBufInfo->nRightOffset - pFbmBufInfo->nLeftOffset;
        mDecoder.mBufferInfo.nLeftOffset = pFbmBufInfo->nLeftOffset;
        mDecoder.mBufferInfo.nTopOffset = pFbmBufInfo->nTopOffset;
        mDecoder.mBufferInfo.nBufNum = pFbmBufInfo->nBufNum;
        mStride = mWidth = mDecoder.mBufferInfo.nWidth = pFbmBufInfo->nBufWidth;
        mHeight = mDecoder.mBufferInfo.nHeight = pFbmBufInfo->nBufHeight;
        mDecoder.mHadGetVideoFbmBufInfo = true;
      } else {
        c2_logv("pFbmBufInfo is null, waiting...");
        mGetFbmRetryCount++;
        if (mGetFbmRetryCount > 300) {
          std::unique_ptr<C2Work> work(new C2Work);
          work->worklets.emplace_back(new C2Worklet);
          work->input.ordinal.frameIndex = mFrameIndex.load();
          work->result = C2_OK;
          work->worklets.front()->output.flags =
              C2FrameData::FLAG_END_OF_STREAM;
          work->worklets.front()->output.buffers.clear();
          work->worklets.front()->output.ordinal.frameIndex = mFrameIndex++;
          mWorkHandlerCb(std::move(work));
          mSendOutEosFlag = true;
          mGetFbmRetryCount = 0;
          c2_logw("may be not get fbm info ,set out eos flag.");
        }
        usleep(10 * 1000);
        return true;
      }
    }

    c2_status_t ret = C2_CORRUPTED;
    int valid_picture_num = ValidPictureNum(mDecoder.pVideoDec, 0);
    if (!mInputMetEos || !mOutEosFlag || valid_picture_num > 1) {
      if (mFirstRound) {
        initDecoderAddress(mPool);
        mFirstRound = false;
      }

      c2_logv(
          "start this round RequestPicture valid_picture_num = %d empty(%d) "
          "release(%d)",
          valid_picture_num, EmptyPictureNum(mDecoder.pVideoDec, 0),
          ReleasePictureNum(mDecoder.pVideoDec, 0));
      // Condition judge greater than 1 is aim to left at least one valid
      // picture to be done in drainInternal
      if (valid_picture_num > 1) {
        c2_logv("loop inside ValidPictureNum %d", valid_picture_num);
        VideoPicture *pVideoPicture = RequestPicture(mDecoder.pVideoDec, 0);
        if (nullptr == pVideoPicture) {
          c2_loge(
              "loop fail to RequestPicture while its "
              "ValidPictureNum is NOT zero.");
          return true;
        }
        c2_logv("RequestPicture pts = %lld", pVideoPicture->nPts);

        if (mFrameRate == 0) {
          VideoPicture *nextPicture = NextPictureInfo(mDecoder.pVideoDec, 0);
          if (nextPicture != nullptr) {
            int pts_diff = nextPicture->nPts - pVideoPicture->nPts;
            mFrameRate = 1000 * 1000 / pts_diff;
            c2_logd("mFrameRate = %u", mFrameRate);
          }
        }
#ifdef FILE_DUMP_ENABLE
        if (OutFileYuvCounts++ < MAX_OUTPUT_YUV_DUMP_COUNTS) {
          DUMP_TO_FILE(mOutFile, pVideoPicture->pData0,
                       pVideoPicture->nWidth * pVideoPicture->nHeight * 3 / 2);
        }
#endif
        std::shared_ptr<C2GraphicBlock> nOutBlock = nullptr;
        transformVideoPictureToOutBlock(&mDecoder.mHoldingOutBlocks,
                                        pVideoPicture, &nOutBlock);
        if (nOutBlock == nullptr) {
          c2_logw("fail to transformVideoPictureToOutBlock");
          return true;
        }
        {
          VideoPicture *nextPicture = NextPictureInfo(mDecoder.pVideoDec, 0);
          if (nextPicture != nullptr) {
            c2_logv("current pts is %lld next pts is %lld", pVideoPicture->nPts,
                  nextPicture->nPts);
            if (pVideoPicture->nPts == nextPicture->nPts) {
              c2_logw("current pts is equal to nextPicture, do not render.");
              nOutBlock.reset();
              return true;
            }
          }
        }
        ret = cloneAndSend(nOutBlock, pVideoPicture, 0);
      }
      c2_logv("finish this round RequestPicture");
    } else {
      ret =
          drainInternal(C2Component::DRAIN_COMPONENT_WITH_EOS, mPool, nullptr);
    }
    struct timeval nTimeStart, nTimeEnd;
    int32_t fetchTime;
    GETTIME(&nTimeStart, nullptr);
    ret = ensureDecoderState(mPool);
    GETTIME(&nTimeEnd, nullptr);
    TIME_DIFF(nTimeStart, nTimeEnd, fetchTime);
    if (ret != C2_OK) {
      c2_loge("ensureDecoderState return %d", ret);
      return true;
    }
  }
  usleep(2 * 1000);
  c2_logv("handleWork end");
  return true;
}

bool VdecComponent::handleDecodingStream() {
  C2_TRACE_VERBOSE();
  if (!mInitilizeDec || mDecoderErrorFlag) {
    usleep(2 * 1000);
    return true;
  }
  // lock the decoder
  struct timeval nTimeStart, nTimeEnd;
  int32_t fetchTime;
  int32_t decodeResult;
  {
    std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
    GETTIME(&nTimeStart, nullptr);
    ATRACE_BEGIN("DecodeVideoStream");
    decodeResult = DecodeVideoStream(mDecoder.pVideoDec, 0, 0, 0, 0);
    ATRACE_END();
    GETTIME(&nTimeEnd, nullptr);
    TIME_DIFF(nTimeStart, nTimeEnd, fetchTime);
  }
//#define DEBUG_SHOW_VE_INFO
#ifdef DEBUG_SHOW_VE_INFO
  if (decodeResult < 0) {
    c2_loge("decodeResult = %d is negative!", decodeResult);
    mDecoderErrorFlag = true;
  } else {
    c2_logd(
        "decodeResult [%s], fetchTime [%d]. ValidPictureNum [%d] "
        "Empty [%d] Release [%d]",
        strDecResult[decodeResult], fetchTime,
        ValidPictureNum(mDecoder.pVideoDec, 0),
        EmptyPictureNum(mDecoder.pVideoDec, 0),
        ReleasePictureNum(mDecoder.pVideoDec, 0));
  }
#endif
  int32_t nTemp = 0;
  if (!mOutEosFlag && mInputMetEos &&
      (decodeResult == VDECODE_RESULT_NO_BITSTREAM) &&
      (mDecoder.mHadGetVideoFbmBufInfo || (mSubmitCounts == 1))) {
    while ((nTemp != VDECODE_RESULT_NO_BITSTREAM)) {
      {
        // std::lock_guard<std::mutex> decLock(mDecodeThread->mLock);
        nTemp = DecodeVideoStream(mDecoder.pVideoDec, 1, 0, 0, 0);
      }
      c2_logd("set input eos ,nTemp %d !", nTemp);
      if (nTemp == VDECODE_RESULT_RESOLUTION_CHANGE) {
        c2_logd("input eos and decode thread detect resolution change.");
        mResolutionChange = true;
        mWorkerThread->pauseThread();
        while (!mWorkerThread->isPaused()) {
          usleep(100 * 1000);
        }
        processRemainPic();
        reopenVideoEngine();
        mDecoder.mHadGetVideoFbmBufInfo = false;
        mGetFbmRetryCount = 0;
        mFirstRound = true;
        mWorkerThread->resumeThread();
        c2_logd("decode thread detect resolution change finish !");
      }
      usleep(1 * 1000);
    }
    mOutEosFlag = true;
    c2_logv("decode result %d !", nTemp);
    mDecodeThread->pauseThread();
    return true;
  }

  if (decodeResult == VDECODE_RESULT_RESOLUTION_CHANGE) {
    c2_logd("decode thread detect resolution change.");
    mResolutionChange = true;
    mWorkerThread->pauseThread();
    while (!mWorkerThread->isPaused()) {
      usleep(100 * 1000);
    }
    processRemainPic();
    reopenVideoEngine();
    mDecoder.mHadGetVideoFbmBufInfo = false;
    mGetFbmRetryCount = 0;
    mFirstRound = true;
    mWorkerThread->resumeThread();
    c2_logd("decode thread detect resolution change finish !");
  }

  usleep(2 * 1000);
  return true;
}

bool VdecComponent::processRemainPic() {
  C2_TRACE();
  while (ValidPictureNum(mDecoder.pVideoDec, 0) > 0) {
    c2_logv("loop inside ValidPictureNum %d",
            ValidPictureNum(mDecoder.pVideoDec, 0));
    VideoPicture *pVideoPicture = RequestPicture(mDecoder.pVideoDec, 0);
    if (nullptr == pVideoPicture) {
      c2_loge(
          "loop fail to RequestPicture while its "
          "ValidPictureNum is NOT zero.");
      return true;
    }
    c2_logd("RequestPicture pts = %lld", pVideoPicture->nPts);

    std::shared_ptr<C2GraphicBlock> nOutBlock = nullptr;
    transformVideoPictureToOutBlock(&mDecoder.mHoldingOutBlocks, pVideoPicture,
                                    &nOutBlock);
    if (nOutBlock == nullptr) {
      c2_logw("fail to transformVideoPictureToOutBlock");
      return true;
    }
    cloneAndSend(nOutBlock, pVideoPicture, 0);
  }

  return true;
}

bool VdecComponent::reopenVideoEngine() {
  C2_TRACE();
  if (mDecoder.mVideoStreamInfo.pCodecSpecificData != NULL) {
    free(mDecoder.mVideoStreamInfo.pCodecSpecificData);
    mDecoder.mVideoStreamInfo.pCodecSpecificData = NULL;
    mDecoder.mVideoStreamInfo.nCodecSpecificDataLen = 0;
    memset(mCodecSpecificData, 0x00, sizeof(mCodecSpecificData));
    mCodecSpecificDataLen = 0;
  }

  VideoPicture *pReleasedPicture = nullptr;
  while ((pReleasedPicture = RequestReleasePicture(mDecoder.pVideoDec)) !=
         nullptr) {
    c2_logd("clear ion memory fd = %d", pReleasedPicture->nBufFd);
  }

  // clear memory, DON'T ignore any one.
  for (auto &it : mDecoder.mHoldingC2Buffers) {
    it.second.reset();
  }
  mDecoder.mHoldingC2Buffers.clear();

  for (auto &it : mDecoder.mHoldingOutBlocks) {
    it.second.reset();
  }
  mDecoder.mHoldingOutBlocks.clear();

  ReopenVideoEngine(mDecoder.pVideoDec, &mDecoder.mVConfig,
                    &mDecoder.mVideoStreamInfo);

  return true;
}

c2_status_t VdecComponent::drain(uint32_t drainMode,
                                 const std::shared_ptr<C2BlockPool> &pool) {
  return drainInternal(drainMode, pool, nullptr);
}

std::shared_ptr<C2Buffer> VdecComponent::createGraphicBuffer(
    const std::shared_ptr<C2GraphicBlock> &block) {
  return createGraphicBuffer(block, C2Rect(block->width(), block->height()));
}

std::shared_ptr<C2Buffer> VdecComponent::createGraphicBuffer(
    const std::shared_ptr<C2GraphicBlock> &block, const C2Rect &crop) {
  return C2Buffer::CreateGraphicBuffer(block->share(crop, ::C2Fence()));
}

std::shared_ptr<C2Buffer> VdecComponent::createLinearBuffer(
    const std::shared_ptr<C2LinearBlock> &block) {
  return createLinearBuffer(block, block->offset(), block->size());
}

std::shared_ptr<C2Buffer> VdecComponent::createLinearBuffer(
    const std::shared_ptr<C2LinearBlock> &block, size_t offset, size_t size) {
  return C2Buffer::CreateLinearBuffer(block->share(offset, size, ::C2Fence()));
}

void VdecComponent::updateColorAspects(VideoPicture *pVideoPicture) {
  C2_TRACE_VERBOSE();
  int32_t mTransferCharacteristics = 0;
  int32_t mMatrixCoeffs = 0;
  int32_t mVideoFullRange = 0;
  int32_t mColourPrimaries = 0;

  C2Color::range_t range;
  C2Color::matrix_t matrix;
  C2Color::primaries_t primaries;
  C2Color::transfer_t transfer;

  mMatrixCoeffs = pVideoPicture->matrix_coeffs;
  mVideoFullRange = pVideoPicture->video_full_range_flag;
  mColourPrimaries = pVideoPicture->colour_primaries;
  mTransferCharacteristics = pVideoPicture->transfer_characteristics;
  c2_logv(
      "mTransferCharacteristics %d, mMatrixCoeffs %d, mVideoFullRange %d, "
      "mColourPrimaries %d !",
      mTransferCharacteristics, mMatrixCoeffs, mVideoFullRange,
      mColourPrimaries);

  switch (mTransferCharacteristics) {
    case VIDEO_TRANSFER_LINEAR:
      transfer = C2Color::TRANSFER_LINEAR;  //*1
      break;
    case VIDEO_TRANSFER_SRGB:
      transfer = C2Color::TRANSFER_SRGB;  //*2
      break;
    case VIDEO_TRANSFER_SMPTE_170M:
      transfer = C2Color::TRANSFER_170M;  //*3
      break;
    case VIDEO_TRANSFER_GAMMA2_2:
      transfer = C2Color::TRANSFER_GAMMA22;  //*4
      break;
    case VIDEO_TRANSFER_GAMMA2_8:
      transfer = C2Color::TRANSFER_GAMMA28;  //*5
      break;
    case VIDEO_TRANSFER_ST2084:
      transfer = C2Color::TRANSFER_ST2084;  //*6
      break;
    case VIDEO_TRANSFER_HLG:
      transfer = C2Color::TRANSFER_HLG;  //*7
      break;
    case VIDEO_TRANSFER_SMPTE_240M:
      transfer = C2Color::TRANSFER_240M;  //*0x40
      break;
    case VIDEO_TRANSFER_IEC61966:
      transfer = C2Color::TRANSFER_XVYCC;  //*0x41
      break;
    case VIDEO_TRANSFER_BT1361:
      transfer = C2Color::TRANSFER_170M;  //*3
      break;
    case VIDEO_TRANSFER_ST428_1:
      transfer = C2Color::TRANSFER_ST428;  //*0x43
      break;
    case VIDEO_TRANSFER_LOGARITHMIC_0:
    case VIDEO_TRANSFER_LOGARITHMIC_1:
    case VIDEO_TRANSFER_BT1361_EXTENDED:
    case VIDEO_TRANSFER_BT2020_0:
    case VIDEO_TRANSFER_BT2020_1:
      transfer = C2Color::TRANSFER_OTHER;  //*0xff
      break;
    case VIDEO_TRANSFER_RESERVED_0:
    case VIDEO_TRANSFER_UNSPECIFIED:
    case VIDEO_TRANSFER_RESERVED_1:
    default:
      transfer = C2Color::TRANSFER_UNSPECIFIED;  //*0
      break;
  }

  switch (mMatrixCoeffs) {
    case VIDEO_MATRIX_COEFFS_BT709:
      matrix = C2Color::MATRIX_BT709;  //*1
      break;
    case VIDEO_MATRIX_COEFFS_BT470M:
      matrix = C2Color::MATRIX_FCC47_73_682;  //*2
      break;
    case VIDEO_MATRIX_COEFFS_BT601_625_0:
    case VIDEO_MATRIX_COEFFS_BT601_625_1:
      matrix = C2Color::MATRIX_BT601;  //*3
      break;
    case VIDEO_MATRIX_COEFFS_SMPTE_240M:
      matrix = C2Color::MATRIX_240M;  //*4
      break;
    case VIDEO_MATRIX_COEFFS_BT2020:
      matrix = C2Color::MATRIX_BT2020;  //*5
      break;
    case VIDEO_MATRIX_COEFFS_BT2020_CONSTANT_LUMINANCE:
      matrix = C2Color::MATRIX_BT2020_CONSTANT;  //*6
      break;
    case VIDEO_MATRIX_COEFFS_SOMPATE:
    case VIDEO_MATRIX_COEFFS_CD_NON_CONSTANT_LUMINANCE:
    case VIDEO_MATRIX_COEFFS_CD_CONSTANT_LUMINANCE:
    case VIDEO_MATRIX_COEFFS_BTICC:
    case VIDEO_MATRIX_COEFFS_YCGCO:
      matrix = C2Color::MATRIX_OTHER;  //*0xff
      break;
    case VIDEO_MATRIX_COEFFS_UNSPECIFIED_0:
    case VIDEO_MATRIX_COEFFS_RESERVED_0:
    case VIDEO_MATRIX_COEFFS_IDENTITY:
    default:
      matrix = C2Color::MATRIX_UNSPECIFIED;  //*0
      break;
  }

  switch (mVideoFullRange) {
    case VIDEO_FULL_RANGE_LIMITED:
      range = C2Color::RANGE_LIMITED;  //*2
      break;
    case VIDEO_FULL_RANGE_FULL:
      range = C2Color::RANGE_FULL;  //*1
      break;
    default: {
      range = C2Color::RANGE_UNSPECIFIED;  //*0
      break;
    }
  }

  switch (mColourPrimaries) {
    case 1:
      primaries = C2Color::PRIMARIES_BT709;  //*1
      break;
    case 4:
      primaries = C2Color::PRIMARIES_BT470_M;  //*2
      break;
    case 5:
      primaries = C2Color::PRIMARIES_BT601_625;  //*3
      break;
    case 6:
      primaries = C2Color::PRIMARIES_BT601_525;  //*4
      break;
    case 7:
      primaries = C2Color::PRIMARIES_OTHER;  //*0xff
      break;
    case 8:
      primaries = C2Color::PRIMARIES_GENERIC_FILM;  //*5
      break;
    case 9:
      primaries = C2Color::PRIMARIES_BT2020;  //*6
      break;
    case 0:
    case 3:
    case 2:
    default: {
      primaries = C2Color::PRIMARIES_UNSPECIFIED;  //*0
      break;
    }
  }

  if (mParaHelper->mColorAspects != nullptr) {
    mParaHelper->mColorAspects->range = range;
    mParaHelper->mColorAspects->matrix = matrix;
    mParaHelper->mColorAspects->primaries = primaries;
    mParaHelper->mColorAspects->transfer = transfer;
    c2_logv(
        "update color aspect ,[video range %d], [matrix coeff %d], [color "
        "primaries %d], [transfer characteristics %d]",
        mParaHelper->mColorAspects->range, mParaHelper->mColorAspects->matrix,
        mParaHelper->mColorAspects->primaries,
        mParaHelper->mColorAspects->transfer);
  }
  return;
}

}  // namespace android
