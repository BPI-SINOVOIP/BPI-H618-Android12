
#include "AWIspApi.h"

//#define LOG_TAG    "AWIspApi"

#ifdef __cplusplus
extern "C" {
#endif

#include "device/isp_dev.h"
#include "isp_dev/tools.h"

#include "isp_events/events.h"
#include "isp_tuning/isp_tuning_priv.h"
#include "isp_manage.h"

#include "iniparser/src/iniparser.h"

#include "isp.h"

#ifdef __cplusplus
}
#endif

#define MAX_ISP_NUM 2

namespace android {


AWIspApi::AWIspApi()
{
    ALOGD("new AWIspApi, F:%s, L:%d",__FUNCTION__, __LINE__);
}

AWIspApi::~AWIspApi()
{
    ALOGD("release AWIspApi, F:%s, L:%d",__FUNCTION__, __LINE__);
}

status_t AWIspApi::awIspApiInit()
{
    
    media_dev_init();
    return NO_ERROR;
}

int AWIspApi::awIspGetIspId(int video_id)
{
    int id = -1;

    id = isp_get_isp_id(video_id);

    ALOGD("F:%s, L:%d, video%d --> isp%d",__FUNCTION__, __LINE__, video_id, id);
    if (id > MAX_ISP_NUM - 1) {
        id = -1;
        ALOGE("F:%s, L:%d, get isp id error!",__FUNCTION__, __LINE__);
    }
    return id;
}

status_t AWIspApi::awIspStart(int isp_id)
{
    int ret = -1;

    ret = isp_init(isp_id);
    ret = isp_run(isp_id);

    if (ret < 0) {
        ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t AWIspApi::awIspGetParams(void * pParams){
  int ret = -1;

  ret = isp_getparams((isp_hal_params *)pParams);

  if (ret < 0) {
    ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
    return UNKNOWN_ERROR;
  }
  return NO_ERROR;
}

status_t AWIspApi::awIspStop(int isp_id)
{
    int ret = -1;

    ret = isp_stop(isp_id);
    ret = isp_pthread_join(isp_id);
    ret = isp_exit(isp_id);

    if (ret < 0) {
        ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t AWIspApi::awIspWaitToExit(int isp_id)
{
    int ret = -1;

    ret = isp_pthread_join(isp_id);
    ret = isp_exit(isp_id);

    if (ret < 0) {
        ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t AWIspApi::awIspApiUnInit()
{
    //status_t ret = UNKNOWN_ERROR;
    media_dev_exit();
    return NO_ERROR;
}

}
