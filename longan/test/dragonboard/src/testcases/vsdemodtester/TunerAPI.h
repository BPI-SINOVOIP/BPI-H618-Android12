/**
*     @file       TunerAPI.h
*
*	@author 	Alex Li (alexli@trident.com.cn)
*
*	@date    	2011/5/6
*
* @if copyright_display
*		Copyright (c) 2011 Trident Microsystems, Inc.
*		All rights reserved
*
*	 	The content of this file or document is CONFIDENTIAL and PROPRIETARY
*		to Trident Microsystems, Inc.  It is subject to the terms of a
*		License Agreement between Licensee and Trident Microsystems, Inc.
*		restricting among other things, the use, reproduction, distribution
*		and transfer.  Each of the embodiments, including this information and
*		any derivative work shall retain this copyright notice.
* @endif
*
*		This tuner test case can cover all avaible demod/tuner cases for dvb.
*
* @if modification_History
*
*		<b>Modification History:\n</b>
*		Date				Name			Comment \n\n
*
*
* @endif
*/
#include <string.h>
#include <stdlib.h>

#include "tplf_fe.h"
#include "trid_errno.h"
#include "trid_util_debug.h"
#include "trid_util_info.h"
#include "trid_util_regmap.h"
#include "trid_xmlparser_api.h"

#define DVB_ATV 1
#define DVB_DTV 2

#ifndef _TUNER_API_H_
#define _TUNER_API_H_

// ATSC
RETURN_TYPE ATSC_TunerInit(void);
RETURN_TYPE ATSC_TuneDTVChan(TunerStdType_e TunerStdType, trid_uint8 ChanNo, ModulationMode_e QamMode);
RETURN_TYPE ATSC_TuneATVChan(TunerStdType_e TunerStdType, trid_uint8 ChanNo, trid_sint32 AdjustFreqKHZ);
trid_bool ATSC_TunerIsLocked(TunerType_e TunerType);
void ATSC_TunerSignalCheck(TunerType_e TunerType, void *pQualityInfo, void *pStrengthInfo);

// DVB
RETURN_TYPE DVB_TunerInit(int TunerType);
RETURN_TYPE DVB_TuneATVChan(int sound_standard, int FreqKHz);
RETURN_TYPE DVB_TuneDTVChan(TunerType_e TunerType, DVBScanParam_t* pScanParam);
trid_bool DVB_TunerIsLocked(TunerType_e TunerType);
RETURN_TYPE DVB_SetConfig(trid_bool bAudoSymMode);  // for DVBC only
void DVB_TunerSignalCheck(TunerType_e TunerType, void *pQualityInfo, void *pStrengthInfo);

// ISDB-T
RETURN_TYPE ISDBT_SetTunerByChanNo(TunerType_e TunerType, int ChanNo);
trid_bool ISDBT_TunerIsLocked(TunerType_e TunerType);

#endif
