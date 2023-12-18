/* ----------------------------------------------------------------------------
 File Name: DTMBIP.h

 Description:

 Version 0.9 : Created                    2014.08.01 Sharp Wang
 Version 1.0 : Modified             2014.09.01 Sharp Wang
---------------------------------------------------------------------------- */
#ifndef __DTMBIP_H__
#define __DTMBIP_H__

#include "DTMBType.h"

typedef struct DTMBIP_Init_Info_t {
    UINT8 TunerType;                /*高频头类型，零中频或其他*/
    double TunerIF;                    /*高频头中频输出频率，单位MHZ*/
    UINT8 TsType;                        /*串行或并行*/
    UINT8 TsClockPolarity;    /*数据采样是在时钟上升沿或下降沿*/
    UINT8 TsClockValid;            /*时钟有效方式，一直有效或随valid信号*/
    UINT8 TsNullPacket;            /*空包使能与否*/
    UINT8 TsSerialOutPut;        /*串行TS时，输出在TS_D7或TS_D0*/
    UINT8 TsSerialOrder;        /*串行TS时，输出是MSB到LSB，还是LSB到MSB*/
    UINT8 TsSerialSyncMode; /*串行TS时，Sync信号第1个字节还是第1个Bit*/
} DTMBIP_Init_Info;

typedef struct DTMBIP_Single_Mode_t {
    UINT8 CarrierMode;
    UINT8 mode;
    UINT8 rate;
    UINT8 interleaverLength;
    UINT8 pnmode;
    UINT8 PnVariable;
} DTMBIP_Single_Mode;

/*
DTMB 内部使用相关函数声明
*/
UINT8 DTMBIP_SetWorkMode(void);
UINT8 DTMBIP_SetAutoMode(void);
UINT8 DTMBIP_SetNCOValue(UINT32 NCOValue, UINT32 CO1, UINT32 CO2);
UINT8 DTMBIP_SetTimeFreq(void);
UINT8 DTMBIP_SetAXIBus(int dev_fd);
UINT8 DTMBIP_FreeAXIBus(int dev_fd);
UINT8 DTMBIP_SetTunerType(UINT8 Type);
UINT8 DTMBIP_SetTsFormat(DTMBIP_Init_Info *InitInfo);
UINT8 DTMBIP_SetManualMode(DTMBIP_Single_Mode *mode);
UINT8 DTMBIP_GetNCOValue(UINT32 *NCOValue);
UINT8 DTMBIP_GetParameters(DTMBIP_Single_Mode *mode);
UINT8 DTMBIP_GetSignalSNRSC(double *SignalSNR);
UINT8 DTMBIP_GetSignalSNRMC(double *SignalSNR);
UINT8 DTMBIP_GetLdpcBER(double *pLdpcBER);
UINT8 DTMBIP_GetSignalBER(double *pSignalBER);
UINT8 DTMBIP_GetFieldStrength(UINT32 *FieldStrength);
UINT8 DTMBIP_GetSignalSNR(double *SignalSNR);
UINT8 DTMBIP_CalculateNCOValue(double TunerIF, UINT32 *NCOValue, UINT32 *CO1, UINT32 *CO2);
UINT8 DTMBIP_SoftReset(void);
UINT8 DTMBIP_SetSpectrum(UINT8 SpectrumMode);
UINT8 DTMBIP_Get_QAM_Mode(DTMBIP_Single_Mode *mode);
UINT8 DTMBIP_DebugSingalStatus(void);
#endif