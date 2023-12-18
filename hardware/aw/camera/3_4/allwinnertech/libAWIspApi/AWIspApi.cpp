
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
#include "include/V4l2Camera/sunxi_camera_v2.h"

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

status_t AWIspApi::awGetFocusStatus()
{
    int res;
    res = isp_get_af_status();
    return res;
}

status_t AWIspApi::awSetFocusRegions(int x1, int y1, int x2, int y2)
{
    int res = -1;
    struct v4l2_win_setting win;
    win.metering_mode = AUTO_FOCUS_METERING_SPOT;
    win.coor.x1 = (x1 * 2000)/1600 - 1000;
    win.coor.y1 = (y1 * 2000)/1200 - 1000;
    win.coor.x2 = (x2 * 2000)/1600 - 1000;
    win.coor.y2 = (y2 * 2000)/1200 - 1000;

    //res = isp_set_attr_cfg(0, ISP_CTRL_AF_METERING, &win);
    ALOGD("####awSetFocusRegions res:%d x1:%d y1:%d x2:%d y2:%d",
        res,win.coor.x1,win.coor.y1,win.coor.x2,win.coor.y2);

    return res;
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

status_t AWIspApi::awIspGetInfoLength(int* i3a_length, int* debug_length)
{
    int ret = -1;
    ret = isp_get_info_length(i3a_length, debug_length);
    return NO_ERROR;
}

status_t AWIspApi::awIspGet3AParameters(void * params)
{
    int ret = -1;
    ret = isp_get_3a_parameters(0, params);
    return NO_ERROR;
}

status_t AWIspApi::awIspGetDebugMessage(void * msg)
{
    int ret = -1;
    ret = isp_get_debug_msg(0, msg);
    return NO_ERROR;
}
status_t AWIspApi::awSetFpsRanage(int isp_id, int fps)
{
        int ret = -1;
        ret = isp_set_fps(isp_id, fps);
        return NO_ERROR;
}
int AWIspApi::awIspGetExposureTime(int isp_id)
{
    int value = 0;
    int ret = -1;

    ret = isp_get_attr_cfg(isp_id, ISP_CTRL_EXPOSURE_TIME, &value);
    if (ret < 0) {
        ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
        return UNKNOWN_ERROR;
    }

    return value;
}

int AWIspApi::awIspGetISO(int isp_id)
{
    int value = 0;
    int ret = -1;

    ret = isp_get_attr_cfg(isp_id, ISP_CTRL_AGAIN_ISO, &value);
    if (ret < 0) {
        ALOGE("F:%s, L:%d, ret:%d",__FUNCTION__, __LINE__, ret);
        return UNKNOWN_ERROR;
    }

    return value;
}

}
