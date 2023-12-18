#include <stdlib.h>
#include <dc_types.h>
#include <unistd.h>
#include <dc_log.h>
#include <pthread.h>
#include <dc_demod.h>
#include <os_adapter.h>
#include <DTMBIP.h>
#include <DTMBIP_User.h>
#include <dtmbip_sunxi.h>
#include <tv_frontend.h>
#include <fcntl.h>

struct DTMBIP_ImplS
{
    DC_DemodT base;
    uint32_t ref;
    uint32_t freq;
    int fd;                    //fd - dtmbip device handle
    TV_FrontendS *frontend_handle;
    enum DemodStateE state;
    pthread_mutex_t mutex;
    int32_t unlock_counter;
};

static int32_t demodInit(struct DTMBIP_ImplS *impl)
{
    int ret = 0, fd;

    //DTMB Demod Work on
    fd = open(DTMBIP_DEV, O_RDWR);
    if (fd < 0) {
        printf("open %s faile\n", DTMBIP_DEV);
        return DTMB_OTHER_ERROR;
    }
    impl->fd = fd;

#if TUNER_USE_ATBM253
    impl->frontend_handle = Frontend_GetInstance(ATBM253);
#elif TUNER_USE_SI2151
    impl->frontend_handle = Frontend_GetInstance(SI2151);
#elif TUNER_USE_R842
    impl->frontend_handle = Frontend_GetInstance(R842);
#endif
    if (impl->frontend_handle == NULL) {
        printf("demodInit - Frontend_GetInstance failture...");
        return DTMB_OTHER_ERROR;
    }

    //DTMD Demod init and Tuner/ADC Work on
    ret = DTMBIP_Init(impl->fd, impl->frontend_handle);
    if (ret) {
        printf("DTMBIP initial failture...");
        close(impl->fd);
        return DTMB_OTHER_ERROR;
    }

    return DTMB_NO_ERROR;
}

static int32_t demodDeinit(struct DTMBIP_ImplS *impl)
{
    int ret;

    printf("demod deinit...");
    //Tuner and ADC Stop Work
    ret = DTMBIP_DeInit(impl->fd, impl->frontend_handle);
    if (ret) {
        printf("DTMBIP_DeInit fail ret=%d\n", ret);
        return -1;
    }

    //DTMB Demod Stop Work
    ret = close(impl->fd);
    if (ret) {
        printf("close fd fail ret=%d\n", ret);
        return -1;
    }

    return 0;
}

static int32_t __DTMBIP_Init(DC_DemodT *demod)
{
    int ret = 0;
    struct DTMBIP_ImplS *impl = NULL;
    impl = DC_ContainerOf(demod, struct DTMBIP_ImplS, base);

    printf("__DTMBIP_Init...");
    pthread_mutex_lock(&impl->mutex);

    if (impl->state != DEMOD_STATE_NOTREADY) {
        printf("invalid operation, current state '%d'", impl->state);
        ret = -1;
        goto out;
    }

    ret = demodInit(impl);
    if (ret != 0) {
        printf("demodInit initial failture...");
        goto out;
    }

    /* success state change. */
    impl->state = DEMOD_STATE_IDEAL;
    printf("demodInit initial success...");

out:
    pthread_mutex_unlock(&impl->mutex);
    return ret;
}

static int32_t __DTMBIP_SetFrequency(DC_DemodT *demod, uint32_t freq)
{
    struct DTMBIP_ImplS *impl = NULL;
    int ret = 0;
    printf("DTMBIP set frequency '%u'", freq);

    if (freq == 0) {
        printf("invalid paramter, freq should not be 0\n");
        return -1;
    }

    impl = DC_ContainerOf(demod, struct DTMBIP_ImplS, base);

    pthread_mutex_lock(&impl->mutex);

    if (impl->state == DEMOD_STATE_WORKING) {
        printf("lock new frequency '%d'", freq);
    } else if (impl->state == DEMOD_STATE_IDEAL) {
        printf("set frequency and work...");
    } else {
        ret = -1;
        printf("invalid operation , current state '%d'", impl->state);
        goto out;
    }

    impl->freq = freq;

    ret = DTMBIP_DTMBSetFrequency(impl->freq, impl->frontend_handle);
    if (ret != TRUE) {
        printf("DTMBIP_DTMBSetFrequency(%u) failture maybe not lock", impl->freq);
        ret = -1;
    } else {
        /* after set frequency, should be working */
        impl->state = DEMOD_STATE_WORKING;
        ret = 0; /* ret == TRUE */
    }

out:
    pthread_mutex_unlock(&impl->mutex);

    return ret;
}

