#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "R842_AW_Hal.h"
#include "R842.h"
#include "os_adapter.h"
#include "fe_log.h"

static R842_config gContextCfg;

static int R842_connection_check(UINT8 *checkflag)
{
    int ret;
    I2C_LEN_TYPE dataI2C;
    memset(&dataI2C, 0, sizeof(I2C_LEN_TYPE));

    dataI2C.RegAddr = 0x00;
    dataI2C.Len = 1;

    if (gContextCfg.tuner_i2c_addr == 0) {
        FE_LOGD("gContextCfg doesn't init, use default R842 I2C address\n");
        gContextCfg.tuner_i2c_addr = R842_I2C_ADDRESS;
    }

    ret = i2c_connection_check(I2C_BUS_DEV, gContextCfg.tuner_i2c_addr);
    if (ret) {
        FE_LOGE("R842_connection_check - i2c_connection_check fail, ret=%d\n", ret);
        return FRONTEND_ERROR;
    }

    if (I2C_Read_Len_R842(&dataI2C) == R842_Fail) {
        FE_LOGE("R842 IIC connect failed!\n");
        return FRONTEND_ERROR;
    }

    FE_LOGV("Read data: 0x%02x\n", dataI2C.Data[0]);
    *checkflag = dataI2C.Data[0];

#if 0
    I2C_LEN_TYPE dataI2C_t, Wdata;
    I2C_TYPE testI2C;
    for (int t = 0; t < 3; t++) {
        FE_LOGE("R842_connection_check test %d:", t);
        memset(&Wdata, 0, sizeof(Wdata));
        Wdata.RegAddr = 14;
        Wdata.Data[0] = 0x01 * t;
        Wdata.Len = 1;
        I2C_Write_Len(&Wdata);

        Wdata.RegAddr = 13;
        Wdata.Data[0] = 0x02 * t;
        I2C_Write_Len(&Wdata);

        Wdata.RegAddr = 12;
        Wdata.Data[0] = 0x03 * t;
        I2C_Write_Len(&Wdata);

		testI2C.RegAddr = 11;
		testI2C.Data = 0x04 * t;
        I2C_Write(&testI2C);

        memset(&dataI2C_t, 0, sizeof(dataI2C_t));
        dataI2C_t.RegAddr = 0x00;
        dataI2C_t.Len = 15;
        I2C_Read_Len_R842(&dataI2C_t);

        FE_LOGE("Convert After");
        for (int i = 0; i <= dataI2C_t.Len; i++)
            FE_LOGE("0x%02x ", dataI2C_t.Data[i]);
        FE_LOGE("\n");
    }
#endif

    return FRONTEND_NO_ERROR;
}

