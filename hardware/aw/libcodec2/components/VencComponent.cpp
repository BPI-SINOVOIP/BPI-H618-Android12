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
#define LOG_TAG "VencComponent"
#include "VencComponent.h"

#include <C2Debug.h>
#include <C2PlatformSupport.h>
#include <Codec2BufferUtils.h>
#include <gralloc1.h>
#include <hardware/gralloc.h>
#include <media/hardware/VideoAPI.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/AUtils.h>
#include <util/C2InterfaceHelper.h>
#include <utils/misc.h>
#include "memoryAdapter.h"

#include <memory>
#include <string>
#include <utility>

#include "C2Log.h"
#include "HwC2Interface.h"

#include GPU_PUBLIC_INCLUDE

extern "C" void ImgRGBA2YVU420SP_neon(unsigned char *pu8RgbBuffer,
                                      unsigned char **pu8SrcYUV,
                                      int *l32Width_stride, int l32Height);
namespace android {

static const char *strCodecType[] = {"ENCODER_H264", "ENCODER_JPEG",
                                     "ENCODER_H264_VER2", "ENCODER_H265",
                                     "ENCODER_VP8"};

static const char *encode_result_to_string(int32_t result) {
  enum VENC_RESULT_TYPE ret = (enum VENC_RESULT_TYPE)result;
  switch (ret) {
    case VENC_RESULT_ERROR:
      return "venc_result_error";
    case VENC_RESULT_NO_FRAME_BUFFER:
      return "venc_result_no_frame_buffer";
    case VENC_RESULT_BITSTREAM_IS_FULL:
      return "venc_result_bitstream_is_full";
    case VENC_RESULT_ILLEGAL_PARAM:
      return "venc_result_illegal_param";
    case VENC_RESULT_NOT_SUPPORT:
      return "venc_result_not_support";
    case VENC_RESULT_BITSTREAM_IS_EMPTY:
      return "venc_result_bitstream_is_empty";
    case VENC_RESULT_NO_MEMORY:
      return "venc_result_no_memory";
    case VENC_RESULT_NO_RESOURCE:
      return "venc_result_no_resource";
    case VENC_RESULT_NULL_PTR:
      return "venc_result_null_ptr";
    case VENC_RESULT_EFUSE_ERROR:
      return "venc_result_efuse_error";
    case VENC_RESULT_OK:
      return "venc_result_ok";
    default:
      return "unexpected encode result.";
  }
}

static const char *colorformat_to_string(VENC_PIXEL_FMT colorFormat) {
  switch (colorFormat) {
    case VENC_PIXEL_YUV420SP:
      return "VENC_PIXEL_YUV420SP";
    case VENC_PIXEL_YVU420SP:
      return "VENC_PIXEL_YVU420SP";
    case VENC_PIXEL_YUV420P:
      return "VENC_PIXEL_YUV420P";
    case VENC_PIXEL_YVU420P:
      return "VENC_PIXEL_YVU420P";
    case VENC_PIXEL_YUV422SP:
      return "VENC_PIXEL_YUV422SP";
    case VENC_PIXEL_YVU422SP:
      return "VENC_PIXEL_YVU422SP";
    case VENC_PIXEL_YUV422P:
      return "VENC_PIXEL_YUV422P";
    case VENC_PIXEL_YVU422P:
      return "VENC_PIXEL_YVU422P";
    case VENC_PIXEL_YUYV422:
      return "VENC_PIXEL_YUYV422";
    case VENC_PIXEL_UYVY422:
      return "VENC_PIXEL_UYVY422";
    case VENC_PIXEL_YVYU422:
      return "VENC_PIXEL_YVYU422";
    case VENC_PIXEL_VYUY422:
      return "VENC_PIXEL_VYUY422";
    case VENC_PIXEL_ARGB:
      return "VENC_PIXEL_ARGB";
    case VENC_PIXEL_RGBA:
      return "VENC_PIXEL_RGBA";
    case VENC_PIXEL_ABGR:
      return "VENC_PIXEL_ABGR";
    case VENC_PIXEL_BGRA:
      return "VENC_PIXEL_BGRA";
    case VENC_PIXEL_AFBC_AW:
      return "VENC_PIXEL_AFBC_AW";
    case VENC_PIXEL_LBC_AW:
      // for v5v200 and newer ic
      return "VENC_PIXEL_LBC_AW";
    default:
      return "unexpected color format result.";
  }
}

static bool find_string_in_name(const char *name, const char *string) {
  if (name == nullptr || string == nullptr) {
    return false;
  }
  if (strstr(name, string) != nullptr) {
    return true;
  }
  return false;
}

static void config_color_aspects(VencH264VideoSignal *pVencH264VideoSignal,
                                 int32_t mTransferCharacteristics,
                                 int32_t mMatrixCoeffs, int32_t mVideoFullRange,
                                 int32_t mPrimaries) {
  ALOGD(
      "mTransferCharacteristics %d mMatrixCoeffs %d mVideoFullRange %d "
      "mPrimaries %d",
      mTransferCharacteristics, mMatrixCoeffs, mVideoFullRange, mPrimaries);
  switch (mTransferCharacteristics) {
    case C2Color::TRANSFER_LINEAR:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_LINEAR;
      break;
    case C2Color::TRANSFER_SRGB:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_SRGB;
      break;
    case C2Color::TRANSFER_170M:
      pVencH264VideoSignal->transfer_characteristics =
          VIDEO_TRANSFER_SMPTE_170M;
      break;
    case C2Color::TRANSFER_GAMMA22:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_GAMMA2_2;
      break;
    case C2Color::TRANSFER_GAMMA28:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_GAMMA2_8;
      break;
    case C2Color::TRANSFER_ST2084:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_ST2084;
      break;
    case C2Color::TRANSFER_HLG:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_HLG;
      break;
    case C2Color::TRANSFER_240M:
      pVencH264VideoSignal->transfer_characteristics =
          VIDEO_TRANSFER_SMPTE_240M;
      break;
    case C2Color::TRANSFER_XVYCC:  //*0x41
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_IEC61966;
      break;
    case C2Color::TRANSFER_BT1361:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_BT1361;
      break;
    case C2Color::TRANSFER_ST428:
      pVencH264VideoSignal->transfer_characteristics = VIDEO_TRANSFER_ST428_1;
      break;
    case C2Color::TRANSFER_OTHER:
      pVencH264VideoSignal->transfer_characteristics =
          VIDEO_TRANSFER_BT1361_EXTENDED;
      break;
    case C2Color::TRANSFER_UNSPECIFIED:
      pVencH264VideoSignal->transfer_characteristics =
          VIDEO_TRANSFER_UNSPECIFIED;
      break;
    default:
      pVencH264VideoSignal->transfer_characteristics =
          VIDEO_TRANSFER_UNSPECIFIED;
      break;
  }
  switch (mMatrixCoeffs) {
    case C2Color::MATRIX_BT709:  //*1
      pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT709;
      break;
    case C2Color::MATRIX_FCC47_73_682:  //*2
      pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT470M;
      break;
    case C2Color::MATRIX_BT601:  //*3
      pVencH264VideoSignal->matrix_coefficients =
          VIDEO_MATRIX_COEFFS_BT601_625_0;
      break;
    case C2Color::MATRIX_240M:  //*4
      pVencH264VideoSignal->matrix_coefficients =
          VIDEO_MATRIX_COEFFS_SMPTE_240M;
      break;
    case C2Color::MATRIX_BT2020:  //*5
      pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_BT2020;
      break;
    case C2Color::MATRIX_BT2020_CONSTANT:  //*6
      pVencH264VideoSignal->matrix_coefficients =
          VIDEO_MATRIX_COEFFS_BT2020_CONSTANT_LUMINANCE;
      break;
    case C2Color::MATRIX_OTHER:  //*0xff
      pVencH264VideoSignal->matrix_coefficients = VIDEO_MATRIX_COEFFS_SOMPATE;
      break;
    case C2Color::MATRIX_UNSPECIFIED:  //*0
      pVencH264VideoSignal->matrix_coefficients =
          VIDEO_MATRIX_COEFFS_UNSPECIFIED_0;
      break;
    default: {
      pVencH264VideoSignal->matrix_coefficients =
          VIDEO_MATRIX_COEFFS_UNSPECIFIED_0;
      break;
    }
  }
  switch (mVideoFullRange) {
    case C2Color::RANGE_LIMITED:  //*2
      pVencH264VideoSignal->full_range_flag = 0;
      break;
    case C2Color::RANGE_FULL:  //*1
      pVencH264VideoSignal->full_range_flag = 1;
      break;
    default: {
      pVencH264VideoSignal->full_range_flag = 0;
      break;
    }
  }
  switch (mPrimaries) {
    case C2Color::PRIMARIES_BT709:  //*1
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)1;
      break;
    case C2Color::PRIMARIES_BT470_M:  //*2
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)4;
      break;
    case C2Color::PRIMARIES_BT601_625:  //*3
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)5;
      break;
    case C2Color::PRIMARIES_BT601_525:  //*4
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)6;
      break;
    case C2Color::PRIMARIES_OTHER:  //*0xff
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)7;
      break;
    case C2Color::PRIMARIES_GENERIC_FILM:  //*5
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)8;
      break;
    case C2Color::PRIMARIES_BT2020:  //*6
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)9;
      break;
    case C2Color::PRIMARIES_UNSPECIFIED:  //*0
      pVencH264VideoSignal->dst_colour_primaries =
          (VENC_COLOR_SPACE)1;  // add for V2test.
      break;
    case C2Color::PRIMARIES_RP431:    //*7
    case C2Color::PRIMARIES_EG432:    //*8
    case C2Color::PRIMARIES_EBU3213:  //*9
    default: {
      pVencH264VideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)0;
      break;
    }
  }

  ALOGD(
      "config color aspect ,[video range %d], [matrix coeff %d], [color "
      "primaries %d], [transfer characteristics %d]",
      pVencH264VideoSignal->full_range_flag,
      pVencH264VideoSignal->matrix_coefficients,
      pVencH264VideoSignal->dst_colour_primaries,
      pVencH264VideoSignal->transfer_characteristics);
  return;
}

