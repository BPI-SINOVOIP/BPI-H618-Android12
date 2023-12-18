
#ifndef __ATBM253DRIVER_H__    
#define __ATBM253DRIVER_H__


#ifdef __cplusplus
extern "C"
{
#endif

/**********************************************
the following specified defines should not be changed. 
************************************************/


/*#define ATBM253_SPECIAL_SETTING_PLT12*/
	
/*#define ATBM253_SPECIAL_SETTING_CVT*/

/*#define ATBM253_SPECIAL_SETTING_KONKA*/

/*Special setting for I2C sync, calibrated for I2C 100K speed.  should not be enabled on 400k*/
/*#define ATBM253_I2C_100K_SYNC_CNT_LMT	*/ 


/*#define ATBM253_SPECIAL_SETTING_SKYDIG*/


/*Tuner's clock output as SOC clock source.*/
/*#define ATBM253_CLK_OUTPUT_CONST */ 

/*#define ATBM253_CLK_OUTPUT_AVAILINK */ /*special setting for AVLINK SOC tuner share clock*/

/*#define ATBM253_SPECIAL_SETTING_MTK */

/*#define ATBM253_SPECIAL_SETTING_CVT_RDA_PQ */

/********************************************************
End of specified defines
**********************************************************/

#define ATBM_LEO_LITE_A_CHIP_ID (0xAA)
#define ATBM_LEO_LITE_B_CHIP_ID (0x55)
#define ATBM_LEO_LITE_D_CHIP_ID (0x56)
#define ATBM_LEO_LITE_E_CHIP_ID (0x57)
#define ATBM_LEO_LITE_F_CHIP_ID (0x58)
#define ATBM_LEO_LITE_G_CHIP_ID (0x59)


#define ATBM253_FIRMWARE_LOAD (1)
#define ATBM253_DUPLICATE_TUNE_CHK (0) /* 1: Check duplicate tune and ignore the repeating action; 0: Do not check duplicate tune*/
#define ATBM253_ATV_AUTO_FINE_TUNE (1) /* 1: Do fine tune instead of normal tune automatically when calling 'ATBM253ChannelTune' ; 0: Don't call fine tune automatically */
#define ATBM253_ATV_FINE_TUNE_RANGE_MAX (1050) /*in KHz*/

#ifdef ATBM253_SPECIAL_SETTING_CVT_RDA_PQ
#define ATBM253_DISABLE_OUTPUT_IN_TUNING  (0)   /*1: disable tuner output in tuning state;  0: tuner aways output signal even in unstable  tuning state*/
#else
#define ATBM253_DISABLE_OUTPUT_IN_TUNING  (1)   /*1: disable tuner output in tuning state;  0: tuner aways output signal even in unstable  tuning state*/
#endif

#ifdef ATBM253_SPECIAL_SETTING_MTK
#define ATBM253_FIR0_BYPASS (0) /*1: bypass fir0; 0: not bypass fir0, to compensate IF evenness in band*/
#else
#define ATBM253_FIR0_BYPASS (1) /*1: bypass fir0; 0: not bypass fir0, to compensate IF evenness in band*/
#endif

#ifdef  ATBM253_CHIP_DEBUG_OPEN
typedef struct
{
    ATBM_BOOL BypassAgc;
    ATBM_BOOL BypassMD;
    ATBM_BOOL EnableMcu;
    ATBM_BOOL ConfigDone;
    ATBM_BOOL WaitAgcStable;
    ATBM_U32 Delay;
}ATBM253DebugOpt_t;
#endif
typedef struct
{
    ATBM_U32 Low;
    ATBM_U32 High;
}ATBM253Data64_t;

#define ATBM253_ARRAY_NUM(array) (sizeof(array)/sizeof(array[0]))

/*list registers struct for a special module,for example PLL, and so on*/
typedef struct
{
    ATBM_U8 BaseReg;
    ATBM_U8 OffReg;
    ATBM_U8 Val;
}ATBM253Reg_t;   

typedef struct
{
    ATBM_U8 BaseReg;
    ATBM_U8 OffReg;
    ATBM_U8 Data[16];
    ATBM_U8 Len;
}ATBM253RegGroup_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[2];
	ATBM_U8 Len;
}ATBM253RegGroupElement2_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[5];
	ATBM_U8 Len;
}ATBM253RegGroupElement5_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[1];
	ATBM_U8 Len;
}ATBM253RegGroupElement1_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[3];
	ATBM_U8 Len;
}ATBM253RegGroupElement3_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[16];
	ATBM_U8 Len;
}ATBM253RegGroupElement16_t;

