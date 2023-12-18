#include <stdio.h>
#include <string.h>
#include "Atbm253_AW_Hal.h"
#include "ATBM253Api.h"
#include "os_adapter.h"
#include "fe_log.h"

static Atbm253_config gContextCfg;
#define ATMB253_DEV_ID  (0)

static ATBM253_SIGNAL_MODE_e Atbm253_tuner_mode(void)
{
    ATBM253_SIGNAL_MODE_e standard;

    switch (gContextCfg.tv_standard) {
    case MODE_DTMB:
        standard = ATBM253_SIGNAL_MODE_DTMB;
        break;
    case MODE_DVBT:
    case MODE_DVBT2:
        standard = ATBM253_SIGNAL_MODE_DVBT;
        break;
    case MODE_DVBC:
    case MODE_DVBC2:
        standard = ATBM253_SIGNAL_MODE_DVBC;
        break;
    case MODE_ISDBT:
        standard = ATBM253_SIGNAL_MODE_ISDBT;
        break;
    case MODE_ATSC:
        standard = ATBM253_SIGNAL_MODE_ATSC;
        break;
    case MODE_SECAM_L:
    case MODE_SECAM_L1:
    case MODE_PAL_I:
    case MODE_PAL_M:
    case MODE_PAL_B:
    case MODE_PAL_DK:
    case MODE_PAL_GH:
    case MODE_NTSC_MN:
        standard = ATBM253_SIGNAL_MODE_ATV;
        break;

    case MODE_ISDBC:
    case MODE_J83B:
    case MODE_DVBS:
    case MODE_DVBS2:
    case MODE_DVBS2X:
    default:
        FE_LOGE("TV standard[%d] is not Support and set to DTMB standard\n",
                gContextCfg.tv_standard);
        standard = ATBM253_SIGNAL_MODE_DTMB;
        break;
    }

    return standard;
}

