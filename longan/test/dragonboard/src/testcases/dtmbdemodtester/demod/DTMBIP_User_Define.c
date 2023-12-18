/* ----------------------------------------------------------------------------
 File Name: DTMBIP_User_Define.c

 Description:

 Version 1.0 : Created                    2021.07.08 Zepeng Wu
---------------------------------------------------------------------------- */
#include "DTMBType.h"
#include "DTMBIP.h"
#include "DTMBIP_User.h"
#include "dtmbip_sunxi.h"
#include "math.h"
#include <dc_log.h>
#include <unistd.h>
#include "Atbm253_AW_Hal.h"
#include "Si2151_AW_Hal.h"
#include "R842_AW_Hal.h"

/*****************************************************************************
 Customers can customize the following functions by themselves
*****************************************************************************/
#define NUMBER_LOST_BEFORE_UNLOCK           6
#define MONITOR_DELAY_MS                    500

static int dtmbip_dev_handle = 0;

/**
 * Moniter Demod Locked status per MONITOR_DELAY_MS ms time
 * @param void
 * @return void
 */
void DTMBIP_MonitorTask(void)
{
    UINT8 LockResult, status;
    UINT8 DTMBIP_LockConut = NUMBER_LOST_BEFORE_UNLOCK;

    while (1) {
        DTMBIP_IsDemodLocked(&LockResult, &status);

        if (LockResult == 1) {
            DTMBIP_LockConut = NUMBER_LOST_BEFORE_UNLOCK;
            /*Locked*/
        } else {
            DTMBIP_LockConut--;
        }

        if (DTMBIP_LockConut == 0) {
            /*信号失锁，报告给应用程序？
            请根据平台实际设计实现。
            */
        }

        DTMBIP_Wait(MONITOR_DELAY_MS);
    }
}

UINT8 DTMBIP_TunerSetParam(double *TunerIF, TV_FrontendS *frontend_handle)
{
    int ret;

    if (TunerIF)
        *TunerIF = (double)(IFOUT_FREQ_5MHZ) / pow(10, 6);//TunerIF = 5.0Mhz

#if TUNER_USE_ATBM253
    Atbm253_config config;
    memset(&config, 0, sizeof(Atbm253_config));
    config.tuner_i2c_addr   = ATBM253_I2C_ADDRESS;
    config.IfFreq_dtv       = IFOUT_FREQ_5MHZ;
    config.IfFreq_atv       = IFOUT_FREQ_5MHZ;
#elif TUNER_USE_SI2151
    Si2151_config config;
    memset(&config, 0, sizeof(Si2151_config));
    config.tuner_i2c_addr   = SI2151_I2C_ADDRESS;
    config.IFFreqHz         = IFOUT_FREQ_5MHZ;
#elif TUNER_USE_R842
    R842_config config;
    memset(&config, 0, sizeof(R842_config));
    config.tuner_i2c_addr   = R842_I2C_ADDRESS;
    config.IFFreqHz         = IFOUT_FREQ_5MHZ;
#endif
    config.tv_standard      = MODE_DTMB;
    ret = Frontend_SetParament(frontend_handle, (void *)&config);
    if (ret)
    {
        printf("Frontend_SetParament fail ret=%d\n", ret);
        return (DTMB_OTHER_ERROR);
    }

    return DTMB_NO_ERROR;
}

/**
 * change config parameter to Tuner and Do initialization for Tuner
 * @param TunerIF - save Tuner IF frequency(Mhz)
 * @return DTMB_NO_ERROR    - Init Tuner success
 * @return DTMB_OTHER_ERROR - Init Tuner fail
 */
