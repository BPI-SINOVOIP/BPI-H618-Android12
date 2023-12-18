/* ----------------------------------------------------------------------------
 File Name: DTMBIP.c

 Description:

 Version 1.0 : Created                    2021.07.08 Zepeng Wu
 ---------------------------------------------------------------------------- */
#include "DTMBType.h"
#include "DTMBIP.h"
#include "DTMBIP_User.h"
#include <math.h>
#include "dtmbip_sunxi.h"
#include "dc_log.h"
#include <stdint.h>

/**
 * spectrum inversion polarity switch
 */
#define DTMBIP_SPECTRUM_ADJUST_OPEN    0

/**
 * frequency sample rate(Mhz)
 */
//#define FREQUENCY_SAMPLE 30.375
#define FREQUENCY_SAMPLE 30.4 //changed by melis
#define DTMBIP_WAITTING_TIME_MS 50

UINT8 DTMBIP_SetWorkMode()
{
    UINT8 err = DTMB_NO_ERROR;

    DTMBIP_WriteRegister(0x0022, 0x7C);
    DTMBIP_WriteRegister(0x02FE, 0x20);
    DTMBIP_WriteRegister(0x02B2, 0x10);
    DTMBIP_WriteRegister(0x02EA, 0x10);
    DTMBIP_WriteRegister(0x02EF, 0x50);
    DTMBIP_WriteRegister(0x0260, 0x00);
    DTMBIP_WriteRegister(0x0275, 0x01);

    return err;
}

/************************************
** 自动设置地面情况下信号格式
*************************************/
/**
 *
 * @param
 * @return
 * @return
 */
