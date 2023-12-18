/**
*     @file       test_atsc.c
*
*	@author 	Alex Li (alexli@trident.com.cn)
*
*	@date    	2010/6/25
*
* @if copyright_display
*		Copyright (c) 2010 Trident Microsystems, Inc.
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
*		This tuner test case can cover all avaible demod/tuner cases for atsc.
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

// tuner driver headfile, g++ can't support put the headfile in the middle.
//#include "DRX_39xyJ2_API.h"
//#include "S5H1409_API.h"
#include "Trid_DRX_API.h"

#if defined __cplusplus || defined __cplusplus__
}
#endif

//#define DRX_39xyJ2_SUPPORT
//#define S5H1409_SUPPORT
//#define TDM_5550_SUPPORT
//#define DRX_SXL_SUPPORT
#define DRX_FUSION_SUPPORT

TRID_DEBUG_CATEGORY_EXTERN(atsc_debug);
#ifdef TRID_CAT_DEFAULT
#undef TRID_CAT_DEFAULT
#endif
#define TRID_CAT_DEFAULT (atsc_debug)

typedef TunerDriver_t* (*DemodFuncHandler)(void);

static trid_bool bFeInitDone = trid_false;
static TunerDriver_t* pFETuner = NULL;
static trid_bool bAFCSupport = trid_false;

static RETURN_TYPE LoadTunerParameter(TplfFeInitParam_t* pInitParam) {
    RETURN_TYPE ret;
    void* pFileBuffer;
    trid_sint32* pIntArr;
    trid_sint32 arrNum = 0;
    void* node;

    SAINT_CHECK(pInitParam != NULL, SYS_BAD_PARAM);

    pInitParam->feType = TPLF_FE_TYPE_ATSC;

    /* Step1 create */
    pFileBuffer = Trid_Util_XmlParser_Create((trid_char*)"/etc/atsc_system.xml");
    if (!pFileBuffer) {
        TRID_ERROR("Failed to parse file [atsc_system.xml]\n");
        return SYS_FAILED;
    }

    // Get tuner  node
    node = Trid_Util_XmlParser_GetNode(pFileBuffer, (trid_char*)"Tuner", NULL);
    if (node == NULL) {
        TRID_ERROR("Get Display Node failed!\n");
        return SYS_FAILED;
    }
    // Get Tuner->DemodType
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"DemodType", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.DemodType = (DemodType_e)pIntArr[0];
    }
    // Get Tuner->RFType
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"RFType", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.RFType = (RFType_e)pIntArr[0];
    }
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, "Val", -1, "RFI2cAddr", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.RFI2cAddr = pIntArr[0];
    }

    // Get Tuner->SerialTS
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"SerialTS", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.TSOutputMode = (TunerMpegOutMod_e)pIntArr[0];
    }

    // Get Tuner->SupportAFC
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"SupportAFC", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        bAFCSupport = pIntArr[0];
    }

    // Get cable DTV try times.
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"CableTryTime", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.CableTryTimes = pIntArr[0];
    }

    // Get Air DTV try times
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"AirTryTime", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.AirTryTimes = pIntArr[0];
    }

    // Get Analog try times
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, (trid_char*)"Val", -1, (trid_char*)"AnalogTryTime", NULL);
    if (!ret) {
        SAINT_CHECK(0, SYS_FAILED);
    } else {
        pInitParam->atscParam.AnalogTryTimes = pIntArr[0];
    }

    // Get I2C Bus No.
    ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, "Val", -1, "I2CBusNo", NULL);
    if (!ret) {
        pInitParam->atscParam.I2CBusNo = 0;  // By default, we configure it to /dev/i2c-0
    } else {
        pInitParam->atscParam.I2CBusNo = pIntArr[0];
    }

#if 0
	//Get Qam Mode
	ret = Trid_Util_XmlParser_GetInt(node, &arrNum, &pIntArr, ( trid_char*)"Val", -1, (trid_char*)"QamMode", NULL);
	if(!ret)
	{
		SAINT_CHECK(0, SYS_FAILED);
	}
	else
	{
		pInitParam->atscParam.QamMode =(ModulationMode_e) pIntArr[0];
	}
#endif
    // Free node
    if (node) Trid_Util_XmlParser_Release(node);

    return SYS_NOERROR;
}