typedef struct
{
	ATBM_U8 BaseReg;
	ATBM_U8 OffReg;
	ATBM_U8 Data[10];
	ATBM_U8 Len;
}ATBM253RegGroupElement10_t;


typedef struct
{
   ATBM253RegGroupElement2_t GroupElement2;
   ATBM253RegGroupElement5_t GroupElement5;
   ATBM253RegGroupElement1_t GroupElement1;
   ATBM253RegGroupElement16_t GroupElement16;
   ATBM253RegGroupElement10_t GroupElement10;
   ATBM253RegGroupElement1_t GroupElement0;
}ATBM253RegGroupCaliATV_t;


typedef struct
{
	ATBM253RegGroupElement2_t GroupElement2;
	ATBM253RegGroupElement5_t GroupElement5;
	ATBM253RegGroupElement1_t GroupElement1;
	ATBM253RegGroupElement3_t GroupElement3_1;
	ATBM253RegGroupElement3_t GroupElement3_2;
	ATBM253RegGroupElement16_t GroupElement16;
	ATBM253RegGroupElement10_t GroupElement10;
	ATBM253RegGroupElement1_t GroupElement0;
}ATBM253RegGroupCaliOther_t;

typedef struct
{
	ATBM253RegGroupElement2_t GroupElement2;
	ATBM253RegGroupElement1_t GroupElement1_1;
	ATBM253RegGroupElement1_t GroupElement1_2;
	ATBM253RegGroupElement3_t GroupElement3_1;
	ATBM253RegGroupElement3_t GroupElement3_2;
	ATBM253RegGroupElement16_t GroupElement16;
	ATBM253RegGroupElement10_t GroupElement10;
	ATBM253RegGroupElement1_t GroupElement0;
}ATBM253RegGroupCaliATSC_t;

typedef struct
{
    ATBM_U8 BaseAddr;
    ATBM_U8 Len;
    ATBM_U8 Data[64];
}ATBM253HalfAddrRegGroup_t;   


typedef struct
{
    ATBM_U8 BaseReg;
    ATBM_U8 OffReg;
    ATBM_U8 Data[256];
    ATBM_U32 Len;
}ATBM253Firmware_t;

typedef struct
{  
    ATBM_U32 FreqStart; /*in KHz*/  
    ATBM_U32 FreqEnd; /*in KHz*/  
    ATBM_U8 NMixer;
    ATBM_U8 NLO;
    ATBM_U8 N_ADC;
}ATBM253Divider_t; 

typedef struct
{
    ATBM_U32 FreqKHz;/*center frequency in KHz*/
    ATBM_U8 vhf_fltr_res1_sel;
    ATBM_U8 vhf_fltr_data;
    ATBM_U8 vhf_fltr_ref_code;    
}ATBM253VHFChIndex_t;

typedef struct
{
    ATBM_U16 FreqMHz;/*center frequency in MHz*/
    ATBM_U16 uhf_filt_freq;
    ATBM_U8 uhf_fltr_gm_switch;
    ATBM_U8 uhf_fltr_manual;
}ATBM253UHFChIndex_t;

typedef struct
{
    ATBM_U16 FreqMHz;/*center frequency in MHz*/
    ATBM_U8 vhf_fltr_ref_code;    
}ATBM253VHF1ChIndex_t;


typedef enum
{
    ATBM253_SAR_ADC_CLK_DSP = 0, /*CLK24M_DSP*/
    ATBM253_SAR_ADC_CLK_OSCI, /*CLK24M_OSCI*/

    ATBM253_SAR_ADC_CLK_MAX
}ATBM253SarAdcClk_e;