uint32_t VencComponent::updateVencStride(uint32_t width) {
  // TODO(kay): Different case, different stride align.
  return ALIGN_XXB(16, width);
}

c2_status_t VencComponent::updateColorFormat(VideoEncoder *pEncoder,
                                             VENC_PIXEL_FMT colorFormat) {
  VideoEncSetParameter(pEncoder, VENC_IndexParamColorFormat, &colorFormat);
  c2_logv("change encoder colorFormat to %s.",
          colorformat_to_string(colorFormat));
  return C2_OK;
}

VencComponent::VencComponent(const char *name,
                             std::shared_ptr<VencParameterHelper> paraHelper)
    : mSawInputEOS(false),
      mSignalledError(false),
      mOutBlock(nullptr),
      // TODO(kay): output buffer size
      mOutBufferSize(524288) {
  memset(COMPONENT_NAME, 0, sizeof(COMPONENT_NAME));
  strncpy(COMPONENT_NAME, name, strlen(name));
  mParaHelper = paraHelper;
  const char avcstring[4] = "avc";
  const char hevcstring[5] = "hevc";
  const char mjpegstring[6] = "mjpeg";
  if (find_string_in_name(name, avcstring)) {
    mCodecType = VENC_CODEC_H264;
    mEncProfile = (int32_t)VENC_H264ProfileMain;
    mEncLevel = (int32_t)VENC_H264Level41;
  } else if (find_string_in_name(name, hevcstring)) {
    mCodecType = VENC_CODEC_H265;
    mEncProfile = (int32_t)VENC_H265ProfileMain;
    mEncLevel = (int32_t)VENC_H265Level41;
  } else if (find_string_in_name(name, mjpegstring)) {
    mCodecType = VENC_CODEC_JPEG;
  } else {
    c2_loge("name %s is NOT avc/hevc/mjpeg. We NOT support it.", name);
  }
  c2_logd("mCodecType %s, mEncProfile %d, mEncLevel %d.",
          strCodecType[mCodecType], mEncProfile, mEncLevel);
#ifdef FILE_DUMP_ENABLE
  GENERATE_VENC_FILE_NAMES();
  CREATE_DUMP_FILE(mInFile);
  CREATE_DUMP_FILE(mOutFile);
#endif
  initEncParams();
}

VencComponent::~VencComponent() {
  C2_TRACE();
  onRelease();
}

c2_status_t VencComponent::onInit() {
  C2_TRACE();
  return C2_OK;
}

c2_status_t VencComponent::onStart() {
  C2_TRACE();
  return C2_OK;
}

c2_status_t VencComponent::onStop() {
  C2_TRACE();
  return C2_OK;
}

void VencComponent::onReset() {
  // TODO(kay): use IVE_CMD_CTL_RESET?
  VideoEncoderReset(mCodecCtx->pEncoder);
  if (mOutBlock) {
    mOutBlock.reset();
  }
  initEncParams();
}

void VencComponent::onRelease() {
  releaseEncoder();
  if (mOutBlock) {
    mOutBlock.reset();
  }
}

c2_status_t VencComponent::onFlush_sm() {
  C2_TRACE();
  return C2_OK;
}

/* TODO(kay): Get value from codec2_encoder.conf in /vendor/etc */
void init_jpeg_exif(EXIFInfo *exifinfo) {
  CHECK(exifinfo);
  exifinfo->ThumbWidth = 640;
  exifinfo->ThumbHeight = 480;

  std::string camera_make("allwinner make");
  strncpy(reinterpret_cast<char *>(exifinfo->CameraMake), camera_make.c_str(),
          camera_make.size() + 1);

  std::string allwinner_model("allwinner model");
  strncpy(reinterpret_cast<char *>(exifinfo->CameraModel),
          allwinner_model.c_str(), allwinner_model.size() + 1);

  std::string date_time("2020:06:21 10:54:05");
  strncpy(reinterpret_cast<char *>(exifinfo->DateTime), date_time.c_str(),
          date_time.size() + 1);

  std::string gps_processing_method("allwinner gps");
  strncpy(reinterpret_cast<char *>(exifinfo->gpsProcessingMethod),
          gps_processing_method.c_str(), gps_processing_method.size() + 1);

  exifinfo->Orientation = 0;
  exifinfo->ExposureTime.num = 2;
  exifinfo->ExposureTime.den = 1000;

  exifinfo->FNumber.num = 20;
  exifinfo->FNumber.den = 10;
  exifinfo->ISOSpeed = 50;

  exifinfo->ExposureBiasValue.num = -4;
  exifinfo->ExposureBiasValue.den = 1;

  exifinfo->MeteringMode = 1;
  exifinfo->FlashUsed = 0;

  exifinfo->FocalLength.num = 1400;
  exifinfo->FocalLength.den = 100;

  exifinfo->DigitalZoomRatio.num = 4;
  exifinfo->DigitalZoomRatio.den = 1;

  exifinfo->WhiteBalance = 1;
  exifinfo->ExposureMode = 1;

  exifinfo->enableGpsInfo = 1;

  exifinfo->gps_latitude = 23.2368;
  exifinfo->gps_longitude = 24.3244;
  exifinfo->gps_altitude = 1234.5;

  exifinfo->gps_timestamp = (uint32_t)time(NULL);

  std::string camera_serial_num("123456789");
  strncpy(reinterpret_cast<char *>(exifinfo->CameraSerialNum),
          camera_serial_num.c_str(), camera_serial_num.size() + 1);

  std::string image_name("exif-name-test");
  strncpy(reinterpret_cast<char *>(exifinfo->ImageName), image_name.c_str(),
          image_name.size() + 1);

  std::string image_description("exif-descriptor-test");
  strncpy(reinterpret_cast<char *>(exifinfo->ImageDescription),
          image_description.c_str(), image_description.size() + 1);
}

void init_h265_gop(VencH265GopStruct *h265Gop) {
  CHECK(h265Gop);
  h265Gop->gop_size = 8;
  h265Gop->intra_period = 16;

  h265Gop->use_lt_ref_flag = 1;
  if (h265Gop->use_lt_ref_flag) {
    h265Gop->max_num_ref_pics = 2;
    h265Gop->num_ref_idx_l0_default_active = 2;
    h265Gop->num_ref_idx_l1_default_active = 2;
    h265Gop->use_sps_rps_flag = 0;
  } else {
    h265Gop->max_num_ref_pics = 1;
    h265Gop->num_ref_idx_l0_default_active = 1;
    h265Gop->num_ref_idx_l1_default_active = 1;
    h265Gop->use_sps_rps_flag = 1;
  }
  // 1:user config the reference info; 0:encoder config the reference info
  h265Gop->custom_rps_flag = 0;
  return;
}

void VencComponent::init_mb_mode(VencMBModeCtrl *pMBMode) {
  CHECK(pMBMode);
  unsigned int mb_num =
      (ALIGN_XXB(16, mSize->width) >> 4) * (ALIGN_XXB(16, mSize->height) >> 4);
  c2_logd(
      "ALIGN_16B for dst_width %d, for dst_height %d. "
      "while ALIGN16 are %d and %d.",
      ALIGN_XXB(16, mSize->width), ALIGN_XXB(16, mSize->height),
      ALIGN16(mSize->width), ALIGN16(mSize->height));
  pMBMode->p_info = (unsigned char *)new VencMBModeCtrlInfo[mb_num];
  CHECK(pMBMode->p_info);
  pMBMode->mode_ctrl_en = 1;
  return;
}

void VencComponent::init_mb_info(VencMBInfo *MBInfo) {
  CHECK(MBInfo);
  if (mCodecType == VENC_CODEC_H265) {
    MBInfo->num_mb =
        (ALIGN_XXB(32, mSize->width) * ALIGN_XXB(32, mSize->height)) >> 10;
  } else if (mCodecType == VENC_CODEC_H264) {
    MBInfo->num_mb =
        (ALIGN_XXB(16, mSize->width) * ALIGN_XXB(16, mSize->height)) >> 8;
  } else {
    c2_logw("fail to init mb info while codec format is NOT H264 or H265.");
    return;
  }
  c2_logv("mb_num:%d, mb_info_queue_addr:%p\n", MBInfo->num_mb, MBInfo->p_para);
}

void VencComponent::init_fix_qp(VencFixQP *fixQP) {
  C2_TRACE();
  CHECK(fixQP);
  fixQP->bEnable = 1;
  fixQP->nIQp = DEFAULT_I_QP;
  fixQP->nPQp = DEFAULT_P_QP;
  c2_logv("encoder fix QP for I %d, P %d.", fixQP->nIQp, fixQP->nPQp);
  return;
}

void VencComponent::init_super_frame_cfg(VencSuperFrameConfig *sSuperFrameCfg,
                                         bool isApplied) {
  CHECK(sSuperFrameCfg);
  if (isApplied == false) {
    return;
  }
  sSuperFrameCfg->eSuperFrameMode = VENC_SUPERFRAME_NONE;
  sSuperFrameCfg->nMaxIFrameBits = DEFAULT_MAX_I_SUPERFRAME_BITS;
  sSuperFrameCfg->nMaxPFrameBits = DEFAULT_MAX_P_SUPERFRAME_BITS;
  c2_logv("super frame config: max I frame bits %d, max P frame bits %d.",
          sSuperFrameCfg->nMaxIFrameBits, sSuperFrameCfg->nMaxPFrameBits);
  return;
}

