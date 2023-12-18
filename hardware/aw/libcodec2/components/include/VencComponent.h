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

#ifndef ANDROID_COMPONENTS_INCLUDE_VENCCOMPONENT_H_
#define ANDROID_COMPONENTS_INCLUDE_VENCCOMPONENT_H_

#include <HwC2Component.h>
#include <VencParameterHelper.h>
#include <utils/Vector.h>
#include <vencoder.h>

#include <map>
#include <memory>

namespace android {

#define MAX_B_FRAMES 1
/*
#define DEFAULT_MAX_REF_FRM         2
#define DEFAULT_MAX_REORDER_FRM     0
*/
#define DEFAULT_QP_MIN 10
#define DEFAULT_QP_MAX 40
#define DEFAULT_MAX_BITRATE 240000000
/*
#define DEFAULT_MAX_SRCH_RANGE_X    256
#define DEFAULT_MAX_SRCH_RANGE_Y    256
#define DEFAULT_MAX_FRAMERATE       120000
#define DEFAULT_NUM_CORES           1
#define DEFAULT_NUM_CORES_PRE_ENC   0
#define DEFAULT_FPS                 30
#define DEFAULT_ENC_SPEED           IVE_NORMAL

#define DEFAULT_MEM_REC_CNT         0
#define DEFAULT_RECON_ENABLE        0
#define DEFAULT_CHKSUM_ENABLE       0
#define DEFAULT_START_FRM           0
#define DEFAULT_NUM_FRMS            0xFFFFFFFF
#define DEFAULT_INP_COLOR_FORMAT       IV_YUV_420SP_VU
#define DEFAULT_RECON_COLOR_FORMAT     IV_YUV_420P
#define DEFAULT_LOOPBACK            0
#define DEFAULT_SRC_FRAME_RATE      30
#define DEFAULT_TGT_FRAME_RATE      30
#define DEFAULT_MAX_WD              1920
#define DEFAULT_MAX_HT              1920
#define DEFAULT_MAX_LEVEL           41
*/
#define DEFAULT_SRC_FRAME_RATE 30
#define DEFAULT_STRIDE 0
#define DEFAULT_WD 1280
#define DEFAULT_HT 720
/*
#define DEFAULT_PSNR_ENABLE         0
#define DEFAULT_ME_SPEED            100
#define DEFAULT_ENABLE_FAST_SAD     0
#define DEFAULT_ENABLE_ALT_REF      0
#define DEFAULT_RC_MODE             IVE_RC_STORAGE
*/
#define DEFAULT_BITRATE 6000000

#define DEFAULT_MIN_QP 6
#define DEFAULT_MAX_QP 45
#define DEFAULT_I_QP 22
#define DEFAULT_I_QP_MAX DEFAULT_QP_MAX
#define DEFAULT_I_QP_MIN DEFAULT_QP_MIN
#define DEFAULT_P_QP 28
#define DEFAULT_P_QP_MAX DEFAULT_QP_MAX
#define DEFAULT_P_QP_MIN DEFAULT_QP_MIN
#define DEFAULT_B_QP 22
#define DEFAULT_B_QP_MAX DEFAULT_QP_MAX
#define DEFAULT_B_QP_MIN DEFAULT_QP_MIN
/*
#define DEFAULT_AIR                 IVE_AIR_MODE_NONE
#define DEFAULT_AIR_REFRESH_PERIOD  30
#define DEFAULT_SRCH_RNG_X          64
#define DEFAULT_SRCH_RNG_Y          48
*/
#define DEFAULT_I_INTERVAL 30
#define DEFAULT_IDR_INTERVAL 30
#define DEFAULT_B_FRAMES 0
//#define DEFAULT_DISABLE_DEBLK_LEVEL 0
//#define DEFAULT_HPEL                1
//#define DEFAULT_QPEL                1
//#define DEFAULT_I4                  1
//#define DEFAULT_EPROFILE            IV_PROFILE_BASE
/* CABAC_MODE */
#define DEFAULT_ENTROPY_MODE true
#define DEFAULT_COLOR_FORMAT VENC_PIXEL_YUV420SP
/*
#define DEFAULT_SLICE_MODE          IVE_SLICE_MODE_NONE
#define DEFAULT_SLICE_PARAM         256
#define DEFAULT_ARCH                ARCH_ARM_A9Q
#define DEFAULT_SOC                 SOC_GENERIC
#define DEFAULT_INTRA4x4            0
#define STRLENGTH                   500
#define DEFAULT_CONSTRAINED_INTRA   0
*/
#define DEFAULT_MAX_I_SUPERFRAME_BITS (30000 * 8)
#define DEFAULT_MAX_P_SUPERFRAME_BITS (15000 * 8)

#define ROI_NUM (4)

#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define ALIGN_XXB(y, x) (((x) + ((y)-1)) & ~((y)-1))
#define ALIGN16(x) ((((x) + 15) >> 4) << 4)
#define ALIGN128(x) ((((x) + 127) >> 7) << 7)
#define ALIGN4096(x) ((((x) + 4095) >> 12) << 12)

/** Used to remove warnings about unused parameters */
#define UNUSED(x) ((void)(x))

/** Get time */
#define GETTIME(a, b) gettimeofday(a, b);

/** Compute difference between start and end */
#define TIME_DIFF(start, end, diff)                    \
  diff = (((end).tv_sec - (start).tv_sec) * 1000000) + \
         ((end).tv_usec - (start).tv_usec);

/** struct naming rule below.
 ** normal member with all lower case,  "_" to seprate noun,
 ** use Pascar style for sub struct member
 ** align struct members to look beautiful,  pay attention to memory align
 **/

/*
typedef struct {
    jpeg_ctx_t sJpegCtx;
    h264_ctx_t sH264Ctx;
    h265_ctx_t sH265Ctx;
}encode_param_t;
*/

typedef struct {
  int quality;
  int mode;
  int bitrate;
  int framerate;
  // EXIFInfo             exifinfo;
  // VencJpegVideoSignal  vs;
  VencBitRateRange bitrateRange;
  // VencOverlayInfoS     sOverlayInfo;
} jpeg_ctx_t;

typedef struct {
  VencHeaderData sHeaderData;
  VencH264Param sH264Param;
  VencMBModeCtrl sMBMode;
  VencMBInfo sMBInfo;
  VencFixQP fixQP;
  VencSuperFrameConfig sSuperFrameCfg;
  VencH264SVCSkip SVCSkip;  // set SVC and skip_frame
  VencH264AspectRatio sAspectRatio;
  VencH264VideoSignal sVideoSignal;
  VencCyclicIntraRefresh sIntraRefresh;
  VencROIConfig sRoiConfig[ROI_NUM];
  VeProcSet sVeProcInfo;
  VencOverlayInfoS sOverlayInfo;
  VencSmartFun sH264Smart;
} h264_ctx_t;

typedef struct {
  int totalRcFrame;
  VencH265Param sH265Param;
  VencH265GopStruct sH265Gop;
  VencHVS sH265Hvs;
  VencH265TendRatioCoef sH265Trc;
  VencSmartFun sH265Smart;
  VencMBModeCtrl sMBMode;
  VencMBInfo sMBInfo;
  VencFixQP fixQP;
  VencSuperFrameConfig sSuperFrameCfg;
  VencH264SVCSkip SVCSkip;  // set SVC and skip_frame
  VencH264AspectRatio sAspectRatio;
  VencH264VideoSignal sVideoSignal;
  VencCyclicIntraRefresh sIntraRefresh;
  VencROIConfig sRoiConfig[ROI_NUM];
  VencAlterFrameRateInfo sAlterFrameRateInfo;
  VeProcSet sVeProcInfo;
  VencOverlayInfoS sOverlayInfo;
} h265_ctx_t;

typedef struct {
  bool useAllocInputBuffer;
  bool usePSkip;
  bool mSpsPpsHeaderReceived;
  bool mSetVideoSingalFlag;
  bool mSetReceiveMBInfoFlag;
  bool mSetDetectMotionFlag;
  VideoEncoder *pEncoder;
  VencBaseConfig sBaseConfig;
  VencAllocateBufferParam sVencAllocBufferParam;
  VencHeaderData sHeaderdata;
  VencInputBuffer sInputBuffer;
  VencOutputBuffer sVencOutputBuffer;
  VencMBInfo MBInfo;
  VencMBInfoPara MBModePara;
  VencMBModeCtrl MBModeInfo;
  h264_ctx_t sH264Ctx;
  h265_ctx_t sH265Ctx;
  jpeg_ctx_t sJpegCtx;
} codec_ctx_t;

class VencComponent {
 public:
  VencComponent(const char *name,
                std::shared_ptr<VencParameterHelper> mParaHelper);
  // From HwC2Component
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

