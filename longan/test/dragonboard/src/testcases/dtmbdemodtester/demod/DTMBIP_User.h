/* ----------------------------------------------------------------------------
 File Name: DTMBIP_User_Define.h

 Description:

 Version 1.0 : Created                    2021.07.08 Zepeng Wu
 ---------------------------------------------------------------------------- */
#ifndef __DTMBIP_USER_H__
#define __DTMBIP_USER_H__
#include <stdio.h>
#include "DTMBType.h"
#include "DTMBIP.h"
#include "tv_frontend.h"

#define TUNER_USE_ATBM253   (1) //mean use ATBM253 as tuner
#define TUNER_USE_SI2151    (0)
#define TUNER_USE_R842      (0)
/**
 * Customers can customize the following functions by themselves
 */
/* Debug message printf */
#define DTMBTBX_Print(fmt, arg...) ALOGE(fmt, ##arg)

void DTMBIP_MonitorTask(void);
void DTMBIP_SetDeviceHanle(int fd);
UINT8 DTMBIP_WriteRegister(UINT16 Register, UINT8 Data);
UINT8 DTMBIP_ReadRegister(UINT16 Register, UINT8 *Data);
void DTMBIP_Wait(UINT16 millisecond);
void DTMBIP_HWReset(void);
UINT8 DTMBIP_TunerSetParam(double *TunerIF, TV_FrontendS *frontend_handle);
UINT8 DTMBIP_TunerInit(double *TunerIF, TV_FrontendS *frontend_handle);
UINT8 DTMBIP_TunerStandby(TV_FrontendS *frontend_handle);
BOOL DTMBIP_TunerLock(UINT32 Freq_Hz, TV_FrontendS *frontend_handle);
UINT8 DTMBIP_GetSignalStrength(UINT8 *SignalStrength);
UINT8 DTMBIP_GetSignalQuality(UINT8 *SignalQuality);
UINT8 DTMBIP_Init(int dtmbip_handle, TV_FrontendS *frontend_handle);
UINT8 DTMBIP_DeInit(int dtmbip_handle, TV_FrontendS *frontend_handle);
UINT8 DTMBIP_standby(TV_FrontendS **frontend_handle);
BOOL DTMBIP_DTMBSetFrequency(UINT32 Frequency, TV_FrontendS *frontend_handle);
BOOL DTMBIP_DTMBManualSetFrequency(UINT32 Frequency, TV_FrontendS *frontend_handle, DTMBIP_Single_Mode *mode);
BOOL DTMBIP_IsDemodLocked(UINT8 *LockResult, UINT8 *Status);
#endif