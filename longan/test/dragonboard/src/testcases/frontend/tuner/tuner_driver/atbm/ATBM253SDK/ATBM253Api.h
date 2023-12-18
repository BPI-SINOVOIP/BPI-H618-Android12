/********************************************************************************************************
*Copyright (C) 2019-2030, Altobeam
*File name: ATBM253Api.h
*Description: Headerfile of ATBM253Api.c,APIs of ATBM253 SDK .
*
*********************************************************************************************************/

#ifndef __ATBM253API_H__    
#define __ATBM253API_H__

#include "ATBM253Definition.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*ATBM253 error code */
#define ATBM253_NO_ERROR                 (0x00) /*no error*/
#define ATBM253_ERROR_BAD_PARAM         (0x01) /*invalid parameter*/
#define ATBM253_ERROR_NOT_INIT         (0x02) /*SDK is not initialized*/
#define ATBM253_ERROR_I2C_FAILED         (0x04) /*I2C R/W failed*/
#define ATBM253_ERROR_CFO_NOT_READY     (0x08) /*CFO Evaluation not ready*/
#define ATBM253_ERROR_NO_DEVICE         (0x10) /*Chip not exist*/
#define ATBM253_ERROR_UNKNOWN          (0x80000000)    /*other error*/
typedef ATBM_U32 ATBM253_ERROR_e; /*Error type*/

/*RF Signal mode*/
typedef enum
{
    ATBM253_SIGNAL_MODE_DTMB = 0, /* DTMB */
    ATBM253_SIGNAL_MODE_DVBT, /*DVB-T/DVB-T2*/
    ATBM253_SIGNAL_MODE_DVBC, /*DVB-C/DVB-C2*/
    ATBM253_SIGNAL_MODE_ISDBT, /* ISDBT */
    ATBM253_SIGNAL_MODE_ATSC, /* ATSC */
    ATBM253_SIGNAL_MODE_ATV, /*Analog TV*/
    
    ATBM253_SIGNAL_MODE_MAX
}ATBM253_SIGNAL_MODE_e;

/*IF output voltage level*/
typedef enum
{
    ATBM253_IF_OUT_LEVEL1 = 0, /* 600 mV */
    ATBM253_IF_OUT_LEVEL2,     /* 800 mV */
    ATBM253_IF_OUT_LEVEL3,     /* 1150 mV */
    ATBM253_IF_OUT_LEVEL4,     /* 1500 mV */

    ATBM253_IF_OUT_LEVEL_MAX
}ATBM253_IF_OUT_LEVEL_e;

/*IF Out Setting */
typedef struct
{
    ATBM_U32 IFOutFreqHz; /*IF out frequency in Hz*/
    ATBM253_IF_OUT_LEVEL_e IFOutLevel; /*IF out level*/
}ATBM253IFOut_t;

/*IF spectrum*/
typedef enum
{
    ATBM253_SPECTRUM_NORMAL = 0, /*normal spectrum output */    
    ATBM253_SPECTRUM_INVERT,     /*inverted spectrum output */
    
    ATBM253_SPECTRUM_MODE_MAX
}ATBM253_SPECTRUM_MODE_e;

/*Clock out control*/
typedef enum
{
    ATBM253_CLK_OUT_DISABLE = 0, /*Clock out disable*/
    ATBM253_CLK_OUT_ENABLE, /*Clock out enable*/
    
    ATBM253_CLK_OUT_MAX
}ATBM253_CLK_OUT_e;
typedef enum
{
    ATBM253_CLK_OUT_AMP_L1 = 0, /*440mV*/
    ATBM253_CLK_OUT_AMP_L2, /*600mV*/
    ATBM253_CLK_OUT_AMP_L3, /*660mV*/
    ATBM253_CLK_OUT_AMP_L4, /*840mV*/
    ATBM253_CLK_OUT_AMP_L5, /*970mV*/
    
    ATBM253_CLK_OUT_AMP_MAX    
}ATBM253_CLK_OUT_AMP_e;
typedef struct
{
    ATBM253_CLK_OUT_e ClkOutEnable; /*clock out enable control*/
    ATBM253_CLK_OUT_AMP_e ClkOutAmp; /*clock out amplitude, valid only if ClkOutEnable is ATBM253_CLK_OUT_ENABLE*/
}ATBM253ClkOutCfg_t;

