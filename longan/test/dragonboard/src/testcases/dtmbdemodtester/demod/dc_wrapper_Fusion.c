#include <stdlib.h>
#include <dc_types.h>
#include <dc_log.h>
#include <pthread.h>

#include <dc_demod.h>

#include <os_adapter.h>

#include <drxfsn.h>
#include <DataType.h>
#include "TunerAPI.h"

/* define to enable DVB-C Demod(DRX_Fusion) message print */
//TRID_DEBUG_CATEGORY(DVB_C_Demod);
//#define TRID_CAT_DEFAULT (DVB_C_Demod)
TRID_DEBUG_CATEGORY(atsc_debug);
TRID_DEBUG_CATEGORY(dvb_debug);

struct Fusion_ImplS
{
    DC_DemodT base;
    uint32_t ref;
    uint32_t freq;

    //uint32_t bandwidth; /* MHZ */
    enum DemodStateE state;
    pthread_mutex_t mutex;
    int32_t unlock_counter;
};

static int32_t demodInit(struct Fusion_ImplS *impl)
{
    int32_t ret = 0;

    ret = DVB_TunerInit(DVB_DTV);
    if (ret) {
        printf("%s DVB-C Demod initial failture ret=%d", __func__, ret);
        ret = -1;
    }

    return ret;
}

