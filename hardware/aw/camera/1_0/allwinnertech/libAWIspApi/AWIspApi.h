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
    status_t awIspGetParams(void * pParameter);
    status_t awIspStop(int isp_id);
    status_t awIspWaitToExit(int isp_id);
    status_t awIspApiUnInit();

};
}
#endif  /* __AWISPAPI_H__ */