typedef enum
{
    ATBM253_RF_BAND_VHF1 = 0,
    ATBM253_RF_BAND_VHF2,
    ATBM253_RF_BAND_UHF,

    ATBM253_RF_BAND_MAX
}ATBM253_RF_BAND_e;

typedef struct
{
    ATBM_U32 StartFreqMHz;    
    ATBM_U32 EndFreqMHz;
    ATBM_S32 RegVal[25];
}ATBM253AgcPowerTarget_t;

typedef struct
{
    ATBM_U16 FreqMHz;
    ATBM_U16 Intercept;
}ATBM253RSSIIntercept_t;

typedef struct
{
    ATBM_U8 LnaGain0405;
    ATBM_U8 LnaGain0406;
}ATBM253LnaGainCode_t;

#define ATBM253_ACI_CALIB_REG_GROUP_NUM_MAX (9)
/* Calibrate_Reg
bit[31:24] register set flag
bit[23:16] register value
bit[15: 0] register address
*/
typedef struct
{
    ATBM_U32 FreqKHzStart;
    ATBM_U32 FreqKHzEnd;
    ATBM253RegGroupCaliOther_t CalResultOther; 
}ATBM253AciCalibrationOther_t;

typedef struct
{
	ATBM_U32 FreqKHzStart;
	ATBM_U32 FreqKHzEnd;
	ATBM253RegGroupCaliATV_t CalResultATV; 
}ATBM253AciCalibrationATV_t;


typedef struct
{
	ATBM_U32 FreqKHzStart;
	ATBM_U32 FreqKHzEnd;
	ATBM253RegGroupCaliATSC_t CalResultATSC; 
}ATBM253AciCalibrationATSC_t;


#define ATBM253_IIR_COEF_SA_REG_NUM (24)
typedef struct
{/* ADC clock and IIR Coef mapping */
    ATBM_U8 AdcClkDiv8;
    ATBM_U8 IIRCoef[ATBM253_IIR_COEF_SA_REG_NUM];
}ATBM253IIRFltCoef_t;

#define ATBM253_IIR_COEF_ADCCLK_NUM (11)
typedef struct
{
    ATBM_U8 IIRFltBw; /*6/7/8/9/10/11/12*/
    ATBM253IIRFltCoef_t IIRCoefSAParam[ATBM253_IIR_COEF_ADCCLK_NUM];
}ATBM253IIRFltCoefParam_t;

#define ATBM253_REG_BITS_MSK(_MSB,_LSB) (((0x01<<((_MSB)+1))-1)&(~((0x01<<(_LSB))-1)))
typedef struct
{
    ATBM_U8 Reg005C,Reg00A4,Reg00FC;
    ATBM_U8 Reg0500,Reg0501,Reg0503,Reg0507;
    ATBM_U8 Reg0A00,Reg0A01;
    ATBM_U8 Reg0F00,Reg0F01,Reg0F02,Reg0F03,Reg0F09,Reg0F0A,Reg0F0C,Reg0F16,Reg0F18,Reg0F19,Reg0F1A,Reg0F22,Reg0F23,Reg0F24;
    ATBM_U8 Reg1AF9;
}ATBM253MultiplexReg_t;

typedef struct
{
    ATBM_U32 StartFreqKHz;
    ATBM_U32 EndFreqKHz;
    ATBM_S8 LnaGap[6];
}ATBM253LnaGap_t;
#define ATBM253_LNA_GAP_ARRAY_END {0,0,{0,0,0,0,0,0}}

typedef struct
{
    ATBM_U32 StartFreqKHz;
    ATBM_U32 EndFreqKHz;
    ATBM_S8 LnaSatuAci[20];
    ATBM_S8 LnaOutMax[20];
    ATBM_S8 LnaOutMs[10];
    ATBM_U16 BBTarget[5];
}ATBM253LnaCapBBTarget_t;
#define ATBM253_LNA_CAP_BB_TARGET_ARRAY_END {0,0,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0}}


typedef struct
{
    ATBM_U32 FreqMHz;
    ATBM_U32 FcalCapRatio;
}ATBM253UhfFltCalib_t;