/* optional */
void VencComponent::init_svc_skip(VencH264SVCSkip *SVCSkip, bool isApplied) {
  CHECK(SVCSkip);
  if (isApplied == false) {
    return;
  }
  SVCSkip->nTemporalSVC = T_LAYER_4;
  switch (SVCSkip->nTemporalSVC) {
    case T_LAYER_4:
      SVCSkip->nSkipFrame = SKIP_8;
      break;
    case T_LAYER_3:
      SVCSkip->nSkipFrame = SKIP_4;
      break;
    case T_LAYER_2:
      SVCSkip->nSkipFrame = SKIP_2;
      break;
    default:
      SVCSkip->nSkipFrame = NO_SKIP;
      break;
  }
  return;
}

void VencComponent::init_aspect_ratio(VencH264AspectRatio *sAspectRatio) {
  CHECK(sAspectRatio);
  sAspectRatio->aspect_ratio_idc = 255;
  sAspectRatio->sar_width = 4;
  sAspectRatio->sar_height = 3;
  c2_logv("encoder aspect ration idr %d, sar width %d, sar height %d.",
          sAspectRatio->aspect_ratio_idc, sAspectRatio->sar_width,
          sAspectRatio->sar_height);
  return;
}

/* optional */
void VencComponent::init_video_signal(VencH264VideoSignal *sVideoSignal,
                                      bool isApplied) {
  CHECK(sVideoSignal);
  if (isApplied == false) {
    return;
  }
  sVideoSignal->video_format = (VENC_VIDEO_FORMAT)5;
  sVideoSignal->src_colour_primaries = (VENC_COLOR_SPACE)0;
  sVideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)1;
  return;
}

void VencComponent::init_intra_refresh(VencCyclicIntraRefresh *sIntraRefresh) {
  CHECK(sIntraRefresh);
  sIntraRefresh->bEnable = 1;
  sIntraRefresh->nBlockNumber = 10;
  c2_logv("encoder intra refresh: enable? %s, block number %d.",
          sIntraRefresh->bEnable ? "true" : "false",
          sIntraRefresh->nBlockNumber);
  return;
}

/* optional */
void VencComponent::init_roi(VencROIConfig *sRoiConfig, bool isApplied) {
  CHECK(sRoiConfig);
  if (isApplied == false) {
    return;
  }
  sRoiConfig[0].bEnable = 1;
  sRoiConfig[0].index = 0;
  sRoiConfig[0].nQPoffset = 10;
  sRoiConfig[0].sRect.nLeft = 0;
  sRoiConfig[0].sRect.nTop = 0;
  sRoiConfig[0].sRect.nWidth = 1280;
  sRoiConfig[0].sRect.nHeight = 320;

  sRoiConfig[1].bEnable = 1;
  sRoiConfig[1].index = 1;
  sRoiConfig[1].nQPoffset = 10;
  sRoiConfig[1].sRect.nLeft = 320;
  sRoiConfig[1].sRect.nTop = 180;
  sRoiConfig[1].sRect.nWidth = 320;
  sRoiConfig[1].sRect.nHeight = 180;

  sRoiConfig[2].bEnable = 1;
  sRoiConfig[2].index = 2;
  sRoiConfig[2].nQPoffset = 10;
  sRoiConfig[2].sRect.nLeft = 320;
  sRoiConfig[2].sRect.nTop = 180;
  sRoiConfig[2].sRect.nWidth = 320;
  sRoiConfig[2].sRect.nHeight = 180;

  sRoiConfig[3].bEnable = 1;
  sRoiConfig[3].index = 3;
  sRoiConfig[3].nQPoffset = 10;
  sRoiConfig[3].sRect.nLeft = 320;
  sRoiConfig[3].sRect.nTop = 180;
  sRoiConfig[3].sRect.nWidth = 320;
  sRoiConfig[3].sRect.nHeight = 180;
  c2_logv("encoder roi is applied.");
}

/* optional */
void VencComponent::init_alter_frame_rate_info(
    VencAlterFrameRateInfo *pAlterFrameRateInfo, bool isApplied) {
  CHECK(pAlterFrameRateInfo);
  if (isApplied == false) {
    return;
  }
  memset(pAlterFrameRateInfo, 0, sizeof(VencAlterFrameRateInfo));
  pAlterFrameRateInfo->bEnable = 1;
  pAlterFrameRateInfo->bUseUserSetRoiInfo = 1;
  pAlterFrameRateInfo->sRoiBgFrameRate.nSrcFrameRate = 25;
  pAlterFrameRateInfo->sRoiBgFrameRate.nDstFrameRate = 5;

  pAlterFrameRateInfo->roi_param[0].bEnable = 1;
  pAlterFrameRateInfo->roi_param[0].index = 0;
  pAlterFrameRateInfo->roi_param[0].nQPoffset = 10;
  pAlterFrameRateInfo->roi_param[0].roi_abs_flag = 1;
  pAlterFrameRateInfo->roi_param[0].sRect.nLeft = 0;
  pAlterFrameRateInfo->roi_param[0].sRect.nTop = 0;
  pAlterFrameRateInfo->roi_param[0].sRect.nWidth = 320;
  pAlterFrameRateInfo->roi_param[0].sRect.nHeight = 320;

  pAlterFrameRateInfo->roi_param[1].bEnable = 1;
  pAlterFrameRateInfo->roi_param[1].index = 0;
  pAlterFrameRateInfo->roi_param[1].nQPoffset = 10;
  pAlterFrameRateInfo->roi_param[1].roi_abs_flag = 1;
  pAlterFrameRateInfo->roi_param[1].sRect.nLeft = 320;
  pAlterFrameRateInfo->roi_param[1].sRect.nTop = 320;
  pAlterFrameRateInfo->roi_param[1].sRect.nWidth = 320;
  pAlterFrameRateInfo->roi_param[1].sRect.nHeight = 320;
  c2_logv("encoder alter framrate info is applied.");
  return;
}

void VencComponent::init_enc_proc_info(VeProcSet *ve_proc_set, bool isApplied) {
  CHECK(ve_proc_set);
  if (isApplied == false) {
    return;
  }
  ve_proc_set->bProcEnable = 1;
  ve_proc_set->nProcFreq = 60;
  return;
}

// Init params got from initial or default config.
// Set parameter to vencoder driver
void VencComponent::init_h264_param(h264_ctx_t *sH264Ctx) {
  C2_TRACE();
  CHECK(sH264Ctx);
  memset(sH264Ctx, 0, sizeof(h264_ctx_t));

  // init sH264Param
  // CABAC
  sH264Ctx->sH264Param.bEntropyCodingCABAC = mEntropyMode;
  sH264Ctx->sH264Param.nBitrate = mBitrate->value;
  sH264Ctx->sH264Param.nFramerate = mFrameRate->value;
  sH264Ctx->sH264Param.nCodingMode = VENC_FRAME_CODING;
  sH264Ctx->sH264Param.nMaxKeyInterval = mIInterval;  // or mIDRInterval??
  sH264Ctx->sH264Param.sProfileLevel.nProfile =
      (VENC_H264PROFILETYPE)mEncProfile;  // VENC_H264Level51
  sH264Ctx->sH264Param.sProfileLevel.nLevel =
      (VENC_H264LEVELTYPE)mEncLevel;  // VENC_H264Level51

#ifdef VBR
  sH264Ctx->sH264Param.sRcParam.eRcMode = AW_VBR;
#endif
  sH264Ctx->sH264Param.sQPRange.nMinqp = DEFAULT_MIN_QP;
  sH264Ctx->sH264Param.sQPRange.nMaxqp = DEFAULT_MAX_QP;
  sH264Ctx->sH264Smart.img_bin_en = 1;
  sH264Ctx->sH264Smart.img_bin_th = 27;
  sH264Ctx->sH264Smart.shift_bits = 2;
  sH264Ctx->sH264Smart.smart_fun_en = 1;

  /* Set parameter to vencoder driver*/
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamH264Param,
                       &sH264Ctx->sH264Param);

  return;
}

// Init params got from initial or default config.
// Set parameter to vencoder driver
void VencComponent::init_h265_param(h265_ctx_t *sH265Ctx) {
  CHECK(sH265Ctx);
  memset(sH265Ctx, 0, sizeof(h265_ctx_t));

  // init sH265Param
  sH265Ctx->sH265Param.nBitrate = mBitrate->value;
  sH265Ctx->sH265Param.nFramerate = mFrameRate->value;
  sH265Ctx->sH265Param.sProfileLevel.nProfile = VENC_H265ProfileMain;
  sH265Ctx->sH265Param.sProfileLevel.nLevel = (VENC_H265LEVELTYPE)mEncLevel;
  sH265Ctx->sH265Param.nQPInit = 30;             // DEFAULT_H265_QP
  sH265Ctx->sH265Param.idr_period = mIInterval;  // 30;
  sH265Ctx->sH265Param.nGopSize = sH265Ctx->sH265Param.idr_period;
  sH265Ctx->sH265Param.nIntraPeriod = sH265Ctx->sH265Param.idr_period;
#ifdef VBR
  sH265Ctx->sH265Param.sRcParam.eRcMode = AW_VBR;
#endif
  sH265Ctx->sH265Param.sQPRange.nMaxqp = DEFAULT_MAX_QP;
  sH265Ctx->sH265Param.sQPRange.nMinqp = DEFAULT_MIN_QP;
  sH265Ctx->sH265Hvs.hvs_en = 1;
  sH265Ctx->sH265Hvs.th_dir = 24;
  sH265Ctx->sH265Hvs.th_coef_shift = 4;

  sH265Ctx->sH265Trc.inter_tend = 63;
  sH265Ctx->sH265Trc.skip_tend = 3;
  sH265Ctx->sH265Trc.merge_tend = 0;

  sH265Ctx->sH265Smart.img_bin_en = 1;
  sH265Ctx->sH265Smart.img_bin_th = 27;
  sH265Ctx->sH265Smart.shift_bits = 2;
  sH265Ctx->sH265Smart.smart_fun_en = 1;

  sH265Ctx->totalRcFrame = 20 * sH265Ctx->sH265Param.nGopSize;
  init_h265_gop(&sH265Ctx->sH265Gop);
  init_mb_mode(&sH265Ctx->sMBMode);
  init_mb_info(&sH265Ctx->sMBInfo);
  init_fix_qp(&sH265Ctx->fixQP);
  init_super_frame_cfg(&sH265Ctx->sSuperFrameCfg, false);
  init_svc_skip(&sH265Ctx->SVCSkip, false);
  init_aspect_ratio(&sH265Ctx->sAspectRatio);
  init_video_signal(&sH265Ctx->sVideoSignal, false);
  init_intra_refresh(&sH265Ctx->sIntraRefresh);
  init_roi(sH265Ctx->sRoiConfig, false);
  init_alter_frame_rate_info(&sH265Ctx->sAlterFrameRateInfo, false);
  init_enc_proc_info(&sH265Ctx->sVeProcInfo, false);
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamH265Param,
                       reinterpret_cast<void *>(&sH265Ctx->sH265Param));
  return;
}