  static constexpr uint32_t NO_DRAIN = ~0u;
  ~VencComponent();

  void setIntf(std::shared_ptr<HwC2Interface<void>::BaseParams> intf) {
    mIntf = intf;
  }

 private:
  // OMX input buffer's timestamp and flags
  char COMPONENT_NAME[100];
  typedef struct {
    int64_t mTimeUs;
    int32_t mFlags;
  } InputBufferInfo;

  // std::shared_ptr<IntfImpl> mIntf;

  int32_t mStride;

  struct timeval mTimeStart;  // Time at the start of decode()
  struct timeval mTimeEnd;    // Time at the end of decode()
#ifdef FILE_DUMP_ENABLE
  char mInFile[200];
  char mOutFile[200];
#endif /* FILE_DUMP_ENABLE */

  int32_t mAVCEncLevel;
  bool mStarted;
  bool mSpsPpsHeaderReceived;
  bool mEncoderInitialized;

  bool mSawInputEOS;
  bool mSignalledError;
  bool mIntra4x4;
  bool mEntropyMode;
  bool mConstrainedIntraFlag;

  std::shared_ptr<C2LinearBlock> mOutBlock;

  // configurations used by component in process
  std::shared_ptr<VencParameterHelper> mParaHelper;
  std::shared_ptr<C2StreamPictureSizeInfo::input> mSize;
  std::shared_ptr<C2StreamIntraRefreshTuning::output> mIntraRefresh;
  std::shared_ptr<C2StreamFrameRateInfo::output> mFrameRate;
  std::shared_ptr<C2StreamBitrateInfo::output> mBitrate;
  std::shared_ptr<C2StreamRequestSyncFrameTuning::output> mRequestSync;