typedef struct
{
    ATBM253InitConfig_t Config; /*user config parameters*/
    ATBM_U8 ChipID; /*Chip ID*/
    ATBM253_SIGNAL_MODE_e SignalMode; /* signal mode*/
    ATBM253_SIGNAL_MODE_e SignalMdBak; /* signal mode*/
    ATBM_U32 FreqKHz;/*Channel frequency in KHz, user passed in*/
    ATBM_U32 FineTuneFreqKHz;
    ATBM253_RF_BAND_e Band; /*cur RF band*/
    ATBM253_RF_BAND_e BandBak; /* RF band backup*/
    ATBM_U32 BWKHz; /*Bandwidth in KHz*/
    ATBM_U32 BWKHzBak; /*Bandwidth backup*/
    ATBM_BOOL InvertSpectrum; /*ATBM_TRUE:invert spectrum;ATBM_FALSE:normal*/
    ATBM_BOOL InvertSpectrumBak;
    ATBM_U32 LTECurCfg; /*0:initial state; 1: 690M; 2: 786M; 3: others;*/

    ATBM253MultiplexReg_t MultiReg;
    ATBM_U32 HMDAdcClkHz;
    ATBM_U32 LMDAdcClkHz;    

    ATBM253Divider_t *pDivHMD;
    ATBM253Divider_t *pDivLMD;
    ATBM253IIRFltCoef_t *pIIRFlt1Hmd;
    ATBM253IIRFltCoef_t *pIIRFlt2Hmd;
    ATBM253IIRFltCoef_t *pIIRFlt1Lmd;
    ATBM253IIRFltCoef_t *pIIRFlt2Lmd;

    ATBM_U16 RSSIInterceptBak;
    ATBM_S8 StartLnaGain; /*from -36 to 30, step is 2*/
    ATBM253LnaGainCode_t *pStartLnaGainCode;
    ATBM_S32 MixerIF; /*Mixer IF in Hz*/
    ATBM_S32 MixerIFBak; /*Mixer IF backup*/
    ATBM_BOOL BypassMD; 
    ATBM_BOOL HighMD; /*ATBM_TRUE:high */
    ATBM_U32 LowCousumMd;
    ATBM_BOOL StandbyState;/*ATBM_TRUE:in standby state; ATBM_FALSE:wakeup*/

	ATBM253AciCalibrationATV_t *PAciCaliBakATV;
	ATBM253AciCalibrationATSC_t *PAciCaliBakATSC;
	ATBM253AciCalibrationOther_t *PAciCaliBakOther;
    ATBM253_FAST_TUNE_MODE_e FastTuneMD; /*Fast Tune mode*/

    ATBM253LnaGap_t *pLnaGapBak;
    ATBM253LnaCapBBTarget_t *pLnaCapBBTargetBak;

    
    ATBM_U16 DAgc2AmpRef0;

    ATBM_U32 ExtSetting;
    ATBM_U8 AgcState;
    ATBM_U8 PllPhNoiseSetting; /*0: Normal setting;  1:other;*/
    ATBM_U8 PllPhNoiseSettingBak;
    ATBM_U8 MixrMode; /*2: 8x;  3:4x;*/
    ATBM_U8 MixrModeBak;
    ATBM_U8 *pMixrBiasConfBak;
    ATBM_U8 RssiLnaGainBak;
    ATBM_BOOL NoIFTimeMinimize;/* Minimize the period when no IF output while tunning */
#ifdef ATBM253_CHIP_DEBUG_OPEN
    ATBM253DebugOpt_t DebugOpt;
#endif
    
    ATBM_BOOL Inited;/*SDK init flag*/
}ATBM253_t;


#define ATBM253ABS(x) (((x)>0)?((x)):(-(x)))
#define ATBM253_UHF_LOW_KHZ (400000)
#define ATBM253_VHF_CHECK(FreqKHz) (((ATBM_U32)(FreqKHz) <= ATBM253_UHF_LOW_KHZ)?(ATBM_TRUE):(ATBM_FALSE))
#define ATBM253_MIXER_MD_DIVISION_KHZ (400000)
#define ATBM253_VHF2_LOW_KHZ (160000)

