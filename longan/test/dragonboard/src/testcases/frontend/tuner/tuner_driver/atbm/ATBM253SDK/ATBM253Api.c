
/********************************************************************************************************
*Copyright (C) 2019-2030, Altobeam
*File name: ATBM253Api.c
*Description: Implement of ATBM253 SDK APIs.
*
*Modification History: 
*    2019-03-20:
*       1. Release v1.0.0
*    2019-06-11:
*       1. Release v1.0.1
*    2019-06-21:
*       1. Release v1.0.2
*    2019-07-19:
*       1. Release v1.0.3
*    2019-08-09:
*       1. Release v1.0.4
*    2019-09-06:
*       1. Release v1.0.5
*    2019-10-10:
*       1. Release v1.0.6    
*	2019-10-29:
*       1. Release v1.0.7
*	2019-12-03:
*       1. Release v1.0.8
*	2019-12-26:
*       1. Release v1.0.9
*	2020-04-20:
*       1. Release v1.1.0
*	2020-05-09:
*       1. Release v1.1.1
*	2020-06-12:
*       1. Release v1.1.2
*	2020-12-17:
*       1. Release v1.1.3
*********************************************************************************************************/
#include "ATBM253Porting.h"
#include "ATBM253Driver.h"
#include "ATBM253Api.h"
#define ATBM253_DEFAULT_I2C_ADDR (0xC0)
#define ATBM253_DEFAULT_IF (5000000)
#define ATBM253_DEFAULT_OSC_FREQKHZ (24000)
#define ATBM253_DEFAULT_OSC_CAP (0x08)
#define ATBM253_MAGIC_CODE (0x4D425441) /*('ATBM')*/
static ATBM253_t ATBM253Tuner[ATBM253_TUNER_COUNT];
static ATBM_BOOL ATBM253MutexInitFlag = ATBM_FALSE;
/********************************************************************
* Function: ATBM253Init
* Description: Do initialization for ATBM253 hardware .
*
* Input: DevId -- ID of ATBM253;
*        pConfig -- Pointer to config data
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Init(ATBM_U32 DevId,ATBM253InitConfig_t *pConfig)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;

    if((ATBM_NULL == pConfig)||(ATBM253_MAGIC_CODE != pConfig->MagicCode)||(DevId >= ATBM253_TUNER_COUNT))
    {
        ATBM253Print(("[Error]ATBM253Init(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    if(!ATBM253MutexInitFlag)
    {
        ret = ATBM253MutexInit();
        if(ATBM253_NO_ERROR != ret)
        {
            ATBM253Print(("[Error]ATBM253MutexInit() Failed!\n"));
            return ret;
        }
        ATBM253MutexInitFlag = ATBM_TRUE;
    }
    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];
    pTuner->Config = *pConfig; /*Save user parameters.*/

    ret |= ATBM253DrvInit(pTuner);
    ATBM253CfgPrint(pTuner); /*Debug info print*/
    ATBM253Print(("[Info]ATBM253Init()v112,ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253DefaultCfgGet
* Description: Get default config of ATBM253 .
*
* Input: N/A
* Output: pConfig -- Pointer to config buffer
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253DefaultCfgGet(ATBM253InitConfig_t *pConfig)
{
    if(ATBM_NULL == pConfig)
    {
        ATBM253Print(("[Error]ATBM253DefaultCfgGet(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    /*Default config of ATBM253*/
    pConfig->MagicCode = ATBM253_MAGIC_CODE;
    pConfig->I2CParam.I2CSlaveAddr = ATBM253_DEFAULT_I2C_ADDR;
    pConfig->I2CParam.Param = 0;
    pConfig->ClkMode = ATBM253_CLK_MODE_CRYSTAL;
    pConfig->OSCFreqKHz = ATBM253_DEFAULT_OSC_FREQKHZ;
    pConfig->OSCCap.CalibValue = ATBM253_DEFAULT_OSC_CAP;
    pConfig->Mode = ATBM253_SIGNAL_MODE_DTMB;
    pConfig->DtvIFOut.IFOutFreqHz = ATBM253_DEFAULT_IF;
#ifdef ATBM253_SPECIAL_SETTING_PLT12
    pConfig->DtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL1;    
#else
    pConfig->DtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL2;    
#endif
    pConfig->AtvIFOut.IFOutFreqHz = ATBM253_DEFAULT_IF;
    pConfig->AtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL1;
    pConfig->DacOutDriveUp = 0x00;
    pConfig->ClkOut.ClkOutEnable = ATBM253_CLK_OUT_DISABLE;
    pConfig->ClkOut.ClkOutAmp = ATBM253_CLK_OUT_AMP_L4;
    pConfig->FEFMode = ATBM253_FEF_INTERNAL;
    pConfig->PinsCfg[0].PinName = ATBM253_PIN_NAME_GPO1;
    pConfig->PinsCfg[0].PinState = ATBM253_GPO_STATE_HIZ;
    pConfig->PinsCfg[1].PinName = ATBM253_PIN_NAME_GPO2;
    pConfig->PinsCfg[1].PinState = ATBM253_GPO_STATE_HIZ;
    pConfig->LTAOut = ATBM253_RF_LTA_OUT_DISABLE;
    pConfig->DtvBackgroundProcess = ATBM_FALSE;
    pConfig->DemodType = 0;

    ATBM253Print(("[Info]ATBM253DefaultCfgGet(),ret =%d\n",ATBM253_NO_ERROR));
    return ATBM253_NO_ERROR;
}

/********************************************************************
* Function: ATBM253CfgSet
* Description: Change some config parameter to ATBM253 .
*
* Input:     DevId -- ID of ATBM253;
*        pCfgCmd -- Pointer to config command data
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253CfgSet(ATBM_U32 DevId,ATBM253CfgCMD_t *pCfg)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    
    if((DevId >= ATBM253_TUNER_COUNT)||(ATBM_NULL == pCfg))
    {
        ATBM253Print(("[Error]ATBM253CfgSet(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }

    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253CfgSet(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }

    ret |= ATBM253DrvCfgSet(pTuner,pCfg);
    ATBM253Print(("[Info]ATBM253CfgSet(),ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253ChannelTune
* Description: Tune to a channel with special parameter.
*
* Input:     DevId -- ID of ATBM253;
*        Mode -- RF signal mode, refer to ATBM253_SIGNAL_MODE_e
*        FreqKHz -- RF center frequency in KHz
*        BandWidthKHz -- Signal bandwidth in KHz
*        SpectrumMode -- IF out with inversed spectrum or not
Attention: ATV  PAL-BG mode:
[FreqKHz <  300 MHz ] set BandWidthKHz 7000KHz,  tune frequency to ATV frequency offset euqals 2.25M 
[ FreqKHz > 300 MHz] set  BandWidthKHz 8000KHz.  tune frequency to ATV frequency offset euqals 2.75M 
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253ChannelTune(ATBM_U32 DevId,ATBM253_SIGNAL_MODE_e Mode,ATBM_U32 FreqKHz,ATBM_U32 BandWidthKHz,ATBM253_SPECTRUM_MODE_e SpectrumMode)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;

    if((DevId >= ATBM253_TUNER_COUNT)
#ifndef ATBM253_CHIP_DEBUG_OPEN /*No limit for GUI debug*/
        ||(FreqKHz > 1000000)||(FreqKHz < 40000)
#endif
        ||(BandWidthKHz > 10000))
    {
        ATBM253Print(("[Error]ATBM253ChannelTune(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }

    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253ChannelTune(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
#if ATBM253_DUPLICATE_TUNE_CHK
    if(!ATBM253DrvIsDuplicateTune(pTuner,Mode,FreqKHz,BandWidthKHz,SpectrumMode))
#endif
    {
#if ATBM253_ATV_AUTO_FINE_TUNE
        if((ATBM253_SIGNAL_MODE_ATV == Mode)&&(ATBM253_FAST_TUNE_MODE_FAST == pTuner->FastTuneMD)
            &&(BandWidthKHz == pTuner->BWKHz)&&(Mode == pTuner->SignalMode)
            &&(((ATBM253_SPECTRUM_NORMAL==SpectrumMode)?(ATBM_FALSE):(ATBM_TRUE)) == pTuner->InvertSpectrum))
        {
            pTuner->FineTuneFreqKHz = FreqKHz;
            ret |= ATBM253DrvATVFineTune(pTuner);
        }
        else
#endif            
        {
            pTuner->FreqKHz = FreqKHz;
            pTuner->BWKHz = BandWidthKHz;
            pTuner->InvertSpectrum = (ATBM253_SPECTRUM_NORMAL==SpectrumMode)?(ATBM_FALSE):(ATBM_TRUE);
            pTuner->SignalMode = Mode;
            ret |= ATBM253DrvFreqTune(pTuner);
        }
        ATBM253Print(("[Info]ATBM253ChannelTune(%d,%d,%d,%d),ret =%d\n",(ATBM_U32)Mode,FreqKHz,BandWidthKHz,(ATBM_U32)SpectrumMode,ret));
    }
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253ATVFineTune
* Description: ATV fine tune.
*
* Input:     DevId -- ID of ATBM253;
*        DeltaFreqKHz -- delta frequency in KHz
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253ATVFineTune(ATBM_U32 DevId,ATBM_S32 DeltaFreqKHz)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;

    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253ATVFineTune(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }

    pTuner = &ATBM253Tuner[DevId];
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253ATVFineTune(),ATBM253 is not initialized!\n"));
        return ATBM253_ERROR_NOT_INIT;
    }
    ret = ATBM253MutexLock();
    pTuner->FineTuneFreqKHz += DeltaFreqKHz;
    ret |= ATBM253DrvATVFineTune(pTuner);
    ATBM253Print(("[Info]ATBM253ATVFineTune(%d),ret =%d\n",DeltaFreqKHz,ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253GetLockStatus
* Description: Get RF signal locking status .
*
* Input:     DevId -- ID of ATBM253;
* Output: pLockStatus -- Pointer to lock status
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetLockStatus(ATBM_U32 DevId,ATBM_BOOL *pLockStatus)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM_U8 Val = 0;
    ATBM253_t *pTuner = ATBM_NULL;

    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253GetLockStatus(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret |= ATBM253MutexUnLock();
    pTuner = &ATBM253Tuner[DevId];
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253GetLockStatus(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    *pLockStatus = ATBM_FALSE;
    ret |= ATBM253RegRead(&pTuner->Config.I2CParam,0x14,0xAA, &Val); /*ro_reg_agc_setting_state[1:0]*/
    if((ATBM253_NO_ERROR == ret)&&((0x02 == Val)||(0x03 == Val)))
    {
        *pLockStatus = ATBM_TRUE;
    }
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253GetIFFreq
* Description: Get IF Frequency(in Hz).
*
* Input:     DevId -- ID of ATBM253;
* Output: pIFFreqHz -- Pointer to IF frequency(in Hz)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetIFFreq(ATBM_U32 DevId,ATBM_U32 *pIFFreqHz)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    ATBM253InitConfig_t Config;

    if((DevId >= ATBM253_TUNER_COUNT)||(ATBM_NULL == pIFFreqHz))
    {
        ATBM253Print(("[Error]ATBM253GetIFFreq(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    pTuner = &ATBM253Tuner[DevId];
    ret = ATBM253MutexLock();
    if(pTuner->Inited)
    {
        Config = pTuner->Config;
    }
    else
    {
        ret |= ATBM253DefaultCfgGet(&Config);
        if(ATBM253_NO_ERROR != ret)
        {
            ATBM253Print(("[Error]ATBM253DefaultCfgGet(),failed!ret = %d\n",ret));
			ret |= ATBM253MutexUnLock();
            return ret;
        }
    }
    if(ATBM253_SIGNAL_MODE_ATV == Config.Mode)
    {
        *pIFFreqHz = pTuner->Config.AtvIFOut.IFOutFreqHz;
    }
    else
    {
        *pIFFreqHz = pTuner->Config.DtvIFOut.IFOutFreqHz;
    }

    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253GetAtvCfo
* Description: Get CFO(in KHz) for analog signal. (CFO = RF_signal - RF_ATBM253)
*
* Input: 	DevId -- ID of ATBM253;
* Output: pCfoHz -- Pointer to CFO(in Hz)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetAtvCfo(ATBM_U32 DevId,ATBM_S32 *pCfoKHz)
{
	ATBM253_ERROR_e ret = ATBM253_NO_ERROR;	
	ATBM253_t *pTuner = ATBM_NULL;
	
	if((DevId >= ATBM253_TUNER_COUNT)||(ATBM_NULL == pCfoKHz))
	{
		ATBM253Print(("[Error]ATBM253GetAtvCfo(),invalid parameter!\n"));
		return ATBM253_ERROR_BAD_PARAM;
	}
    *pCfoKHz = 0;
    ret = ATBM253MutexLock();
	pTuner = &ATBM253Tuner[DevId];
	if(!pTuner->Inited)
	{
		ATBM253Print(("[Error]ATBM253GetAtvCfo(DevId=%d),ATBM253 is not initialized!\n",DevId));
        ret |= ATBM253MutexUnLock();
		return ATBM253_ERROR_NOT_INIT;
	}
	ret = ATBM253DrvGetATVCfo(&pTuner->Config.I2CParam,pCfoKHz);    

    ret |= ATBM253MutexUnLock();
	return ret;
}

/********************************************************************
* Function: ATBM253GetRSSI
* Description: Get current RSSI(in dBm) .
*
* Input:     DevId -- ID of ATBM253;
* Output: pRSSI -- Pointer to RSSI
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetRSSI(ATBM_U32 DevId,ATBM_S32 *pRSSI)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;    
    ATBM_S16 SignalPow = 0;    
    ATBM253_t *pTuner = ATBM_NULL;
    
    if((DevId >= ATBM253_TUNER_COUNT)||(ATBM_NULL == pRSSI))
    {
        ATBM253Print(("[Error]ATBM253GetRSSI(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253GetRSSI(),ATBM253 is not initialized!\n"));
		ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    ret |= ATBM253DrvGetRSSI(&pTuner->Config.I2CParam,&SignalPow);

	if(SignalPow < -1168)  /*-73*/
	{
		SignalPow = SignalPow + 16;
	}else if(SignalPow < -1120)  /*-70*/
	{
		SignalPow = SignalPow + 8;
	}else
	{	
	}

    *pRSSI = SignalPow/16;

    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253Standby
* Description: Make ATBM253 enter into standby state .
*
* Input:     DevId -- ID of ATBM253;
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Standby(ATBM_U32 DevId)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    
    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253Standby(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];    
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253Standby(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    ret |= ATBM253DrvStandby(pTuner);    
    ATBM253Print(("[Info]ATBM253Standby(),ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253Wakeup
* Description:  Make ATBM253 enter into wakeup state .
*
* Input:     DevId -- ID of ATBM253;
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Wakeup(ATBM_U32 DevId)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    
    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253Wakeup(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret |= ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];    
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253Wakeup(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    ret |= ATBM253DrvWakeup(pTuner);
    ATBM253Print(("[Info]ATBM253Wakeup(),ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253GetSDKVer
* Description:  Get version of ATBM253 SDK .
*
* Input:     DevId -- ID of ATBM253;
* Output: pVersion -- Pointer to version number(e.g. 102 means v1.0.2)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetSDKVer(ATBM_U32 DevId,ATBM_U32 *pVersion)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    
    if((DevId >= ATBM253_TUNER_COUNT)||(ATBM_NULL == pVersion))
    {
        ATBM253Print(("[Error]ATBM253GetSDKVer(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];    
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253GetSDKVer(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    ret |= ATBM253DrvGetSDKVer(pTuner,pVersion);
    ATBM253Print(("[Info]ATBM253GetSDKVer(),ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253Process
* Input:     DevId -- ID of ATBM253;
* Description:  This API should be called constantly by user application, for example every 200ms one time. 
********************************************************************/
ATBM253_ERROR_e ATBM253Process(ATBM_U32 DevId)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    ATBM253_t *pTuner = ATBM_NULL;
    
    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253Process(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret = ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];    
    if(!pTuner->Inited)
    {
        ATBM253Print(("[Error]ATBM253Process(),ATBM253 is not initialized!\n"));
        ret |= ATBM253MutexUnLock();
        return ATBM253_ERROR_NOT_INIT;
    }
    ret |= ATBM253DrvProcess(pTuner);
    ret |= ATBM253MutexUnLock();
    return ret;
}

/********************************************************************
* Function: ATBM253Detect
* Description: Check if ATBM253 exist or not.
*
* Input:     pI2CInfo -- I2C paramters, including slave address and other user paramter;
*        
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- ATBM253 exist; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Detect(ATBM253I2CAddr_t *pI2CInfo)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;

    if(ATBM_NULL == pI2CInfo)
    {
        ATBM253Print(("[Error]pI2CInfo(),invalid parameter!\n"));
        return ATBM253_ERROR_BAD_PARAM;
    }
    if(!ATBM253MutexInitFlag)
    {
        ret = ATBM253MutexInit();
        if(ATBM253_NO_ERROR != ret)
        {
            ATBM253Print(("[Error]ATBM253MutexInit() Failed!\n"));
            return ret;
        }
        ATBM253MutexInitFlag = ATBM_TRUE;
    }
    ret = ATBM253MutexLock();
    ret |= ATBM253DrvDetect(pI2CInfo);
    ATBM253Print(("[Info]ATBM253Detect(),ret =%d\n",ret));
    ret |= ATBM253MutexUnLock();
    return ret;
}


#ifdef ATBM253_CHIP_DEBUG_OPEN
ATBM253_t *ATBM253ObjGet(ATBM_U32 DevId)
{
    ATBM253_t *pTuner = ATBM_NULL;
    
    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253Wakeup(),invalid parameter!\n"));
        return ATBM_NULL;
    }
    pTuner = &ATBM253Tuner[DevId];    
    
    return pTuner;
}
#endif


/********************************************************************
* Function: ATBM253SecamLLSpecialDagcRefIFSet
* Description: This is optional function for ATV SECAM LL signal DagcRef and IF setting.
*	ATV SECAM LL signal: set ATV IF output level1 and set DagcRef 0x04D0
*	ATV Other signal:	 set ATV IF and DagcRef from ATV user setting.
*
*   Attention: the parameters in this function was set according to SOC platform.
*
* Input:     DevId   ---- ID of ATBM253;
*            IsSecamLL -- 1: ATV SECAM LL signal, 0: other ATV signal;
*        
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- ATBM253 exist; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253SecamLLSpecialDagcRefIFSet(ATBM_U32 DevId, ATBM_U32 IsSecamLL)
{
	ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
	ATBM_U16 dagcRef = 0x04E0;
	ATBM253_t *pTuner = ATBM_NULL;
	unsigned char  Data[2] = {0};
	ATBM253CfgCMD_t Config;
	if(DevId >= ATBM253_TUNER_COUNT)
	{
		ATBM253Print(("[Error]ATBM253SecamLLSpeicalDagcRefIFSet(%d),invalid parameter!\n",DevId));
		return ATBM253_ERROR_BAD_PARAM;
	}
	ret |= ATBM253MutexLock();
	pTuner = &ATBM253Tuner[DevId];    
	if(!pTuner->Inited)
	{
		ret |= ATBM253MutexUnLock();
		ATBM253Print(("[Error]ATBM253SecamLLSpeicalDagcRefIFSet(%d),ATBM253 is not initialized!\n",DevId));
		return ATBM253_ERROR_NOT_INIT;
	}	 
	if(ATBM253_NO_ERROR != ATBM253DrvDetect(&pTuner->Config.I2CParam))
	{
		ret |= ATBM253MutexUnLock();
		return ATBM253_NO_ERROR;
	}

	if(IsSecamLL)
	{
		dagcRef = 0x04D0;
	}else
	{
		dagcRef = pTuner->DAgc2AmpRef0;
	}

	Data[0] = (ATBM_U8)(dagcRef&0xFF);
	Data[1] = (ATBM_U8)((dagcRef>>8)&0xFF);
	ret |= ATBM253DrvRegDataFlushWrite(&pTuner->Config.I2CParam,0x13,0xA6, Data,2);/*0x13,0xA6*/

	/*If you want to change some parameters, call 'ATBM253CfgSet' to do it. */
	Config.CfgCmd = ATBM253_CFG_CMD_ATV_IF_OUT_SETTING;
	Config.Cfg.IFOut.IFOutFreqHz = pTuner->Config.AtvIFOut.IFOutFreqHz ;
	if(IsSecamLL)
	{
		Config.Cfg.IFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL1;
	}else
	{
		Config.Cfg.IFOut.IFOutLevel = pTuner->Config.AtvIFOut.IFOutLevel;
	}
	ret |= ATBM253DrvCfgSet(pTuner,&Config);

	ret |= ATBM253MutexUnLock();
	return ret;
}


#if 1 
ATBM253_ERROR_e ATBM253AGCLockingFreeze(ATBM_U32 DevId, ATBM_U32 freeze_flag)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;

    ATBM253_t *pTuner = ATBM_NULL;
    unsigned char  Val = 0;

    if(DevId >= ATBM253_TUNER_COUNT)
    {
        ATBM253Print(("[Error]ATBM253AGCLockingFreeze(%d),invalid parameter!\n",DevId));
        return ATBM253_ERROR_BAD_PARAM;
    }
    ret |= ATBM253MutexLock();
    pTuner = &ATBM253Tuner[DevId];    
    if(!pTuner->Inited)
    {
        ret |= ATBM253MutexUnLock();
        ATBM253Print(("[Error]ATBM253AGCLockingFreeze(%d),ATBM253 is not initialized!\n",DevId));
        return ATBM253_ERROR_NOT_INIT;
    }	 
	if(ATBM253_NO_ERROR != ATBM253DrvDetect(&pTuner->Config.I2CParam))
	{
		ret |= ATBM253MutexUnLock();
		return ATBM253_NO_ERROR;
	}

    if(ATBM253_SIGNAL_MODE_ATV == pTuner->SignalMode)
    {
        ret |= ATBM253MutexUnLock();
        return ATBM253_NO_ERROR;
    }

    if(!pTuner->Config.DtvBackgroundProcess)
    {
        ATBM253Print(("[Error]ATBM253AGCLockedFreeze(%d),DtvBackgroundProcess option is FALSE!\n",DevId));
        ret |= ATBM253MutexUnLock();
        return ATBM253_NO_ERROR;
    }
    
	ret |= ATBM253RegRead(&pTuner->Config.I2CParam,0x14,0xAA, &Val); /*ro_reg_agc_setting_state[1:0]*/
	if(Val < 0x02)
	{
		ret |= ATBM253MutexUnLock();
		return ATBM253_NO_ERROR;
	}    

	ret |= ATBM253RegRead(&pTuner->Config.I2CParam,0x14,0x27, &Val);

    if(freeze_flag)
    {
        if(Val == 0)
        {
            ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x4C, 0x01);
            ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x47, 0x01);        
            if(ATBM_FALSE == pTuner->BypassMD) 
            {
                ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x27, 0x01);    
            }
        }
    }
    else
    {
        if(Val == 1)
        {
            ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x4C, 0x00);
            ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x47, 0x00);
            if(ATBM_FALSE == pTuner->BypassMD)
            {
                ret |=ATBM253RegWrite(&pTuner->Config.I2CParam,0x14,0x27, 0x00);     
            }
        }
    }

    ATBM253Print(("[Info]ATBM253AGCLockedFreeze(%d),freeze_flag:%d,Val:%d ret =%d\n",DevId, freeze_flag,Val,ret));

    ret |= ATBM253MutexUnLock();
    return ret;
}
#endif




ATBM253_ERROR_e ATBM253ChipDebugStatusGet(ATBM_U32 DevId,ATBM_U8 *pStatus)
{
	ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
	ATBM_U8 Data[16] = {0};
	ATBM253_t *pTuner = ATBM_NULL;
	ATBM253I2CAddr_t *pI2CAddr = ATBM_NULL;

	if(DevId >= ATBM253_TUNER_COUNT)
	{
		ATBM253Print(("[Error]ATBM253ChipDebugStatusGet(%d),invalid parameter!\n",DevId));
		return ATBM253_ERROR_BAD_PARAM;
	}
	ret |= ATBM253MutexLock();
	pTuner = &ATBM253Tuner[DevId];    
	if(!pTuner->Inited)
	{
		ret |= ATBM253MutexUnLock();
		ATBM253Print(("[Error]ATBM253ChipDebugStatusGet(%d),ATBM253 is not initialized!\n",DevId));
		return ATBM253_ERROR_NOT_INIT;
	}

	pI2CAddr = &pTuner->Config.I2CParam;

	(void)ATBM253RegWrite(pI2CAddr,0x01,0x0D, 0x01); /*Latch on*/
	(void)ATBM253I2CRead(pI2CAddr,0x14,0xAA, Data,1);
	pStatus[0] = Data[0];/* agc state */

	(void)ATBM253I2CRead(pI2CAddr,0x14,0xAC, Data,2);
	pStatus[1] = Data[0];/* selection finish */
	pStatus[2] = Data[1];/* mode selection */

	(void)ATBM253I2CRead(pI2CAddr,0x14,0xBB, Data,1);	
	pStatus[3] = Data[0];/* lna_gain_virtual */

	(void)ATBM253I2CRead(pI2CAddr,0x14,0xC2, Data,1);
	pStatus[4] = Data[0];/* ifvga2_gain_step */

	(void)ATBM253I2CRead(pI2CAddr,0x14,0xA6, Data,1);    
	pStatus[5] = Data[0];/* power_rssi_lna_out_dbm */

	(void)ATBM253I2CRead(pI2CAddr,0x14,0xD0, Data,1);
	pStatus[6] = Data[0];/* signal_power_rf_dbm */

	(void)ATBM253I2CRead(pI2CAddr,0x15,0x44, Data,3); /* ro_params_dagc2_scale U(21,10) */
	pStatus[7] = Data[0];/* dagc2_scale */
	pStatus[8] = Data[1];
	pStatus[9] = Data[2];

	(void)ATBM253RegWrite(pI2CAddr,0x01,0x0D, 0x00); /*Latch off*/

	ret |= ATBM253MutexUnLock();
	return ret;

}