void init_jpeg_param(jpeg_ctx_t *sJpegCtx) {
  CHECK(sJpegCtx);
  memset(sJpegCtx, 0, sizeof(jpeg_ctx_t));

  sJpegCtx->quality = 95;
  sJpegCtx->mode = 1;
  sJpegCtx->bitrate = 12 * 1024 * 1024;
  sJpegCtx->framerate = 30;
  sJpegCtx->bitrateRange.bitRateMax = 14 * 1024 * 1024;
  sJpegCtx->bitrateRange.bitRateMin = 10 * 1024 * 1024;
  return;
}

void VencComponent::initEncParams() {
  mCodecCtx = (codec_ctx_t *)malloc(sizeof(codec_ctx_t));

  mIInterval = DEFAULT_I_INTERVAL;
  mIDRInterval = DEFAULT_IDR_INTERVAL;
  mEntropyMode = DEFAULT_ENTROPY_MODE;
  mBframes = DEFAULT_B_FRAMES;  // UNUSED
  mStarted = false;
  mSpsPpsHeaderReceived = false;
  mSawInputEOS = false;
  mSignalledError = false;
  mIntra4x4 = false;
  mEntropyMode = true;
  mOutBufferSize = 0u;
  // TODO(kay): set it by MACRO
  mVideoColorFormat = DEFAULT_COLOR_FORMAT;
  // TODO(kay): set it also by property and MACRO
#ifndef FILE_DUMP_ENABLE
  mMemoryCopyMode = false;
#else
  mMemoryCopyMode = true;
#endif
  mEncoderInitialized = false;
  mSize = std::make_shared<C2StreamPictureSizeInfo::input>();
  mIntraRefresh = std::make_shared<C2StreamIntraRefreshTuning::output>();
  mFrameRate = std::make_shared<C2StreamFrameRateInfo::output>();
  mBitrate = std::make_shared<C2StreamBitrateInfo::output>();
  mRequestSync = std::make_shared<C2StreamRequestSyncFrameTuning::output>();
  gettimeofday(&mTimeStart, nullptr);
  gettimeofday(&mTimeEnd, nullptr);
  c2_logd(
      "init default encoder params: IDR interval %d,"
      "video color format %s, input data transfer mode: %s.",
      mIDRInterval, colorformat_to_string(mVideoColorFormat),
      mMemoryCopyMode ? "copy mode" : "zero copy mode");
  return;
}

/*
c2_status_t VencComponent::setFrameRate(const uint32_t &nFrameRate) {
  if (nFrameRate == 0u || nFrameRate > 60u) {
      nFrameRate = mFrameRate->value;
  }
  VideoEncSetParameter(mCodecCtx->pEncoder,
                       VENC_IndexParamFramerate,
                       &nFrameRate);
  c2_logd("video encoder set parameter with nFrameRate %u.", nFrameRate);
  return C2_OK;
}
*/

c2_status_t VencComponent::setBitRate() {
  C2_TRACE();
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamBitrate,
                       reinterpret_cast<void *>(&mBitrate->value));
  c2_logd("video encoder set parameter with bitrate %u.", mBitrate->value);
  return C2_OK;
}

c2_status_t VencComponent::setFrameType() {
  C2_TRACE();
  int value = 1;
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamForceKeyFrame,
                       &value);
  c2_logd("video encoder set parameter with KeyFrame %d.", value);
  return C2_OK;
}

c2_status_t VencComponent::setQp() {
  C2_TRACE();
  return C2_OK;
}

c2_status_t VencComponent::setVbvParams() {
  C2_TRACE();
  return C2_OK;
}

c2_status_t VencComponent::setGopParams() {
  C2_TRACE();
  return C2_OK;
}

c2_status_t VencComponent::setProfileParams() { return C2_OK; }

void VencComponent::initFeatureWorkFlag() {
#ifdef DETECT_MOTION
  mCodecCtx->mSetVideoSingalFlag = true;
#endif
#ifdef USE_VIDEO_SIGNAL
  mCodecCtx->mSetVideoSingalFlag = true;
#endif
#ifdef GET_MB_INFO
  mCodecCtx->mSetReceiveMBInfoFlag = true;
#endif
  // TODO(kay): set them by property_get and /vendor/etc/codec2.config
  // TODO(kay): Show them by log
}

c2_status_t VencComponent::setDetectMotion(bool isApplied) {
  if (isApplied) {
    MotionParam mMotionPara;
    mMotionPara.nMaxNumStaticFrame = 4;
    mMotionPara.nMotionDetectEnable = 1;
    mMotionPara.nMotionDetectRatio = 0;
    mMotionPara.nMV64x64Ratio = 0.01;
    mMotionPara.nMVXTh = 6;
    mMotionPara.nMVYTh = 6;
    mMotionPara.nStaticBitsRatio = 0.2;
    mMotionPara.nStaticDetectRatio = 2;
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamMotionDetectStatus,
                         &mMotionPara);
  }
  return C2_OK;
}

c2_status_t VencComponent::setVideoSignal(bool isApplied) {
  VencH264VideoSignal mVencH264VideoSignal;
  memset(&mVencH264VideoSignal, 0, sizeof(VencH264VideoSignal));
  if (isApplied) {
    mVencH264VideoSignal.video_format = DEFAULT;
    mVencH264VideoSignal.full_range_flag = 1;
    mVencH264VideoSignal.src_colour_primaries = VENC_BT709;
    mVencH264VideoSignal.dst_colour_primaries = VENC_BT709;
  } else {
    std::shared_ptr<C2StreamColorAspectsInfo::output> colorAspect =
        mParaHelper->getCodedColorAspects_l();
    if (colorAspect) {
      config_color_aspects(&mVencH264VideoSignal, colorAspect->transfer,
                           colorAspect->matrix, colorAspect->range,
                           colorAspect->primaries);
      c2_logd("setVideoSignal, config color aspects finish!");
    }
  }
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamH264VideoSignal,
                       &mVencH264VideoSignal);

  return C2_OK;
}

c2_status_t VencComponent::setReceiveMBInfo(bool isApplied) {
  if (isApplied) {
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamMBInfoOutput,
                         &mCodecCtx->sH264Ctx.sMBInfo);
  }
  return C2_OK;
}

c2_status_t VencComponent::setIntraRefreshParams() {
  VencCyclicIntraRefresh sVencCyclicIntraRefresh;
  if (mIntraRefresh->mode == C2Config::INTRA_REFRESH_DISABLED ||
      mIntraRefresh->period < 1) {
    sVencCyclicIntraRefresh.bEnable = 0;
  } else {
    sVencCyclicIntraRefresh.bEnable = 1;
    sVencCyclicIntraRefresh.nBlockNumber = mIntraRefresh->period;
  }
  VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamIntraPeriod,
                       &sVencCyclicIntraRefresh);
  c2_logd("intra fresh %s with period %d.",
          sVencCyclicIntraRefresh.bEnable ? "enabled" : "disabled",
          sVencCyclicIntraRefresh.nBlockNumber);
  return C2_OK;
}

