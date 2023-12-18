/******************************************************************************
 *
 *  Copyright (C) 2017 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_vendor_brcm.h
 *
 *  Description:   A wrapper header file of bt_vendor_lib.h
 *
 *                 Contains definitions specific for interfacing with Broadcom
 *                 Bluetooth chipsets
 *
 ******************************************************************************/

#ifndef BT_LIBBT_INCLUDE_BT_VENDOR_SPRD_OSI_H_
#define BT_LIBBT_INCLUDE_BT_VENDOR_SPRD_OSI_H_

/* A2DP COMPAT  Start */
#if (defined (OSI_COMPAT_ANROID_6_0) || defined (OSI_COMPAT_ANROID_4_4_4))
#include "a2d_api.h"
#include "a2d_sbc.h"

#ifndef A2DP_SBC_IE_SAMP_FREQ_16
#define A2DP_SBC_IE_SAMP_FREQ_16 A2D_SBC_IE_SAMP_FREQ_16
#endif
#ifndef A2DP_SBC_IE_SAMP_FREQ_32
#define A2DP_SBC_IE_SAMP_FREQ_32 A2D_SBC_IE_SAMP_FREQ_32
#endif
#ifndef A2DP_SBC_IE_SAMP_FREQ_44
#define A2DP_SBC_IE_SAMP_FREQ_44 A2D_SBC_IE_SAMP_FREQ_44
#endif
#ifndef A2DP_SBC_IE_SAMP_FREQ_48
#define A2DP_SBC_IE_SAMP_FREQ_48 A2D_SBC_IE_SAMP_FREQ_48
#endif
#ifndef A2DP_SBC_IE_CH_MD_MONO
#define A2DP_SBC_IE_CH_MD_MONO A2D_SBC_IE_CH_MD_MONO
#endif
#ifndef A2DP_SBC_IE_CH_MD_DUAL
#define A2DP_SBC_IE_CH_MD_DUAL A2D_SBC_IE_CH_MD_DUAL
#endif
#ifndef A2DP_SBC_IE_CH_MD_STEREO
#define A2DP_SBC_IE_CH_MD_STEREO A2D_SBC_IE_CH_MD_STEREO
#endif
#ifndef A2DP_SBC_IE_CH_MD_JOINT
#define A2DP_SBC_IE_CH_MD_JOINT A2D _SBC_IE_CH_MD_JOINT
#endif
#ifndef A2DP_SBC_IE_BLOCKS_4
#define A2DP_SBC_IE_BLOCKS_4 A2D_SBC_IE_BLOCKS_4
#endif
#ifndef A2DP_SBC_IE_BLOCKS_8
#define A2DP_SBC_IE_BLOCKS_8 A2D_SBC_IE_BLOCKS_8
#endif
#ifndef A2DP_SBC_IE_BLOCKS_12
#define A2DP_SBC_IE_BLOCKS_12 A2D_SBC_IE_BLOCKS_12
#endif
#ifndef A2DP_SBC_IE_BLOCKS_16
#define A2DP_SBC_IE_BLOCKS_16 A2D_SBC_IE_BLOCKS_16
#endif
#ifndef A2DP_SBC_IE_ALLOC_MD_S
#define A2DP_SBC_IE_ALLOC_MD_S A2D_SBC_IE_ALLOC_MD_S
#endif
#ifndef A2DP_SBC_IE_ALLOC_MD_L
#define A2DP_SBC_IE_ALLOC_MD_L A2D_SBC_IE_ALLOC_MD_L
#endif
#ifndef A2DP_SBC_IE_SUBBAND_4
#define A2DP_SBC_IE_SUBBAND_4 A2D_SBC_IE_SUBBAND_4
#endif
#ifndef A2DP_SBC_IE_SUBBAND_4
#define A2DP_SBC_IE_SUBBAND_8 A2D_SBC_IE_SUBBAND_8
#endif

/* A2DP COMPAT  End */




#else //endif OSI_COMPAT_ANROID_6_0
#include "a2dp_constants.h"
#include "a2dp_sbc_constants.h"
#include "a2dp_error_codes.h"

#endif

#endif  // BT_LIBBT_INCLUDE_BT_VENDOR_SPRD_OSI_H_

