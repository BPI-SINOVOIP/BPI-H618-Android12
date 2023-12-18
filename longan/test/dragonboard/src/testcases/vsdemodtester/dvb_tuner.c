/**
*     @file       dvb_tuner.c
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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

// for g++ compile
#if defined __cplusplus || defined __cplusplus__
extern "C" {
#endif

#include "tplf_fe.h"
#include "trid_errno.h"
#include "trid_util_debug.h"
#include "trid_util_info.h"
#include "trid_xmlparser_api.h"
#include "TunerAPI.h"

// tuner driver headfile, g++ can't support put the headfile in the middle.
#include "Trid_DRX_API.h"

#if defined __cplusplus || defined __cplusplus__
}
#endif
TRID_DEBUG_CATEGORY_EXTERN(dvb_debug);
#ifdef TRID_CAT_DEFAULT
#undef TRID_CAT_DEFAULT
#endif
#define TRID_CAT_DEFAULT (dvb_debug)

#if 1
#ifdef TRID_ERROR
#undef TRID_ERROR
#endif
#define TRID_ERROR                            \
    printf("[%s-%d]==>", __func__, __LINE__); \
    printf

#ifdef TRID_INFO
#undef TRID_INFO
#endif
#define TRID_INFO                             \
    printf("[%s-%d]==>", __func__, __LINE__); \
    printf
#endif

typedef TunerDriver_t* (*DemodFuncHandler)(void);

static trid_bool bFeInitDone = trid_false;
static TunerDriver_t* pFETuner = NULL;

static RETURN_TYPE DVB_LoadTunerParameter(int TunerType, TplfFeInitParam_t* pInitParam) {
    RETURN_TYPE ret;
    void* pFileBuffer;
    trid_sint32* pIntArr;
    trid_sint32 arrNum = 0;
    void* node;

    SAINT_CHECK(pInitParam != NULL, SYS_BAD_PARAM);

    pInitParam->feType = TPLF_FE_TYPE_DVB;

    /* Step1 create */
    pFileBuffer = Trid_Util_XmlParser_Create((trid_char*)"/etc/dvb_system.xml");
    if (!pFileBuffer) {
        TRID_ERROR("Failed to parse file [dvb_system.xml]\n");
        return SYS_FAILED;
    }
    if (TunerType == DVB_DTV || TunerType == DVB_ATV) {
        // Get tuner  node
        node = Trid_Util_XmlParser_GetNode(pFileBuffer, (trid_char*)"Tuner", NULL);
        if (node == NULL) {
            TRID_ERROR("Get Display Node failed!\n");
            return SYS_FAILED;
        }
        // Get Tuner->DemodType
        ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"DVBT2C_DemodType", NULL);
        if (!ret) {
            SAINT_CHECK(0, SYS_FAILED);
        } else {
            pInitParam->dvbtParam.DVBTC_DemodType = (DemodType_e)pIntArr[0];
        }
        // Get Tuner->RFType
        ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"DVBT2C_RFType", NULL);
        if (!ret) {
            SAINT_CHECK(0, SYS_FAILED);
        } else {
            pInitParam->dvbtParam.DVBTC_RFType = (RFType_e)pIntArr[0];
        }

        // Get Tuner->SerialTS
        ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"DVBT2C_SerialTS", NULL);
        if (!ret) {
            SAINT_CHECK(0, SYS_FAILED);
        } else {
            pInitParam->dvbtParam.DVBTC_TSOutputMode = (TunerMpegOutMod_e)pIntArr[0];
        }

        // Get RFI2cAddr
        ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"DVBT2C_RFI2cAddr", NULL);
        if (!ret) {
            SAINT_CHECK(0, SYS_FAILED);
        } else {
            pInitParam->dvbtParam.DVBTC_RFI2cAddr = (ModulationMode_e)pIntArr[0];
        }
    }

    // Free node
    if (node) Trid_Util_XmlParser_Release(node);

    return SYS_NOERROR;
}

static DemodFuncHandler DVB_LoadDemod(const char* DemodLibName) {
    DemodFuncHandler pDemodFuncHandler = NULL;
    void* pHandler = NULL;

    dlerror();  // clear error in case
    pHandler = dlopen(DemodLibName, RTLD_NOW | RTLD_GLOBAL);
    if (!pHandler) {
        TRID_ERROR("dlopen %s\n ERROR: %s", DemodLibName, dlerror());
        return NULL;
    }

    pDemodFuncHandler = (DemodFuncHandler)dlsym(pHandler, "Trid_Demod_GetHandler");
    if (!pDemodFuncHandler) {
        TRID_ERROR("Can't find symbol: %s, Error: %s\n", "Trid_Demod_GetHandler", dlerror());
        dlclose(pHandler);
        pHandler = NULL;
    }

    return pDemodFuncHandler;
}