static int32_t demodDeinit(struct Fusion_ImplS *impl)
{
    int32_t ret = 0;
    printf("DVB-C Demod deinit...\n");

    //ret = DRX_Close(impl->demod);
    if(ret) {
        printf("%s DVB-C Demod Close failture ret=%d", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t demod_SetFrequency(DRXStandard_t standard, uint32_t freq)
{
    DVBScanParam_t Param;
    int32_t ret;

    Param.Freq = freq/1000;
    Param.SymbolRate = 6900;
    Param.ModulationMode = _QAM64_;
    Param.BandWidth = _SCAN_BANDWIDTH_8M_;
    Param.priority = 1;
    // auto symbol rate
    DVB_SetConfig(trid_true);

    // tune
    ret = DVB_TuneDTVChan(_CABLE_DIGITAL_TUNER_, &Param);
    if (ret) {
        printf("Failed to tune DVBC DTV channel[%d KHz]\n", Param.Freq);
        return -1;
    }

#if 1
    int bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
    if (bLock) {
        printf("\n===>>>DVBC DTV [%d KHz] is locked at QAM64 Symbol rate = %d", Param.Freq, Param.SymbolRate);
    } else {
        printf("%s DVB_TunerIsLocked can't lock [%d KHz] at QAM64", __func__, Param.Freq);
        return -1;
    }
#endif

    return 0;
}

static int32_t __Fusion_Init(DC_DemodT *demod)
{
    int ret = 0;
    struct Fusion_ImplS *impl = NULL;
    impl = DC_ContainerOf(demod, struct Fusion_ImplS, base);

    printf("__Fusion_Init...\n");
    pthread_mutex_lock(&impl->mutex);

    if (impl->state != DEMOD_STATE_NOTREADY) {
        printf("invalid operation, current state '%d'", impl->state);
        ret = -1;
        goto out;
    }

    ret = demodInit(impl);
    if (ret != 0) {
        printf("demodInit initial failture...\n");
        goto out;
    }

    /* success state change. */
    impl->state = DEMOD_STATE_IDEAL;
    printf("demodInit initial success...\n");

out:
    pthread_mutex_unlock(&impl->mutex);
    return ret;
}

static int32_t __Fusion_SetFrequency(DC_DemodT *demod, uint32_t freq)
{
    struct Fusion_ImplS *impl = NULL;
    int ret = 0;
    printf("Fusion set frequency '%u'\n", freq);

    if (freq == 0) {
        printf("invalid freq paramter, should not be 0\n");
        impl->state = DEMOD_STATE_ERROR;
        return -1;
    }

    impl = DC_ContainerOf(demod, struct Fusion_ImplS, base);

    pthread_mutex_lock(&impl->mutex);

    if (impl->state == DEMOD_STATE_WORKING) {
        printf("lock new frequency '%d'", freq);
    } else if (impl->state == DEMOD_STATE_IDEAL) {
        printf("set frequency and work...\n");
    } else {
        ret = -1;
        printf("invalid operation , current state '%d'", impl->state);
        impl->state = DEMOD_STATE_ERROR;
        goto out;
    }

    impl->freq = freq;

    ret = demod_SetFrequency(DRX_STANDARD_ITU_A, impl->freq);
    if (ret) {
        printf("DVB-C IP set freq(%u) failture..., maybe not lock...", impl->freq);
        impl->state = DEMOD_STATE_ERROR;
        goto out;
    }

    /* after set frequency, should be working */
    impl->state = DEMOD_STATE_WORKING;

out:
    pthread_mutex_unlock(&impl->mutex);

    return ret;
}

static int32_t __Fusion_GetWorkStatus(DC_DemodT *demod, DemodWorkStatusT *status)
{
    struct Fusion_ImplS *impl = DC_ContainerOf(demod, struct Fusion_ImplS, base);
    int ret = 0;
    uint8_t bLock;

    TunerSignalQualityInfo_t mSignalQualityInfo;
    TunerSignalStrengthInfo_t mSignalStrength;
    memset(&mSignalQualityInfo, 0x00, sizeof(TunerSignalQualityInfo_t));
    memset(&mSignalStrength, 0x00, sizeof(TunerSignalStrengthInfo_t));    

    status->state = impl->state;
    if (impl->state != DEMOD_STATE_WORKING) {
        /* not working... */
        ret = -1;
        printf("not working... current state '%d'", status->state);
        goto out;
    }

    status->freq = impl->freq;

    bLock = DVB_TunerIsLocked(_CABLE_DIGITAL_TUNER_);
    if (bLock) {
        /* locked */
        impl->unlock_counter = 0;
        status->is_lock = 1;
    } else {
        ret = -1;
        impl->unlock_counter++;
        status->is_lock = 0;
        /* should we continue to check the quality of the singal??
         * or just goto out??
         */
        goto out;
    }

    //ret = DVB_TunerSignalCheck(_CABLE_DIGITAL_TUNER_, &mSignalQualityInfo, &mSignalStrength);
    if (ret) {
        printf("%s DVB_TunerSignalCheck fail ret=%d", __func__, ret);
        goto out;
    }
    status->strength = mSignalStrength.Value;
    status->quality  = mSignalQualityInfo.Value;

out:
    pthread_mutex_unlock(&impl->mutex);
    return ret;
}

static int32_t __Fusion_Reset(DC_DemodT *demod)
{
    struct Fusion_ImplS *impl = NULL;
    int ret = 0;

    impl = DC_ContainerOf(demod, struct Fusion_ImplS, base);
    pthread_mutex_lock(&impl->mutex);

    if (impl->state != DEMOD_STATE_NOTREADY) {
        ret = demodDeinit(impl);
        if (ret) {
            printf("%s demodDeinit DeInitial fail, ret=%d", __func__, ret);
            impl->state = DEMOD_STATE_ERROR;
            goto out;
        }
    }

    ret = demodInit(impl);
    if (ret) {
        impl->state = DEMOD_STATE_ERROR;
        printf("demodInit Re-Initial failture...\n");
        goto out;
    }

    if (impl->freq) {
        ret = demod_SetFrequency(DRX_STANDARD_ITU_A, impl->freq);
        if (ret) {
            printf("DRX IP Reset freq-[%uKHZ] fail", impl->freq);
            impl->state = DEMOD_STATE_ERROR;
            goto out;
        }
        /* after set frequency, should be working */
        impl->state = DEMOD_STATE_WORKING;
    } else {
        impl->state = DEMOD_STATE_IDEAL;
    }

out:
    pthread_mutex_unlock(&impl->mutex);
    return ret;
}

static void __Fusion_Stop(DC_DemodT *demod)
{
    struct Fusion_ImplS *impl = NULL;
    int ret = 0;

    impl = DC_ContainerOf(demod, struct Fusion_ImplS, base);
    printf("demod stop...\n");
    pthread_mutex_lock(&impl->mutex);
    if (impl->state == DEMOD_STATE_NOTREADY) {
        /* already stop nothing to do... */
    } else {
        ret = demodDeinit(impl);
        if (ret) {
            impl->state = DEMOD_STATE_ERROR;
            printf("%s demodDeinit fail ret=%d", __func__, ret);
            goto out;
        }
    }

    impl->state = DEMOD_STATE_NOTREADY;

out:
    pthread_mutex_unlock(&impl->mutex);
    return;
}

static struct DemodOpsS fusion_ops =
{
    .init = __Fusion_Init,
    .set_frequency = __Fusion_SetFrequency,
    .get_workstatus = __Fusion_GetWorkStatus,
    .reset = __Fusion_Reset,
    .stop = __Fusion_Stop
};

/*
 * only one device, we need singleton instance
 * and shold only has one owner
 */
DC_DemodT *Fusion_SingletonGetInstance(void)
{
    static struct Fusion_ImplS *impl = NULL;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    DC_DemodT *demod = NULL;

    pthread_mutex_lock(&mutex);

    if (!impl) {
        impl = malloc(sizeof(*impl));
        if (!impl) {
            printf("%s malloc Fusion_ImplS fail", __func__);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        memset(impl, 0x00, sizeof(*impl));

        impl->base.ops = &fusion_ops;
        pthread_mutex_init(&impl->mutex, NULL);
        impl->state = DEMOD_STATE_NOTREADY;
        impl->ref = 0;
    }
    demod = &impl->base;
    demod->connectParam = calloc(1, sizeof(TunerConnectParam));

    /* Enable to print DVB-C Demod(DRX_Fusion) message */
    //trid_debug_init("DVB_C_Demod\n");
    //TRID_DEBUG_CATEGORY_INIT(DVB_C_Demod, "DVB_C_Demod", 0, "DVB_C_Demod\n");

    pthread_mutex_unlock(&mutex);

    return demod;
}
