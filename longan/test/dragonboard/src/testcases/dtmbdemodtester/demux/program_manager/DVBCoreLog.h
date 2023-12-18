#ifndef DVBCORE_LOG_H
#define DVBCORE_LOG_H

#include <DVBCoreTypes.h>
#include <stdio.h>

#define DVBCORE_LOGV(fmt, arg...) printf(fmt, ##arg)
#define DVBCORE_LOGD(fmt, arg...) printf(fmt, ##arg)
#define DVBCORE_LOGI(fmt, arg...) printf(fmt, ##arg)
#define DVBCORE_LOGW(fmt, arg...) printf(fmt, ##arg)
#define DVBCORE_LOGE(fmt, arg...) printf(fmt, ##arg)

#define DVBCORE_TRESPASS() 

#define DVBCORE_FORCE_CHECK(e) DVBCORE_CHECK(e)

#define DVBCORE_LOG_CHECK(e, fmt, arg...)                           \
    do {                                                        \
        if (!(e))                                                 \
        {                                                       \
            DVBCORE_LOGE("check (%s) failed:"fmt, #e, ##arg);       \
        }                                                       \
    } while (0)

#define DVBCORE_CHECK(e)                                            \
    do {                                                        \
        if (!(e))                                                 \
        {                                                       \
            DVBCORE_LOGE("check (%s) failed.", #e);                 \
        }                                                       \
    } while (0)

#define DVBCORE_BUF_DUMP(buf, len) \
    do { \
        char *_buf = (char *)buf;\
        char str[1024] = {0};\
        unsigned int index = 0, _len;\
        _len = (unsigned int)len;\
        snprintf(str, 1024, ":%d:[", _len);\
        for (index = 0; index < _len; index++)\
        {\
            snprintf(str + strlen(str), 1024 - strlen(str), "%02hhx ", _buf[index]);\
        }\
        str[strlen(str) - 1] = ']';\
        DVBCORE_LOGD("%s", str);\
    }while (0)
	

#define DVBCORE_UNUSE(param) (void)param

#define DVBCORE_ASSERT(e) \
        LOG_ALWAYS_FATAL_IF(                        \
                !(e),                               \
                "<%s:%d>DVBCORE_ASSERT(%s) failed.",     \
                __FUNCTION__, __LINE__, #e)      \

#endif