static RETURN_TYPE DVB_InitParam(TplfFeInitParam_t* pInitParam) {
    trid_sint32 ret = SYS_FAILED;
    TplfDVBTFeInitParam_t* pTunerParam;
    DemodFuncHandler handler;

    if (!pInitParam) {
        TRID_ERROR("Init Param is null\n");
        return SYS_NULL_POINTER;
    }

    if (TPLF_FE_TYPE_DVB != pInitParam->feType) {
        TRID_ERROR("Not DVB TYPE\n");
        return SYS_BAD_PARAM;
    }

    pTunerParam = &(pInitParam->dvbtParam);
    SAINT_CHECK(pTunerParam->DVBTC_DemodType < DemodType_MAX, SYS_BAD_PARAM);

    bFeInitDone = trid_false;
    if (0) {
        ;  // for code format
    }
#if defined(PRO_SXL)
    else if (pTunerParam->DVBTC_DemodType == DemodType_DRX_SXL) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsxl.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_SXL, RF:%d, RFI2cAdd:0x%x,SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
                  DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SXL;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif

#if defined(PRO_UXL)
    else if (pTunerParam->DVBTC_DemodType == DemodType_DRX_UXL) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxuxl.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_UXL, RF:%d, RFI2cAdd:0x%x,SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
                  DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_UXL;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif

#if defined(PRO_FUSION)
    else if (pTunerParam->DVBTC_DemodType == DemodType_DRX_Fusion) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxfusion.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType:DRX_Fusion  RF:%d, RFI2cAdd:0x%x, SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
                  DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_Fusion;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif
#if defined(PRO_SX6)
    else if (pTunerParam->DVBTC_DemodType == DemodType_DRX_SX6) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsx6.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType:DRX_SX6  RF:%d, RFI2cAdd:0x%x, SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
                  DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SX6;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif
#if defined(PRO_SX7) || defined(PRO_SX7G) || defined(PRO_SX8) || defined(PRO_UNION1) || defined(PRO_UNION2) || defined(TV303)
    else if (pTunerParam->DVBTC_DemodType == DemodType_DRX_SX6) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsx7.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType:DRX_SX7  RF:%d, RFI2cAdd:0x%x, SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
                  DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.RFInitParam[0].I2CBusNo = 2;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SX6;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#if defined(TV303)
    else if (pTunerParam->DVBTC_DemodType == DemodType_ATBM783X) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libAtbm783x.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType:DRX_ATBM783X  RF:%d, RFI2cAdd:0x%x, SHLib:%s\n", pTunerParam->DVBTC_RFType,
                  pTunerParam->DVBTC_RFI2cAddr, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SX6;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif

#endif
#if defined(SILABS_DVBT2_SUPPORT)
    else if (pTunerParam->DVBTC_DemodType == DemodType_SI_2168) {
// Added for SiLab DVBT2 demod.
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#include "Si2168_API.h"
#endif
#define DEMOD_SHLIB_NAME "libSi2168.so"
        Si2168InitParam_t InitParam;
        printf("Choose DemodType:Si2168  RF:%d, RFI2cAdd:0x%x, SHLib:%s\n", pTunerParam->DVBTC_RFType, pTunerParam->DVBTC_RFI2cAddr,
               DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->DVBTC_RFType;
        InitParam.I2CAddr = pTunerParam->DVBTC_RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->DVBTC_TSOutputMode;
        handler = DVB_LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SX6;
            printf("calling si2168 Init\n");
            ret = pFETuner->Init(&InitParam);
            printf("si2168 Init Done\n");
            bFeInitDone = trid_true;
        }
    }
#endif

    else {
        TRID_ERROR("Find no support Demod Type\n");
        SAINT_CHECK(0, SYS_FAILED);
        return SYS_FAILED;
    }

    if (!bFeInitDone) {
        TRID_ERROR("Failed to init demod.\n");
        ret = SYS_FAILED;
    } else {
        TRID_INFO("Init demod [%s] successfully\n", DEMOD_SHLIB_NAME);
        ret = SYS_NOERROR;
    }

    return ret;
}

RETURN_TYPE DVB_TuneDTVChan(TunerType_e TunerType, DVBScanParam_t* pScanParam) {
    TuneDTVChanInfo_t TuneChan;

    TuneChan.TunerType = TunerType;
    TuneChan.Param.Freq = pScanParam->Freq;
    TuneChan.Param.SymbolRate = pScanParam->SymbolRate;  // For DVBT, the symbol rate is no use.
    TuneChan.Param.ModulationMode = pScanParam->ModulationMode;
    TuneChan.Param.BandWidth = pScanParam->BandWidth;  //_RF_BANDWIDTH_DEFAULT_;
    TuneChan.Param.priority = pScanParam->priority;

    return pFETuner->SetAttr(0, _TUNE_DTV_CHAN_, &TuneChan);
}

