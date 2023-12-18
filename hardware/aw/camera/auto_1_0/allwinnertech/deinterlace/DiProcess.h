#ifndef __DIPROCESS__H__
#define __DIPROCESS__H__

#include <sys/mman.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <utils/Errors.h>

namespace android {
//using android::status_t;

class DiProcess{

public:
    /* Constructs DiProcess instance. */
    DiProcess();

    /* Destructs DiProcess instance. */
    ~DiProcess();

public:

    status_t init();
    status_t diProcess(unsigned char *pre_src, unsigned char *now_src, unsigned char *next_src,
                                int src_width, int src_height, int in_format,
                                unsigned char *dst, int dst_width, int dst_height,
                                int out_format, int nField);
    status_t release();

private:
    int mDiFd;
    int mDiReqId;

};
};

#endif