static RETURN_TYPE PrintTunerParam(TplfFeInitParam_t* pInitParam) {
    RETURN_TYPE ret = SYS_NOERROR;

    SAINT_CHECK(pInitParam != NULL, SYS_BAD_PARAM);

    printf("\n==================================================================================\n");
    switch (pInitParam->atscParam.DemodType) {
        case DemodType_DRX_39xyJ2:
            printf("Demod Type: DRX_39xyJ2 ");
            break;

        case DemodType_S5H1409:
            printf("Demod Type: S5H1409 ");
            break;

        case DemodType_S5H1411:
            printf("Demod Type: S5H1411 ");
            break;

        case DemodType_TDM5550_ATSC:
            printf("Demod Type: TDM5550 ");
            break;

        case DemodType_DRX_SXL:
            printf("Demod Type: DRX_SXL ");
            break;

        case DemodType_DRX_UXL:
            printf("Demod Type: DRX_UXL ");
            break;
        case DemodType_DRX_SX6:
            printf("Demod Type: DRX_SX6 ");
            break;

        default:
            printf("Demod Type: Unknown\n");
            ret = SYS_BAD_PARAM;
            break;
    }

    switch (pInitParam->atscParam.RFType) {
        case _RF_Nutune_FK160X_:
            printf(" RF Type: _RF_Nutune_FK160X_ ");
            break;

        case _RF_DTT768x0_:
            printf(" RF Type: DTT768x0 ");
            break;

        case _RF_DTT7611_:
            printf(" RF Type: DTT7611 ");
            break;

        case _RF_DTVS205FH201A_:
            printf(" RF Type: DTVS205FH201A ");
            break;

        case _RF_ALPSTDQU4_:
            printf(" RF Type: ALPSTDQU4 ");
            break;

        case _RF_TDVFH072F_:
            printf(" RF Type: TDVFH072F ");
            break;

        case _RF_MT2131_:
            printf(" RF Type: MT2131 ");
            break;

        case _RF_DTT768xxEXTIFAMP_:
            printf(" RF Type: DTT768xxEXTIFAMP ");
            break;

        case _RF_CTF582X_:
            printf(" RF Type: CTF582X ");
            break;

        case _RF_MT2063_:
            printf(" RF Type: MT2063 ");
            break;

        case _RF_TDA18271C2_:
            printf(" RF Type: TDA18271C2 ");
            break;

        case _RF_U4A20A_:
            printf(" RF Type: U4A20A ");
            break;

        case _RF_VA1Y9UF2002_:
            printf(" RF Type: VA1Y9UF2002 ");
            break;

        case _RF_UVS1805BDMW1_:
            printf(" RF Type: UVS1805BDMW1 ");
            break;

        case _RF_DTVS205CH201A_:
            printf(" RF Type: DTVS205CH201A ");
            break;

        case _RF_DTVS20CFL081A_:
            printf(" RF Type: DTVS20CFL081A ");
            break;

        case _RF_DTVS20FFL102A_:
            printf(" RF Type: DTVS20FFL102A ");
            break;

        case _RF_TD1136_:
            printf(" RF Type: TD1136 ");
            break;

        case _RF_TA231x_:
            printf(" RF Type: TA231x ");
            break;

        case _RF_TDA18272_:
            printf(" RF Type: TDA18272 ");
            break;

        case _RF_VA1E1UF2402_:
            printf(" RF Type: VA1E1UF2402 ");
            break;

        default:
            printf(" RF Type: Unknown ");
            ret = SYS_BAD_PARAM;
            break;
    }

    // TS
    if (pInitParam->atscParam.TSOutputMode == PARALLEL_OUTPUT)
        printf(" TS output: Parallel  ");
    else if (pInitParam->atscParam.TSOutputMode == SERIAL_OUTPUT)
        printf(" TS output: Serial  ");
#if 0	
	//QAM mode
	if(pInitParam->atscParam.QamMode ==_QAM256_)
		printf(" QAM mode: 256 ");
	else if(pInitParam->atscParam.QamMode ==_QAM64_)
		printf(" QAM mode: 64 ");
	else if(pInitParam->atscParam.QamMode ==_DEFAULT_MODULATION_MODE_)
		printf(" QAM mode: auto ");
#endif
    printf("\n==================================================================================\n\n");

    return ret;
}