c2_status_t VencComponent::initEncoder() {
  C2_TRACE();
  CHECK(!mStarted);
  std::shared_ptr<C2StreamGopTuning::output> gop;
  {
    mSize = mParaHelper->getSize_l();
    mBitrate = mParaHelper->getBitrate_l();
    mFrameRate = mParaHelper->getFrameRate_l();
    mIntraRefresh = mParaHelper->getIntraRefresh_l();
    if (mCodecType == VENC_CODEC_H264) {  // VER2??
      mEncLevel = (VENC_H264LEVELTYPE)mParaHelper->getAvcLevel_l();
    } else if (mCodecType == VENC_CODEC_H265) {
      mEncLevel = (VENC_H265LEVELTYPE)mParaHelper->getHevcLevel_l();
    } else {
      c2_loge("get encoder level with %d but NOT H264/H265 type.",
              mParaHelper->getAvcLevel_l());
    }
    mIInterval = mParaHelper->getSyncFramePeriod_l();
    mIDRInterval = mParaHelper->getSyncFramePeriod_l();

    gop = mParaHelper->getGop_l();
    c2_logd(
        "mSize: width %d height %d, mBitrate: %d, mFrameRate: %f,"
        "mIntraRefresh: %f, mEncLevel: %d, mIInterval: %d, mIDRInterval: %d.",
        mSize->width, mSize->height, mBitrate->value, mFrameRate->value,
        mIntraRefresh->period, mEncLevel, mIInterval, mIDRInterval);
  }

  // TODO(kay): To replace default values by static class.
  // mIInterval = mIDRInterval = DEFAULT_I_INTERVAL;
  // mFrameRate->value = DEFAULT_SRC_FRAME_RATE;
  // mBitrate->value = DEFAULT_BITRATE;

  if (gop && gop->flexCount() > 0) {
    uint32_t syncInterval = 1;
    uint32_t iInterval = 1;
    uint32_t maxBframes = 0;
    mParaHelper->ParseGop(*gop, &syncInterval, &iInterval, &maxBframes);
    if (syncInterval > 0) {
      c2_logd("Updating IDR interval from GOP: old %u new %u", mIDRInterval,
              syncInterval);
      if (mCodecType == VENC_CODEC_H264) {
        mEncLevel = mParaHelper->getAvcLevel_l();
      } else if (mCodecType == VENC_CODEC_H265) {
        mEncLevel = mParaHelper->getHevcLevel_l();
      }
      mIDRInterval = syncInterval;
    }
    if (iInterval > 0) {
      c2_logd("Updating I interval from GOP: old %u new %u", mIInterval,
              iInterval);
      mIInterval = iInterval;
    }
    if (mBframes != maxBframes) {
      c2_logd("Updating max B frames from GOP: old %u new %u", mBframes,
              maxBframes);
      mBframes = maxBframes;
    }
  }

  uint32_t width = mSize->width;
  uint32_t height = mSize->height;

  mStride = updateVencStride(width);
  c2_logd(
      "init encode params: width %d height %d level %d "
      "colorFormat %s bframes %d",
      width, height, mEncLevel, colorformat_to_string(mVideoColorFormat),
      mBframes);

  // open encoder driver
  mCodecCtx->pEncoder = VideoEncCreate(mCodecType);

  // init encoder with params
  if (mCodecType == VENC_CODEC_H264) {
    uint32_t vbv_size = 12 * 1024 * 1024;

    init_h264_param(&mCodecCtx->sH264Ctx);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamSetVbvSize,
                         &vbv_size);
    setVideoSignal(mCodecCtx->mSetVideoSingalFlag);
    setDetectMotion(mCodecCtx->mSetDetectMotionFlag);
    setReceiveMBInfo(mCodecCtx->mSetReceiveMBInfoFlag);
  } else if (mCodecType == VENC_CODEC_H265) {
    uint32_t vbv_size = 12 * 1024 * 1024;
    uint32_t value = 1;

    init_h265_param(&mCodecCtx->sH265Ctx);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamSetVbvSize,
                         &vbv_size);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamH265Param,
                         &mCodecCtx->sH265Ctx.sH265Param);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamChannelNum,
                         &value);  // TODO(kay): What is channel??
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamProcSet,
                         &mCodecCtx->sH265Ctx.sVeProcInfo);
    setReceiveMBInfo(mCodecCtx->mSetReceiveMBInfoFlag);
  } else if (mCodecType == VENC_CODEC_JPEG) {
    // jpeg mode == 1, encoding frame more than 1
    init_jpeg_param(&mCodecCtx->sJpegCtx);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamJpegEncMode,
                         &mCodecCtx->sJpegCtx.mode);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamBitrate,
                         &mCodecCtx->sJpegCtx.bitrate);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamFramerate,
                         &mCodecCtx->sJpegCtx.framerate);
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamSetBitRateRange,
                         &mCodecCtx->sJpegCtx.bitrateRange);
  }

  /* Init base config for encoder */
  // TODO(kay): How to align width and height for input and output.
  mCodecCtx->sBaseConfig.nInputWidth = (unsigned int)mSize->width;
  mCodecCtx->sBaseConfig.nInputHeight = (unsigned int)mSize->height;
  mCodecCtx->sBaseConfig.nStride = (unsigned int)mStride;
  mCodecCtx->sBaseConfig.nDstWidth = (unsigned int)mSize->width;
  mCodecCtx->sBaseConfig.nDstHeight = (unsigned int)mSize->height;
  mCodecCtx->sBaseConfig.eInputFormat = mVideoColorFormat;
  mCodecCtx->sBaseConfig.memops = MemAdapterGetOpsS();
  c2_logd(
      "set parameters with base config: input width %d, input height %d,"
      " stride %d, dst width %d, dst height %d, input format %s(%d).",
      mSize->width, mSize->height, mStride, mSize->width, mSize->height,
      colorformat_to_string(mVideoColorFormat), mVideoColorFormat);
#ifdef CONF_IMG_GPU
  mCodecCtx->sBaseConfig.nStride = ALIGN_XXB(32, mStride);
#endif
  if (mCodecCtx->usePSkip) {
    VideoEncSetParameter(mCodecCtx->pEncoder, VENC_IndexParamSetPSkip,
                         &mCodecCtx->usePSkip);
  }
  int32_t ret = VideoEncInit(mCodecCtx->pEncoder, &mCodecCtx->sBaseConfig);
  if (ret != 0) {
    VideoEncDestroy(mCodecCtx->pEncoder);
    mCodecCtx->pEncoder = nullptr;
    c2_logw("VideoEncInit failed, result: %d", ret);
    return C2_CORRUPTED;
  }
  if (mCodecType == VENC_CODEC_H264) {
    ret = VideoEncGetParameter(mCodecCtx->pEncoder, VENC_IndexParamH264SPSPPS,
                               &mCodecCtx->sHeaderdata);
  } else if (mCodecType == VENC_CODEC_H265) {
    ret = VideoEncGetParameter(mCodecCtx->pEncoder, VENC_IndexParamH265Header,
                               &mCodecCtx->sHeaderdata);
  }
  if (ret < 0) {
    c2_loge("get sps info error");
    VideoEncDestroy(mCodecCtx->pEncoder);
    mCodecCtx->pEncoder = nullptr;
    return C2_CORRUPTED;
  }

  if (mMemoryCopyMode) {
    // TODO(kay): mInternalInputBufferCount;
    mCodecCtx->sVencAllocBufferParam.nBufferNum = 4;
    // TODO(kay): figure out stride mode.
    mCodecCtx->sVencAllocBufferParam.nSizeY =
        ALIGN16(mSize->width) * ALIGN16(mSize->height) + 1024;
    mCodecCtx->sVencAllocBufferParam.nSizeC =
        (ALIGN16(mSize->width) * ALIGN16(mSize->height) >> 1) + 1024;
    ret = AllocInputBuffer(mCodecCtx->pEncoder,
                           &mCodecCtx->sVencAllocBufferParam);
    if (ret != 0) {
      VideoEncDestroy(mCodecCtx->pEncoder);
      c2_loge("AllocInputBuffer,error");
      return C2_CORRUPTED;
    }
  }
  c2_logv("init_codec successfull");

  mSpsPpsHeaderReceived = false;
  mStarted = true;
  mColorFormatChange = false;

  return C2_OK;
}

c2_status_t VencComponent::releaseEncoder() {
  C2_TRACE();
  if (!mStarted) {
    return C2_OK;
  }
  if (mMemoryCopyMode) {
    ReleaseAllocInputBuffer(mCodecCtx->pEncoder);
    mMemoryCopyMode = false;
  }
  if (mCodecCtx->sH264Ctx.sMBMode.p_info != nullptr) {
    delete mCodecCtx->sH264Ctx.sMBMode.p_info;
    mCodecCtx->sH264Ctx.sMBMode.p_info = nullptr;
  }
  if (mCodecCtx->sH265Ctx.sMBMode.p_info != nullptr) {
    delete mCodecCtx->sH265Ctx.sMBMode.p_info;
    mCodecCtx->sH265Ctx.sMBMode.p_info = nullptr;
  }
  if (mCodecCtx->pEncoder == NULL) {
    c2_loge("encoder handle is NULL!!!");
  } else {
    VideoEncDestroy(mCodecCtx->pEncoder);
  }
  mCodecCtx->pEncoder = nullptr;
  mStarted = false;
  return C2_OK;
}

c2_status_t VencComponent::copyYUVtoEncoder(VencInputBuffer *sInputBuffer,
                                            uint8_t *srcAddr, int stride,
                                            int height) {
  c2_logv(
      "pAddrVirY = %p, pAddrVirC = %p, bufAddr = %p,"
      " stride = %d, height = %d.",
      sInputBuffer->pAddrVirY, sInputBuffer->pAddrVirC, srcAddr, stride,
      height);
  memcpy(sInputBuffer->pAddrVirY, srcAddr, stride * height);
  memcpy(sInputBuffer->pAddrVirC, srcAddr + stride * height,
         stride * height * 3 >> 1);
  return C2_OK;
}

// TODO(kay): move to single utils cpp
#ifndef CONF_ARMV7_A_NEON_
c2_status_t VencComponent::ConvertRGBToPlanarYUVTwoPlane(
    uint8_t *dstY, uint8_t *dstU, size_t dstStride, size_t dstVStride,
    size_t bufferSize, const C2GraphicView &src) {
  CHECK(dstY != nullptr);
  CHECK_EQ((src.width() & 1), 0);
  CHECK_EQ((src.height() & 1), 0);

  if (dstStride * dstVStride * 3 / 2 > bufferSize) {
    c2_logd("conversion buffer is too small for converting from RGB to YUV");
    return C2_NO_MEMORY;
  }

  uint8_t *dstV = dstU + (dstStride >> 1) * (dstVStride >> 1);

  const C2PlanarLayout &layout = src.layout();
  const uint8_t *pRed = src.data()[C2PlanarLayout::PLANE_R];
  const uint8_t *pGreen = src.data()[C2PlanarLayout::PLANE_G];
  const uint8_t *pBlue = src.data()[C2PlanarLayout::PLANE_B];

#define CLIP3(x, y, z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))
  for (size_t y = 0; y < src.height(); ++y) {
    for (size_t x = 0; x < src.width(); ++x) {
      uint8_t red = *pRed;
      uint8_t green = *pGreen;
      uint8_t blue = *pBlue;

      // using ITU-R BT.601 conversion matrix
      unsigned luma =
          CLIP3(0, (((red * 66 + green * 129 + blue * 25) >> 8) + 16), 255);

      dstY[x] = luma;

      if ((x & 1) == 0 && (y & 1) == 0) {
        unsigned U =
            CLIP3(0, (((-red * 38 - green * 74 + blue * 112) >> 8) + 128), 255);

        unsigned V =
            CLIP3(0, (((red * 112 - green * 94 - blue * 18) >> 8) + 128), 255);

        dstU[x >> 1] = U;
        dstV[x >> 1] = V;
      }
      pRed += layout.planes[C2PlanarLayout::PLANE_R].colInc;
      pGreen += layout.planes[C2PlanarLayout::PLANE_G].colInc;
      pBlue += layout.planes[C2PlanarLayout::PLANE_B].colInc;
    }

    if ((y & 1) == 0) {
      dstU += dstStride >> 1;
      dstV += dstStride >> 1;
    }

    pRed -= layout.planes[C2PlanarLayout::PLANE_R].colInc * src.width();
    pGreen -= layout.planes[C2PlanarLayout::PLANE_G].colInc * src.width();
    pBlue -= layout.planes[C2PlanarLayout::PLANE_B].colInc * src.width();
    pRed += layout.planes[C2PlanarLayout::PLANE_R].rowInc;
    pGreen += layout.planes[C2PlanarLayout::PLANE_G].rowInc;
    pBlue += layout.planes[C2PlanarLayout::PLANE_B].rowInc;

    dstY += dstStride;
  }
  return C2_OK;
}
#endif