typedef enum
{/*Name of PIN*/
    ATBM253_PIN_NAME_GPO1 = 0, /*GPO1*/
    ATBM253_PIN_NAME_GPO2, /*GPO2*/
    
    ATBM253_PIN_MAX
}ATBM253_PIN_NAME_e;

typedef enum
{/*State of GPO*/
    ATBM253_GPO_STATE_LOW = 0,
    ATBM253_GPO_STATE_HIGH,
    ATBM253_GPO_STATE_HIZ,
    ATBM253_GPO_STATE_MAX
}ATBM253_GPO_STATE_e;

/*Pin config parameter*/
typedef struct
{
    ATBM253_PIN_NAME_e PinName; /*Pin name*/
    ATBM253_GPO_STATE_e PinState; /*GPO State, Low/High*/
}ATBM253PinCfg_t;

/*OSC calibration*/
typedef struct
{
    ATBM_U8 CalibValue; /*OSC PPM calibration, from 0x00 to 0x0F. */
}ATBM253OSCCalib_t;

/*RF Loopthrough output*/
typedef enum
{
    ATBM253_RF_LTA_OUT_DISABLE = 0, /*Close RF loopthrough output*/
    ATBM253_RF_LTA_OUT_ENABLE, /*Open RF loopthrough output*/

    ATBM253_RF_LTA_OUT_MAX
}ATBM253_RF_LTA_OUT_e;

/*FEF Mode*/
typedef enum
{
    ATBM253_FEF_INTERNAL = 0, /*FEF is detected by ATBM253 itself*/
    ATBM253_FEF_EXTERNAL_GPO1, /*FEF is detected by Demodulator and passed in from GPO1*/
    ATBM253_FEF_EXTERNAL_GPO2, /*FEF is detected by Demodulator and passed in from GPO2*/
    ATBM253_FEF_IGNORE, /*Ignore FEF*/

    ATBM253_FEF_MODE_MAX
}ATBM253_FEF_MODE_e;

/*Clock Mode*/
typedef enum
{
    ATBM253_CLK_MODE_CRYSTAL = 0, /*XTALO/I connected with crystal*/
    ATBM253_CLK_MODE_EXT, /* XTALI connected with external clock */

    ATBM253_CLK_MODE_MAX
}ATBM253_CLK_MODE_e;


/*Fast Tune Mode*/
typedef enum
{
    ATBM253_FAST_TUNE_MODE_NORMAL = 0, /*normal tune*/
    ATBM253_FAST_TUNE_MODE_FAST, /*fast tune*/

    ATBM253_FAST_TUNE_MODE_MAX
}ATBM253_FAST_TUNE_MODE_e;


/*ATBM253 config setting command*/
typedef enum
{    
    ATBM253_CFG_CMD_DTV_IF_OUT_SETTING = 0, /*set DTV IF output parameter,refer to ATBM253IFOut_t*/
    ATBM253_CFG_CMD_ATV_IF_OUT_SETTING, /*set ATV IF output parameter,refer to ATBM253IFOut_t*/
    ATBM253_CFG_CMD_CLK_OUT, /*set clock output parameter,refer to ATBM253ClkOutCfg_t*/
    ATBM253_CFG_CMD_PIN_SETTING, /*set PIN parameter,refer to ATBM253PinCfg_t*/
    ATBM253_CFG_CMD_OSC_CAP_SET, /*set OSC PPM calibration value,refer to ATBM253OSCCalib_t*/
    ATBM253_CFG_CMD_RF_LTA, /*set RF loop through output status, refer to ATBM253_RF_LTA_OUT_e*/
    ATBM253_CFG_CMD_FAST_TUNE, /*set fast tune mode, refer to ATBM253_FAST_TUNE_MODE_e*/
    ATBM253_CFG_CMD_MAX
}ATBM253_CFG_CMD_e;