static DemodFuncHandler LoadDemod(const char* DemodLibName) {
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

static RETURN_TYPE ATSC_InitParam(TplfFeInitParam_t* pInitParam) {
    RETURN_TYPE ret;
    TplfATSCFeInitParam_t* pTunerParam;
    DemodFuncHandler handler;

    TRID_INFO("begin to init the demod. \n");

    if (!pInitParam) {
        TRID_ERROR("Init Param is null.\n");
        return SYS_NULL_POINTER;
    }

    if (TPLF_FE_TYPE_ATSC != pInitParam->feType) {
        return SYS_BAD_PARAM;
    }

    pTunerParam = &(pInitParam->atscParam);

    SAINT_CHECK(pTunerParam->DemodType < DemodType_MAX, SYS_BAD_PARAM);

    TRID_INFO("DEMOD = %d, tuner = %d\n", pTunerParam->DemodType, pTunerParam->RFType);

    bFeInitDone = trid_false;
    if (0) {
        ;  // for code format
    }
#if defined(PRO_SXL) || defined(PRO_SX5)
    else if (pTunerParam->DemodType == DemodType_DRX_SXL) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsxl.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_SXL, RF:%d, SHLib:%s\n", pTunerParam->RFType, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->RFType;
        InitParam.I2CAddr = pTunerParam->RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->TSOutputMode;
        handler = LoadDemod(DEMOD_SHLIB_NAME);
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
    else if (pTunerParam->DemodType == DemodType_DRX_UXL) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxuxl.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_UXL, RF:%d, SHLib:%s\n", pTunerParam->RFType, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->RFType;
        InitParam.I2CAddr = pTunerParam->RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.TSMode = pTunerParam->TSOutputMode;
        handler = LoadDemod(DEMOD_SHLIB_NAME);
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
    else if (pTunerParam->DemodType == DemodType_DRX_Fusion) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxfusion.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_Fusion, RF:%d, SHLib:%s\n", pTunerParam->RFType, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->RFType;
        InitParam.I2CAddr = pTunerParam->RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.RFInitParam[0].I2CBusNo = pTunerParam->I2CBusNo;
        InitParam.TSMode = pTunerParam->TSOutputMode;
        handler = LoadDemod(DEMOD_SHLIB_NAME);
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
    else if (pTunerParam->DemodType == DemodType_DRX_SX6) {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsx6.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_SX6, RF:%d, SHLib:%s\n", pTunerParam->RFType, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->RFType;
        InitParam.I2CAddr = pTunerParam->RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.RFInitParam[0].I2CBusNo = pTunerParam->I2CBusNo;
        InitParam.TSMode = pTunerParam->TSOutputMode;
        handler = LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();  //&DRX_SX66;
            ret = pFETuner->Init(&InitParam);
            bFeInitDone = trid_true;
        }
    }
#endif

#if defined(PRO_SX7) || defined(PRO_SX7G) || defined(PRO_SX8) || defined(PRO_UNION1) || defined(PRO_UNION2) || defined(TV303)
    else if (pTunerParam->DemodType == DemodType_DRX_SX6)  // SX7 is same as SX6
    {
#ifdef DEMOD_SHLIB_NAME
#undef DEMOD_SHLIB_NAME
#endif
#define DEMOD_SHLIB_NAME "libdrxsx7.so"

        TridDRXInitParam_t InitParam;
        TRID_INFO("Choose DemodType_DRX_SX7, RF:%d, SHLib:%s\n", pTunerParam->RFType, DEMOD_SHLIB_NAME);
        InitParam.RFType = pTunerParam->RFType;
        InitParam.I2CAddr = pTunerParam->RFI2cAddr;
        InitParam.bSupportInversion = trid_true;
        InitParam.bThreadInit = trid_false;
        InitParam.RFInitParam[0].UseDefault = trid_true;
        InitParam.RFInitParam[0].I2CBusNo = pTunerParam->I2CBusNo;
        InitParam.TSMode = pTunerParam->TSOutputMode;
        handler = LoadDemod(DEMOD_SHLIB_NAME);
        if (!handler) {
            TRID_ERROR("Failed to open %s \n", DEMOD_SHLIB_NAME);
            ASSERT(0);
        } else {
            pFETuner = handler();
            ret = pFETuner->Init(&InitParam);
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

RETURN_TYPE ATSC_TunerInit(void) {
    TplfFeInitParam_t InitParam;
    trid_sint32 ret = SYS_FAILED;

    // Load parameters
    ret = LoadTunerParameter(&InitParam);
    if (ret != SYS_NOERROR) {
        TRID_ERROR("Failed to load tuner parameter.\n");
        return SYS_FAILED;
    }

    // Print tuner config parameters
    PrintTunerParam(&InitParam);

    // Init Parameter
    ret = ATSC_InitParam(&InitParam);

    return ret;
}

RETURN_TYPE ATSC_TuneDTVChan(TunerStdType_e TunerStdType, trid_uint8 ChanNo, ModulationMode_e QamMode) {
    TuneATSCNTSCChanInfo_t TuneChan;
    TuneChan.ChanNo = ChanNo;
    TuneChan.TunerStdType = TunerStdType;
    TuneChan.AdjustFreqKHZ = 0;
    TuneChan.ModulationMode = QamMode;

    return pFETuner->SetAttr(0, _TUNE_ATSC_NTSC_CHAN_, &TuneChan);
}

RETURN_TYPE ATSC_TuneATVChan(TunerStdType_e TunerStdType, trid_uint8 ChanNo, trid_sint32 AdjustFreqKHZ) {
    TuneATSCNTSCChanInfo_t TuneChan;
    TuneChan.ChanNo = ChanNo;
    TuneChan.TunerStdType = TunerStdType;
    TuneChan.AdjustFreqKHZ = AdjustFreqKHZ;
    TuneChan.ModulationMode = _ANALOG_MODE_;

    return pFETuner->SetAttr(0, _TUNE_ATSC_NTSC_CHAN_, &TuneChan);
}

trid_bool ATSC_TunerIsLocked(TunerType_e TunerType) {
    //	RETURN_TYPE ret;
    TunerLockInfo_t TunerLock;

    TunerLock.TunerType = TunerType;
    pFETuner->GetAttr(0, _CHECK_LOCK_, &TunerLock);

    return TunerLock.bLock;
}

void ATSC_TunerSignalCheck(TunerType_e TunerType, void *pQualityInfo, void *pStrengthInfo) {
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
