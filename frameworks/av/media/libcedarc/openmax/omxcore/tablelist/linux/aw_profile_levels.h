/*
 * =====================================================================================
 *   Copyright (c)  Allwinner Technology Co. Ltd.
 *   All rights reserved.
 *    Filename:  omx_pub_def.h
 *    Description:
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:
 *
 *         Author:  Gan Qiuye(ganqiuye@allwinnertech.com)
 *        Company:  Allwinnertech.com
 *
 * =====================================================================================
 */
#ifndef __AW_PROFILE_LEVELS_H__
#define __AW_PROFILE_LEVELS_H__

 /* H.263 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE DecoderSupportedH263ProfileLevels[] = {
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
  {-1, -1}};

/* MPEG4 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE DecoderSupportedMPEG4ProfileLevels[] ={
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5},
  {-1,-1}};

/* AVC Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE DecoderSupportedAVCProfileLevels[] ={
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
  {-1,-1}};

VIDEO_PROFILE_LEVEL_TYPE DecoderSupportedHEVCProfileLevels[] = {
  { OMX_VIDEO_HEVCProfileMain, OMX_VIDEO_HEVCMainTierLevel51 },
  {-1,-1}};

/* MPEG2 Supported Levels & profiles */
//CTS test: android.media.cts.AdaptivePlaybackTest#testMpeg2_adaptiveDrc, etc
VIDEO_PROFILE_LEVEL_TYPE DecoderSupportedMPEG2ProfileLevels[] = {
  {OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelHL},
  {OMX_VIDEO_MPEG2ProfileMain  , OMX_VIDEO_MPEG2LevelHL},
  {-1,-1}};

#endif