/*ATBM253 config setting */
typedef struct
{
    ATBM253_CFG_CMD_e CfgCmd; /*config setting command code*/
    union{
        ATBM253IFOut_t IFOut; /*'CfgCmd' is 'ATBM253_CFG_CMD_DTV_IF_OUT_SETTING' or 'ATBM253_CFG_CMD_ATV_IF_OUT_SETTING'*/
        ATBM253ClkOutCfg_t ClkOut; /*'CfgCmd' is 'ATBM253_CFG_CMD_CLK_OUT'*/
        ATBM253PinCfg_t PinCfg; /*'CfgCmd' is 'ATBM253_CFG_CMD_PIN_SETTING'*/
        ATBM253OSCCalib_t OSCCap; /*'CfgCmd' is 'ATBM253_CFG_CMD_OSC_CAP_SET'*/
        ATBM253_RF_LTA_OUT_e LTAOut; /*'CfgCmd' is 'ATBM253_CFG_CMD_RF_LTA'*/
        ATBM253_FAST_TUNE_MODE_e FastTuneMD; /*'CfgCmd' is 'ATBM253_CFG_CMD_FAST_TUNE'*/
    }Cfg;
}ATBM253CfgCMD_t;

typedef struct
{
    ATBM_U8 I2CSlaveAddr; /*I2C Slave Address, e.g. 0xC0*/
    ATBM_U32 Param; /*Other parameter, e.g. I2C bus number*/
}ATBM253I2CAddr_t;
/*ATBM253 configuration parameters when SDK init.*/
typedef struct
{
    ATBM_U32 MagicCode; /*Magic code. Fixed by 'ATBM253DefaultCfgGet', don't modify it!*/
    ATBM253I2CAddr_t I2CParam; /*I2C parameter*/
    ATBM253_CLK_MODE_e ClkMode; /*tuner clock mode*/
    ATBM_U32 OSCFreqKHz; /*OSC Frequency in KHz, e.g. 24000KHz*/
    ATBM253OSCCalib_t OSCCap; /*OSC PPM calibration*/
    ATBM253_SIGNAL_MODE_e Mode; /*RF signal modulation*/
    ATBM253IFOut_t DtvIFOut; /*parameters of DTV IF out*/
    ATBM253IFOut_t AtvIFOut; /*parameters of ATV IF out*/
    ATBM_U8 DacOutDriveUp; /*dac output drive up*/
    ATBM253ClkOutCfg_t ClkOut; /*control of Clock out*/
    ATBM253_FEF_MODE_e FEFMode; /*FEF Mode(AGC Freeze in DVBT2 mode)*/
    ATBM253PinCfg_t PinsCfg[ATBM253_PIN_MAX]; /*Config parameters of PINs*/
    ATBM253_RF_LTA_OUT_e LTAOut; /*RF Loopthrough output*/
    ATBM_BOOL DtvBackgroundProcess; /*The background process option for DTV mode, ATBM_TRUE means 'ATBM253AGCLockingFreeze' will be called by SOC, otherwise not call it. */
    ATBM_U32 DemodType; /*special edition parameter,typically not used; 0:Demod not specified; 1:Hisi Demod;*/
}ATBM253InitConfig_t;