static int Atbm253_hal_init(void)
{
    ATBM253_ERROR_e ret;
    ATBM253InitConfig_t private_Cfg;
    int rtval;

    rtval = i2c_connection_check(I2C_BUS_DEV, gContextCfg.tuner_i2c_addr);
    if (rtval) {
        FE_LOGE("Atbm253_init - i2c_connection_check fail, rtval=%d\n", rtval);
        return FRONTEND_ERROR;
    }

    /*Get the default configuration from ATBM253 SDK. */
    ret = ATBM253DefaultCfgGet(&private_Cfg);
    if(ret != ATBM253_NO_ERROR) {
        FE_LOGE("ATBM253DefaultCfgGet fail ret=%u\n", ret);
        return FRONTEND_ERROR;
    }

    /*Change some setting according to actual requirement.*/
    private_Cfg.I2CParam.I2CSlaveAddr = gContextCfg.tuner_i2c_addr;
    /*user parameter, which may be used in ATBM253I2CRead/Write. */
    private_Cfg.I2CParam.Param = 0;
    /*OSC PPM calibration, from 0x00 to 0x0F.*/
    private_Cfg.OSCCap.CalibValue = 0x08;
    private_Cfg.DtvIFOut.IFOutFreqHz = gContextCfg.IfFreq_dtv;
    private_Cfg.DtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL3;
    private_Cfg.AtvIFOut.IFOutFreqHz = gContextCfg.IfFreq_atv;
    private_Cfg.AtvIFOut.IFOutLevel = ATBM253_IF_OUT_LEVEL3;
    private_Cfg.Mode = Atbm253_tuner_mode();

    ret = ATBM253Init(ATMB253_DEV_ID, &private_Cfg);
    if (ret != ATBM253_NO_ERROR) {
        FE_LOGE("ATBM253Init fail ret=%u\n", ret);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_SetParament(void *tuner_config)
{
    Atbm253_config *config;

    config = (Atbm253_config *)tuner_config;
    if (config->bandwidth)
        gContextCfg.bandwidth = config->bandwidth;
    if (config->FreqHz)
        gContextCfg.FreqHz = config->FreqHz;
    if (config->IfFreq_atv)
        gContextCfg.IfFreq_atv = config->IfFreq_atv;
    if (config->IfFreq_dtv)
        gContextCfg.IfFreq_dtv = config->IfFreq_dtv;
    if (config->spectrum)
        gContextCfg.spectrum = config->spectrum;
    if (config->tuner_i2c_addr)
        gContextCfg.tuner_i2c_addr = config->tuner_i2c_addr;
    if (config->tv_standard)
        gContextCfg.tv_standard = config->tv_standard;

    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_FreqLock(void)
{
    ATBM_U32 BwKHz;
    ATBM253_SPECTRUM_MODE_e spectrum;
    ATBM253_SIGNAL_MODE_e standard;
    ATBM253_ERROR_e ret;
    unsigned long FreqKHz = (gContextCfg.FreqHz) / 1000;

    /* update tuner mode */
    standard = Atbm253_tuner_mode();

    switch (gContextCfg.bandwidth) {
    case TV_MODE_BW_6MHZ:
        BwKHz = 6000;
        break;
    case TV_MODE_BW_7MHZ:
        BwKHz = 7000;
        break;
    case TV_MODE_BW_8MHZ:
        BwKHz = 8000;
        break;
    default:
        BwKHz = 8000;//Bandwidth is 8Mhz;
        break;
    }

    if (gContextCfg.spectrum == NORMAL_SPECTRUM)
        spectrum = ATBM253_SPECTRUM_NORMAL;
    else
        spectrum = ATBM253_SPECTRUM_INVERT;

    FE_LOGV("ATBM253 FreqKHz=%ld, bandwidth=%u standard=%d, specturm=%d\n",
            FreqKHz, BwKHz, standard, spectrum);
    ret = ATBM253ChannelTune(ATMB253_DEV_ID, standard, FreqKHz, BwKHz, spectrum);
    if (ret) {
        FE_LOGE("ATBM253ChannelTune Channel tune failed, ret:%d\n", ret);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_Get_WorkStatus(SUPPORT_WORKSTATUS_E index, void *argv)
{
    ATBM253_ERROR_e ret = ATBM253_NO_ERROR;
    int rtval;

    if (!argv) {
        FE_LOGE("Index(%d) argv is NULL, please check!\n", index);
        return FRONTEND_ERROR;
    }

    /* VS Demod will check tuner status befor tuner init
     * so need to check i2c connection here
    */
    rtval = i2c_connection_check(I2C_BUS_DEV, gContextCfg.tuner_i2c_addr);
    if (rtval) {
        FE_LOGE("Atbm253_Get_WorkStatus - i2c_connection_check fail, rtval=%d\n", rtval);
        return FRONTEND_ERROR;
    }

    switch (index) {
    case DEVICE_DETECT:
    {
        U8 *Addr = (U8 *)argv;
        ATBM253I2CAddr_t I2CParm;

        I2CParm.I2CSlaveAddr = *Addr;
        ret = ATBM253Detect(&I2CParm);
        if (ret != ATBM253_NO_ERROR) {
            FE_LOGE("ATBM253 isn't exist ret=0x%x fail\n", ret);
            return FRONTEND_ERROR;
        }
        break;
    }
    case IFFREQ_GET:
    {
        ATBM_U32 *IFFreqHz = (ATBM_U32 *)argv;
        ret = ATBM253GetIFFreq(ATMB253_DEV_ID, IFFreqHz);
        if (ret != ATBM253_NO_ERROR) {
            FE_LOGE("ATBM253GetIFFreq ret=0x%x fail\n", ret);
            return FRONTEND_ERROR;
        }
        break;
    }
    case LOCKSTATUS_GET:
    {
        int *DeviceLock = (int *)argv;
        ATBM_BOOL tmp;
        ret = ATBM253GetLockStatus(ATMB253_DEV_ID, &tmp);
        if (ret != ATBM253_NO_ERROR) {
            FE_LOGE("ATBM253GetLockStatus ret=0x%x fail\n", ret);
            *DeviceLock = 0;
            return FRONTEND_ERROR;
        }

        if (tmp == ATBM_TRUE)
            *DeviceLock = 1;//lock
        else
            *DeviceLock = 0;//unlock

        break;
    }
    case RSSI_GET:
    {
        ATBM_S32 *RSSI = (ATBM_S32 *)argv;
        ret = ATBM253GetRSSI(ATMB253_DEV_ID, RSSI);
        if(ATBM253_NO_ERROR != ret) {
            FE_LOGE("ATBM253GetRSSI failed, ret:%d\n", ret);
            return FRONTEND_ERROR;
        }
        FE_LOGV("RSSI:%d dBm\n",*RSSI);
        break;
    }
    default:
        FE_LOGE("Not support ATBM253 Workstatus Index(%d), Please check!", index);
        return FRONTEND_ERROR;
    }

    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_Reset(void)
{
    /* unused */
    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_Standby(void)
{
    ATBM253_ERROR_e ret;

    ret = ATBM253Standby(ATMB253_DEV_ID);
    if (ret != ATBM253_NO_ERROR)
    {
        FE_LOGE("ATBM253 go to Standby mode fail\n");
        return FRONTEND_ERROR;
    }
    FE_LOGD("ATBM253Standby\n");

    return FRONTEND_NO_ERROR;
}

static int Atbm253_hal_Wakeup(void)
{
    int ret;

    ret = ATBM253Wakeup(ATMB253_DEV_ID);
    if (ret != ATBM253_NO_ERROR)
    {
        FE_LOGE("ATBM253 Wakeup fail\n");
        return FRONTEND_ERROR;
    }
    FE_LOGD("ATBM253Wakeup\n");

    return FRONTEND_NO_ERROR;
}

FrontendOpsS _Atbm253_Ops = {
    .init               = Atbm253_hal_init,
    .set_parament       = Atbm253_hal_SetParament,
    .set_frequency      = Atbm253_hal_FreqLock,
    .get_workstatus     = Atbm253_hal_Get_WorkStatus,
    .reset              = Atbm253_hal_Reset,
    .standby            = Atbm253_hal_Standby,
    .wakeup             = Atbm253_hal_Wakeup,
};

FrontendOpsS *get_Atbm253Ops()
{
    return &_Atbm253_Ops;
}

void *get_Atbm253Cfg()
{
    memset(&gContextCfg, 0, sizeof(gContextCfg));

    return (void *)&gContextCfg;
}