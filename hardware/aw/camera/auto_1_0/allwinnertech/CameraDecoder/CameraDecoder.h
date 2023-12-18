#ifndef __CAMERA_DECODER_H__
#define __CAMERA_DECODER_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <pthread.h>
#include "vencoder.h"  //* video encode library in "LIBRARY/CODEC/VIDEO/ENCODER"
#include "memoryAdapter.h"
#include "vdecoder.h"

#define DBG_ENABLE 0

namespace android {

class CameraDecoder {

public:
    /* Constructs CameraDecoder instance. */
    CameraDecoder();

    /* Destructs CameraDecoder instance. */
    ~CameraDecoder();

public:

    int DecorderOpen(int outFormat,int codecFormat,int framerate,int frameWidth,int frameHeight);
    int DecorderClose();
    int DecorderEnter(const void *in,void *out,int size ,int64_t timestamp);

private:
    VideoDecoder *  mDecoder;
    VConfig   mVideoConf;
    VideoStreamInfo  mVideoInfo;
    VideoStreamDataInfo  mDataInfo;
    struct ScMemOpsS* mMemOpsS;

};
}

#endif  /* __CAMERA_DECODER_H__ */