// standard: 1:M 2:BG 3:I 4:DK 5:L 6:L2
RETURN_TYPE DVB_TuneATVChan(int sound_standard, int FreqKHz) {
    TunerInfo_e TunerInfo;

    if (0 == sound_standard)  // auto, set to BG
        sound_standard = 2;   // BG
    if (0 == FreqKHz) FreqKHz = 72250;

    TunerInfo.SoundSystemInfo.SoundStandard = sound_standard;

    pFETuner->SetAttr(0, _SET_SOUND_SYSTEM_, &TunerInfo);
    TunerInfo.TuneATVChanInfo.FreqKHZ = FreqKHz;

    return pFETuner->SetAttr(0, _TUNE_ATV_CHAN_, &TunerInfo);
}

trid_bool DVB_TunerIsLocked(TunerType_e TunerType) {
    // RETURN_TYPE ret;
    TunerLockInfo_t TunerLock;

    TunerLock.TunerType = TunerType;

    pFETuner->GetAttr(0, _CHECK_LOCK_, &TunerLock);
    return TunerLock.bLock;
}

RETURN_TYPE DVB_TunerInit(int Tunertype) {
    TplfFeInitParam_t InitParam;
    RETURN_TYPE ret;

    // Load parameters
    ret = DVB_LoadTunerParameter(Tunertype, &InitParam);
    if (ret != SYS_NOERROR) {
        TRID_ERROR("Failed to load tuner parameter.\n");
        return SYS_FAILED;
    }

    // Init
    ret = DVB_InitParam(&InitParam);
    if (ret != SYS_NOERROR) {
        TRID_ERROR("Failed to init DVB parameter.\n");
        return SYS_FAILED;
    }

    return ret;
}

RETURN_TYPE DVB_SetConfig(trid_bool bAudoSymMode) { return pFETuner->SetAttr(0, _SET_DVBC_CONFIG_, &bAudoSymMode); }

void DVB_TunerSignalCheck(TunerType_e TunerType, void *pQualityInfo, void *pStrengthInfo) {
    // RETURN_TYPE ret;

    TunerSignalQualityInfo_t *SignalQualityInfo;
    TunerSignalStrengthInfo_t *SignalStrength;

    SignalQualityInfo = (TunerSignalQualityInfo_t*)pQualityInfo;
    SignalStrength    = (TunerSignalStrengthInfo_t*)pStrengthInfo;
    // ret = SYS_FAILED;
    if ((TunerType == _AIR_DIGITAL_TUNER_) || (TunerType == _CABLE_DIGITAL_TUNER_)) {
        SignalQualityInfo->TunerType = TunerType;

        pFETuner->GetAttr(0, _GET_SIGNAL_QUALITY_, SignalQualityInfo);

        switch (SignalQualityInfo->SQLevel) {
            case _TUNER_SIGNAL_GOOD_:
                printf("Signal Quality: \33[1m\033[40;31mGood\033[0m, value=%ld\n", SignalQualityInfo->Value);
                break;

            case _TUNER_SIGNAL_NORMAL_:
                printf("Signal Quality: \33[1m\033[40;31mNormal\033[0m,value=%ld\n", SignalQualityInfo->Value);
                break;

            case _TUNER_SIGNAL_POOR_:
                printf("Signal Quality: \33[1m\033[40;31mPoor\033[0m,value=%ld\n", SignalQualityInfo->Value);
                break;

            default:

                break;
        }
        printf("        CN value=%ld,  CN Power=%ld\n", SignalQualityInfo->CN_Value, SignalQualityInfo->CN_Power);
        printf("        BER value=%ld, BER Power=%ld\n", SignalQualityInfo->BER_Value, SignalQualityInfo->BER_Power);
        printf("        Packet Error Number=%ld\n", SignalQualityInfo->PacketErrorNum);

        pFETuner->GetAttr(0, _GET_SIGNAL_STRENGTH_, SignalStrength);
        printf("Signal strength=%ld\n", SignalStrength->Value);
    }

    return;
}

RETURN_TYPE ISDBT_SetTunerByChanNo(TunerType_e TunerType, int ChanNo) {
    RETURN_TYPE ret = SYS_NOERROR;
    trid_uint8 TunerID = 0;
    trid_uint32 CmdID = 0;
    (void)ret;

    AribTuneByFreq_t sArib_TuneByFreq = {0};

    CmdID = _TUNE_ARIB_CHANNO_;
    sArib_TuneByFreq.AdjustFreqKHZ = 0;
    sArib_TuneByFreq.ChanNo = ChanNo;
    sArib_TuneByFreq.TunerType = TunerType;

    return pFETuner->SetAttr(TunerID, CmdID, &sArib_TuneByFreq);
}

trid_bool ISDBT_TunerIsLocked(TunerType_e TunerType) {
    TunerLockInfo_t TunerLock;

    TunerLock.TunerType = TunerType;

    pFETuner->GetAttr(0, _CHECK_LOCK_, &TunerLock);
    return TunerLock.bLock;
}