UINT8 DTMBIP_TunerInit(double *TunerIF, TV_FrontendS *frontend_handle)
{
    int ret;

    ret = DTMBIP_TunerSetParam(TunerIF, frontend_handle);
    if (ret)
    {
        printf("DTMBIP_TunerSetParam fail ret=%d\n", ret);
        return (DTMB_OTHER_ERROR);
    }

    ret = Frontend_Init(frontend_handle);
    if (ret)
    {
        printf("Frontend_Init fail ret=%d\n", ret);
        return (DTMB_OTHER_ERROR);
    }

    ret = Frontend_wakeup(frontend_handle);
    if (ret)
    {
        printf("Frontend_wakeup fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    return (DTMB_NO_ERROR);
}

/**
 * Make Tuner enter into standy state
 * @param TV_FrontendS *frontend_handle - tuner handle
 * @return DTMB_NO_ERROR    - enter standy success
 * @return DTMB_OTHER_ERROR - enter standy fail
 */
UINT8 DTMBIP_TunerStandby(TV_FrontendS *frontend_handle)
{
    UINT8 ret = DTMB_NO_ERROR;

    ret = Frontend_standby(frontend_handle);
    if (ret)
    {
        printf("DTMB Tuner standby fail ret=%d\n", ret);
        return DTMB_OTHER_ERROR;
    }

    return ret;
}

/**
 * set or change IF center frequency of Tuner IF output
 * @param Frequency - New IF center frequency
 * @return TRUE - Switch channel sueccessfully
 * @return FALSE - Switch channel fail
 */
BOOL DTMBIP_TunerLock(UINT32 Freq_Hz, TV_FrontendS *frontend_handle)
{
    int ret;
#if TUNER_USE_ATBM253
    Atbm253_config config;
#elif TUNER_USE_SI2151
    Si2151_config config;
#elif TUNER_USE_R842
    R842_config config;
#endif

    memset(&config, 0, sizeof(config));
    config.bandwidth = TV_MODE_BW_8MHZ;
    config.spectrum = INVERTED_SPECTRUM;
    config.FreqHz = Freq_Hz;
    ret = Frontend_SetParament(frontend_handle, (void *)&config);
    if (ret)
    {
        printf("Frontend_SetParament fail ret=%d\n", ret);
        return (DTMB_OTHER_ERROR);
    }

    ret = Frontend_SetFreqHz(frontend_handle);
    if (ret)
    {
        printf("Tuner Lock failed, ret=%d\n", ret);
        return FALSE;
    }

    return TRUE;
}

/**
 * save fd to global dtmbip device handle
 * @param fd - /dev/dtmbip file device handle
 * @return void
 */
void DTMBIP_SetDeviceHanle(int fd)
{
    dtmbip_dev_handle = fd;
}

/**
 * write value to DTMB IP register
 * @param Register - DTMB Demod IP register
 * @param Data     - the value which is write to IP reg
 * @return DTMB_NO_ERROR    - Read operation success
 * @return DTMB_OTHER_ERROR - Read operation fail
 */
UINT8 DTMBIP_WriteRegister(UINT16 Register, UINT8 Data)
{
    int ret;
    struct dtmb_regsetting regbuf;

    /* a DTMB register length is 4byte */
    regbuf.reg = Register * 4;
    regbuf.val = Data;
    ret = ioctl(dtmbip_dev_handle, DTMBIP_CMD_WRITEREG, &regbuf);
    if (ret < 0) {
        printf("faile ret=%d Addr:0x%x data:0x%x\n", ret, Register, Data);
        return DTMB_OTHER_ERROR;
    }

    //printf("DTMBIP Write Register addr:0x%04x, ic addr:0x%04x Write Date:0x%02x\n",
        //Register,Register *4, Data);

    return (DTMB_NO_ERROR);
}

/**
 * Read value of DTMB IP register
 * @param Register - DTMB Demod IP register
 * @param Data     - the value which is read from IP reg
 * @return DTMB_NO_ERROR    - Read operation success
 * @return DTMB_OTHER_ERROR - Read operation fail
 */
UINT8 DTMBIP_ReadRegister(UINT16 Register, UINT8 *Data)
{
    int ret;
    struct dtmb_regsetting regbuf;

    /* a DTMB register length is 4byte */
    regbuf.reg = Register * 4;
    regbuf.val = 0;
    ret = ioctl(dtmbip_dev_handle, DTMBIP_CMD_READREG, &regbuf);
    if (ret < 0) {
        printf("faile ret=%d reg = 0x%x\n", ret, Register);
        return DTMB_OTHER_ERROR;
    }

    *Data = regbuf.val;
    //printf("DTMBIP Read  Register addr:0x%04x, ic addr:0x%04x Read Date:0x%02x\n",
        //Register, Register *4, *Data);

    return (DTMB_NO_ERROR);
}

/**
 * wait special time
 * @param ms - mean millisecond
 * @return void
 */
void DTMBIP_Wait(UINT16 ms)
{
    usleep(ms * 1000);
}

/**
 * HardWare Reset DTMB Demod IP
 * @param  void
 * @return void
 */
void DTMBIP_HWReset(void)
{
    // TODO:
    return;
}