c2_status_t VencComponent::ConvertRGBAtoEncoder(
    VencInputBuffer *sInputBuffer, uint8_t *bufAddr, int width, int height,
    const C2GraphicView *const input) {
  if (width % 32 == 0) {
    unsigned char *addr[2];
    addr[0] = sInputBuffer->pAddrVirY;
    addr[1] = sInputBuffer->pAddrVirC;
#ifdef CONF_ARMV7_A_NEON
    UNUSED(input);
    int widthandstride[2];
    widthandstride[0] = width;
    widthandstride[1] = width;
    // Convert RGB to YV12, YV12 input colorFormat is REQUIRED!
    updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_YVU420SP);
    ImgRGBA2YVU420SP_neon((unsigned char *)bufAddr, addr, widthandstride,
                          height);
#else
    // Convert RGB to I420, I420 input colorFormat is REQUIRED!
    updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_YUV420P);
    ConvertRGBToPlanarYUVTwoPlane(addr[0], addr[1], width, height,
                                  width * height * 3 >> 1, *input);
#endif
  } else {
    unsigned char *addr[2];
    addr[0] = sInputBuffer->pAddrVirY;
    addr[1] = sInputBuffer->pAddrVirC;
#ifdef CONF_ARMV7_A_NEON
    int widthandstride[2];

    widthandstride[0] = width;
    widthandstride[1] = ALIGN_XXB(32, width);

    memset(addr[0], 0x80, width * height);
    memset(addr[1], 0x80, width * height >> 1);
    ImgRGBA2YVU420SP_neon((unsigned char *)bufAddr, addr, widthandstride,
                          height);
    updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_YVU420SP);
#else
    // Convert RGB to I420
    ConvertRGBToPlanarYUVTwoPlane(addr[0], addr[1], width, height,
                                  width * height * 3 >> 1, *input);
    updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_YUV420P);
#endif
  }
  c2_logd(
      "pAddrVirY = %p, pAddrVirC = %p, bufAddr = %p, "
      "width = %d, height = %d.",
      sInputBuffer->pAddrVirY, sInputBuffer->pAddrVirC, bufAddr, width, height);
  return C2_OK;
}

c2_status_t VencComponent::submitInputData(const C2GraphicView *const input,
                                           const int32_t fd, uint64_t workIndex,
                                           int64_t pts) {
  if (input->width() < mSize->width || input->height() < mSize->height) {
    /* Expect width height to be configured */
    c2_logw("unexpected Capacity Aspect %d(%d) x %d(%d)", input->width(),
            mSize->width, input->height(), mSize->height);
    return C2_BAD_VALUE;
  }
  c2_logv("input width = %d, height = %d", input->width(), input->height());

  VencInputBuffer *sInputBuffer = &mCodecCtx->sInputBuffer;
  const C2PlanarLayout &layout = input->layout();

  uint8_t *yPlane =
      const_cast<uint8_t *>(input->data()[C2PlanarLayout::PLANE_Y]);
  uint8_t *uPlane =
      const_cast<uint8_t *>(input->data()[C2PlanarLayout::PLANE_U]);
  uint8_t *vPlane =
      const_cast<uint8_t *>(input->data()[C2PlanarLayout::PLANE_V]);
  int32_t yStride = layout.planes[C2PlanarLayout::PLANE_Y].rowInc;
  int32_t uStride = layout.planes[C2PlanarLayout::PLANE_U].rowInc;
  int32_t vStride = layout.planes[C2PlanarLayout::PLANE_V].rowInc;
  uint32_t width = mSize->width;
  uint32_t stride = width;
  uint32_t height = mSize->height;
  switch (layout.type) {
    case C2PlanarLayout::TYPE_RGB:
      [[fallthrough]];
    case C2PlanarLayout::TYPE_RGBA:
      mVideoColorFormat = VENC_PIXEL_RGBA;
      updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_RGBA);
      break;
    case C2PlanarLayout::TYPE_YUV:
      if (IsYUV420(*input) && IsI420(*input)) {
        mVideoColorFormat = VENC_PIXEL_YUV420P;
        updateColorFormat(mCodecCtx->pEncoder, mVideoColorFormat);
      } else if (IsYUV420(*input) && IsI420(*input) && !IsNV21(*input) &&
                 !(IsNV12(*input))) {
        mVideoColorFormat = VENC_PIXEL_YVU420P;
        updateColorFormat(mCodecCtx->pEncoder, mVideoColorFormat);
      } else if (IsYUV420(*input) && IsNV21(*input)) {
        mVideoColorFormat = VENC_PIXEL_YVU420SP;
        updateColorFormat(mCodecCtx->pEncoder, mVideoColorFormat);
      } else if (IsYUV420(*input) && IsNV12(*input)) {
        mVideoColorFormat = VENC_PIXEL_YUV420SP;
        updateColorFormat(mCodecCtx->pEncoder, mVideoColorFormat);
      } else {
        c2_loge("unknown color format extrated from input layerout.");
      }

      break;
    default:
      c2_loge("Unrecognized plane type: %d", layout.type);
      return C2_BAD_VALUE;
  }

  c2_logv(
      "yPlane = %p, uPlane = %p. vPlane = %p. yStride = %d. "
      "uStride = %d. vStride = %d.",
      yPlane, uPlane, vPlane, yStride, uStride, vStride);
  if (!(yStride == uStride && uStride == vStride)) {
    c2_logw("y/u/v stride is not consistent.");
  }

  if (mMemoryCopyMode) {
    GetOneAllocInputBuffer(mCodecCtx->pEncoder, &mCodecCtx->sInputBuffer);
    c2_logv(
        "encoder input buffer physical addrY = %p, addrC = %p, "
        "nStride = %d, height = %d.",
        sInputBuffer->pAddrPhyY, sInputBuffer->pAddrPhyC, stride, height);
  }
  if (mSawInputEOS == true) {
    mCodecCtx->sInputBuffer.nFlag |= VENC_BUFFERFLAG_EOS;
  }
  // width and height are always even (as block size is 16x16)
  CHECK_EQ((width & 1u), 0u);
  CHECK_EQ((height & 1u), 0u);

  switch (mVideoColorFormat) {
    case VENC_PIXEL_RGBA: {
      if (yStride != ALIGN_XXB(64, width)) {
        c2_logv(
            "AndroidOpaque format (RGBX) requires 64Byte-align buffer. "
            "yStride = %d ALIGN_XXB(64, width) = %d",
            yStride, ALIGN_XXB(64, width));
      }
      // Android ScreenRecord sets ABGR color format.
      sInputBuffer->nShareBufFd = -1;
      if (!mMemoryCopyMode) {
        updateColorFormat(mCodecCtx->pEncoder, VENC_PIXEL_ABGR);
        sInputBuffer->nID = workIndex;
        sInputBuffer->nShareBufFd = fd;
        c2_logv(
            "encoder input buffer physical addrY = %p, addrC = %p, "
            "nStride = %d, height = %d.",
            sInputBuffer->pAddrPhyY, sInputBuffer->pAddrPhyC, stride, height);
      } else {
        ConvertRGBAtoEncoder(&mCodecCtx->sInputBuffer, yPlane, width, height,
                             input);
        CdcMemFlushCache(mCodecCtx->sBaseConfig.memops,
                         mCodecCtx->sInputBuffer.pAddrVirY, width * height);
        CdcMemFlushCache(mCodecCtx->sBaseConfig.memops,
                         mCodecCtx->sInputBuffer.pAddrVirC,
                         width * height >> 1);
      }
      break;
    }
    case VENC_PIXEL_YUV420P:
    case VENC_PIXEL_YVU420P:
    case VENC_PIXEL_YUV420SP:
    case VENC_PIXEL_YVU420SP:
      sInputBuffer->nShareBufFd = -1;
      if (yStride != ALIGN_XXB(16, width)) {
        c2_loge("YUV format requires 16Byte-align buffer.");
      }
      if (!mMemoryCopyMode) {
        sInputBuffer->nID = workIndex;
        sInputBuffer->nShareBufFd = fd;
        c2_logv(
            "encoder input buffer physical addrY = %p, addrC = %p, "
            "nStride = %d, height = %d.",
            sInputBuffer->pAddrPhyY, sInputBuffer->pAddrPhyC, stride, height);
      } else {
        copyYUVtoEncoder(&mCodecCtx->sInputBuffer, yPlane, mStride, height);
        CdcMemFlushCache(mCodecCtx->sBaseConfig.memops,
                         mCodecCtx->sInputBuffer.pAddrVirY, width * height);
        CdcMemFlushCache(mCodecCtx->sBaseConfig.memops,
                         mCodecCtx->sInputBuffer.pAddrVirC,
                         width * height >> 1);
      }
      break;
    default:
      c2_loge("Unrecognized plane type: %d", layout.type);
      return C2_BAD_VALUE;
  }

  if (mFrameRate->value <= 0) {
    mFrameRate->value = 30;
  }

  mCodecCtx->sInputBuffer.nPts = pts;
  // mCodecCtx->sInputBuffer.nPts = workIndex * 1000 / mFrameRate->value;

  c2_logv("add one input buffer to encoder with pts %lld.",
          mCodecCtx->sInputBuffer.nPts);
  if (mMemoryCopyMode) {
    FlushCacheAllocInputBuffer(mCodecCtx->pEncoder, &mCodecCtx->sInputBuffer);
  }
  AddOneInputBuffer(mCodecCtx->pEncoder, &mCodecCtx->sInputBuffer);
  return C2_OK;
}

