#include "stdio.h"
#include "Si2151_AW_Hal.h"
#include "Si2151_L1_API.h"
#include "Si2151_L2_API.h"
#include "os_adapter.h"
#include "fe_log.h"

static struct Si2151_Context {
    L1_Si2151_Context handle;
} Si2151_Context;

enum TV_TYPE {
    TYPE_DTV = 0,
    TYPE_ATV,
};

static Si2151_config gContextCfg;

static int Si2151_hal_standard_check(U8 tv_bandwidth, U8 tv_standard, U8 *bw,
                                 U8 *standard, U32 *atv_FreqOffset, enum TV_TYPE *atvOrdtv)
{
    switch (tv_bandwidth) {
    case TV_MODE_BW_6MHZ:
        *bw = Si2151_DTV_MODE_PROP_BW_BW_6MHZ;
        break;
    case TV_MODE_BW_7MHZ:
        *bw = Si2151_DTV_MODE_PROP_BW_BW_7MHZ;
        break;
    case TV_MODE_BW_8MHZ:
        *bw = Si2151_DTV_MODE_PROP_BW_BW_8MHZ;
        break;
    default:
        FE_LOGE("Bandwidth[%d] is not support!\n", tv_bandwidth);
        return FRONTEND_ERROR;
    }

    switch (tv_standard) {
    case MODE_DTMB:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_DTMB;
        break;
    case MODE_DVBT:
    case MODE_DVBT2:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_DVBT;
        break;
    case MODE_ATSC:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_ATSC;
        break;
    case MODE_ISDBT:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_ISDBT;
        break;
    case MODE_DVBC:
    case MODE_DVBC2:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_DVBC;
        break;
    case MODE_ISDBC:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_ISDBC;
        break;
    case MODE_J83B:
        *standard = Si2151_DTV_MODE_PROP_MODULATION_QAM_US;
        break;

    case MODE_SECAM_L:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 2250000;  // 7.25Mh
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L;
        break;
    case MODE_SECAM_L1:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 2250000;  // 7.25Mh
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LP;
        break;
    case MODE_PAL_I:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 2250000;  // 7.25Mh
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I;
        break;
    case MODE_PAL_M:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 400000;//5.76Mhz
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
        break;
    case MODE_NTSC_MN:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 760000;//5.76Mhz
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
        break;
    case MODE_PAL_B:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 1400000;  // 6.4Mhz
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
        break;
    case MODE_PAL_DK:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 1750000;  // 6.75Mhz
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
        break;
    case MODE_PAL_GH:
        *atvOrdtv = TYPE_ATV;
        *atv_FreqOffset = 1750000;  // 6.75HMz
        *standard = Si2151_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
        break;

    case MODE_DVBS:
    case MODE_DVBS2:
    case MODE_DVBS2X:
    default:
        FE_LOGE("standard[%d] Not support!\n", tv_standard);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_init(void)
{
    int ret = 0;
    L1_Si2151_Context *handle = &Si2151_Context.handle;

    ret = i2c_connection_check(I2C_BUS_DEV, gContextCfg.tuner_i2c_addr);
    if (ret) {
        FE_LOGE("Si2151_init - i2c_connection_check fail, ret=%d\n", ret);
        return FRONTEND_ERROR;
    }

    Si2151_L1_API_Init(handle, (int)gContextCfg.tuner_i2c_addr);

    handle->cmd->power_up.clock_mode =  Si2151_POWER_UP_CMD_CLOCK_MODE_XTAL;
    handle->cmd->power_up.en_xout    =  Si2151_POWER_UP_CMD_EN_XOUT_EN_XOUT;
    handle->cmd->config_clocks.clock_mode = Si2151_CONFIG_CLOCKS_CMD_CLOCK_MODE_XTAL;
    handle->i2c->trackRead = handle->i2c->trackWrite = 1;

    ret = Si2151_Init(handle);
    if (ret) {
        FE_LOGE("Si2151_Init fail ret=%s\n", Si2151_L1_API_ERROR_TEXT(ret));
        return FRONTEND_ERROR;
    }

    ret = Si2151_L1_GET_REV(handle); // get firmware version
    if (ret) {
        FE_LOGE("Si2151_L1_GET_REV fail with ret=%s\n", Si2151_L1_API_ERROR_TEXT(ret));
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_SetParament(void *tuner_config)
{
    Si2151_config *config;

    config = (Si2151_config *)tuner_config;
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

static int Si2151_hal_FreqLock(void)
{
    U8 bw, standard, spectrum;
    enum TV_TYPE atvOrdtv = TYPE_DTV;
    U32 atv_FreqOffset;
    L1_Si2151_Context *handle = &Si2151_Context.handle;
    unsigned long FreqHz = gContextCfg.FreqHz;
    int ret = 0;

    ret = Si2151_hal_standard_check(gContextCfg.bandwidth, gContextCfg.tv_standard,
                                &bw, &standard, &atv_FreqOffset, &atvOrdtv);
    if (ret == FRONTEND_ERROR)
        return ret;

    switch (gContextCfg.spectrum) {
    case NORMAL_SPECTRUM:
        if (atvOrdtv == TYPE_ATV)
            spectrum = Si2151_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_NORMAL;
        else
            spectrum = Si2151_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL;
        break;
    case INVERTED_SPECTRUM:
        if (atvOrdtv == TYPE_ATV)
            spectrum = Si2151_ATV_VIDEO_MODE_PROP_INVERT_SPECTRUM_INVERTED;
        else
            spectrum = Si2151_DTV_MODE_PROP_INVERT_SPECTRUM_INVERTED;
        break;
    default:
        FE_LOGE("spectrum[%d] Not support!\n", gContextCfg.spectrum);
        return FRONTEND_ERROR;
    }

    FE_LOGV("Si2151 FreqHz=%ld, bandwidth=%u standard=%u, specturm=%u atvoffset=%d atvOrdtv=%d\n",
            FreqHz, bw, standard, spectrum, atv_FreqOffset, atvOrdtv);
    if (atvOrdtv == TYPE_ATV)
        ret = Si2151_ATVTune(handle, FreqHz - atv_FreqOffset, standard, spectrum);
    else
        ret = Si2151_DTVTune(handle, FreqHz, bw, standard, spectrum);

    if (ret) {
        FE_LOGE("Si2151 Tune init failed, ret:%s\n", Si2151_L1_API_ERROR_TEXT(ret));
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_Get_WorkStatus(SUPPORT_WORKSTATUS_E index, void *argv)
{
    int ret = 0;
    L1_Si2151_Context *handle = &Si2151_Context.handle;

    if (!argv) {
        FE_LOGE("Index(%d) argv is NULL, please check!\n", index);
        return FRONTEND_ERROR;
    }

    /* VS Demod will check tuner status befor tuner init
     * so need to check i2c connection here
    */
    ret = i2c_connection_check(I2C_BUS_DEV, gContextCfg.tuner_i2c_addr);
    if (ret) {
        FE_LOGE("Si2151_Get_WorkStatus - i2c_connection_check fail, ret=%d\n", ret);
        return FRONTEND_ERROR;
    }

    switch (index) {
    case DEVICE_DETECT:
    {
        /* TODO: Default is Device alive */
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

        /* TODO: Default is Device alive */
        *DeviceLock = TRUE;

        break;
    }
    case RSSI_GET:
    {
        char *RSSI = (char *)argv;

        ret = Si2151_L1_TUNER_STATUS(handle);
        if (ret) {
            FE_LOGE("Error reading TUNER_STATUS ret=%s\n", Si2151_L1_API_ERROR_TEXT(ret));
            return FRONTEND_ERROR;
        }
        *RSSI = handle->rsp->tuner_status.rssi;
        FE_LOGV("SI2151 RSSI:%d dBm\n",*RSSI);

        break;
    }
    default:
        FE_LOGE("Not support SI2151 Workstatus Index(%d), Please check!", index);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_Reset(void)
{
    /* unused */
    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_Standby(void)
{
    int ret;
    L1_Si2151_Context *handle = &Si2151_Context.handle;

    ret = Si2151_Standby(handle);
    if (ret)
    {
        FE_LOGE("Si2151 go to Standby mode fail\n");
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Si2151_hal_Wakeup(void)
{
    int ret = 0;

    if (ret)
    {
        FE_LOGE("Si2151 Wakeup fail\n");
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

FrontendOpsS _Si2151_Ops = {
    .init               = Si2151_hal_init,
    .set_parament       = Si2151_hal_SetParament,
    .set_frequency      = Si2151_hal_FreqLock,
    .get_workstatus     = Si2151_hal_Get_WorkStatus,
    .reset              = Si2151_hal_Reset,
    .standby            = Si2151_hal_Standby,
    .wakeup             = Si2151_hal_Wakeup,
};

FrontendOpsS *get_Si2151Ops()
{
    return &_Si2151_Ops;
}