static R842_ErrCode R842_standard_check(U8 bandwidth, U8 tv_standard, R842_Standard_Type *type,
                                        R842_IfAgc_Type *agc_type)
{
    R842_ErrCode ret = R842_Success;

    *agc_type = R842_IF_AGC1;//default use AGC1
    switch (tv_standard) {
    case MODE_DTMB:
        if (bandwidth == TV_MODE_BW_6MHZ)
            *type = R842_DTMB_6M_IF_5M;
        else if (bandwidth == TV_MODE_BW_8MHZ)
            *type = R842_DTMB_8M_IF_5M;
        else
            ret = R842_Fail;
        break;

    case MODE_DVBT:
        if (bandwidth == TV_MODE_BW_6MHZ)
            *type = R842_DVB_T_6M_IF_5M;
        else if (bandwidth == TV_MODE_BW_7MHZ)
            *type = R842_DVB_T_7M_IF_5M;
        else if (bandwidth == TV_MODE_BW_8MHZ)
            *type = R842_DVB_T_8M_IF_5M;
        else
            ret = R842_Fail;
        break;

    case MODE_DVBT2:
        if (bandwidth == TV_MODE_BW_6MHZ)
            *type = R842_DVB_T2_6M_IF_5M;
        else if (bandwidth == TV_MODE_BW_7MHZ)
            *type = R842_DVB_T2_7M_IF_5M;
        else if (bandwidth == TV_MODE_BW_8MHZ)
            *type = R842_DVB_T2_8M_IF_5M;
        else
            ret = R842_Fail;
        break;

    case MODE_ATSC:
        *type = R842_ATSC_IF_5M;
        break;

    case MODE_ISDBT:
        *type = R842_ISDB_T_IF_5M;
        break;

    case MODE_DVBC:
        if (bandwidth == TV_MODE_BW_6MHZ)
            *type = R842_DVB_C_6M_IF_5M;
        else if (bandwidth == TV_MODE_BW_8MHZ)
            *type = R842_DVB_C_8M_IF_5M;
        else
            ret = R842_Fail;
        break;

    case MODE_J83B:
        *type = R842_J83B_IF_5M;
        break;

    case MODE_SECAM_L:
        *type = R842_SECAM_L;
        break;
    case MODE_SECAM_L1:
        *type = R842_SECAM_L1;
        break;
    case MODE_PAL_I:
        *type = R842_PAL_I;
        break;
    case MODE_NTSC_MN:
    case MODE_PAL_M:
        *type = R842_MN_5800;
        break;
    case MODE_PAL_B:
        *type = R842_PAL_B_7M;
        break;
    case MODE_PAL_DK:
        *type = R842_PAL_DK;
        break;
    case MODE_PAL_GH:
        *type = R842_PAL_BGH_8M;
        break;

    case MODE_DVBC2:
    case MODE_ISDBC:
    case MODE_DVBS:
    case MODE_DVBS2:
    case MODE_DVBS2X:
    default:
        FE_LOGE("R842 ATV standard Not support yet and use default config\n");
        ret = R842_Fail;
    }

    return ret;
}