void VencComponent::finishWork(uint64_t workIndex,
                               const std::shared_ptr<C2BlockPool> &pool,
                               const std::unique_ptr<C2Work> &work,
                               VencOutputBuffer *sVencOutputBuffer) {
  uint32_t nOutbufferSize = sVencOutputBuffer->nSize0 +
                            sVencOutputBuffer->nSize1 +
                            sVencOutputBuffer->nSize2;
  if (nOutbufferSize == 0u) {
    c2_loge(
        "output buffer size is 0u. venc output buffer "
        "size0 %d, size1 %d, size2 %d.",
        sVencOutputBuffer->nSize0, sVencOutputBuffer->nSize1,
        sVencOutputBuffer->nSize2);
    return;
  }
  if (!mOutBlock) {
    uint64_t grallocUsage =
        GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_SW_READ_OFTEN;
    C2MemoryUsage usage = C2AndroidMemoryUsage::FromGrallocUsage(grallocUsage);
    // TODO(kay): error handling, proper usage, etc.
    c2_status_t err = pool->fetchLinearBlock(nOutbufferSize, usage, &mOutBlock);
    if (err != C2_OK) {
      c2_loge("fetch linear block err = %d", err);
      work->result = err;
      work->workletsProcessed = 1u;
      return;
    }
  }
  C2WriteView wView = mOutBlock->map().get();
  uint8_t *addr = reinterpret_cast<uint8_t *>(wView.data());
  c2_logv(
      "OutBlock data addr %p, output buffer data0 %p (size0 %d), "
      "data1 %p (size1 %d).",
      addr, sVencOutputBuffer->pData0, sVencOutputBuffer->nSize0,
      sVencOutputBuffer->pData1, sVencOutputBuffer->nSize1);
  memcpy(addr, sVencOutputBuffer->pData0, sVencOutputBuffer->nSize0);
  if (sVencOutputBuffer->nSize1) {
    memcpy(addr + sVencOutputBuffer->nSize0, sVencOutputBuffer->pData1,
           sVencOutputBuffer->nSize1);
  }
  std::shared_ptr<C2Buffer> buffer =
      createLinearBuffer(mOutBlock, 0, nOutbufferSize);
  if (VENC_BUFFERFLAG_KEYFRAME ==
      (sVencOutputBuffer->nFlag & VENC_BUFFERFLAG_KEYFRAME)) {
    c2_logv("IDR frame produced");
    buffer->setInfo(std::make_shared<C2StreamPictureTypeMaskInfo::output>(
        0u /* stream id */, C2Config::SYNC_FRAME));
  }

  mOutBlock = nullptr;

  auto fillWork = [buffer](const std::unique_ptr<C2Work> &work) {
    work->worklets.front()->output.flags = (C2FrameData::flags_t)0;
    work->worklets.front()->output.buffers.clear();
    work->worklets.front()->output.buffers.push_back(buffer);
    work->worklets.front()->output.ordinal = work->input.ordinal;
    work->workletsProcessed = 1u;
  };
  if (work /* && c2_cntr64_t(workIndex) == work->input.ordinal.frameIndex)*/) {
    fillWork(work);
    work->worklets.front()->output.ordinal.timestamp = workIndex;
    if (mSawInputEOS) {
      work->worklets.front()->output.flags = C2FrameData::FLAG_END_OF_STREAM;
    }
  } else {
    // finish(workIndex, fillWork);
  }
}

void VencComponent::process(const std::unique_ptr<C2Work> &work,
                            const std::shared_ptr<C2BlockPool> &pool) {
  // Initialize output work
  work->result = C2_OK;
  work->workletsProcessed = 0u;
  work->worklets.front()->output.flags = work->input.flags;
  int64_t pts = (int64_t)work->input.ordinal.timestamp.peeku();

  c2_status_t error;
  int32_t ret = VENC_RESULT_OK;
  bool bOneBitstreamEncoded = false;
  uint64_t timeDelay = 0;
  uint64_t timeTaken = 0;
  uint64_t workIndex = work->input.ordinal.frameIndex.peekull();

  // Initialize encoder if not already initialized
  if (mEncoderInitialized == false) {
    if (C2_OK != initEncoder()) {
      c2_loge("Failed to initialize encoder");
      mSignalledError = true;
      work->result = C2_CORRUPTED;
      work->workletsProcessed = 1u;
      return;
    }
    mEncoderInitialized = true;
  }
  if (mSignalledError) {
    return;
  }
  // If workletsProcessed is NOT set, then work with input data would be
  // handled by CCodec and submit again??
  if (!mCodecCtx->mSpsPpsHeaderReceived) {
    if (mCodecCtx->sHeaderdata.pBuffer && mCodecCtx->sHeaderdata.nLength) {
      mCodecCtx->mSpsPpsHeaderReceived = true;
    } else {
      c2_logw("meta data has NOT been got. pBuffer %p, nLength %d.",
              mCodecCtx->sHeaderdata.pBuffer, mCodecCtx->sHeaderdata.nLength);
      return;
    }
    int32_t nSpsPphHeaderlength = mCodecCtx->sHeaderdata.nLength;
    std::unique_ptr<C2StreamInitDataInfo::output> csd =
        C2StreamInitDataInfo::output::AllocUnique(nSpsPphHeaderlength, 0u);
    if (!csd) {
      c2_loge("CSD allocation failed");
      mSignalledError = true;
      work->result = C2_NO_MEMORY;
      work->workletsProcessed = 1u;
      return;
    }
    memcpy(csd->m.value, mCodecCtx->sHeaderdata.pBuffer, nSpsPphHeaderlength);
#ifdef FILE_DUMP_ENABLE
    DUMP_TO_FILE(mOutFile, csd->m.value, nSpsPphHeaderlength);
#endif
    work->worklets.front()->output.configUpdate.push_back(std::move(csd));
    if (work->input.buffers.empty()) {
      work->workletsProcessed = 1u;
      return;
    }
  }

  // handle dynamic config parameters
  {
    // TODO(kay):
    // IntfImpl::Lock lock = mIntf->lock();
    // std::shared_ptr<C2StreamIntraRefreshTuning::output> intraRefresh =
    //    mIntf->getIntraRefresh_l();
    std::shared_ptr<C2StreamBitrateInfo::output> bitrate =
        mParaHelper->getBitrate_l();
    std::shared_ptr<C2StreamRequestSyncFrameTuning::output> requestSync =
        mParaHelper->getRequestSync_l();
    // lock.unlock();
    if (bitrate != mBitrate) {
      mBitrate = bitrate;
      setBitRate();
    }
    /*
       if (intraRefresh != mIntraRefresh) {
       mIntraRefresh = intraRefresh;
       setIntraRefresh();
       }
       */
    // We do NOT support this feature.
    if (requestSync != mRequestSync) {
      // we can handle IDR immediately
      if (requestSync->value) {
        // unset request
        C2StreamRequestSyncFrameTuning::output clearSync(0u, C2_FALSE);
        std::vector<std::unique_ptr<C2SettingResult>> failures;
        mIntf->config({&clearSync}, C2_MAY_BLOCK, &failures);
        c2_logv("Got sync request");
        setFrameType();
      }
      mRequestSync = requestSync;
    }
  }

  if (work->input.flags & C2FrameData::FLAG_END_OF_STREAM) {
    c2_logd("found input eos.");
    mSawInputEOS = true;
  }

  /* In normal mode, store inputBufferInfo and this will be returned
     when encoder consumes this input */
  // if (!mInputDataIsMeta && (inputBufferInfo != NULL)) {
  //     for (size_t i = 0; i < MAX_INPUT_BUFFER_HEADERS; i++) {
  //         if (NULL == mInputBufferInfo[i]) {
  //             mInputBufferInfo[i] = inputBufferInfo;
  //             break;
  //         }
  //     }
  // }
  std::shared_ptr<const C2GraphicView> view;
  std::shared_ptr<C2Buffer> inputBuffer;
  if (!work->input.buffers.empty()) {
    inputBuffer = work->input.buffers[0];
    view = std::make_shared<const C2GraphicView>(
        inputBuffer->data().graphicBlocks().front().map().get());
    if (view->error() != C2_OK) {
      c2_loge("graphic view map err = %d", view->error());
      work->workletsProcessed = 1u;
      return;
    }
  }

  do {
    if (mSawInputEOS && work->input.buffers.empty()) break;
    native_handle_t *pBufferHandle = const_cast<native_handle_t *>(
        inputBuffer->data().graphicBlocks()[0].handle());
    private_handle_t *hnd = reinterpret_cast<private_handle_t *>(pBufferHandle);
    if (hnd == nullptr) {
      c2_loge("private handle from c2buffer is nullptr.");
      return;
    }
    c2_logv(
        "OutBlock handle with share_fd [%d], hnd width [%d], "
        "hnd height [%d] stride[%d].",
        hnd->share_fd, hnd->width, hnd->height, hnd->stride);
    // Submit YUV data to encoder
    error = submitInputData(view.get(), hnd->share_fd, workIndex, pts);
    if (error != C2_OK) {
      c2_loge("submitInputData failed : %d", error);
      mSignalledError = true;
      work->result = error;
      work->workletsProcessed = 1u;
      return;
    }

#ifdef FILE_DUMP_ENABLE
    DUMP_TO_FILE(mInFile, mCodecCtx->sInputBuffer.pAddrVirY,
                 (mSize->height * mSize->width));  // Only For YUV input
    DUMP_TO_FILE(mInFile, mCodecCtx->sInputBuffer.pAddrVirC,
                 (mSize->height * mSize->width >> 1));  // Only For YUV input
#endif

    GETTIME(&mTimeStart, nullptr);
    /* Compute time elapsed between end of previous decode()
     * to start of current decode() */
    TIME_DIFF(mTimeEnd, mTimeStart, timeDelay);

    ret = VideoEncodeOneFrame(mCodecCtx->pEncoder);
    c2_logv("### encoder result: %s.", encode_result_to_string(ret));
    if (ret == VENC_RESULT_OK) {
      bOneBitstreamEncoded = true;
    }
    if (ret != OK) {
      // handle encode result
      c2_loge("Encode Frame failed = %s\n", encode_result_to_string(ret));
      mSignalledError = true;
      work->result = C2_CORRUPTED;
      work->workletsProcessed = 1u;
      return;
    }
  } while (ret != VENC_RESULT_OK);

  // Hold input buffer reference
  if (!mMemoryCopyMode && bOneBitstreamEncoded == true) {
    mBuffers[mCodecCtx->sInputBuffer.nID] = inputBuffer;
    c2_logv("buffer track: +++ store +++ input buffer of ID [%lu].",
            mCodecCtx->sInputBuffer.nID);
  }

  GETTIME(&mTimeEnd, nullptr);
  /* Compute time taken for decode() */
  TIME_DIFF(mTimeStart, mTimeEnd, timeTaken);

  c2_logv("timeTaken=%6llu us and delay=%6llu us.", timeTaken, timeDelay);

  /*If encoder frees up an input buffer, mark it as free */
  VencInputBuffer usedInputBuffer;
  ret = AlreadyUsedInputBuffer(mCodecCtx->pEncoder, &usedInputBuffer);
  if (!mMemoryCopyMode && ret == 0) {
    if (mBuffers.count(usedInputBuffer.nID) == 0u) {
      c2_logw("buffer not tracked with ID %lu.", usedInputBuffer.nID);
    } else {
      // Release input buffer reference
      mBuffers.erase(usedInputBuffer.nID);
      c2_logv("buffer track: --- erase --- input buffer of ID [%lu].",
              usedInputBuffer.nID);
    }
  }
  if (mMemoryCopyMode) {
    ReturnOneAllocInputBuffer(mCodecCtx->pEncoder, &usedInputBuffer);
  }

  // TODO(kay): If there are more than one piece of bitstream frame ??
  if (ValidBitstreamFrameNum(mCodecCtx->pEncoder) > 0) {
    GetOneBitstreamFrame(mCodecCtx->pEncoder, &mCodecCtx->sVencOutputBuffer);
    if (!mCodecCtx->sVencOutputBuffer.nSize0 ||
        !mCodecCtx->sVencOutputBuffer.pData0) {
      c2_loge("unexpected error when getting one bitstream frame.");
    }
    // We consider long long type as uint64_t
    uint64_t workId = (uint64_t)mCodecCtx->sVencOutputBuffer.nPts;
    finishWork(workId, pool, work, &mCodecCtx->sVencOutputBuffer);
#ifdef FILE_DUMP_ENABLE
    DUMP_TO_FILE(mOutFile, mCodecCtx->sVencOutputBuffer.pData0,
                 mCodecCtx->sVencOutputBuffer.nSize0);
    DUMP_TO_FILE(mOutFile, mCodecCtx->sVencOutputBuffer.pData1,
                 mCodecCtx->sVencOutputBuffer.nSize1);
#endif
    FreeOneBitStreamFrame(mCodecCtx->pEncoder, &mCodecCtx->sVencOutputBuffer);
  }
  if (mSawInputEOS) {
    drainInternal(C2Component::DRAIN_COMPONENT_WITH_EOS, pool, work);
  }
}

