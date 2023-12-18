#ifndef COMMON_DEFINE_H_
#define COMMON_DEFINE_H_

#ifndef BUILD_WITH_LINUX
#include "fe_log.h"
#else
#define DC_LOGE(fmt, arg...)    printf(fmt, ##arg)
#define DC_LOGD(fmt, arg...)    printf(fmt, ##arg)
#define DC_LOGV(fmt, arg...)    printf(fmt, ##arg)
#endif

#include "DataType.h"

enum FRONTEND_ERRCODE {
    FRONTEND_NO_ERROR = 0,
    FRONTEND_ERROR    = 1,
};

#define TV_UNUSE(x)          (void)x
#endif // !COMMON_DEFINE_H_