#define ATBM253_I2C_MAX_CHK_CNT (1)
#define ATBM253RegListWrite(Id,RegList,Num) for(;;){\
    ATBM_U32 n = 0;\
    ATBM253Reg_t *pReg = RegList;\
    for(n=0;n<Num;n++){\
        ret |= ATBM253RegWriteNbytes(Id,pReg->BaseReg,pReg->OffReg,&pReg->Val,1);\
        pReg++;}\
    break;\
}

#define ATBM253RegListWriteExt(Id,RegList,Num, RegNewValue) for(;;){\
    ATBM_U32 n = 0;\
    ATBM253Reg_t *pReg = RegList;\
    for(n=0;n<Num;n++){\
        if((pReg->OffReg != RegNewValue.OffReg)||(pReg->BaseReg != RegNewValue.BaseReg)){\
        ret |= ATBM253RegWriteNbytes(Id,pReg->BaseReg,pReg->OffReg,&pReg->Val,1);}\
        else{ret |= ATBM253RegWriteNbytes(Id,RegNewValue.BaseReg,RegNewValue.OffReg,&RegNewValue.Val,1);}\
        pReg++;}\
    break;\
}

#define ATBM253RegRevise(RegList,Num,RegBase,RegOff,NewVal) for(;;){\
    ATBM_U32 n = 0;\
    ATBM253Reg_t *pReg = RegList;\
    for(n=0;n<Num;n++){ if((pReg->BaseReg == RegBase)&&(pReg->OffReg == RegOff)){pReg->Val=NewVal;}\
        pReg++;}\
    break;\
}
#define ATBM253_BURST_WRITE_START {
#define ATBM253_BURST_WRITE_END }
#define ATBM253_REG_ADDR_SET(_REG,_BaseAddr,_OffAddr) _REG.BaseReg = _BaseAddr;_REG.OffReg = _OffAddr;

#ifdef  ATBM253_CHIP_DEBUG_OPEN
extern ATBM253_ERROR_e ATBM253DrvChipDebugOption(ATBM253_t *pTuner);
extern ATBM253_ERROR_e ATBM253DrvChipDebugInitStart(ATBM253_t *pTuner);
extern ATBM253_ERROR_e ATBM253DrvChipDebugInitEnd(ATBM253_t *pTuner);
extern void ATBM253DrvChipDebugOptGet(ATBM253_t *pTuner);
extern void ATBM253DrvChipDebugOnDspStart(ATBM253_t *pTuner);
extern void ATBM253DrvChipDebugOnAgcStable(ATBM253_t *pTuner);
extern void ATBM253DrvChipDebugBeforeDspStart(ATBM253_t *pTuner);
extern ATBM_S32 ATBM253DrvChipDebugPowerTargetSet(ATBM253_t *pTuner,ATBM253Reg_t *pReg,ATBM_U32 RegCnt);
extern ATBM253_ERROR_e ATBM253DrvChipDebugUHFFilterGMManaulRatioGet(ATBM253_t *pTuner,double *pGmRatio,double *pManaulRatio,
                                                                                      double *pRFRatio,int *pRFOffset,ATBM_BOOL *pDebugOpen);
extern ATBM253_ERROR_e ATBM253DrvFirmwareDebug(ATBM253_t *pTuner,ATBM253Firmware_t **pFwm);
extern ATBM253_ERROR_e ATBM253DrvFirmwareEnable(ATBM253_t *pTuner);
extern ATBM253_ERROR_e ATBM253DrvFirmwareDisable(ATBM253_t *pTuner);
extern void ATBM253DrvIIRSetting(ATBM253_t *pTuner);
extern void ATBM253ATVFineTuneEnd(ATBM253_t *pTuner);
extern ATBM_BOOL ATBM253DrvChipDebugLnaGapGet(ATBM253_t *pTuner,ATBM253LnaGap_t **ppLnaGap);
extern ATBM_BOOL ATBM253DrvChipDebugLnaCapBBTargetGet(ATBM253_t *pTuner,ATBM253LnaCapBBTarget_t **ppTarget);
#endif