c2_status_t VencComponent::drainInternal(
    uint32_t drainMode, const std::shared_ptr<C2BlockPool> &pool,
    const std::unique_ptr<C2Work> &work) {
  bool bOneBitstreamEncoded = false;
  int32_t ret = 0;
  int64_t pts = (int64_t)work->input.ordinal.timestamp.peeku();
  c2_logv("VencComponent drainInternal......");
  if (drainMode == NO_DRAIN) {
    c2_logw("drain with NO_DRAIN: no-op");
    return C2_OK;
  }
  if (drainMode == C2Component::DRAIN_CHAIN) {
    c2_logw("DRAIN_CHAIN not supported");
    return C2_OMITTED;
  }

  while (true) {
    if (mSawInputEOS && work->input.buffers.empty()) {
      c2_loge("mSawInputEOS is true and buffer is empty");
      break;
    }

    std::shared_ptr<const C2GraphicView> view;
    std::shared_ptr<C2Buffer> inputBuffer;
    if (!work->input.buffers.empty()) {
      inputBuffer = work->input.buffers[0];
      view = std::make_shared<const C2GraphicView>(
          inputBuffer->data().graphicBlocks().front().map().get());
      if (view->error() != C2_OK) {
        c2_loge("graphic view map err = %d", view->error());
        break;
      }
    }

    native_handle_t *pBufferHandle = const_cast<native_handle_t *>(
        inputBuffer->data().graphicBlocks()[0].handle());
    private_handle_t *hnd = reinterpret_cast<private_handle_t *>(pBufferHandle);
    if (hnd == nullptr) {
      c2_loge("private handle from c2buffer is nullptr.");
      break;
    }
    c2_logv(
        "OutBlock handle with share_fd [%d], hnd width [%d], "
        "hnd height [%d] stride[%d].",
        hnd->share_fd, hnd->width, hnd->height, hnd->stride);
    // Submit YUV data to encoder
    c2_status_t error;
    uint64_t workIndex = work->input.ordinal.frameIndex.peekull();
    error = submitInputData(view.get(), hnd->share_fd, workIndex, pts);
    if (error != C2_OK) {
      c2_loge("submitInputData failed : %d", error);
      mSignalledError = true;
      work->result = error;
      work->workletsProcessed = 1u;
      break;
    }

    ret = VideoEncodeOneFrame(mCodecCtx->pEncoder);

    if (ret == VENC_RESULT_OK) {
      bOneBitstreamEncoded = true;
    }

    if (ret != VENC_RESULT_OK) {
      // handle encode result
      c2_loge("Encode Frame failed = %s\n", encode_result_to_string(ret));
      mSignalledError = true;
      work->result = C2_CORRUPTED;
      work->workletsProcessed = 1u;
      return C2_CORRUPTED;
    }
    /*If encoder frees up an input buffer, mark it as free */
    VencInputBuffer usedInputBuffer;
    ret = AlreadyUsedInputBuffer(mCodecCtx->pEncoder, &usedInputBuffer);
    if (ret == 0) {
      if (mBuffers.count(usedInputBuffer.nID) == 0u) {
        c2_logd("buffer not tracked");
      } else {
        // Release input buffer reference
        mBuffers.erase(usedInputBuffer.nID);
        c2_logv("buffer track: --- erase --- input buffer of ID [%lu].",
                usedInputBuffer.nID);
      }
    }

    if (ValidBitstreamFrameNum(mCodecCtx->pEncoder) > 0) {
      GetOneBitstreamFrame(mCodecCtx->pEncoder, &mCodecCtx->sVencOutputBuffer);
      // We consider long long type as uint64_t
      uint64_t workId = (uint64_t)mCodecCtx->sVencOutputBuffer.nPts;
      finishWork(workId, pool, work, &mCodecCtx->sVencOutputBuffer);
      FreeOneBitStreamFrame(mCodecCtx->pEncoder, &mCodecCtx->sVencOutputBuffer);
    } else {
      if (work->workletsProcessed != 1u) {
        work->worklets.front()->output.flags = work->input.flags;
        work->worklets.front()->output.ordinal = work->input.ordinal;
        work->worklets.front()->output.buffers.clear();
        work->workletsProcessed = 1u;
      }
      break;
    }
  }

  if (work->workletsProcessed != 1u) {
    work->worklets.front()->output.flags = work->input.flags;
    work->worklets.front()->output.ordinal = work->input.ordinal;
    work->worklets.front()->output.buffers.clear();
    work->workletsProcessed = 1u;
  }

  return C2_OK;
}

c2_status_t VencComponent::drain(uint32_t drainMode,
                                 const std::shared_ptr<C2BlockPool> &pool) {
  return drainInternal(drainMode, pool, nullptr);
}

std::shared_ptr<C2Buffer> VencComponent::createLinearBuffer(
    const std::shared_ptr<C2LinearBlock> &block) {
  return createLinearBuffer(block, block->offset(), block->size());
}

std::shared_ptr<C2Buffer> VencComponent::createLinearBuffer(
    const std::shared_ptr<C2LinearBlock> &block, size_t offset, size_t size) {
  return C2Buffer::CreateLinearBuffer(block->share(offset, size, ::C2Fence()));
}

std::shared_ptr<C2Buffer> VencComponent::createGraphicBuffer(
    const std::shared_ptr<C2GraphicBlock> &block) {
  return createGraphicBuffer(block, C2Rect(block->width(), block->height()));
}

std::shared_ptr<C2Buffer> VencComponent::createGraphicBuffer(
    const std::shared_ptr<C2GraphicBlock> &block, const C2Rect &crop) {
  return C2Buffer::CreateGraphicBuffer(block->share(crop, ::C2Fence()));
}

}  // namespace android