static int R842_hal_init(void)
{
    R842_ErrCode ret;
    UINT8 checkflag = 0;

    ret = R842_connection_check(&checkflag);
    if (ret) {
        FE_LOGE("R842_connection_check fail\n");
        return FRONTEND_ERROR;
    }

    ret = R842_Init();
    if (!ret || !checkflag) {
        FE_LOGE("R842_Init fail ret=%d checkflag=0x%x\n", ret, checkflag);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int R842_hal_SetParament(void *tuner_config)
{
    R842_config *config;

    config = (R842_config *)tuner_config;

    /* R842 support other IF, but we chance 5Mhz to default IFFreq */
    gContextCfg.IFFreqHz = IFOUT_FREQ_5MHZ;

    if (config->bandwidth)
        gContextCfg.bandwidth = config->bandwidth;
    if (config->FreqHz)
        gContextCfg.FreqHz = config->FreqHz;
    if (config->IFFreqHz)
        gContextCfg.IFFreqHz = config->IFFreqHz;
    if (config->spectrum)
        gContextCfg.spectrum = config->spectrum;
    if (config->tuner_i2c_addr)
        gContextCfg.tuner_i2c_addr = config->tuner_i2c_addr;
    if (config->tv_standard)
        gContextCfg.tv_standard = config->tv_standard;

    return FRONTEND_NO_ERROR;
}

static int R842_hal_FreqLock(void)
{
    UINT8 Lock = 0;
    R842_ErrCode ret;
    R842_Set_Info R842_Info;
    R842_Standard_Type tv_type;
    R842_IfAgc_Type agc_type;

    ret = R842_standard_check(gContextCfg.bandwidth, gContextCfg.tv_standard, &tv_type, &agc_type);
    if (ret == R842_Fail) {
        FE_LOGE("R842_standard_check failed\n");
        return FRONTEND_ERROR;
    }

    /* Because the tvfe_AgcIO node is not generated after the system is started,
     * this method cannot take effect, and it needs to be changed to ioctl to
     * implement it later */
    if (tv_type != R842_DTMB_6M_IF_5M && tv_type != R842_DTMB_8M_IF_5M) {
        system("echo 1 > /sys/class/dtmbip/dtmbip/tvfe_AgcIO");
        FE_LOGV("Select to TV-AGC");
    }

    FE_LOGV("R842 FreqHz=%ld, bandwidth=%s standard=%s tv_type=%d agc_type=%d\n",
            gContextCfg.FreqHz, TV_BANDWIDTH(gContextCfg.bandwidth),
            TV_STANDARD(gContextCfg.tv_standard),tv_type, agc_type);

    R842_Info.RF_KHz = gContextCfg.FreqHz / 1000;
    R842_Info.R842_IfAgc_Select = agc_type;
    R842_Info.R842_Standard = tv_type;

    ret = R842_SetPllData(R842_Info);
    if (ret == R842_Fail) {
        FE_LOGE("R842_SetPllData init failed, ret:%d\n", ret);
        return FRONTEND_ERROR;
    }

    Lock = R842_PLL_Lock();
    if (!Lock) {
        FE_LOGE("R842_PLL_Lock lock failed\n");
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int R842_hal_Get_WorkStatus(SUPPORT_WORKSTATUS_E index, void *argv)
{
    if (!argv) {
        FE_LOGE("Index(%d) argv is NULL, please check!\n", index);
        return FRONTEND_ERROR;
    }

    switch (index) {
    case DEVICE_DETECT:
    {
        R842_ErrCode ret;
        UINT8 checkflag = 0;

        ret = R842_connection_check(&checkflag);
        if (ret || !checkflag) {
            FE_LOGE("R842_connection_check fail\n");
            return FRONTEND_ERROR;
        }

        break;
    }
    case IFFREQ_GET:
    {
        unsigned int *IFFreqHz = (unsigned int *)argv;

        *IFFreqHz = gContextCfg.IFFreqHz;

        break;
    }
    case LOCKSTATUS_GET:
    {
        BOOL *DeviceLock = (BOOL *)argv;
        UINT8 Lock = 0;

        Lock = R842_PLL_Lock();
        if (Lock == 1)
            *DeviceLock = TRUE;
        else
            *DeviceLock = FALSE;

        break;
    }
    case RSSI_GET:
    {
        INT32 *RSSIDbm = (INT32 *)argv;
        R842_Standard_Type tv_type;
        R842_ErrCode ret;
        UINT32 RFKhz = gContextCfg.FreqHz / 1000;
        R842_IfAgc_Type agc_type;

        ret = R842_standard_check(gContextCfg.bandwidth, gContextCfg.tv_standard, &tv_type, &agc_type);
        if (ret == R842_Fail) {
            FE_LOGE("R842_standard_check failed\n");
            return FRONTEND_ERROR;
        }
        R842_GetTotalRssi(RFKhz, tv_type, RSSIDbm);

        FE_LOGV("R842 RSSI:%d dBm\n",*RSSIDbm);

        break;
    }
    default:
        FE_LOGE("Not support R842 Workstatus Index(%d), Please check!", index);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int R842_hal_Reset(void)
{
    /* unused */
    return FRONTEND_NO_ERROR;
}

static int R842_hal_Standby(void)
{
    R842_ErrCode ret;

    ret = R842_Standby();
    if (ret == R842_Fail)
    {
        FE_LOGE("R842 go to Standby mode fail\n");
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int R842_hal_Wakeup(void)
{
    R842_ErrCode ret;

    ret = R842_WakeUp();
    if (ret == R842_Fail)
    {
        FE_LOGE("R842 Wakeup fail\n");
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

FrontendOpsS _R842_Ops = {
    .init               = R842_hal_init,
    .set_parament       = R842_hal_SetParament,
    .set_frequency      = R842_hal_FreqLock,
    .get_workstatus     = R842_hal_Get_WorkStatus,
    .reset              = R842_hal_Reset,
    .standby            = R842_hal_Standby,
    .wakeup             = R842_hal_Wakeup,
};

FrontendOpsS *get_R842Ops()
{
    return &_R842_Ops;
}