ATBM253_ERROR_e ATBM253RegWrite(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 Value);
ATBM253_ERROR_e ATBM253RegRead(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 *pValue);
ATBM253_ERROR_e ATBM253DrvRegLatch(ATBM253I2CAddr_t *pI2CAddr,ATBM_BOOL OnOff);
void ATBM253DrvPLLDivCal(ATBM253_t *pTuner,ATBM_U8 *pNInt,ATBM_U32 *pNFrac,ATBM_BOOL HighMD);
ATBM_U8 ATBM253MultiplexRegValue(ATBM253_t *pTuner,ATBM_U8 BaseAddr,ATBM_U8 OffAddr,ATBM_U8 Value,ATBM_U8 Msb,ATBM_U8 Lsb);
ATBM253_ERROR_e ATBM253DrvMultRegRead(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvSwReset(ATBM253_t *pTuner);
void ATBM253DrvSetDefaultParam(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvInit(ATBM253_t *pTuner);
#if ATBM253_DUPLICATE_TUNE_CHK
ATBM_BOOL ATBM253DrvIsDuplicateTune(ATBM253_t *pTuner,ATBM253_SIGNAL_MODE_e Mode,ATBM_U32 FreqKHz,ATBM_U32 BandWidthKHz,ATBM253_SPECTRUM_MODE_e SpectrumMode);
#endif
ATBM253_ERROR_e ATBM253DrvFreqTune(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvCfgSet(ATBM253_t *pTuner,ATBM253CfgCMD_t *pCfg);
ATBM253_ERROR_e ATBM253DrvFastTuneModeSet(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvAgcState0Setting(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvAgcState2Setting(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvAgcStateChk(ATBM253_t *pTuner);
void ATBM253DrvADCClkCal(ATBM253_t *pTuner, ATBM_U32 *pHLOAdcClkHz, ATBM_U32  *pLLOAdcClkHz);
ATBM253_ERROR_e ATBM253DrvGetRSSI(ATBM253I2CAddr_t *pI2CAddr,ATBM_S16 *pRSSIx16);
ATBM253_ERROR_e ATBM253DrvGetATVCfo(ATBM253I2CAddr_t *pI2CAddr,ATBM_S32 *pCfoKHz);
ATBM253_ERROR_e ATBM253DrvStandby(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvWakeup(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253TunerDrvMixIFAdjust(ATBM253_t *pTuner,ATBM_S32 CfoHz);
ATBM253_ERROR_e ATBM253DrvATVFineTune(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvGetSDKVer(ATBM253_t *pTuner,ATBM_U32 *pVer);
void ATBM253DrvIIRCoefJudgeStateSet(ATBM_BOOL Open);
ATBM253_ERROR_e ATBM253DrvIIRCoefJudge(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvLiteGLteTargetSet(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvLteISDBTTargetSet(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvLiteBLteTargetSet(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvLteTargetSet(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvProcess(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvRegDataFlushWrite(ATBM253I2CAddr_t *pSlvAddr,ATBM_U8 BaseAddr,ATBM_U8 OffAddr,ATBM_U8 *pData,ATBM_U32 Size);
ATBM253_ERROR_e ATBM253DrvRegDataBurstWrite(ATBM253_t *pTuner,ATBM_U8 *pData,ATBM_U32 Size);
ATBM253_ERROR_e ATBM253DrvRegHalfAddrBurstWrite(ATBM253_t *pTuner,ATBM_U8 BaseAddr,ATBM_U8 *pData,ATBM_U8 Length);
ATBM253_ERROR_e ATBM253DrvRxFltAndSpectrumParse(ATBM253_t *pTuner);
ATBM253_ERROR_e ATBM253DrvDetect(ATBM253I2CAddr_t *pI2CAddr);
ATBM253_ERROR_e ATBM253DrvLoLargeThrSetting(ATBM253_t *pTuner,ATBM_U8 AgcState);

void ATBM253CfgPrint(ATBM253_t *pTuner);
void ATBM253DrvChipStatusPrint(ATBM253_t *pTuner);

#ifdef __cplusplus
}
#endif

#endif /*__ATBM253DRIVER_H__*/
                                                                         