static int32_t __DTMBIP_GetWorkStatus(DC_DemodT *demod, DemodWorkStatusT *status)
{
    struct DTMBIP_ImplS *impl = DC_ContainerOf(demod, struct DTMBIP_ImplS, base);
    int ret = 0, err = 0, err_off = 0, err_flag = 0;
    uint8_t lock_result, lock_status;

    pthread_mutex_lock(&impl->mutex);

    status->state = impl->state;
    if (impl->state != DEMOD_STATE_WORKING) {
        /* not working... */
        ret = -1;
        printf("not working... current state '%d'", status->state);
        goto out;
    }

    DTMBIP_IsDemodLocked(&lock_result, &lock_status);
    if (lock_result == 1) {
        /* locked */
        impl->unlock_counter = 0;
        status->is_lock = 1;
    } else {
        impl->unlock_counter++;
        status->is_lock = 0;
    }

    status->freq = impl->freq;

    err = DTMBIP_GetSignalStrength(&status->strength);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetSignalQuality(&status->quality);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetSignalSNRMC(&status->snrmc);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetSignalSNRSC(&status->snrsc);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetSignalSNR(&status->snr);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetLdpcBER(&status->ldpc_ber);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetSignalBER(&status->ber);
    err_flag |= (!!err)<<(err_off++);

    err = DTMBIP_GetFieldStrength(&status->field_strength);
    err_flag |= (!!err)<<(err_off++);

    if (err_flag != 0) {
        printf("call any sub function failure, err flag '0x%x'", err_flag);
        ret = -1;
        goto out;
    }

out:
    pthread_mutex_unlock(&impl->mutex);
    return ret;
}

static int32_t __DTMBIP_Reset(DC_DemodT *demod)
{
    struct DTMBIP_ImplS *impl = NULL;
    int ret = 0;

    impl = DC_ContainerOf(demod, struct DTMBIP_ImplS, base);
    pthread_mutex_lock(&impl->mutex);

    demodDeinit(impl);

    ret = demodInit(impl);
    if (ret != 0) {
        impl->state = DEMOD_STATE_ERROR;
        printf("demodInit initial failture...");
        goto out;
    }

    if (impl->freq != 0) {
        ret = DTMBIP_DTMBSetFrequency(impl->freq, impl->frontend_handle);
        if (ret != TRUE) {
            printf("DTMBIP set freq(%u) failture..., maybe not lock...", impl->freq);
        }
        ret = 0;
        /* after set frequency, should be working */
        impl->state = DEMOD_STATE_WORKING;
    } else {
        impl->state = DEMOD_STATE_IDEAL;
    }

out:
    pthread_mutex_unlock(&impl->mutex);

    return 0;
}

static void __DTMBIP_Stop(DC_DemodT *demod)
{
    struct DTMBIP_ImplS *impl = NULL;

    printf("demod stop...");
    impl = DC_ContainerOf(demod, struct DTMBIP_ImplS, base);
    pthread_mutex_lock(&impl->mutex);
    if (impl->state == DEMOD_STATE_NOTREADY) {
        /* already stop nothing to do... */
    } else {
        demodDeinit(impl);
    }

    impl->state = DEMOD_STATE_NOTREADY;

    pthread_mutex_unlock(&impl->mutex);

    return;
}

static struct DemodOpsS dtmbip_ops =
{
    .init = __DTMBIP_Init,
    .set_frequency = __DTMBIP_SetFrequency,
    .get_workstatus = __DTMBIP_GetWorkStatus,
    .reset = __DTMBIP_Reset,
    .stop = __DTMBIP_Stop
};

/*
 * only one device, we need singleton instance
 * and shold only has one owner
 */
DC_DemodT *DTMBIP_SingletonGetInstance(void)
{
    static struct DTMBIP_ImplS *impl = NULL;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    DC_DemodT *demod = NULL;

    pthread_mutex_lock(&mutex);

    if (!impl) {
        impl = malloc(sizeof(*impl));
        memset(impl, 0x00, sizeof(*impl));

        impl->base.ops = &dtmbip_ops;
        pthread_mutex_init(&impl->mutex, NULL);
        impl->state = DEMOD_STATE_NOTREADY;
        impl->ref = 0;
    }

    demod = &impl->base;
    demod->connectParam = calloc(1, sizeof(TunerConnectParam));

    pthread_mutex_unlock(&mutex);
    return demod;
}