/********************************************************************
* Function: ATBM253Init
* Description: Do initialization for ATBM253 hardware .
*
* Input: DevId -- ID of ATBM253;
*        pConfig -- Pointer to config data
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Init(ATBM_U32 DevId,ATBM253InitConfig_t *pConfig);

/********************************************************************
* Function: ATBM253DefaultCfgGet
* Description: Get default config of ATBM253 .
*
* Input:     N/A
* Output: pConfig -- Pointer to config buffer
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253DefaultCfgGet(ATBM253InitConfig_t *pConfig);

/********************************************************************
* Function: ATBM253CfgSet
* Description: Change some config parameter to ATBM253 .
*
* Input: DevId -- ID of ATBM253;
*        pCfgCmd -- Pointer to config command data
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253CfgSet(ATBM_U32 DevId,ATBM253CfgCMD_t *pCfgCmd);


/********************************************************************
* Function: ATBM253ChannelTune
* Description: Tune to a channel with input parameters.
*
* Input: DevId -- ID of ATBM253;
*        Mode -- RF signal mode, refer to ATBM253_SIGNAL_MODE_e
*        FreqKHz -- RF center frequency in KHz
*        BandWidthKHz -- Signal bandwidth in KHz
*        SpectrumMode -- IF out with inversed spectrum or not
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253ChannelTune(ATBM_U32 DevId,ATBM253_SIGNAL_MODE_e Mode,ATBM_U32 FreqKHz,ATBM_U32 BandWidthKHz,ATBM253_SPECTRUM_MODE_e SpectrumMode);

/********************************************************************
* Function: ATBM253ATVFineTune
* Description: ATV fine tune.
*
* Input:     DevId -- ID of ATBM253;
*        DeltaFreqKHz -- delta frequency in KHz
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253ATVFineTune(ATBM_U32 DevId,ATBM_S32 DeltaFreqKHz);

/********************************************************************
* Function: ATBM253GetLockStatus
* Description: Get RF signal locking status .
*
* Input: DevId -- ID of ATBM253;
* Output: pLockStatus -- Pointer to lock status
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetLockStatus(ATBM_U32 DevId,ATBM_BOOL *pLockStatus);

/********************************************************************
* Function: ATBM253GetIFFreq
* Description: Get IF Frequency(in Hz).
*
* Input: DevId -- ID of ATBM253;
* Output: pIFFreqHz -- Pointer to IF frequency(in Hz)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetIFFreq(ATBM_U32 DevId,ATBM_U32 *pIFFreqHz);

/********************************************************************
* Function: ATBM253GetAtvCfo
* Description: Get CFO(in KHz) for analog signal. (CFO = RF_signal - RF_ATBM253)
*
* Input: 	DevId -- ID of ATBM253;
* Output: pCfoHz -- Pointer to CFO(in Hz)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetAtvCfo(ATBM_U32 DevId,ATBM_S32 *pCfoKHz);

/********************************************************************
* Function: ATBM253GetRSSI
* Description: Get current RSSI(in dBm) .
*
* Input: DevId -- ID of ATBM253;
* Output: pRSSIx16 -- Pointer to RSSI
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetRSSI(ATBM_U32 DevId,ATBM_S32 *pRSSI);

/********************************************************************
* Function: ATBM253Standby
* Description: Make ATBM253 enter into standby state .
*
* Input: DevId -- ID of ATBM253;
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Standby(ATBM_U32 DevId);

/********************************************************************
* Function: ATBM253Wakeup
* Description:  Make ATBM253 enter into wakeup state .
*
* Input: DevId -- ID of ATBM253;
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Wakeup(ATBM_U32 DevId);

/********************************************************************
* Function: ATBM253GetSDKVer
* Description:  Get version of ATBM253 SDK .
*
* Input: DevId -- ID of ATBM253;
* Output: pVersion -- Pointer to version number(e.g. 102 means v1.0.2)
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253GetSDKVer(ATBM_U32 DevId,ATBM_U32 *pVersion);

/********************************************************************
* Function: ATBM253Process
* Input: DevId -- ID of ATBM253;
* Description:  This API should be called constantly by user application, for example every 200ms one time. 
********************************************************************/
ATBM253_ERROR_e ATBM253Process(ATBM_U32 DevId);

/********************************************************************
* Function: ATBM253Detect
* Description: Check if ATBM253 exist or not.
*
* Input: pI2CInfo -- I2C paramters, including slave address and other user paramter;
*        
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- ATBM253 exist; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253Detect(ATBM253I2CAddr_t *pI2CInfo);

/********************************************************************
* Function: ATBM253SecamLLSpeicalDagcRefIFSet
* Description:Optional function for ATV SECAM LL signal DagcRef and IF setting.
* Sample:
ATBM_U32 IsSecamLL = 0;
if(SECAM LL signal)
{
	IsSecamLL = 1;
}else
{
	IsSecamLL = 0;
}
ATBM253SecamLLSpeicalDagcRefIFSet(0, IsSecamLL);
********************************************************************/
ATBM253_ERROR_e ATBM253SecamLLSpecialDagcRefIFSet(ATBM_U32 DevId, ATBM_U32 IsSecamLL);
ATBM253_ERROR_e ATBM253AGCLockingFreeze(ATBM_U32 DevId, ATBM_U32 freeze_flag);

ATBM253_ERROR_e ATBM253ChipDebugStatusGet(ATBM_U32 DevId,ATBM_U8 *pStatus);

#ifdef __cplusplus
}
#endif

#endif /*__ATBM253API_H__*/
                                                                         
