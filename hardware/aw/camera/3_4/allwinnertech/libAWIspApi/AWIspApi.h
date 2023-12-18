#ifndef __AWISPAPI_H__
#define __AWISPAPI_H__

#include <log/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Errors.h>

namespace android {

class AWIspApi {

public:
    /* Constructs AWIspApi instance. */
    AWIspApi();

    /* Destructs AWIspApi instance. */
    ~AWIspApi();

public:

    status_t awIspApiInit();
    int      awIspGetIspId(int video_id);
    status_t awIspStart(int isp_id);
    status_t awIspStop(int isp_id);
    status_t awIspWaitToExit(int isp_id);
    status_t awIspApiUnInit();
    int      awGetFocusStatus();
    int      awSetFocusRegions(int x1, int y1, int x2, int y2);
    int awIspGetExposureTime(int isp_id);
    int awSetFpsRanage(int isp_id, int fps);
    int awIspGetISO(int isp_id);

    status_t awIspGetInfoLength(int* i3a_length, int* debug_length);
    status_t awIspGet3AParameters(void * params);
    status_t awIspGetDebugMessage(void * message);

};
}
#endif  /* __AWISPAPI_H__ */
