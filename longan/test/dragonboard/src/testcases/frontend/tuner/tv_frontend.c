#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "tv_frontend.h"
#include "Si2151_AW_Hal.h"
#include "Atbm253_AW_Hal.h"
#include "R842_AW_Hal.h"
#include "fe_log.h"

struct FRONTEND_ImplS
{
    TV_FrontendS base;
    pthread_mutex_t mutex;
};

static FrontendOpsS *frontend_type_pick(enum SUPPORT_TUNER type)
{
    FrontendOpsS *tuner_ops = NULL;

    switch (type) {
    case ATBM253:
        tuner_ops = get_Atbm253Ops();
        break;

    case R842:
        tuner_ops = get_R842Ops();
        break;

    case SI2151:
        tuner_ops = get_Si2151Ops();
        break;

    default:
        FE_LOGE("type index [%d] is overflow, not support yet!\n", type);
        break;
    }

    return (tuner_ops);
}

/*
 * only one device, we need singleton instance
 * and shold only has one owner
 */
TV_FrontendS *Frontend_GetInstance(enum SUPPORT_TUNER type)
{
    static struct FRONTEND_ImplS *impl = NULL;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    TV_FrontendS *frontend = NULL;

    pthread_mutex_lock(&mutex);

    if (!impl) {
        impl = malloc(sizeof(*impl));
        if (!impl) {
            FE_LOGE("malloc frontend impl fail\n");
            goto error;
        }
        memset(impl, 0x00, sizeof(*impl));

        impl->base.ops = frontend_type_pick(type);
        if (!impl->base.ops) {
            FE_LOGE("pick frontend ops fail\n");
            goto error;
        }

        pthread_mutex_init(&impl->mutex, NULL);
    }

    frontend = &impl->base;

    pthread_mutex_unlock(&mutex);

    return frontend;

error:
    if (impl)
        free(impl);

    pthread_mutex_unlock(&mutex);

    return NULL;
}