UINT8 DTMBIP_SetAutoMode(void)
{
    UINT8 err;

    printf("Set AutoMode \n");
    /*配置为自动模式*/
    err = DTMBIP_WriteRegister(0x0002, 0x00);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
UINT8 DTMBIP_Get_QAM_Mode(DTMBIP_Single_Mode *mode)
{
    UINT8 err;

    err = DTMBIP_GetParameters(mode);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
UINT8 DTMBIP_DebugSingalStatus(void)
{
    double SNR, BER, LBER;
    DTMBIP_Single_Mode mode;
    UINT32 FieldStrength, NCOValue;

    DTMBIP_GetSignalSNR(&SNR);
    DTMBIP_GetSignalBER(&BER);
    DTMBIP_GetParameters(&mode);
    DTMBIP_GetFieldStrength(&FieldStrength);
    DTMBIP_GetLdpcBER(&LBER);
    DTMBIP_GetNCOValue(&NCOValue);
    printf("SNR: %d,BER: %d,LBER: %d,FS: %d,NCO: 0x%06x\n",
                   (uint32_t)(SNR * 100), (uint32_t)(BER * 100), (uint32_t)(LBER * 100), FieldStrength, NCOValue);
    printf("Mode:");

    switch (mode.mode) {
    case DTMB_DTMB_4QAM_NR:
        printf("4QAM_NR, ");
        break;

    case DTMB_DTMB_4QAM:
        printf("4QAM, ");
        break;

    case DTMB_DTMB_16QAM:
        printf("16QAM, ");
        break;

    case DTMB_DTMB_32QAM:
        printf("32QAM, ");
        break;

    case DTMB_DTMB_64QAM:
        printf("64QAM, ");
        break;

    default:
        break;
    }

    printf("PNMode:");

    switch (mode.pnmode) {
    case DTMB_PN_945:
        printf("945, ");
        break;

    case DTMB_PN_595:
        printf("595, ");
        break;

    case DTMB_PN_420:
        printf("420, ");
        break;

    default:
        break;
    }

    printf("Rate:");

    switch (mode.rate) {
    case DTMB_RATE_04:
        printf("0.4, ");
        break;

    case DTMB_RATE_06:
        printf("0.6, ");
        break;

    case DTMB_RATE_08:
        printf("0.8, ");
        break;

    default:
        break;
    }

    printf("Carrier:");

    switch (mode.CarrierMode) {
    case DTMB_CARRIER_SINGLE:
        printf("Single, ");
        break;

    case DTMB_CARRIER_MULTI:
        printf("Multi, ");
        break;

    default:
        break;
    }

    printf("Interleave:");

    switch (mode.interleaverLength) {
    case DTMB_INTERLEAVER_720:
        printf("720, ");
        break;

    case DTMB_INTERLEAVER_240:
        printf("240, ");
        break;

    default:
        break;
    }

    printf("PNVariable:");

    switch (mode.PnVariable) {
    case DTMB_PN_VARIABLE:
        printf("Variable ");
        break;

    case DTMB_PN_FIXED:
        printf("Fixed ");
        break;

    default:
        break;
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/**********************************************
判断信号是否锁住
**********************************************/
UINT8 DTMBIP_IsDemodLocked(UINT8 *LockResult, UINT8 *Status)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 locked = 0;
    static UINT8 tmp1 = 0, tmp2 = 0;

    err = DTMBIP_ReadRegister(0x0011, &locked);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x0010, Status);
    err_flag |= (!!err) << (err_off++);

    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    if (tmp1 != *Status || tmp2 != locked)
        printf("Status reg (0x10)=0x%02x, locked reg (0x11)=0x%02x!\n", *Status, locked);

    tmp1 = *Status;
    tmp2 = locked;

    if ((locked) & 0x01)
        *LockResult = 1;
    else
        *LockResult = 0;

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 手动设置地面情况下信号格式
*************************************/
UINT8 DTMBIP_SetManualMode(DTMBIP_Single_Mode *mode)
{
    UINT8 err = DTMB_NO_ERROR;
    UINT8 ReadData = 0, WriteData = 0;

    printf("Set Manual Mode !\n");
    /*配置为手动模式*/
    DTMBIP_WriteRegister(0x0002, 0xFF);
    DTMBIP_ReadRegister(0x0003, &ReadData);
    WriteData = ReadData & 0xE8;

    WriteData |= mode->pnmode;
    WriteData |= (mode->PnVariable << 2);
    WriteData |= (mode->CarrierMode << 4);
    DTMBIP_WriteRegister(0x0003, WriteData);
    printf("0x0003  0x%02x !\n", WriteData);

    DTMBIP_ReadRegister(0x0004, &ReadData);
    WriteData = ReadData & 0x88;
    WriteData |= mode->mode;
    WriteData |= (mode->rate << 4);
    WriteData |= (mode->interleaverLength << 6);
    DTMBIP_WriteRegister(0x0004, WriteData);
    printf("0x0004  0x%02x !\n", WriteData);

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 获取正常工作时的NCO
*************************************/
UINT8 DTMBIP_GetNCOValue(UINT32 *NCOValue)
{
    UINT8 err = DTMB_NO_ERROR;
    UINT8 ReadData = 0;

    DTMBIP_ReadRegister(0x0059, &ReadData);
    *NCOValue = ReadData;

    DTMBIP_ReadRegister(0x005A, &ReadData);
    *NCOValue <<= 8;
    *NCOValue |= ReadData;

    DTMBIP_ReadRegister(0x005B, &ReadData);
    *NCOValue <<= 8;
    *NCOValue |= ReadData;

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置初始工作时的NCO
*************************************/
UINT8 DTMBIP_SetNCOValue(UINT32 NCOValue, UINT32 CO1, UINT32 CO2)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 temp1 = 0, temp2 = 0, temp3 = 0;

    temp1 = (UINT8)((NCOValue & 0x00ff0000) >> 16);
    temp2 = (UINT8)((NCOValue & 0x0000ff00) >> 8);
    temp3 = (UINT8)((NCOValue & 0x000000ff));
    printf("Updage NCO 0x%02x,0x%02x,0x%02x\n", temp1, temp2, temp3);
    err = DTMBIP_WriteRegister(0x0028, temp1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_WriteRegister(0x0029, temp2);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_WriteRegister(0x002A, temp3);
    err_flag |= (!!err) << (err_off++);

    temp1 = (UINT8)((CO1 & 0x0000ff00) >> 8);
    temp2 = (UINT8)((CO1 & 0x000000ff));
    printf("Updage CO1 0x%02x,0x%02x\n", temp1, temp2);
    err = DTMBIP_WriteRegister(0x0188, temp1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_WriteRegister(0x0189, temp2);
    err_flag |= (!!err) << (err_off++);

    temp1 = (UINT8)((CO2 & 0x0000ff00) >> 8);
    temp2 = (UINT8)((CO2 & 0x000000ff));
    printf("Updage CO2 0x%02x,0x%02x\n", temp1, temp2);
    err = DTMBIP_WriteRegister(0x0186, temp1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_WriteRegister(0x0187, temp2);
    err_flag |= (!!err) << (err_off++);

    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置初始工作时的定时频率
*************************************/
UINT8 DTMBIP_SetTimeFreq(void)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    double FrequencySample = FREQUENCY_SAMPLE;
    UINT32 TF = 0;
    UINT8 temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0;

    /* Calculation method required by DTMB IP */
    TF = (UINT32)((FrequencySample - 30.24) / 30.24 * 0x40000000);
    temp1 = (UINT8)((TF & 0xff000000) >> 24);
    temp2 = (UINT8)((TF & 0x00ff0000) >> 16);
    temp3 = (UINT8)((TF & 0x0000ff00) >> 8);
    temp4 = (UINT8)((TF & 0x000000ff));
    printf("Updage Time Frequency 0x%02x,0x%02x,0x%02x,0x%02x\n",
                   temp1, temp2, temp3, temp4);
    err = DTMBIP_WriteRegister(0x0038, temp1);
    err_flag |= (!!err) << (err_off++);

    err = DTMBIP_WriteRegister(0x0039, temp2);
    err_flag |= (!!err) << (err_off++);

    err = DTMBIP_WriteRegister(0x003A, temp3);
    err_flag |= (!!err) << (err_off++);

    err = DTMBIP_WriteRegister(0x003B, temp4);
    err_flag |= (!!err) << (err_off++);

    temp1 = (UINT8)(FrequencySample / 30.24 * 25);
    temp1 &= 0x3f;
    temp1 |= 0x80;
    err = DTMBIP_WriteRegister(0x0197, temp1);
    err_flag |= (!!err) << (err_off++);

    err = DTMBIP_ReadRegister(0x0022, &temp1);
    err_flag |= (!!err) << (err_off++);

    temp1 |= 0x08;
    err = DTMBIP_WriteRegister(0x0022, temp1);
    err_flag |= (!!err) << (err_off++);

    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置AXI总线
*************************************/
UINT8 DTMBIP_SetAXIBus(int dev_fd)
{
    UINT8 err = DTMB_NO_ERROR;
    int ret;

	ret = ioctl(dev_fd, DTMBIP_CMD_SETAXIBUS, 0);
	if (ret < 0) {
		printf("DTMBIP_CMD_SETAXIBUS faile\n");
		err = DTMB_OTHER_ERROR;
	}

    return err;
}

UINT8 DTMBIP_FreeAXIBus(int dev_fd)
{
    UINT8 err = DTMB_NO_ERROR;
    int ret;

    ret = ioctl(dev_fd, DTMBIP_CMD_FREE_AXIBUS, 0);
    if (ret < 0) {
        printf("DTMBIP_FreeAXIBus faile\n");
        err = DTMB_OTHER_ERROR;
    }

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置工作频谱
*************************************/
UINT8 DTMBIP_SetSpectrum(UINT8 SpectrumMode)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData = 0, WriteData = 0;

    err = DTMBIP_ReadRegister(0x0021, &ReadData);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    if (SpectrumMode == DTMB_SPECTRUM_NEGATIVE) {
        WriteData = ReadData & 0xDF;
    } else if (DTMB_SPECTRUM_POSITIVE) {
        WriteData = ReadData | 0x20;
    }

    err = DTMBIP_WriteRegister(0x0021, WriteData);
    err_flag |= (!!err) << (err_off++);

    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    return err;
}

static double DTMBIP_ABS(double x)
{
    return x < 0 ? -x : x;
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 根据tuner中频和采样频率计算NCO
*************************************/
UINT8 DTMBIP_CalculateNCOValue(double TunerIF, UINT32 *NCOValue, UINT32 *CO1, UINT32 *CO2)
{
    double FrequencySample = FREQUENCY_SAMPLE;
    double Temp;

    if (TunerIF == 0) {
        *NCOValue = 0;
    } else {
        Temp = DTMBIP_ABS(FrequencySample - TunerIF);

        if (Temp < TunerIF)
            *NCOValue = (UINT32)(Temp / FrequencySample * 0x800000);
        else
            *NCOValue = (UINT32)(TunerIF / FrequencySample * 0x800000);
    }

    *CO1 = (UINT32)(7.56 / (5000 * FrequencySample) * 0x800000);
    *CO2 = (UINT32)(7.56 / (2048 * FrequencySample) * 0x800000);

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 获取地面实际信号的格式
*************************************/
UINT8 DTMBIP_GetParameters(DTMBIP_Single_Mode *mode)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1, ReadData2;

    err = DTMBIP_ReadRegister(0x0005, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x0006, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    mode->PnVariable = ReadData1 & 0x04;
    mode->PnVariable >>= 2;

    mode->CarrierMode = ReadData1 & 0x10;
    mode->CarrierMode >>= 4;

    mode->rate = ReadData2 & 0x30;
    mode->rate >>= 4;

    mode->interleaverLength = ReadData2 & 0x40;
    mode->interleaverLength >>= 6;

    mode->mode = ReadData2 & 0x07;
    mode->pnmode = ReadData1 & 0x03;

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
多载波情况下，获取信噪比
****************************/
UINT8 DTMBIP_GetSignalSNRMC(double *SignalSNR)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1 = 0, ReadData2 = 0;
    UINT32 value_reg;

    err = DTMBIP_ReadRegister(0x02C9, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x02C8, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    value_reg = ReadData1 * 256 + ReadData2;
    if ((value_reg != 0) && (value_reg != 0xFFFF)) {
        if (value_reg < 16)
            value_reg = 16;

        //*SignalSNR = 10 * log10((double)(value_reg) / 16);
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
单载波情况下，获取信噪比
****************************/
UINT8 DTMBIP_GetSignalSNRSC(double *SignalSNR)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1 = 0, ReadData2 = 0;
    UINT32 value_reg;
    UINT8 Pnmode;
    UINT32 PnValue = 0;
    double value_temp = 0;

    err = DTMBIP_ReadRegister(0x0005, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    Pnmode = ReadData1 & 0x03;
    if (Pnmode == DTMB_PN_945)
        PnValue = 945;
    else if (Pnmode == DTMB_PN_595)
        PnValue = 595;
    else if (Pnmode == DTMB_PN_420)
        PnValue = 420;

    err = DTMBIP_ReadRegister(0x02A5, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x02A4, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("line=%d fail err_flag=0x%x\n", __LINE__, err_flag);
        return (DTMB_OTHER_ERROR);
    }

    value_reg = ReadData1 * 256 + ReadData2;
    if ((value_reg != 0) && (value_reg != 0xFFFF)) {
        value_temp = 40.5 * (double)(PnValue) / value_reg;
        if (value_temp < 1) {
            value_temp = 1;
        }

        //*SignalSNR = 10 * log10(value_temp);
    } else {
        *SignalSNR = 0;
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
获取信噪比
****************************/
UINT8 DTMBIP_GetSignalSNR(double *SignalSNR)
{
    UINT8 err = 0;
    UINT8 ReadData1 = 0;
    UINT8 CarrierMode;
    /*获取工作模式,DTMB*/
    err = DTMBIP_ReadRegister(0x0005, &ReadData1);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    CarrierMode = ReadData1 & 0x10;
    CarrierMode >>= 4;

    if (CarrierMode == DTMB_CARRIER_SINGLE)
        err = DTMBIP_GetSignalSNRSC(SignalSNR);
    else if (CarrierMode == DTMB_CARRIER_MULTI)
        err = DTMBIP_GetSignalSNRMC(SignalSNR);

    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
地面信号情况下获取LDPC误码率
****************************/
UINT8 DTMBIP_GetLdpcBER(double *pLdpcBER)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1 = 0, ReadData2 = 0;

    err = DTMBIP_ReadRegister(0x00AA, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x00A9, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    *pLdpcBER = (double)((ReadData2 & 0x0f) * 256 + ReadData1) / 4096;

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
地面信号情况下获取误码率
****************************/
UINT8 DTMBIP_GetSignalBER(double *pSignalBER)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1 = 0, ReadData2 = 0;

    err = DTMBIP_ReadRegister(0x00A7, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x00A8, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    *pSignalBER = (double)(ReadData1 * 256 + ReadData2) / 6666664;

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/****************************
地面信号情况下获取信号场强
****************************/
UINT8 DTMBIP_GetFieldStrength(UINT32 *FieldStrength)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData1 = 0, ReadData2 = 0;

    err = DTMBIP_ReadRegister(0x0061, &ReadData1);
    err_flag |= (!!err) << (err_off++);
    err = DTMBIP_ReadRegister(0x0062, &ReadData2);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    *FieldStrength = (UINT16)(ReadData1 * 256 + ReadData2);

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 芯片软复位
*************************************/
UINT8 DTMBIP_SoftReset(void)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;

    err = DTMBIP_WriteRegister(0x0009, 0x10);
    err_flag |= (!!err) << (err_off++);

    DTMBIP_Wait(1); /* wait 1 ms */

    err = DTMBIP_WriteRegister(0x0009, 0x00);
    err_flag |= (!!err) << (err_off++);
    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置tuner类型，
** 零中频DTMB_TUNER_ZERO_IF
** 或其他
*************************************/
UINT8 DTMBIP_SetTunerType(UINT8 Type)
{
    UINT8 err = 0, err_flag = 0, err_off = 0;
    UINT8 ReadData = 0, WriteData = 0;

    err = DTMBIP_ReadRegister(0x0007, &ReadData);
    err_flag |= (!!err) << (err_off++);

    if (Type == DTMB_TUNER_ZERO_IF)
        WriteData = ReadData | 0x20;
    else
        WriteData = ReadData & 0xDF;

    err = DTMBIP_WriteRegister(0x0007, WriteData);
    err_flag |= (!!err) << (err_off++);

    if (err_flag) {
        printf("fail err_flag=0x%x\n", err_flag);
        return (DTMB_OTHER_ERROR);
    }

    return (err);
}

/**
 *
 * @param
 * @return
 * @return
 */
/************************************
** 设置TS的输出格式
*************************************/
UINT8 DTMBIP_SetTsFormat(DTMBIP_Init_Info *InitInfo)
{
    UINT8 err;
    UINT8 ReadData = 0, WriteData = 0;
    UINT8 Serial, CLK_Polarity, CLK_Valid, Null_Packet, Serial_Output, Serial_Order, Serial_Sync;

    Serial = InitInfo->TsType;
    CLK_Polarity = InitInfo->TsClockPolarity;
    CLK_Valid = InitInfo->TsClockValid;
    Null_Packet = InitInfo->TsNullPacket;
    Serial_Output = InitInfo->TsSerialOutPut;
    Serial_Order = InitInfo->TsSerialOrder;
    Serial_Sync = InitInfo->TsSerialSyncMode;

    err = DTMBIP_ReadRegister(0x0007, &ReadData);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    WriteData = ReadData & 0x20;

    if (Serial == DTMB_TS_OUTPUT_SERIAL)
        WriteData |= 0x01;

    if (CLK_Polarity == DTMB_TS_DATA_SAMPLE_FALLING)
        WriteData |= 0x02;

    if (CLK_Valid == DTMB_TS_CLK_WITH_TSVLD)
        WriteData |= 0x04;

    if (Null_Packet == DTMB_TS_NULLPACKET_DELETED)
        WriteData |= 0x08;

    if (Serial_Output == DTMB_TS_SERIAL_OUTPUT_D0)
        WriteData |= 0x10;

    if (Serial_Order == DTMB_TS_SERIAL_LSB_TO_MSB)
        WriteData |= 0x40;

    if (Serial_Sync == DTMB_TS_SERIAL_SYNC_BIT)
        WriteData |= 0x80;

    err = DTMBIP_WriteRegister(0x0007, WriteData);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    printf("TS Format 0x%02x\n", WriteData);

    return (DTMB_NO_ERROR);
}

static UINT8 DTMBIP_SetADCTimerFeq(int dev_fd, BOOL flag)
{
    int ret;
    UINT8 err = DTMB_NO_ERROR;

    if (flag == TRUE)
    {
        //set adc timer frquency to 30.4Mhz
        ret = ioctl(dev_fd, DTMBIP_CMD_SETADC4DTMB, 0);
        if (ret < 0)
        {
            printf("DTMBIP_CMD_SETAXIBUS faile\n");
            err = DTMB_OTHER_ERROR;
        }
    }
    else
    {
        //reset adc timer frequency to default(54Mhz)
        ret = ioctl(dev_fd, DTMBIP_CMD_RESETADC4DTMB, 0);
        if (ret < 0)
        {
            printf("DTMBIP_CMD_SETAXIBUS faile\n");
            err = DTMB_OTHER_ERROR;
        }
    }

    return err;
}

/**
 *
 * @param
 * @return
 * @return
 */
/********************************
获取信号强度
如果tuner的AGC是自控的，请调用tuner内部的信号强度函数
********************************/
UINT8 DTMBIP_GetSignalStrength(UINT8 *SignalStrength)
{
    UINT8 err = DTMB_NO_ERROR;
    UINT32 FieldStrength;

    err = DTMBIP_GetFieldStrength(&FieldStrength);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    if (FieldStrength >= 0x800)
        FieldStrength = 0x800;
    if (FieldStrength <= 0x418)
        FieldStrength = 0x418;

    /*可以调整这个计算公式，去得到期望的合理的SignalStrength值，归一化到0～100
    */
    *SignalStrength = (UINT8)((0x800 - FieldStrength) / 12);

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/********************************
获取信号质量
********************************/
UINT8 DTMBIP_GetSignalQuality(UINT8 *SignalQuality)
{
    UINT8 err;
    double SignalSNR;

    err = DTMBIP_GetSignalSNR(&SignalSNR);
    if (err) {
        printf("fail err_flag=0x%x\n", err);
        return (DTMB_OTHER_ERROR);
    }

    *SignalQuality = (UINT8)SignalSNR;
    (*SignalQuality) *= 3;
    if (*SignalQuality >= 98) {
        *SignalQuality = 98;
    }

    return (DTMB_NO_ERROR);
}

/**
 *
 * @param
 * @return
 * @return
 */
/********************************
DTMB 和 tuner 初始化，
在系统初始化时调用,
输入AXI总线偏移地址
********************************/
UINT8 DTMBIP_Init(int dtmbip_handle, TV_FrontendS *frontend_handle)
{
    UINT8 ret = DTMB_NO_ERROR;
    UINT32 NCOValue = 0, CO1 = 0, CO2 = 0;
    DTMBIP_Init_Info InitInfo;

    ret = DTMBIP_SetADCTimerFeq(dtmbip_handle, TRUE);
    if (ret) {
        printf("DTMBIP_SetADCTimerFeq fail ret=%d\n", ret);
        goto out;
    }

    //ADC Work on
    //ret = DtvAdcControl(TRUE);
    if (ret) {
        printf("DtvAdcControl ADC WorkOn fail ret=%d\n", ret);
        goto out;
    }

    DTMBIP_SetDeviceHanle(dtmbip_handle);
    /*初始化配置DTMBIP*/
    InitInfo.TunerType = DTMB_TUNER_NORMAL; /*高频头类型，零中频或其他 */
    InitInfo.TunerIF = 5.0;  /*高频头的中频，单位MHz，根据实际使用做修改,零中频高频头请直接赋值0*/
    InitInfo.TsType = DTMB_TS_OUTPUT_PARALLEL;/*TS流输出方式：串行或者并行两种*/
    InitInfo.TsClockPolarity = DTMB_TS_DATA_SAMPLE_RISING; /*数据采样是在时钟上升沿或下降沿*/
    InitInfo.TsClockValid = DTMB_TS_CLK_ALWAYS;/*时钟有效方式，一直有效或随valid信号*/
    InitInfo.TsNullPacket = DTMB_TS_NULLPACKET_DELETED; /*空包使能与否*/
    InitInfo.TsSerialOutPut = DTMB_TS_SERIAL_OUTPUT_D7; /*串行TS时，输出在TS_D7或TS_D0*/
    InitInfo.TsSerialOrder = DTMB_TS_SERIAL_MSB_TO_LSB; /*串行TS时，输出是MSB到LSB，还是LSB到MSB*/
    InitInfo.TsSerialSyncMode = DTMB_TS_SERIAL_SYNC_BIT; /*串行TS时，Sync信号第1个字节还是第1个Bit*/
    /*硬复位DTMBIP*/
    DTMBIP_HWReset();

    /*高频头初始化 & Reinit TunerIF */
    ret = DTMBIP_TunerInit(&InitInfo.TunerIF, frontend_handle);
    if (ret) {
        printf("DTMBIP_TunerInit fail ret=%d\n", ret);
        goto out;
    }
    /* AXI 总线配置*/
    ret = DTMBIP_SetAXIBus(dtmbip_handle);
    if (ret) {
        printf("DTMBIP_TunerInit fail ret=%d\n", ret);
        goto out;
    }

    DTMBIP_SoftReset();

    /*根据tuner IF和晶振计算NCO*/
    DTMBIP_CalculateNCOValue(InitInfo.TunerIF, &NCOValue, &CO1, &CO2);
    /*设置NCO*/
    DTMBIP_SetNCOValue(NCOValue, CO1, CO2);

    /*设置定时频率*/
    DTMBIP_SetTimeFreq();
    /*设置tuner类型，零中频或其他*/
    DTMBIP_SetTunerType(InitInfo.TunerType);
    /*设置TS输出格式*/
    DTMBIP_SetTsFormat(&InitInfo);
    /*设置工作模式，地面*/
    DTMBIP_SetWorkMode();

out:
    return (ret);
}

/**
 *
 * @param
 * @return
 * @return
 */
UINT8 DTMBIP_DeInit(int dtmbip_handle, TV_FrontendS *frontend_handle)
{
    UINT8 ret;

    //Tuner Stop Work
    ret = DTMBIP_TunerStandby(frontend_handle);
    if (ret)
    {
        printf("Frontend_standby fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    //ADC Stop Work
    //ret = DtvAdcControl(FALSE);
    if (ret) {
        printf("DtvAdcControl ADC Standby fail ret=%d\n", ret);
        return -1;
    }

    ret = DTMBIP_FreeAXIBus(dtmbip_handle);
    if (ret) {
        printf("DtvAdcControl ADC Standby fail ret=%d\n", ret);
        return -1;
    }

    ret = DTMBIP_SetADCTimerFeq(dtmbip_handle, FALSE);
    if (ret) {
        printf("DTMBIP_SetADCTimerFeq fail ret=%d\n", ret);
        return -1;
    }

    return DTMB_NO_ERROR;
}

UINT8 DTMBIP_standby(TV_FrontendS **frontend_handle)
{
    int32_t ret;

#if TUNER_USE_ATBM253
    *frontend_handle = Frontend_GetInstance(ATBM253);
#elif TUNER_USE_SI2151
    *frontend_handle = Frontend_GetInstance(SI2151);
#elif TUNER_USE_R842
    *frontend_handle = Frontend_GetInstance(R842);
#endif
    if (*frontend_handle == NULL) {
        printf("demodInit - Frontend_GetInstance failture...");
        return DTMB_OTHER_ERROR;
    }

    ret = DTMBIP_TunerSetParam(NULL, *frontend_handle);
    if (ret)
    {
        printf("DTMBIP_TunerSetParam fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    ret = Frontend_Init(*frontend_handle);
    if (ret)
    {
        printf("Frontend_Init fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    //Tuner require to init first before Standby
    //ret = DTMBIP_TunerStandby(*frontend_handle);
    if (ret)
    {
        printf("DTMBIP_TunerStandby fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    //Tuner ADC Stop Work
    //ret = DtvAdcControl(FALSE);
    if (ret) {
        printf("DtvAdcControl ADC Standby fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    return DTMB_NO_ERROR;
}

/**
 *
 * @param
 * @return
 * @return
 */
/********************************
地面信号时锁频函数,单位HZ
********************************/
BOOL DTMBIP_DTMBSetFrequency(UINT32 Frequency, TV_FrontendS *frontend_handle)
{
    UINT8 CheckLockTemp, LockResult, status;
    BOOL SignalDetect = FALSE;
#if DTMBIP_SPECTRUM_ADJUST_OPEN
    UINT8 ReadData0 = 0, ReadData1 = 0, ReadData2 = 0;
#endif

    /********************
    ** 需要首先锁定高频头
    ********************/
    if (DTMBIP_TunerLock(Frequency, frontend_handle) != TRUE) {
        return FALSE;
    }

    /*设置为自动工作模式*/
    DTMBIP_SetAutoMode();
#if DTMBIP_SPECTRUM_ADJUST_OPEN
    DTMBIP_SetSpectrum(DTMB_SPECTRUM_NEGATIVE);
    DTMBIP_SoftReset();

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked&LockResult, &status);

        if ((status & 0xf0) > 0x20) {
            SignalDetect = TRUE;
        }

        if ((SignalDetect == FALSE) && (CheckLockTemp == 15)) {
            printf(("---> [DTMBIP] DTMBIP DTMB No Signal \n"));
            return FALSE;
        }

        if (LockResult == 1) {
            DTMBIP_ReadRegister(0x0006, &ReadData0);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData1);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData2);
            CheckLockTemp++;
            CheckLockTemp++;

            if ((ReadData0 == ReadData1) && (ReadData1 == ReadData2)) {
                printf(("---> [DTMBIP] DTMBIP DTMB is locked \n"));
                return TRUE;
            } else {
                break;
            }
        }
    }

    /*Do Spectrum Adjust Try*/
    DTMBIP_SetSpectrum(DTMB_SPECTRUM_POSITIVE);
    DTMBIP_SoftReset();
    SignalDetect = FALSE;

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked(&LockResult, &status);

        if ((status & 0xf0) > 0x20) {
            printf(("---> [DTMBIP] DTMBIP DTMB Has Signal \n"));
            SignalDetect = TRUE;
        }

        if ((SignalDetect == FALSE) && (CheckLockTemp == 15)) {
            printf(("---> [DTMBIP] DTMBIP DTMB No Signal \n"));
            return FALSE;
        }

        if (LockResult == 1) {
            DTMBIP_ReadRegister(0x0006, &ReadData0);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData1);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData2);
            CheckLockTemp++;
            CheckLockTemp++;

            if ((ReadData0 == ReadData1) && (ReadData1 == ReadData2)) {
                printf(("---> [DTMBIP] DTMBIP DTMB is locked \n"));
                return TRUE;
            } else {
                break;
            }
        }
    }

    DTMBIP_SetSpectrum(DTMB_SPECTRUM_NEGATIVE);
    DTMBIP_SoftReset();
#else
    DTMBIP_SoftReset();

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked(&LockResult, &status);

        if ((status & 0xf0) > 0x20) {
            SignalDetect = TRUE;
        }

        printf("LockResult: %d, SignalDetect: %d\n",
                       LockResult, SignalDetect);

        if ((SignalDetect == FALSE) && (CheckLockTemp >= 19)) {
            printf("---> [DTMBIP] DTMBIP DTMB No Signal \n");
            return FALSE;
        }

        if (LockResult == 1) {
            printf("---> [DTMBIP] DTMBIP DTMB is locked \n");
            return TRUE;
        }
    }

#endif
    return FALSE;
}

/**
 * Set the mode of terrestrial broadcasting single by manual
 * @param
 * @return
 * @return
 */
/********************************
手动设置地面情况下信号格式
********************************/
BOOL DTMBIP_DTMBManualSetFrequency(UINT32 Frequency, TV_FrontendS *frontend_handle,
                                   DTMBIP_Single_Mode *mode)
{
    UINT8 CheckLockTemp, LockResult, status;
    BOOL SignalDetect = FALSE;
#if DTMBIP_SPECTRUM_ADJUST_OPEN
    UINT8 ReadData0 = 0, ReadData1 = 0, ReadData2 = 0;
#endif

    /********************
    ** 需要首先锁定高频头
    ********************/
    if (DTMBIP_TunerLock(Frequency, frontend_handle) != TRUE) {
        return FALSE;
    }

    /*设置为手动工作模式*/
    DTMBIP_SetManualMode(mode);
#if DTMBIP_SPECTRUM_ADJUST_OPEN
    UINT8 locked;
    DTMBIP_SetSpectrum(DTMB_SPECTRUM_NEGATIVE);
    DTMBIP_SoftReset();

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked(&LockResult, &status);

        if ((status & 0xf0) > 0x20) {
            SignalDetect = TRUE;
        }

        if ((SignalDetect == FALSE) && (CheckLockTemp == 15)) {
            printf("---> [DTMBIP] DTMBIP DTMB No Signal \n");
            return FALSE;
        }

        if (LockResult == 1) {
            DTMBIP_ReadRegister(0x0006, &ReadData0);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData1);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData2);
            CheckLockTemp++;
            CheckLockTemp++;

            if ((ReadData0 == ReadData1) && (ReadData1 == ReadData2)) {
                printf("---> [DTMBIP] DTMBIP DTMB is locked \n");
                return TRUE;
            } else {
                break;
            }
        }
    }

    /*Do Spectrum Adjust Try*/
    DTMBIP_SetSpectrum(DTMB_SPECTRUM_POSITIVE);
    DTMBIP_SoftReset();
    SignalDetect = FALSE;

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked(&LockResult, &status);

        if ((status & 0xf0) > 0x20) {
            SignalDetect = TRUE;
        }

        if ((SignalDetect == FALSE) && (CheckLockTemp == 15)) {
            printf("---> [DTMBIP] DTMBIP DTMB No Signal \n");
            return FALSE;
        }

        if (LockResult == 1) {
            DTMBIP_ReadRegister(0x0006, &ReadData0);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData1);
            DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
            DTMBIP_ReadRegister(0x0006, &ReadData2);
            CheckLockTemp++;
            CheckLockTemp++;

            if ((ReadData0 == ReadData1) && (ReadData1 == ReadData2)) {
                printf("---> [DTMBIP] DTMBIP DTMB is locked \n");
                return TRUE;
            } else {
                break;
            }
        }
    }

    DTMBIP_SetSpectrum(DTMB_SPECTRUM_NEGATIVE);
    DTMBIP_SoftReset();
#else
    DTMBIP_SoftReset();

    for (CheckLockTemp = 0; CheckLockTemp < 20; CheckLockTemp++) {
        DTMBIP_Wait(DTMBIP_WAITTING_TIME_MS);
        DTMBIP_IsDemodLocked(&LockResult, &status);

        /*无信号2秒退出*/
        if ((status & 0xf0) > 0x20) {
            SignalDetect = TRUE;
        }

        if ((SignalDetect == FALSE) && (CheckLockTemp == 15)) {
            printf("---> [DTMBIP] DTMBIP DTMB No Signal \n");
            return FALSE;
        }

        if (LockResult == 1) {
            printf("---> [DTMBIP] DTMBIP DTMB is locked \n");
            return TRUE;
        }
    }

#endif
    return FALSE;
}