  uint32_t mOutBufferSize;
  int32_t mBframes;
  int32_t mIInterval;
  int32_t mIDRInterval;

  VENC_PIXEL_FMT mVideoColorFormat;

  /* For encoder context */
  codec_ctx_t *mCodecCtx;
  VENC_CODEC_TYPE mCodecType;
  int32_t mEncLevel;
  int32_t mEncProfile;
  int32_t mQPMin;
  int32_t mQPMax;
  // input buffer copied from c2 to encoder. not use at best.
  bool mMemoryCopyMode;
  bool mColorFormatChange;

  void initEncParams();
  c2_status_t initEncoder();
  c2_status_t releaseEncoder();
  c2_status_t setQp();
  c2_status_t setBitRate();
  c2_status_t setFrameType();
  c2_status_t setIntraRefreshParams();
  c2_status_t setMeParams();
  c2_status_t setGopParams();
  c2_status_t setProfileParams();
  c2_status_t setDeblockParams();
  c2_status_t setVbvParams();
  c2_status_t setDetectMotion(bool isApplied);
  c2_status_t setVideoSignal(bool isApplied);
  c2_status_t setReceiveMBInfo(bool isApplied);

  c2_status_t submitInputData(const C2GraphicView *const input,
                              const int32_t fd, uint64_t workIndex,
                              int64_t pts);
  void finishWork(uint64_t workIndex, const std::shared_ptr<C2BlockPool> &pool,
                  const std::unique_ptr<C2Work> &work,
                  VencOutputBuffer *sVencOutputtBuffer);
  c2_status_t drainInternal(uint32_t drainMode,
                            const std::shared_ptr<C2BlockPool> &pool,
                            const std::unique_ptr<C2Work> &work);

  void init_mb_mode(VencMBModeCtrl *pMBMode);
  void init_mb_info(VencMBInfo *MBInfo);
  void init_h264_param(h264_ctx_t *sH264Ctx);
  void init_h265_param(h265_ctx_t *sH265Ctx);
  void setSuperFrameCfg();
  void setSVCSkipCfg();
  void initFeatureWorkFlag();

  c2_status_t ConvertRGBAtoEncoder(VencInputBuffer *sInputBuffer,
                                   uint8_t *bufAddr, int width, int height,
                                   const C2GraphicView *const input);

  c2_status_t copyYUVtoEncoder(VencInputBuffer *sInputBuffer, uint8_t *srcAddr,
                               int stride, int height);

#ifndef CONF_ARMV7_A_NEON_
  c2_status_t ConvertRGBToPlanarYUVTwoPlane(uint8_t *dstY, uint8_t *dstU,
                                            size_t dstStride, size_t dstVStride,
                                            size_t bufferSize,
                                            const C2GraphicView &src);
#endif

  std::function<void(std::unique_ptr<C2Work>)> mWorkHandlerCb;
  std::shared_ptr<HwC2Interface<void>::BaseParams> mIntf;

  void init_fix_qp(VencFixQP *fixQP);
  void init_super_frame_cfg(VencSuperFrameConfig *sSuperFrameCfg,
                            bool isApplied);
  void init_svc_skip(VencH264SVCSkip *SVCSkip, bool isApplied);
  void init_aspect_ratio(VencH264AspectRatio *sAspectRatio);
  void init_video_signal(VencH264VideoSignal *sVideoSignal, bool isApplied);
  void init_intra_refresh(VencCyclicIntraRefresh *sIntraRefresh);
  void init_roi(VencROIConfig *sRoiConfig, bool isApplied);
  void init_alter_frame_rate_info(VencAlterFrameRateInfo *pAlterFrameRateInfo,
                                  bool isApplied);
  void init_enc_proc_info(VeProcSet *ve_proc_set, bool isApplied);

  uint32_t updateVencStride(uint32_t width);
  c2_status_t updateColorFormat(VideoEncoder *pEncoder,
                                VENC_PIXEL_FMT colorFormat);

  // manage mBuffers carefully, otherwise memory would overwrite.
  std::map<int32_t, std::shared_ptr<C2Buffer>> mBuffers;
  C2_DO_NOT_COPY(VencComponent);
};
}  // namespace android

#endif  //  ANDROID_COMPONENTS_INCLUDE_VENCCOMPONENT_H_
